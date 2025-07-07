package zuc

import (
	"encoding/binary"
	"fmt"
)

const (
	chunk = 16
)

type ZUC128Mac struct {
	zucState32
	k0        [8]uint32
	t         uint32
	x         [chunk]byte
	nx        int
	len       uint64
	tagSize   int
	initState zucState32
}

// NewHash create hash for zuc-128 eia, with arguments key and iv.
// Both key/iv size are 16 in bytes.
func NewHash(key, iv []byte) (*ZUC128Mac, error) {
	k := len(key)
	ivLen := len(iv)
	mac := &ZUC128Mac{}
	mac.tagSize = 4

	switch k {
	default:
		return nil, fmt.Errorf("zuc/eia: invalid key size %d, expect 16 in bytes", k)
	case 16: // ZUC-128
		if ivLen != IVSize128 {
			return nil, fmt.Errorf("zuc/eia: invalid iv size %d, expect %d in bytes", ivLen, IVSize128)
		}
		mac.loadKeyIV16(key, iv)
	}

	// initialization
	for i := 0; i < 32; i++ {
		mac.bitReorganization()
		w := mac.f32()
		mac.enterInitMode(w >> 1)
	}

	// work state
	mac.bitReorganization()
	mac.f32()
	mac.enterWorkMode()

	mac.initState.r1 = mac.r1
	mac.initState.r2 = mac.r2

	copy(mac.initState.lfsr[:], mac.lfsr[:])
	mac.Reset()
	return mac, nil
}

func genIV4EIA(count, bearer, direction uint32) []byte {
	iv := make([]byte, 16)
	binary.BigEndian.PutUint32(iv, count)
	copy(iv[9:12], iv[1:4])
	iv[4] = byte(bearer << 3)
	iv[12] = iv[4]
	iv[8] = iv[0] ^ byte(direction<<7)
	iv[14] = byte(direction << 7)
	return iv
}

// NewEIAHash create hash for zuc-128 eia, with arguments key, count, bearer and direction
func NewEIAHash(key []byte, count, bearer, direction uint32) (*ZUC128Mac, error) {
	return NewHash(key, genIV4EIA(count, bearer, direction))
}

func (m *ZUC128Mac) Size() int {
	return m.tagSize
}

func (m *ZUC128Mac) BlockSize() int {
	return chunk
}

// Reset resets the Hash to its initial state.
func (m *ZUC128Mac) Reset() {
	m.t = 0
	m.nx = 0
	m.len = 0
	m.r1 = m.initState.r1
	m.r2 = m.initState.r2
	copy(m.lfsr[:], m.initState.lfsr[:])
	m.genKeywords(m.k0[:len(m.k0)/2])
}

func blockGeneric(m *ZUC128Mac, p []byte) {
	var k64, t64 uint64
	t64 = uint64(m.t) << 32
	for len(p) >= chunk {
		m.genKeywords(m.k0[4:])
		for i := 0; i < 4; i++ {
			k64 = uint64(m.k0[i])<<32 | uint64(m.k0[i+1])
			w := binary.BigEndian.Uint32(p[i*4:])
			for j := 0; j < 32; j++ {
				t64 ^= ^(uint64(w>>31) - 1) & k64
				w <<= 1
				k64 <<= 1
			}
		}
		copy(m.k0[:4], m.k0[4:])
		p = p[chunk:]
	}
	m.t = uint32(t64 >> 32)
}

func (m *ZUC128Mac) Write(p []byte) (nn int, err error) {
	nn = len(p)
	m.len += uint64(nn)
	if m.nx > 0 {
		n := copy(m.x[m.nx:], p)
		m.nx += n
		if m.nx == chunk {
			block(m, m.x[:])
			m.nx = 0
		}
		p = p[n:]
	}
	if len(p) >= chunk {
		n := len(p) &^ (chunk - 1)
		block(m, p[:n])
		p = p[n:]
	}
	if len(p) > 0 {
		m.nx = copy(m.x[:], p)
	}
	return
}

func (m *ZUC128Mac) checkSum(additionalBits int, b byte) [4]byte {
	if m.nx >= chunk {
		panic("m.nx >= chunk")
	}
	kIdx := 0
	if m.nx > 0 || additionalBits > 0 {
		var k64, t64 uint64
		t64 = uint64(m.t) << 32
		m.x[m.nx] = b
		nRemainBits := 8*m.nx + additionalBits
		if nRemainBits > 2*32 {
			m.genKeywords(m.k0[4:6])
		}
		words := (nRemainBits + 31) / 32
		for i := 0; i < words-1; i++ {
			k64 = uint64(m.k0[i])<<32 | uint64(m.k0[i+1])
			w := binary.BigEndian.Uint32(m.x[i*4:])
			for j := 0; j < 32; j++ {
				t64 ^= ^(uint64(w>>31) - 1) & k64
				w <<= 1
				k64 <<= 1
			}
		}
		nRemainBits -= (words - 1) * 32
		kIdx = words - 1
		if nRemainBits > 0 {
			k64 = uint64(m.k0[kIdx])<<32 | uint64(m.k0[kIdx+1])
			w := binary.BigEndian.Uint32(m.x[(words-1)*4:])
			for j := 0; j < nRemainBits; j++ {
				t64 ^= ^(uint64(w>>31) - 1) & k64
				w <<= 1
				k64 <<= 1
			}
			m.k0[kIdx] = uint32(k64 >> 32)
			m.k0[kIdx+1] = m.k0[kIdx+2]
		}
		m.t = uint32(t64 >> 32)
	}
	m.t ^= m.k0[kIdx]
	m.t ^= m.k0[kIdx+1]

	var digest [4]byte
	binary.BigEndian.PutUint32(digest[:], m.t)
	return digest
}

// Finish this function hash nbits data in p and return mac value
// In general, we will use byte level function, this is just for test/verify.
func (m *ZUC128Mac) Finish(p []byte, nbits int) []byte {
	if len(p) < (nbits+7)/8 {
		panic("invalid p length")
	}
	nbytes := nbits / 8
	nRemainBits := nbits - nbytes*8
	if nbytes > 0 {
		m.Write(p[:nbytes])
	}
	var b byte
	if nRemainBits > 0 {
		b = p[nbytes]
	}
	digest := m.checkSum(nRemainBits, b)
	return digest[:]
}

// Sum appends the current hash to in and returns the resulting slice.
// It does not change the underlying hash state.
func (m *ZUC128Mac) Sum(in []byte) []byte {
	// Make a copy of d so that caller can keep writing and summing.
	d0 := *m
	hash := d0.checkSum(0, 0)
	return append(in, hash[:]...)
}
