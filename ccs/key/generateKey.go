package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"log"

	"github.com/tjfoc/gmsm/sm2"
	"github.com/tjfoc/gmsm/x509"
)

func GenerateSm2Key() {
	priv, err := sm2.GenerateKey(nil) // 生成密钥对
	fmt.Printf("私钥:\n%x\n", priv)
	if err != nil {
		fmt.Print(err)
	}
	privPem, err := x509.WritePrivateKeyToPem(priv, nil) // 生成密钥文件
	if err != nil {
		fmt.Println(err)
	}

	pubKey, _ := priv.Public().(*sm2.PublicKey)
	fmt.Printf("公钥:\n%x\n", pubKey)
	pubkeyPem, err := x509.WritePublicKeyToPem(pubKey)   

	err = ioutil.WriteFile("priv.key", privPem, os.FileMode(0644))
	if err != nil {
		fmt.Println(err)
	}

	err = ioutil.WriteFile("pub.key", pubkeyPem, os.FileMode(0644))
	if err != nil {
		fmt.Println(err)
	}
	log.Printf("密钥文件生成完毕")

	/*privPem, err = ioutil.ReadFile("priv.pem")
	if err != nil{
		fmt.Println(err)
	}
	pubkeyPem, err := ioutil.ReadFile("pub.key")
	if err != nil{
		fmt.Println(err)
	}

	pr, err := x509.ReadPrivateKeyFromPem(privPem, nil) // 读取密钥
	if err != nil {
		fmt.Println(err)
	}
	fmt.Printf("私钥:\n%x\n", pr)
	pu, err := x509.ReadPublicKeyFromPem(pubkeyPem) // 读取公钥
	if err != nil {
		fmt.Println(err)
	}
	fmt.Printf("公钥:\n%x\n", pu)
	log.Printf("密钥文件读取完毕")*/
}

func main() {
	GenerateSm2Key()
}

/*package main

import (
	"encoding/base64"
	"fmt"
	"io/ioutil"
	"os"
	"testing"
	"github.com/tjfoc/gmsm/sm2"
	"github.com/tjfoc/gmsm/x509"
	"github.com/tjfoc/gmsm/pkcs12"
	//"github.com/tjfoc/gmsm/x509"
	"fmt"
)

func Test_P12Dncrypt() {
	certificate, priv, err := pkcs12.SM2P12Decrypt("priv.p12", "ling1111") //根据密码读取P12证书
	if err != nil {
		fmt.Println(err)
	}
	privPem, _ := ioutil.ReadFile("priv.pem")
	privatekey, err := x509.ReadPrivateKeyFromPem(privPem, nil)
	if err != nil {
		t.Fatal(err)
	}
	fmt.Println(certificate.Issuer)
	// fmt.Println(privatekey.D.Cmp(priv.D) == 0)
	fmt.Printf("%x", priv)
	fmt.Printf("%x", priv.D)
	fmt.Println(priv.IsOnCurve(priv.X, priv.Y))
}

func main() {
	Test_P12Dncrypt()
}*/
