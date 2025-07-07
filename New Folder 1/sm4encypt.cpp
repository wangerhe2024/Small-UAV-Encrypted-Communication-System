#define uchar unsigned char

#include "rclcpp/rclcpp.hpp"
#include "sm4enc_service/srv/sm4enc.hpp"
#include <iostream>
#include <vector>
// 引入SM4加密库
#include "gmssl/sm4.h"
#include "gmssl/hex.h"

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

//创建一个类节点，名字叫做Organization,继承自Node.
class Sm4encypt : public rclcpp::Node 
{

public:
    Sm4encypt() : Node("Sm4encypt")
    {
        // 热心组织的自我介绍
        RCLCPP_INFO(this->get_logger(), "sm4加密节点上线");
        // 实例化回调组, 作用为避免死锁(请自行百度ROS2死锁)
        callback_group_Sm4encypt = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        // 实例化发汉堡的的服务
        Sm4encypt_Server = this->create_service<sm4enc_service::srv::Sm4enc>("Sm4enc",
                                    std::bind(&Sm4encypt::Sm4encypt_callback,this,_1,_2),
                                    rmw_qos_profile_services_default,
                                    callback_group_Sm4encypt);
    }

    void SM4encrypt(const std::vector<uchar> &plain_data, const std::string &key, std::vector<uchar> &cipher_data)
    {
        SM4_KEY sm4_key;
        uint8_t key_bytes[16] = {0};
        size_t key_len;

        hex_to_bytes(key.c_str(), key.length(), key_bytes, &key_len);
        sm4_set_encrypt_key(&sm4_key, key_bytes);

        uint8_t ctr[16] = {0};
        cipher_data.resize(plain_data.size());
        sm4_ctr_encrypt(&sm4_key, ctr, plain_data.data(), plain_data.size(), cipher_data.data());
    }

private:
    std::vector<uchar>  plainbuf;
    std::vector<uchar>  cipyerbuf;

    // 声明一个服务回调组
    rclcpp::CallbackGroup::SharedPtr callback_group_Sm4encypt;

    // 声明一个服务端
    rclcpp::Service<sm4enc_service::srv::Sm4enc>::SharedPtr Sm4encypt_Server;

    // 声明一个回调函数，当收到要汉堡请求时调用该函数
    void Sm4encypt_callback(const sm4enc_service::srv::Sm4enc::Request::SharedPtr request,
                               const sm4enc_service::srv::Sm4enc::Response::SharedPtr response)
    {
        RCLCPP_INFO(this->get_logger(), "来自%s的请求", request->node.data.c_str());
        if (request->upkey)
        {
            SM4_key  = request->sm4key;
            RCLCPP_INFO(this->get_logger(), "加密密钥更新为%s", request->sm4key.data.c_str());
            response->success = 2;
        }
        else 
        {   plainbuf.assign(request->plaintext.begin(),request->plaintext.end());
            SM4encrypt(plainbuf, SM4_key, cipyerbuf);
            response->ciphertext.insert(response->ciphertext.begin(), cipyerbuf.begin(), cipyerbuf.end());
            response->success = 1;
        }
    }

    std::string SM4_key = "fedcba98765432100123456789abcdef";
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<Sm4encypt>();
    // 把节点的执行器变成多线程执行器, 避免死锁
    rclcpp::executors::MultiThreadedExecutor exector;
    exector.add_node(node);
    exector.spin();
    rclcpp::shutdown();
    return 0;
}
