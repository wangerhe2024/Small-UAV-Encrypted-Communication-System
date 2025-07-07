package main

import (
	"bytes"
	"fmt"
	"log"
	"net"
	"os"
	"syscall"
	"time"
	"unsafe"

	"crypto/cipher"
	"crypto/rand"
	"encoding/hex"
	"io"
	"io/ioutil"
	"sync/atomic"

	"crypto/hmac"

	"github.com/pedroalbanese/gmsm/sm3"

	// "github.com/emmansun/gmsm/sm3"
	"github.com/emmansun/gmsm/sm4"
	"github.com/tjfoc/gmsm/sm2"
	"github.com/tjfoc/gmsm/x509"
	"github.com/xtaci/tcpraw"
)

var allHeadLen int = 57 // IP头：20 UDP头：8  添加头：15 42 帧头：14

var (
	block     cipher.Block
	sm4_iv    []byte
	Sm4KeyStr string
)

var (
	QgcRandomstring     string
	UavRandomstring     string
	FromQgcRandomstring string
	FromUavRandomstring string
)

func main() {
	if len(os.Args) != 2 {
		os.Stderr.WriteString("Usage: tuntap CONFIG_FILE\n")
		os.Exit(1)
	}
	cfg, err := readConfig(os.Args[1])
	checkErr(err)
	tun, err := os.OpenFile("/dev/net/tun", os.O_RDWR, 0)
	checkErr(err)

	var ifr ifreq

	if len(cfg.Dev) >= len(ifr.name) {
		log.Fatalf("Device name %s is too long.", cfg.Dev)
	}

	copy(ifr.name[:], cfg.Dev)
	if cfg.TAP {
		ifr.flags = IFF_TAP | IFF_NO_PI
	} else {
		ifr.flags = IFF_TUN | IFF_NO_PI
	}

	_, _, e := syscall.Syscall(
		syscall.SYS_IOCTL,
		tun.Fd(),
		TUNSETIFF,
		uintptr(unsafe.Pointer(&ifr)),
	)
	if e != 0 {
		log.Fatalf("Can not allocate %s device: %v.", cfg.Dev, e)
	}

	cfg.Dev = string(ifr.name[:bytes.IndexByte(ifr.name[:], 0)])

	var raddr *net.TCPAddr
	if cfg.Remote != "" {
		raddr, err = net.ResolveTCPAddr("tcp", cfg.Remote)
		checkErr(err)
	}

	log.Printf("%s: Local: %v, remote: %v.", cfg.Dev /*con.LocalAddr()*/, cfg.Local, raddr)

	var rac chan *net.TCPAddr
	// 地面控制站
	if raddr == nil {
		cmd_server_side(cfg.Dev, cfg.MaxPay, cfg.NetAddr)
		log.Print("qgc start.")
		con, err := tcpraw.Listen("tcp", cfg.Local)
		checkErr(err)

		// 地面控制站认证
		ServerAuth("0.0.0.0:12344")
		time.Sleep(1 * time.Second)
		InitKeys()
		go KeyUpdate()

		rac = make(chan *net.TCPAddr, 1)
		if cfg.LogDown > 0 {
			go logUpDown(cfg.Dev, time.Duration(cfg.LogDown)*time.Second)
		}
		go senderUDP(tun, con, cfg, raddr, rac)
		receiverUDP(tun, con, cfg, rac)
	} else if cfg.Hello > 0 { // 小型无人机
		cmd_client_side(cfg.Dev, cfg.MaxPay, cfg.NetAddr)
		log.Print("uav start.")
		con, err := tcpraw.Dial("tcp", cfg.Remote)
		checkErr(err)

		// 小型无人机认证
		ClientAuth("10.42.0.246:12344")
		time.Sleep(1 * time.Second)
		InitKeys()
		go KeyUpdate()

		go hello(con, raddr, time.Duration(cfg.Hello)*time.Second)
		if cfg.LogDown > 0 {
			go logUpDown(cfg.Dev, time.Duration(cfg.LogDown)*time.Second)
		}
		go senderUDP(tun, con, cfg, raddr, rac)
		receiverUDP(tun, con, cfg, rac)
	}
}

func WriteToFile(FileName string, KeyContent []byte) {
	err := ioutil.WriteFile(FileName, KeyContent, os.FileMode(0644))
	if err != nil {
		fmt.Println(err)
	}
}

func InforLog(FileName string, infor string, key string) {
	timeStr := time.Now().Format("2006-01-02 15:04:05")
	testRetFile, err := os.OpenFile(FileName, os.O_CREATE|os.O_APPEND|os.O_RDWR, 0666)
	if err != nil {
		log.Println(err)
	}
	_, err = fmt.Fprintln(testRetFile, timeStr+"   "+infor+"\n"+key)
	if err != nil {
		log.Println(err)
	}
}

func KeyUpdate() {
	for {
		timeAfterTrigger := time.After(60 * time.Second)
		<-timeAfterTrigger

		Sm4Key, err := ioutil.ReadFile("key/sm4.key")
		if err != nil {
			fmt.Println(err)
		}
		NewKey := sm3.Sm3Sum([]byte(Sm4Key))

		NewKeyTemp := []byte(NewKey[:])
		NewSm4Key := NewKeyTemp[0 : len(NewKeyTemp)/2]
		log.Print("updated sm4 share key:")
		fmt.Printf("%x\n", NewSm4Key)

		WriteToFile("key/sm4.key", NewSm4Key)

		Sm4KeyStr := hex.EncodeToString(NewSm4Key)
		// 密钥字符串用于QT显示
		InforLog("log/KeyUpdate.log", "SM4共享密钥更新", Sm4KeyStr)
		sm4_key, _ := hex.DecodeString(Sm4KeyStr)

		// 更换密钥后重新建立加密实例 SM4
		block, _ = sm4.NewCipher(sm4_key)
		sm4_iv = make([]byte, sm4.BlockSize)
	}
}

func InitKeys() {
	// sm4 16字节
	Sm4Key, err := ioutil.ReadFile("key/sm4.key")
	if err != nil {
		fmt.Println(err)
	}
	Sm4KeyStr = hex.EncodeToString(Sm4Key)
	sm4_key, _ := hex.DecodeString(Sm4KeyStr)
	block, _ = sm4.NewCipher(sm4_key)
	sm4_iv = make([]byte, sm4.BlockSize)
}

// 地面控制站
func ServerAuth(listen string) {
	listener, err := net.Listen("tcp", listen)
	if err != nil {
		fmt.Println("Listen tcp server failed,err:", err)
		return
	}
	for {
		conn, err := listener.Accept()
		og.Pri	1去啊撒·nt("accept.")
		if err != nil {
			fmt.Println("Listen.Accept failed,err:", err)
			continue
		}
		go ServerAuthProcess(conn)
		break
	}
}

func ServerAuthProcess(conn net.Conn) {
	defer conn.Close()
	var CloseFlag int = 0

	// 地面控制站读取自身密钥
	privPem, err := ioutil.ReadFile("key/priv.key")
	if err != nil {
		fmt.Println(err)
	}
	QgcPriv, err := x509.ReadPrivateKeyFromPem(privPem, nil) // 读取密钥
	if err != nil {
		fmt.Println(err)
	}
	// 地面控制站读取无人机公钥
	UavpubkeyPem, err := ioutil.ReadFile("key/uavpub.key")
	if err != nil {
		fmt.Println(err)
	}
	UavQgcPub, err := x509.ReadPublicKeyFromPem(UavpubkeyPem) // 读取公钥
	if err != nil {
		fmt.Println(err)
	}

	// 1.地面控制站向小型无人机发送hello数据包
	SendBuf := make([]byte, 8192)
	Content := "hello"
	FlagLoca := "01"
	SendBuf = []byte(FlagLoca + Content)
	conn.Write(SendBuf)
	InforLog("log/AuthDis.log", "地面控制站", "(1)向无人机发送hello包")

	for {
		var buf [8192]byte
		n, err := conn.Read(buf[:])
		if err != nil {
			fmt.Println("Read from tcp server failed,err:", err)
			break
		}
		data := string(buf[:n])

		// 具体认证过程
		switch data[:2] {
		case "02":
			fmt.Printf("Recived from uav RandomString:%s\n", data[2:n])
			InforLog("log/AuthDis.log", "地面控制站", "(4)收到无人机随机字符串:"+data[2:n])
			// 3.地面控制站向小型无人机发送签名值
			FromUavRandomstring = data[2:n]
			hashValue := sm3.Sm3Sum([]byte(FromUavRandomstring))

			hashValueTemp := []byte(hashValue[:])

			QgcSign, err := QgcPriv.Sign(rand.Reader, hashValueTemp, nil) // 签名
			if err != nil {
				fmt.Println(err)
			}
			FlagLoca := "03"
			SendBuf = []byte(FlagLoca + string(QgcSign))
			conn.Write(SendBuf)
			fmt.Printf("sent to uav signValue\n")
			InforLog("log/AuthDis.log", "地面控制站", "(5)向无人机发送随机字符串的签名值")
		case "04":
			fmt.Printf("Recived from uav verify inform:%s\n", data[2:n])
			InforLog("log/AuthDis.log", "地面控制站", "(8)收到无人机身份确认消息:"+data[2:n])
			// 5.地面控制站向小型无人机发送随机字符串
			FlagLoca := "05"
			QgcRandomstring = GenerateRandomString(20)
			SendBuf = []byte(FlagLoca + QgcRandomstring)
			conn.Write(SendBuf)
			InforLog("log/AuthDis.log", "地面控制站", "(9)向无人机发送随机字符串")
		case "06":
			hashValue := sm3.Sm3Sum([]byte(QgcRandomstring))
			hashValueTemp := []byte(hashValue[:])

			ok := UavQgcPub.Verify(hashValueTemp, []byte(data[2:n])) // 密钥验证
			if ok != true {
				fmt.Printf("Verify uav error\n")
				InforLog("log/AuthDis.log", "地面控制站", "(12)验签失败")
			} else {
				fmt.Printf("Verify uav ok\n")
				InforLog("log/AuthDis.log", "地面控制站", "(12)验签成功")
			}
			// 7.地面控制站向小型无人机发送确认消息
			FlagLoca := "07"
			Content := "uavverify"
			SendBuf = []byte(FlagLoca + Content)
			conn.Write(SendBuf)
			// InforLog("log/AuthDis.log", "地面控制站", "(13)向小型无人机发送身份确认消息")

			// 产生共享密钥
			RanStr := GenerateRandomString(20)
			ShareKey := sm3.Sm3Sum([]byte(RanStr))
			ShareKeyTemp := []byte(ShareKey[:])
			ckey, err := sm2.Encrypt(UavQgcPub, ShareKeyTemp, rand.Reader, 1)
			if err != nil {
				fmt.Printf("Error: failed to encrypt %v\n", err)
				return
			}
			ShareSm4Key := ShareKeyTemp[0 : len(ShareKeyTemp)/2]
			fmt.Printf("Sm4 share key:%x\n", ShareSm4Key)
			InforLog("log/AuthDis.log", "地面控制站", "(15)共享密钥生成完毕\n"+hex.EncodeToString(ShareSm4Key))
			WriteToFile("key/sm4.key", ShareSm4Key)

			// 8.地面控制站向小型无人机发送密态共享密钥
			FlagLoca = "08"
			SendBuf = []byte(FlagLoca + string(ckey))
			conn.Write(SendBuf)
			fmt.Printf("sent to uav share key\n")
			InforLog("log/AuthDis.log", "地面控制站", "(16)向小型无人机发送共享密钥")
		case "09":
			fmt.Printf("Recived from uav start inform:%s\n", data[2:n])
			InforLog("log/AuthDis.log", "地面控制站", "(19)接收到无人机共享密钥成功接收数据包："+data[2:n])
			fmt.Printf("Start encryption communication with uav\n")
			InforLog("log/AuthDis.log", "地面控制站", "和无人机开始加密通信")
			CloseFlag = 1
		}
		if CloseFlag == 1 {
			break
		}
	}
}

// 小型无人机
func ClientAuth(target string) {
	conn, err := net.Dial("tcp", target)
	if err != nil {
		fmt.Println("Connect to TCP server failed ,err:", err)
		return
	}
	go ClientAuthProcess(conn)
}

// 小型无人机
func ClientAuthProcess(conn net.Conn) {
	defer conn.Close()
	var CloseFlag int = 0
	SendBuf := make([]byte, 8192)

	// 小型无人机读取自身密钥
	privPem, err := ioutil.ReadFile("key/priv.key")
	if err != nil {
		fmt.Println(err)
	}
	UavPriv, err := x509.ReadPrivateKeyFromPem(privPem, nil) // 读取密钥
	if err != nil {
		fmt.Println(err)
	}
	// 小型无人机读取地面控制站公钥
	QgcpubkeyPem, err := ioutil.ReadFile("key/qgcpub.key")
	if err != nil {
		fmt.Println(err)
	}
	QgcUavPub, err := x509.ReadPublicKeyFromPem(QgcpubkeyPem) // 读取公钥
	if err != nil {
		fmt.Println(err)
	}

	for {
		var buf [8192]byte
		n, err := conn.Read(buf[:])
		if err != nil {
			fmt.Println("Read from tcp server failed,err:", err)
			break
		}
		data := string(buf[:n])
		// 具体认证过程
		switch data[:2] {
		case "01":
			fmt.Printf("Recived from qgc hello pkt:%s\n", data[2:n])
			InforLog("log/AuthDis.log", "无人机", "(2)收到地面控制站hello包:"+data[2:n])
			// 2.小型无人机向地面控制站发送随机字符串
			FlagLoca := "02"
			UavRandomstring = GenerateRandomString(20)
			SendBuf = []byte(FlagLoca + UavRandomstring)
			conn.Write(SendBuf)
			InforLog("log/AuthDis.log", "无人机", "(3)向地面控制站发送随机字符串")

		case "03":
			hashValue := sm3.Sm3Sum([]byte(UavRandomstring))
			hashValueTemp := []byte(hashValue[:])

			ok := QgcUavPub.Verify(hashValueTemp, []byte(data[2:n])) // 密钥验证
			if ok != true {
				fmt.Printf("Verify qgc error\n")
				InforLog("log/AuthDis.log", "无人机", "(6)验签失败")
			} else {
				fmt.Printf("Verify qgc ok\n")
				InforLog("log/AuthDis.log", "无人机", "(6)验签成功")
			}
			// 4.小型无人机向地面控制站发送确认消息
			FlagLoca := "04"
			Content := "qgcverify"
			SendBuf = []byte(FlagLoca + Content)
			conn.Write(SendBuf)
			InforLog("log/AuthDis.log", "无人机", "(7)向地面控制站发送身份确认消息")
		case "05":
			fmt.Printf("Recived from qgc RandomString:%s\n", data[2:n])
			InforLog("log/AuthDis.log", "无人机", "(10)接收到地面控制站随机字符串:"+data[2:n])
			// 6.小型无人机向地面控制站发送签名值
			FromQgcRandomstring = data[2:n]
			hashValue := sm3.Sm3Sum([]byte(FromQgcRandomstring))
			hashValueTemp := []byte(hashValue[:])

			UavSign, err := UavPriv.Sign(rand.Reader, hashValueTemp, nil) // 签名
			if err != nil {
				fmt.Println(err)
			}
			FlagLoca := "06"
			SendBuf = []byte(FlagLoca + string(UavSign))
			conn.Write(SendBuf)
			fmt.Printf("sent to qgc signValue\n")
			InforLog("log/AuthDis.log", "无人机", "(11)向地面控制站发送该随机字符串的签名值")
		case "07":
			fmt.Printf("Recived from qgc verify inform:%s\n", data[2:n])
			InforLog("log/AuthDis.log", "无人机", "(14)从地面控制站接收到身份确认消息："+data[2:n])
			fmt.Printf("Complete identity authentication\n")
		case "08":
			fmt.Printf("Recived from qgc share key:%x\n", data[2:n])
			dkey, _ := sm2.Decrypt(UavPriv, buf[2:n], 1)
			ShareSm4Key := dkey[0 : len(dkey)/2]
			fmt.Printf("Sm4 share key:%x\n", ShareSm4Key)
			// InforLog("log/AuthDis.log", "无人机", "(17)从地面控制站接收到共享密钥\n"+hex.EncodeToString(ShareSm4Key)+"\n"+hex.EncodeToString(ShareZucKey))
			InforLog("log/AuthDis.log", "无人机", "(17)从地面控制站接收到共享密钥\n"+hex.EncodeToString(ShareSm4Key))

			WriteToFile("key/sm4.key", ShareSm4Key)

			// 1.地面控制站向小型无人机发送ok数据包
			SendBuf := make([]byte, 8192)
			Content := "ok"
			FlagLoca := "09"
			SendBuf = []byte(FlagLoca + Content)
			conn.Write(SendBuf)
			InforLog("log/AuthDis.log", "无人机", "(18)向地面控制站发送已接收共享密钥数据包")
			fmt.Printf("Start encryption communication with qgc\n")
			InforLog("log/AuthDis.log", "无人机", "和地面控制站开始加密通信")
			CloseFlag = 1
		}
		if CloseFlag == 1 {
			break
		}
	}
}

/**************************************************
* 以上主要为身份认证 密钥协商部分代码
**************************************************/
func InforTxt(FileName string, infor string) {
	testRetFile, err := os.OpenFile(FileName, os.O_CREATE|os.O_RDWR, 0666)
	if err != nil {
		log.Println(err)
	}
	_, err = fmt.Fprintln(testRetFile, infor)
	if err != nil {
		log.Println(err)
	}
}

func checkNetErr(err error) bool {
	if err == nil {
		return false
	}
	if e, ok := err.(*net.OpError); ok {
		if e, ok := e.Err.(*os.SyscallError); ok {
			switch e.Err {
			case syscall.ECONNREFUSED, syscall.EHOSTUNREACH, syscall.ENETUNREACH:
				return true
			}
		}
	}
	log.Fatal("Network error: ", err)
	panic(nil)
}

type header struct {
	Id      uint64 // Schould have a random initial value.
	FragN   byte
	FragNum byte
	Len     uint16
	MsgId   byte
	IsPlain byte
	Hash    byte
}

const (
	headerLen = 15
	idLen     = 8
)

func (h *header) Encode(buf []byte) {
	buf[0] = h.MsgId
	id := h.Id
	buf[1] = byte(id)
	buf[2] = byte(id >> 8)
	buf[3] = byte(id >> 16)
	buf[4] = byte(id >> 24)
	buf[5] = byte(id >> 32)
	buf[6] = byte(id >> 40)
	buf[7] = byte(id >> 48)
	buf[8] = byte(id >> 56)
	buf[9] = h.FragN
	buf[10] = h.FragNum
	buf[11] = byte(h.Len)
	buf[12] = byte(h.Len >> 8)
	// buf[12] = h.MsgId
	buf[13] = h.IsPlain
	buf[14] = h.Hash
}

func (h *header) Decode(buf []byte) {
	h.Id = uint64(buf[1]) | uint64(buf[2])<<8 |
		uint64(buf[3])<<16 | uint64(buf[6])<<24 |
		uint64(buf[5])<<32 | uint64(buf[4])<<40 |
		uint64(buf[7])<<48 | uint64(buf[8])<<56
	h.FragN = buf[9]
	h.FragNum = buf[10]
	h.Len = uint16(buf[11]) | uint16(buf[12])<<8
	h.MsgId = buf[0]
	h.IsPlain = buf[13]
	h.Hash = buf[14]
}

/*func blkAlignUp(n int) int {
	return (n + blkMask) &^ blkMask
}*/

func IsContain(items []byte, item byte) bool {
	for _, eachItem := range items {
		if eachItem == item {
			return true
		}
	}
	return false
}

func WhichOne(msgid byte) string {
	var Content string
	switch msgid {
	case 0x00:
		Content = "HEARTBEAT:用于发送心跳包,以确保通信正常"
	case 0x03:
		Content = "SYS_STATUS:用于发送系统状态信息,例如电池电量,CPU负载等,"
	case 0x02:
		Content = "GPS_RAW_INT:用于发送GPS原始数据,例如卫星数量,位置,速度等"
	case 0x1E:
		Content = "ATTITUDE:用于发送姿态信息,例如飞行器的俯仰,横滚和偏航角"
	case 0x34:
		Content = "COMMAND_LONG:用于发送长命令,例如飞行器的起飞,降落,返航等。"
	case 0x39:
		Content = "MISSION_ITEM:用于发送任务项,例如飞行器需要执行的航点,任务等"
	case 0x35:
		Content = "RC_CHANNELS_RAW:用于发送遥控通道的原始值,例如油门,方向,俯仰,横滚等"
	case 0x4A:
		Content = "VFR_HUD:用于发送垂直速度和水平速度等基本飞行信息"
	case 0x18:
		Content = "NAV_CONTROLLER_OUTPUT:用于发送导航控制器的输出信息,例如误差,目标速度等"
	default:
		Content = "暂时不知道的MAVLink消息"
	}
	return Content
}

// var AllMsgId = []byte{0x00, 0x03, 0x02, 0x1E, 0x34, 0x39, 0x35, 0x4A, 0x18}
var AllMsgId = []byte{0x03, 0x02, 0x1E, 0x34, 0x39, 0x35, 0x4A, 0x18} // 排除了心跳包

var lastSent int64

func senderUDP(tun io.Reader, con *tcpraw.TCPConn, cfg *config, raddr *net.TCPAddr, rac <-chan *net.TCPAddr) {
	buffer := make([]byte, 8192)
	var h header

	// Initialize h.Id to random number.
	_, err2 := rand.Read(buffer[:idLen])
	checkErr(err2)
	h.Decode(buffer)

	pkt := make([]byte, headerLen+cfg.MaxPay)
	for {
		buf := buffer
		n, err := tun.Read(buf[headerLen:])
		// fmt.Printf("read n is:%s", n)
		checkErr(err)
		if n == 0 {
			continue
		}
		buf = buf[:n+headerLen]

		h.FragNum = byte((n + cfg.MaxPay - 1) / cfg.MaxPay)

		// Calculate lengths to equally fill all h.FragNum packets.
		payLen := (n + int(h.FragNum) - 1) / int(h.FragNum)
		usedLen := headerLen + payLen
		// pktLen := blkAlignUp(usedLen)
		pktLen := usedLen

		for h.FragN = 0; h.FragN < h.FragNum; h.FragN++ {
			if len(buf) < usedLen {
				usedLen = len(buf)
				payLen = usedLen - headerLen
				// pktLen = blkAlignUp(usedLen)
				// pktLen := usedLen
			}
			h.Len = uint16(payLen)
			h.IsPlain = 0xff // 密文

			// 心跳包
			// fmt.Printf("最初:\n%x\n", buf[:pktLen])
			if len(buf) > (allHeadLen + 8) {
				// MAVLink v1.0
				if (buf[allHeadLen] == 0xfe) && (IsContain(AllMsgId, buf[allHeadLen+5]) == true) { // Mavlink v1 数据包起始标志，0xFE
					fmt.Printf("最初的 tap 数据包:\n%x\n", buf[0:pktLen])
					h.MsgId = 0x01
					h.IsPlain = 0x00
					h.Hash = 0x01
					h.Encode(buf)

					fmt.Printf("加密前的 MAVLink v1 数据包:\n%x\n", buf[allHeadLen:pktLen])
					cipher.NewCTR(block, sm4_iv).XORKeyStream(pkt[allHeadLen:pktLen], buf[allHeadLen:pktLen])
					fmt.Printf("加密后的 MAVLink v1 数据包:\n%x\n", pkt[allHeadLen:pktLen])

					key, _ := hex.DecodeString(Sm4KeyStr)
					h := hmac.New(sm3.New, key)
					h.Write(pkt[allHeadLen:pktLen])
					mac := h.Sum(nil)

					// [size]byte 转换为 []byte
					macTemp := []byte(mac[:])

					pktLen = pktLen + 32
					copy(pkt[pktLen-32:pktLen], macTemp)
					copy(pkt[0:allHeadLen], buf[0:allHeadLen])
					fmt.Printf("最后的 tap 数据包:\n%x\n", pkt[0:pktLen])
				} else if (buf[allHeadLen] == 0x80) && (buf[allHeadLen+1] == 0x60) {
					fmt.Printf("图传数据最初:\n%x\n", buf[1:pktLen])

					h.MsgId = 0x02
					h.IsPlain = 0x00
					h.Hash = 0x00
					h.Encode(buf)

					fmt.Printf("图传数据加密前:\n%x\n", buf[1:pktLen])
					cipher.NewCTR(block, sm4_iv).XORKeyStream(pkt[1:pktLen], buf[1:pktLen])
					copy(pkt[:1], buf[:1])
					fmt.Printf("图传数据加密后:\n%x\n", pkt[1:pktLen])
				} else {
					h.MsgId = 0x00
					h.IsPlain = 0x01
					h.Hash = 0x00
					h.Encode(buf)
					copy(pkt, buf)
				}
			} else {
				h.MsgId = 0x00
				h.IsPlain = 0x01
				h.Hash = 0x00
				h.Encode(buf)
				copy(pkt, buf)
			}

			if rac != nil {
				select {
				case raddr = <-rac:
					log.Printf("%s: Remote address changed to %v.", cfg.Dev, raddr)
				default:
				}
			}
			if raddr == nil {
				break
			}
			if cfg.Hello > 0 {
				atomic.StoreInt64(&lastSent, nanosec())
			}

			// con.WriteToUDP(pkt[:pktLen], raddr)
			_, err := con.WriteTo(pkt[:pktLen], raddr)
			if checkNetErr(err) {
				break
			}
			buf = buf[payLen:]
		}
		h.Id++
	}
}

func hello(con *tcpraw.TCPConn, raddr *net.TCPAddr, hello time.Duration) {
	buf := make([]byte, headerLen+idLen)
	var h header
	// Initialize h.Id to random number.
	_, err2 := rand.Read(buf[:idLen])
	checkErr(err2)
	h.Decode(buf)
	h.Len = idLen
	for {
		last := atomic.LoadInt64(&lastSent)
		now := nanosec()
		idle := now - last
		wait := hello - time.Duration(idle)
		if wait <= 0 {
			h.Encode(buf)
			copy(buf[headerLen:], buf[:idLen])
			// cipher.NewCBCEncrypter(blkCipher, iv).CryptBlocks(buf, buf)
			// _, err := con.WriteToUDP(buf, raddr)
			_, err := con.WriteTo(buf, raddr)
			checkNetErr(err)
			h.Id++
			atomic.StoreInt64(&lastSent, now)
			wait = hello
		}
		time.Sleep(wait)
	}
}

type defrag struct {
	Id    uint64
	Frags [][]byte
}

var lastRecv int64

func receiverUDP(tun io.Writer, con *tcpraw.TCPConn, cfg *config, rac chan<- *net.TCPAddr) {
	de_buf := make([]byte, 8192)
	buf := make([]byte, 8192)
	dtab := make([]*defrag, 3)
	for i := range dtab {
		dtab[i] = &defrag{Frags: make([][]byte, 0, (8192+cfg.MaxPay-1)/cfg.MaxPay)}
	}
	var (
		h   header
		pra = new(net.TCPAddr)
	)

	WriteDataFlag := 0
	WriteVideoFlag := 0

	for {
		var (
			n     int
			err   error
			raddr *net.TCPAddr
		)
		/*if rac == nil {
			n, err = con.Read(buf)
		} else {
			n, raddr, err = con.ReadFromUDP(buf)
		}*/
		var tmpaddr net.Addr
		n, tmpaddr, err = con.ReadFrom(buf)
		if checkNetErr(err) {
			continue
		}
		if n <= 0 {
			continue
		}
		raddr, _ = net.ResolveTCPAddr("tcp", tmpaddr.String())

		switch {
		case n < headerLen+idLen:
			log.Printf("%s: Received packet is to short.", cfg.Dev)
		/*case n&blkMask != 0:
		log.Printf(
			"%s: Received packet length %d is not multiple of block size %d.",
			cfg.Dev, n, blkCipher.BlockSize(),
		)*/
		default:
			if buf[0] == 0x01 {
				fmt.Printf("最初的 tap 数据包:\n%x\n", buf[0:n])
				key, _ := hex.DecodeString(Sm4KeyStr)
				h := hmac.New(sm3.New, key)
				h.Write(buf[allHeadLen : n-32])
				// mac := hex.EncodeToString(h.Sum(buf[allHeadLen:n-32]))
				mac := h.Sum(nil)
				macTemp := []byte(mac[:])

				if bytes.Equal(macTemp, buf[n-32:n]) {
					if WriteDataFlag == 30 {
						InforTxt("log/before_data.txt", hex.EncodeToString(buf[allHeadLen:n]))
					}
					n = n - 32
					fmt.Printf("解密前的 MAVLink v1 数据包:\n%x\n", buf[allHeadLen:n])

					cipher.NewCTR(block, sm4_iv).XORKeyStream(de_buf[allHeadLen:n], buf[allHeadLen:n])
					fmt.Printf("解密后的 MAVLink v1 数据包:\n%x\n", de_buf[allHeadLen:n])
					if WriteDataFlag == 50 {
						InforTxt("log/after_data.txt", hex.EncodeToString(de_buf[allHeadLen:n]))
						InforTxt("log/mavlink_id.txt", WhichOne(de_buf[allHeadLen+5]))
						WriteDataFlag = 0
					}
					copy(buf[allHeadLen:n], de_buf[allHeadLen:n])
					fmt.Printf("最后的 tap 数据包:\n%x\n", buf[0:n])
				} else {
					continue
				}
				WriteDataFlag = WriteDataFlag + 1
			}
			if buf[0] == 0x02 {
				fmt.Printf("图传数据解密前:\n%x\n", buf[1:n])
				if WriteVideoFlag == 50 {
					InforTxt("log/before_video.txt", hex.EncodeToString(buf[allHeadLen:n]))
				}
				cipher.NewCTR(block, sm4_iv).XORKeyStream(de_buf[1:n], buf[1:n])
				copy(de_buf[:1], buf[:1])
				fmt.Printf("图传数据解密后:\n%x\n", de_buf[1:n])
				if WriteVideoFlag == 50 {
					InforTxt("log/after_video.txt", hex.EncodeToString(de_buf[allHeadLen:n]))
					WriteVideoFlag = 0
				}
				copy(buf[0:n], de_buf[0:n])
				WriteVideoFlag = WriteVideoFlag + 1
			}

			h.Decode(buf)

			pktLen := headerLen + int(h.Len)

			if n != pktLen {
				log.Printf("%s: Bad packet size: %d != %d.", cfg.Dev, n, pktLen)
				continue
			}
			if h.FragNum == 0 {
				// Hello packet.
				if h.Len != idLen ||
					!bytes.Equal(buf[:idLen], buf[headerLen:headerLen+idLen]) {

					log.Printf("%s: Bad hello packet.", cfg.Dev)
					continue
				}
				if cfg.LogDown > 0 {
					atomic.StoreInt64(&lastRecv, nanosec())
				}
				break
			}
			if h.FragN >= h.FragNum {
				log.Printf("%s: Bad header (FragN >= FragNum).", cfg.Dev)
				continue
			}
			if cfg.LogDown > 0 {
				atomic.StoreInt64(&lastRecv, nanosec())
			}
			if h.FragNum > 1 {
				var (
					cur *defrag
					cn  int
				)
				for i, d := range dtab {
					if d.Id == h.Id {
						cur = d
						cn = i
						break
					}
				}
				if cur == nil {
					cur = dtab[len(dtab)-1]
					copy(dtab[1:], dtab)
					dtab[0] = cur
					cur.Id = h.Id
					cur.Frags = cur.Frags[:h.FragNum]
				} else if len(cur.Frags) != int(h.FragNum) {
					log.Printf(
						"%s: Header do not match previous fragment.",
						cfg.Dev,
					)
					continue
				}
				frag := cur.Frags[h.FragN]
				if frag == nil {
					frag = make([]byte, 0, cfg.MaxPay)
				}
				frag = frag[:h.Len]
				cur.Frags[h.FragN] = frag
				copy(frag, buf[headerLen:])
				for _, frag = range cur.Frags {
					if len(frag) == 0 {
						// Found lack of fragment.
						break
					}
				}
				if len(frag) == 0 {
					// Lack of some fragment.
					continue
				}
				// All fragments received.
				n = headerLen
				for i, frag := range cur.Frags {
					n += copy(buf[n:], frag)
					cur.Frags[i] = frag[:0]
				}
				cur.Frags = cur.Frags[:0]
				copy(dtab[cn:], dtab[cn+1:])
				dtab[len(dtab)-1] = cur
			}

			_, err = tun.Write(buf[headerLen:n])

			if err != nil {
				if pathErr, ok := err.(*os.PathError); ok &&
					pathErr.Err == syscall.EINVAL {

					log.Printf("%s: Invalid IP datagram.", cfg.Dev)
					continue
				}
				checkErr(err)
			}
		}
		// Received correct Hello packet or TUN/TAP payload.
		if rac != nil && (!pra.IP.Equal(raddr.IP) || pra.Port != raddr.Port) {
			// Inform senderUDP about remote address.
			select {
			case rac <- raddr:
				pra = raddr
			default:
			}
		}
	}
}

func logUpDown(dev string, logDown time.Duration) {
	atomic.StoreInt64(&lastRecv, nanosec()-int64(logDown))
	var up bool
	for {
		last := atomic.LoadInt64(&lastRecv)
		now := nanosec()
		idle := now - last
		wait := logDown - time.Duration(idle)
		if wait <= 0 {
			if up {
				log.Printf("%s: Remote is down.", dev)
				up = false
			}
			wait = logDown / 4
		} else {
			if !up {
				log.Printf("%s: Remote is up.", dev)
				up = true
			}
		}
		time.Sleep(wait)
	}
}

