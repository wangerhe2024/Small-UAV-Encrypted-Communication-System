#include "gmssl/rand.h"
#include "gmssl/sm2.h"
#include "gmssl/sm3.h"
#include "rclcpp/rclcpp.hpp"
#include "uav_identify_service/srv/identify.hpp"

using std::placeholders::_1;
using std::placeholders::_2;

// 创建一个类节点，名字叫做Gc,继承自Node.
class Gc : public rclcpp::Node {

public:
  // 初始化地面控制站ID为00
  Gc() : Node("Gc"), Gc_Id("00"), Gc_status("0") {
    // 热心组织的自我介绍
    RCLCPP_INFO(this->get_logger(), "地面站%s上线，等待无人机连接", Gc_Id);

    // 实例化回调组, 作用为避免死锁
    callback_group_gc = this->create_callback_group(
        rclcpp::CallbackGroupType::MutuallyExclusive);
    // 实例化发汉堡的的服务
    Gc_Server = this->create_service<uav_identify_service::srv::Identify>(
        "Identify", std::bind(&Gc::Gc_callback, this, _1, _2),
        rmw_qos_profile_services_default, callback_group_gc);
  }

  void sm3(unsigned char *input, int ilen, unsigned char output[32]) {
    SM3_CTX ctx;

    sm3_init(&ctx);
    sm3_update(&ctx, input, ilen);
    sm3_finish(&ctx, output);
  }

  void GenerateRandomString(char *output, int outlen = 64) {
    uint8_t tempbyte;
    const char hex_chars[] = "0123456789ABCDEF"; // 16进制字符集
    // std::cout << "outlen: " << outlen << std::endl;
    for (int i = 0; i < outlen - 1; i++) {
      rand_bytes(&tempbyte, sizeof(tempbyte));
      // output[i] = char(tempbyte % 94 + 33); // 生成可打印字符
      output[i] = hex_chars[tempbyte % 16]; // 将字节映射到16进制字符
      // std::cout << "output["<< i << "]:" << output[i] << std::endl;
    }

    output[outlen - 1] = '\0'; // 确保字符串以 null 终止
    // std::cout << "Random String output: " << output << std::endl;
  }

private:
  // 地面控制站ID为00
  char Gc_Id[8], Gc_status[8];
  uint8_t temp, hashValue;
  char GcRandomstring[64], hashstring[100], *UavRandomstring;

  // 声明一个服务回调组
  rclcpp::CallbackGroup::SharedPtr callback_group_gc;

  // 声明一个服务端
  rclcpp::Service<uav_identify_service::srv::Identify>::SharedPtr Gc_Server;

  // 声明一个回调函数，当收到请求时调用该函数
  void Gc_callback(
      const uav_identify_service::srv::Identify::Request::SharedPtr request,
      const uav_identify_service::srv::Identify::Response::SharedPtr response) {

    // 首先判断是不是无人机
    if (request->status == "1") {
      // 打印Poorer家的人数
      if (request->service_step_id == 0) {
        RCLCPP_INFO(this->get_logger(),
                    "(0)地面站%s号收到来自无人机%s号的身份验证请求.", Gc_Id,
                    request->uav_id.c_str());

        // case 0: // 收到无人机身份认证请求
        //  进行第1步
        RCLCPP_INFO(this->get_logger(),
                    "(1)正在进行地面站%s号对无人机%s号的第%d步认证.", Gc_Id,
                    request->uav_id.c_str(), request->service_step_id);
        // // InforLog("log/AuthDis.log",
        // "(1)正在进行地面站%s号对无人机%s号的第%s步认证.", Gc_Id.c_str(),
        // request->uav_id.c_str(), request->service_step_id.c_str());

        // 更新地面控制站识别码
        response->gc_id = Gc_Id;

        //------------------------------------
        // 产生随机字符串
        GenerateRandomString(GcRandomstring);
        // std::cout << "Generated Random String: " << GcRandomstring <<
        // std::endl;

        // GcRandomstring[100] = '11111111';
        // sm3(&GcRandomstring, 64, &GcRandomstring);

        //-------------------------------------------

        RCLCPP_INFO(this->get_logger(), "(1)产生随机字符串:{%s}",
                    GcRandomstring);
        // // // InforLog("log/AuthDis.log",

        // 发送随机字符串
        response->gc_attachment = GcRandomstring;
        RCLCPP_INFO(this->get_logger(),
                    "(1)地面站%s号向无人机%s号发送随机字符串:{%s}", Gc_Id,
                    request->uav_id.c_str(), GcRandomstring);
        // // InforLog("log/AuthDis.log",
        // "(1)地面站%s号已向无人机%s号发送随机字符串%s, Gc_Id.c_str()",
        // request->uav_id.c_str(), GcRandomstring);

        // 更新流程编码

        response->service_step_id = 2;
        // Gc_Server->async_send_request(request, std::bind(&Gc::gc_callback,
        // this, _1)); fmt.Printf("Recived from qgc hello pkt:%s\n", data [2:n])
        // // // InforLog("log/AuthDis.log", "无人机",
        // "(2)收到地面控制站hello包:" + data [2:n])
        // // 2.小型无人机向地面控制站发送随机字符串
        // FlagLoca : = "02"
        // SendBuf = [] byte(FlagLoca + UavRandomstring)
        // conn.Write(SendBuf)
        // // // InforLog("log/AuthDis.log", "无人机",
        // "(3)向地面控制站发送随机字符串")
      } else {
        switch (request->service_step_id) {
        case 3:
          // 进行第3步
          RCLCPP_INFO(this->get_logger(),
                      "(3)正在进行地面站%s号对无人机%s号的第%d步认证.", Gc_Id,
                      request->uav_id.c_str(), request->service_step_id);
          // // InforLog("log/AuthDis.log",
          // "(3)正在进行地面站%s号对无人机%s号的第%s步认证.", Gc_Id.c_str(),
          // request->uav_id.c_str(), request->service_step_id.c_str());

          //------------------------------------------------------------
          // SM3产生Hash串
          // tempchar = GcRandomstring + request->uav_id.c_str();
          // hashValue = sm3.Sm3Sum([] byte(tempchar));
          // hashstring = GcRandomstring + request->uav_id;
          std::strcpy(hashstring, request->uav_id.c_str());
          // std::cout << "hashstring output: " << hashstring << std::endl;
          std::strcat(hashstring, GcRandomstring);
          // std::cout << "hashstring output: " << hashstring << std::endl;
          sm3((unsigned char *)hashstring, sizeof(hashstring),
              (unsigned char *)hashstring);
          // std::cout << "hashstring output: " << hashstring << std::endl;
          //------------------------------------------------------------

          //------------------------------------------------------------------
          RCLCPP_INFO(this->get_logger(),
                      "(3)地面站%s号收到无人机%s号发送的随机字符串:{%s}", Gc_Id,
                      request->uav_id.c_str(), request->uav_attachment.c_str());

          // 密钥验证
          // if (QgcUavPub.Verify(hashValue, [] byte(response->uav_attachment)))
          // std::cout << "uav_attachment: " << request->uav_attachment.c_str()
          // << std::endl;
          if (memcmp(hashstring, request->uav_attachment.c_str(),
                     strlen(hashstring)) == 0) {
            RCLCPP_INFO(this->get_logger(), "(3)无人机%s号验签成功.",
                        request->uav_id.c_str());
            // // InforLog("log/AuthDis.log", "(3)无人机%s号验签成功.",
            // request->uav_id.c_str());
            response->gc_id = Gc_Id;
            response->service_step_id = 4;
          } else {
            RCLCPP_INFO(this->get_logger(), "(3)无人机%s号验签失败.",
                        request->uav_id.c_str());
            // // InforLog("log/AuthDis.log", "(3)无人机%s号验签失败.",
            // request->uav_id.c_str());

            RCLCPP_INFO(this->get_logger(), "(98)验签失败，退出流程.");
            // // InforLog("log/AuthDis.log", "(98)验签失败，退出流程.");

            // 更新流程编码
            response->service_step_id = 98;
          }
          break;

          // 4.小型无人机向地面控制站发送确认消息
          // FlagLoca : = "04"
          // Content : = "qgcverify" ;
          // SendBuf = [] byte(FlagLoca + Content);
          // conn.Write(SendBuf);
          // // // InforLog("log/AuthDis.log", "无人机",
          // "(7)向地面控制站发送身份确认消息") ;

        case 5:
          RCLCPP_INFO(this->get_logger(),
                      "(5)正在进行无人机%s号对地面站%s号的第%d步认证.", Gc_Id,
                      request->uav_id.c_str(), request->service_step_id);
          // // InforLog("log/AuthDis.log",
          // "(5)正在进行无人机%s号对地面站%s号的第%s步认证.", Gc_Id.c_str(),
          // request->uav_id.c_str(), request->service_step_id.c_str());

          RCLCPP_INFO(this->get_logger(),
                      "(5)地面站%s号从无人机%s号收到随机字符串{%s}.", Gc_Id,
                      request->uav_id.c_str(), request->uav_attachment.c_str());
          // // InforLog("log/AuthDis.log",
          // "(5)地面站%s号从无人机%s号收到随机字符串：%s.", Gc_Id.c_str(),
          // request->uav_id.c_str(), request->uav_attachment.c_str());

          //------------------------------------------------------------
          // tempchar = request->uav_attachment.c_str() + Gc_Id.c_str();
          // hashValue = sm3.Sm3Sum([] byte(tempchar))
          std::strcpy(hashstring, Gc_Id);
          // std::cout << "hashstring output: " << hashstring << std::endl;
          std::strcat(hashstring, request->uav_attachment.c_str());
          // std::cout << "hashstring output: " << hashstring << std::endl;
          sm3((unsigned char *)hashstring, sizeof(hashstring),
              (unsigned char *)hashstring);

          //------------------------------------------------------------

          RCLCPP_INFO(this->get_logger(),
                      "(5)地面站%s号向无人机%s号发送SM3哈希串：%s.", Gc_Id,
                      request->uav_id.c_str(), hashstring);
          // // InforLog("log/AuthDis.log",
          // "(5)地面站%s号向无人机%s号发送SM3哈希串：%s.", Gc_Id.c_str(),
          // request->uav_id.c_str(), hashValue);
          response->gc_id = Gc_Id;
          response->gc_attachment = hashstring;

          // fmt.Printf("Recived from qgc RandomString:%s\n", data [2:n])
          // 	// // InforLog("log/AuthDis.log", "无人机",
          // "(10)接收到地面控制站随机字符串:" + data [2:n])
          // 	// 6.小型无人机向地面控制站发送签名值
          // 	FromQgcRandomstring = data [2:n] hashValue : = sm3.Sm3Sum([]
          // byte(FromQgcRandomstring)) 													   hashValueTemp : = [] byte(hashValue[:])

          // 																		   UavSign,
          // 	err : = UavPriv.Sign(rand.Reader, hashValueTemp, nil) // 签名
          // 			if err != nil{
          // 	fmt.Println(err)} FlagLoca : = "06" SendBuf = [] byte(FlagLoca +
          // string(UavSign)) conn.Write(SendBuf) fmt.Printf("sent to qgc
          // signValue\n") // // InforLog("log/AuthDis.log", "无人机",
          // "(11)向地面控制站发送该随机字符串的签名值")
          response->service_step_id = 6;
          break;

        case 7:
          RCLCPP_INFO(this->get_logger(), "(7)无人机%s号验签成功，是可信实体",
                      request->uav_id.c_str());
          rclcpp::shutdown();
          break;

        case 98:
          RCLCPP_INFO(this->get_logger(), "(98)验签失败，退出流程.");
          rclcpp::shutdown();
          break;
          // // InforLog("log/AuthDis.log", "(98)验签失败，退出流程.");

        case 99:
          RCLCPP_INFO(this->get_logger(), "(99)非法请求，请检查对象.");
          rclcpp::shutdown();
          break;
          // InforLog("log/AuthDis.log", "(99)非法请求，请检查对象.");
        }
      }
    } else {
      response->service_step_id = 99;
      RCLCPP_INFO(this->get_logger(),
                  "(99)收到一个非法请求，消息来自标识为%s的非无人机实体",
                  request->status.c_str());
      rclcpp::shutdown();
      // InforLog("log/AuthDis.log",
      // "(99)收到一个非法请求，消息来自标识为%s的地面控制站",
      // request->status.c_str());
    }
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<Gc>();
  // 把节点的执行器变成多线程执行器, 避免死锁
  rclcpp::executors::MultiThreadedExecutor exector;
  exector.add_node(node);
  exector.spin();
  rclcpp::shutdown();
  return 0;
}
