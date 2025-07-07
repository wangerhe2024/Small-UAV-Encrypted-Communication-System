#include "rclcpp/rclcpp.hpp"
#include "uav_identify_service/srv/identify.hpp"
#include <iostream>
#include <string>
#include "gmssl/sm2.h"
#include "gmssl/sm3.h"
#include "gmssl/rand.h"

using namespace std::chrono_literals;

using std::placeholders::_1;

// 创建一个类节点，名字叫做Uav,继承自Node.
class Uav : public rclcpp::Node
{

public:
    // 构造函数
    Uav() : Node("Uav"), step(1)
    {
        // 打印一句自我介绍
        RCLCPP_INFO(this->get_logger(), "无人机上线，正在寻找地面站.");
        // 实例化客户端, 指明客户端的接口类型，同时指定要请求的服务的名称
        Uav_Client = this->create_client<uav_identify_service::srv::Identify>("Identify");
    }

    int Link_to_Gc(int argc, char **argv)
    {
        // 输入参数格式错误的时候报错并退出程序
        if (argc != 2)
        {
            RCLCPP_ERROR(this->get_logger(), "请设定无人机ID, 格式为ros2 run uav_identify 3(无人机ID)");
            rclcpp::shutdown();
            return 1;
        }
        else
        {

            RCLCPP_INFO(this->get_logger(), "连接地面站");

            // 构造请求
            auto request = std::make_shared<uav_identify_service::srv::Identify::Request>();

            // 等待服务端上线
            while (!Uav_Client->wait_for_service(1s))
            {
                // 等待时检测rclcpp的状态
                if (!rclcpp::ok())
                {
                    // 检测到Ctrl+C直接退出
                    RCLCPP_ERROR(this->get_logger(), "连接被取消");
                    rclcpp::shutdown();
                    return 1;
                }
                // 否则一直等
                RCLCPP_INFO(this->get_logger(), "等待地面站上线");
            }
            // 格式正确, 获取参数, 放入request中
            RCLCPP_INFO(this->get_logger(), "连接成功");

            //Uav_Id = argv[1];
            strcpy(Uav_Id, argv[1]);

            // 构造认证请求
            request->status = "1";              // 标记无人机标识
            request->uav_id = Uav_Id;           // 设定无人机ID
            request->service_step_id = 0;       // 刷新服务流程编码
            request->uav_attachment = "Noload"; // 空负载

            // 发送异步请求，然后等待返回，返回时调用回调函数
            Uav_Client->async_send_request(request, std::bind(&Uav::Uav_callback, this, _1));

            RCLCPP_INFO(this->get_logger(), "(0)无人机认证请求已发送");

            return true;
        }
    }

    void sm3(unsigned char *input, int ilen, unsigned char output[32])
    {
        SM3_CTX ctx;

        sm3_init(&ctx);
        sm3_update(&ctx, input, ilen);
        sm3_finish(&ctx, output);
    }

    void GenerateRandomString(char *output, int outlen = 64)
	{
		uint8_t tempbyte;
		const char hex_chars[] = "0123456789ABCDEF";  // 16进制字符集
		// std::cout << "outlen: " << outlen << std::endl;
		for (int i = 0; i < outlen - 1; i++)
		{
			rand_bytes(&tempbyte, sizeof(tempbyte));
			//output[i] = char(tempbyte % 94 + 33); // 生成可打印字符
			output[i] = hex_chars[tempbyte % 16];     // 将字节映射到16进制字符
			//std::cout << "output["<< i << "]:" << output[i] << std::endl;
		}

		output[outlen - 1] = '\0'; // 确保字符串以 null 终止
		//std::cout << "Random String output: " << output << std::endl;
	}

private:
    size_t step;

    char Uav_Id[8];
    uint8_t temp, hashValue;
    char hashstring[100], UavRandomstring[64];

    // 创建一个客户端
    rclcpp::Client<uav_identify_service::srv::Identify>::SharedPtr Uav_Client;

    // 创建接收到的回调函数
    void Uav_callback(rclcpp::Client<uav_identify_service::srv::Identify>::SharedFuture response)
    {
        auto result = response.get();
        auto request = std::make_shared<uav_identify_service::srv::Identify::Request>();

        if (result->gc_id == "00") // 如果确实是地面站
        {
            switch (result->service_step_id)
            {
            case 2:
                // result->gc_attachment.c_str(); // 取回随机数
                RCLCPP_INFO(this->get_logger(), "(2)无人机%s号收到地面站%s号发送的随机字符串%s", Uav_Id, result->gc_id.c_str(), result->gc_attachment.c_str());
                // // InforLog("log/AuthDis.log", "(2)无人机%s号已收到地面站%s号发送的随机字符串%s", Uav_id.c_str(), result->gc_id.c_str(), result->gc_attachment);

                //-------------------------------------------------------------
                // tempchar = result->gc_attachmentt.c_str() + Uav_Id.c_str();
                // hashValue = sm3.Sm3Sum([] byte(tempchar));
                // hashValue = temp + Uav_Id;
                // sm3(&hashValue, sizeof(hashValue), &hashValue);

                std::strcpy(hashstring, Uav_Id);
                // std::cout << "hashstring output:" << hashstring << std::endl;
                std::strcat(hashstring, result->gc_attachment.c_str());
                // std::cout << "hashstring output:" << hashstring << std::endl;
                sm3((unsigned char *)hashstring, sizeof(hashstring), (unsigned char *)hashstring);
                // std::cout << "hashstring output:" << hashstring << std::endl;
                //-------------------------------------------------------------

                request->status = "1";                // 标记无人机标识
                request->uav_id = Uav_Id;             // 设定无人机ID
                request->service_step_id = 3;         // 刷新服务流程编码
                request->uav_attachment = hashstring; // 负载

                RCLCPP_INFO(this->get_logger(), "(2)无人机%s号向地面站%s号发送SM3哈希串:{%s}", Uav_Id, result->gc_id.c_str(), hashstring);
                // // InforLog("log/AuthDis.log", "(2)无人机%s号向地面站%s号发送SM3哈希串%s", Uav_id.c_str(), result->gc_id.c_str(), result->gc_attachment);

                // 发送异步请求，然后等待返回，返回时调用回调函数
                Uav_Client->async_send_request(request, std::bind(&Uav::Uav_callback, this, _1));

                break;

            case 4:
                // auto request = std::make_shared<uav_identify_service::srv::Identify::Request>();
                RCLCPP_INFO(this->get_logger(), "(4)无人机%s号验签成功，开始对地面站%s进行身份验证.", Uav_Id, result->gc_id.c_str());
                // // InforLog("log/AuthDis.log", "(4)无人机%s号验签成功，开始对地面站%s进行身份验证.", Uav_id.c_str(), result->gc_id.c_str());

                //------------------------------------
                // 产生随机字符串
                // GcRandomstring = GenerateRandomString(20);
                // rand_bytes(&UavRandomstring, 64);
                GenerateRandomString(UavRandomstring, 64);
                // UavRandomstring = "11111111";

                //-------------------------------------------

                RCLCPP_INFO(this->get_logger(), "(4)产生随机字符串%s", UavRandomstring);
                // // // InforLog("log/AuthDis.log",

                // 发送随机字符串
                request->uav_attachment = UavRandomstring;
                request->status = "1";        // 标记无人机标识
                request->uav_id = Uav_Id;     // 设定无人机ID
                request->service_step_id = 5; // 刷新服务流程编码

                RCLCPP_INFO(this->get_logger(), "(4)无人机%s号向地面站%s号发送随机字符串%s", Uav_Id, result->gc_id.c_str(), UavRandomstring);
                // // InforLog("log/AuthDis.log", "(4)无人机%s号向地面站%s号发送随机字符串%s", Uav_id.c_str(), result->gc_id.c_str(), result->gc_attachment);

                // 发送异步请求，然后等待返回，返回时调用回调函数
                Uav_Client->async_send_request(request, std::bind(&Uav::Uav_callback, this, _1));

                break;

            case 6:
                // auto request = std::make_shared<uav_identify_service::srv::Identify::Request>();

                //------------------------------------------------------------
                // tempchar = GcRandomstring + result->gc_id.c_str();
                // hashValue = sm3.Sm3Sum([] byte(tempchar));
                // hashValue = UavRandomstring + result->gc_id;
                // sm3(&hashValue, 96, &hashValue);
                std::strcpy(hashstring, result->gc_id.c_str());
                std::strcat(hashstring, UavRandomstring);
                sm3((unsigned char *)hashstring, sizeof(hashstring), (unsigned char *)hashstring);
                //------------------------------------------------------------

                //------------------------------------------------------------
                // if (QgcUavPub.Verify(hashValue, [] byte(result->gc_attachmentt)))
                if (memcmp(hashstring, result->gc_attachment.c_str(), strlen(hashstring)) == 0)
                {
                    RCLCPP_INFO(this->get_logger(), "(6)地面站%s号验签成功，是可信实体.", result->gc_id.c_str());
                    // // InforLog("log/AuthDis.log", "(6)地面站%s号验签成功.", result->gc_id.c_str());



                    request->status = "1";          // 标记无人机标识
                    request->uav_id = Uav_Id;     // 设定无人机ID
                    request->service_step_id = 7; // 刷新服务流程编码

                    // 发送异步请求，然后等待返回，返回时调用回调函数
                    Uav_Client->async_send_request(request, std::bind(&Uav::Uav_callback, this, _1));
                    rclcpp::shutdown();
                }
                else
                {

                    RCLCPP_INFO(this->get_logger(), "(97)地面站%s号验签失败.", result->gc_id.c_str());
                    // // InforLog("log/AuthDis.log", "(97)地面站%s号验签失败.", result->gc_id.c_str());

                    RCLCPP_INFO(this->get_logger(), "(98)验签失败，退出流程.");
                    // // InforLog("log/AuthDis.log", "(98)验签失败，退出流程.");

                    // 更新流程编码
                    request->status = "1";          // 标记无人机标识
                    request->uav_id = Uav_Id;     // 设定无人机ID
                    request->service_step_id = 98;
                    // 发送异步请求，然后等待返回，返回时调用回调函数
                    Uav_Client->async_send_request(request, std::bind(&Uav::Uav_callback, this, _1));
                    rclcpp::shutdown();
                }
                break;

            default:
                RCLCPP_INFO(this->get_logger(), "(98)验签失败，退出流程.");
                // // InforLog("log/AuthDis.log", "(98)验签失败，退出流程.");
                rclcpp::shutdown();
                break;
            }
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "(99)非法请求，请检查对象.");
            rclcpp::shutdown();
            // // InforLog("log/AuthDis.log", "(99)非法请求，请检查对象.");
        }
    }
    // // 使用response的get()获取
    // auto result = response.get();
    // // 如果确实是Poorer, 则领取成功
    // if(result->success == true)
    // {
    //     RCLCPP_INFO(this->get_logger(), "成功领取%d个汉堡", result->num_of_hamburger);
    //     rclcpp::shutdown();
    // }
    // // 不是Poorer或者汉堡数量不够, 则领取失败
    // else
    // {
    //     RCLCPP_INFO(this->get_logger(), "领取汉堡失败, 原因可能为: 1.你不是Poorer 2.汉堡不够了");
    //     rclcpp::shutdown();
    // }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    // 产生一个Uav的节点
    auto node = std::make_shared<Uav>();
    node->Link_to_Gc(argc, argv);
    // 运行节点，并检测rclcpp状态
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
