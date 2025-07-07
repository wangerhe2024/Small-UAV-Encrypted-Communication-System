//go:build (amd64 && !purego) || (arm64 && !purego)
// +build amd64,!purego arm64,!purego

package zuc

import (
	"github.com/emmansun/gmsm/internal/subtle"
)

//go:noescape
func genKeyStreamRev32Asm(keyStream []byte, pState *zucState32)

func xorKeyStream(c *zucState32, dst, src []byte) {
	if supportsAES {
		words := len(src) / 4
		// handle complete words first
		if words > 0 {
			dstWords := dst[:words*4]
			genKeyStreamRev32Asm(dstWords, c)
			subtle.XORBytes(dst, src, dstWords)
		}
		// handle remain bytes
		if words*4 < len(src) {
			var singleWord [4]byte
			genKeyStreamRev32Asm(singleWord[:], c)
			subtle.XORBytes(dst[words*4:], src[words*4:], singleWord[:])
		}
	} else {
		xorKeyStreamGeneric(c, dst, src)
	}
}
