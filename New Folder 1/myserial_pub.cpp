// #include "rclcpp/rclcpp.hpp"
// #include "std_msgs/msg/string.hpp"

// #include <serial/serial.h>

// #define BAUDRATE 115200

// class Serial_read_node : public rclcpp::Node
// {
// public:
//     Serial_read_node(const std::string &name) : Node(name)
//     {
//         RCLCPP_INFO(this->get_logger(), "This is Tree's Test");

//         serial_publisher_ = this->create_publisher<std_msgs::msg::String>("serial_msg", 10);

//         timer_ = this->create_wall_timer(std::chrono::milliseconds(1000), std::bind(&Serial_read_node::timer_callback, this));

//         serial_Init();
//     }

// private:
//     serial::Serial ros_ser;

//     rclcpp::TimerBase::SharedPtr timer_;

//     rclcpp::Publisher<std_msgs::msg::String>::SharedPtr serial_publisher_;

//     std_msgs::msg::String msg_s;

//     void timer_callback()
//     {
//         msg_s.data = "Waiting data...\r\n";
//         size_t n = ros_ser.available();
//         if (n != 0)
//         {
//             // std_msgs::msg::String msg_s;
//             msg_s.data = ros_ser.read((ros_ser.available()>20) ? ros_ser.available() : 20);
//             RCLCPP_INFO(this->get_logger(), "Publisher:%s", msg_s.data.c_str());

//         }
//         else
//         {
//             RCLCPP_INFO(this->get_logger(), "Serial:%s", msg_s.data.c_str());

//             // ros_ser.write(message.data.c_str());

//         }

//         serial_publisher_->publish(msg_s);
//     }

//     void serial_Init()
//     {

//         serial::Timeout to = serial::Timeout::simpleTimeout(1000);
//         serial::parity_t pt = serial::parity_t::parity_none;
//         serial::bytesize_t bt = serial::bytesize_t::eightbits;
//         serial::flowcontrol_t ft = serial::flowcontrol_t::flowcontrol_none;
//         serial::stopbits_t st = serial::stopbits_t::stopbits_one;

//         // ros_ser.setPort("/dev/ttyS0");
//         ros_ser.setPort("/dev/pts/21"); // TEST
//         ros_ser.setBaudrate(BAUDRATE);
//         ros_ser.setTimeout(to);
//         ros_ser.setParity(pt);
//         ros_ser.setBytesize(bt);
//         ros_ser.setFlowcontrol(ft);
//         ros_ser.setStopbits(st);

//         try
//         {
//             ros_ser.open();
//         }
//         catch (const std::exception &e)
//         {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open the serial port.");
//             // return 0;
//         }
//     }
// };

// int main(int argc, char **argv)
// {

//     // auto logger = rclcpp::get_logger("my_logger");

//     rclcpp::init(argc, argv);
//     auto node = std::make_shared<Serial_read_node>("Serial_read_node");

//     rclcpp::spin(node);

//     rclcpp::shutdown();
//     return 0;
// }

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "sm4enc_service/srv/sm4enc.hpp"

#include <serial/serial.h>

#define BAUDRATE 115200

using namespace std::chrono_literals;

using std::placeholders::_1;

class Serial_read_node : public rclcpp::Node
{
public:
    Serial_read_node(const std::string &name) : Node(name)
    {
        RCLCPP_INFO(this->get_logger(), "This is Tree's Test");

        serial_publisher_ = this->create_publisher<std_msgs::msg::String>("serial_msg", 10);

        Serial_Client = this->create_client<sm4enc_service::srv::Sm4enc>("Sm4enc");

        timer_ = this->create_wall_timer(std::chrono::milliseconds(1000), std::bind(&Serial_read_node::timer_callback, this));

        serial_Init();
        enc_init();
    }

private:
    serial::Serial ros_ser;

    rclcpp::TimerBase::SharedPtr timer_;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr serial_publisher_;

    rclcpp::Client<sm4enc_service::srv::Sm4enc>::SharedPtr Serial_Client;

    std_msgs::msg::String msg_s;

    void Serialenc_callback(rclcpp::Client<sm4enc_service::srv::Sm4enc>::SharedFuture response)
    {
        auto result = response.get();
        auto request = std::make_shared<sm4enc_service::srv::Sm4enc::Request>();

        if (result->success)
        {
            msg_s.data = result->ciphertext;
        }
        RCLCPP_INFO(this->get_logger(), "发布加密数据:%s", msg_s.data.c_str());
        serial_publisher_->publish(msg_s);
    }

    void enc_init()
    {
        // 等待加密服务端上线
        while (!Serial_Client->wait_for_service(1s))
        {
            // 等待时检测rclcpp的状态
            if (!rclcpp::ok())
            {
                // 检测到Ctrl+C直接退出
                RCLCPP_ERROR(this->get_logger(), "加密节点连接失败");
                rclcpp::shutdown();
                return;
            }
            // 否则一直等
            RCLCPP_INFO(this->get_logger(), "等待加密节点上线");
        }
        // 格式正确, 获取参数, 放入request中
        RCLCPP_INFO(this->get_logger(), "加密节点连接成功");
    }

    void timer_callback()
    {
        msg_s.data = "等待串口数据...\r\n";
        size_t n = ros_ser.available();
        if (n != 0)
        {
            // std_msgs::msg::String msg_s;
            msg_s.data = ros_ser.read((ros_ser.available() > 20) ? ros_ser.available() : 20);
            RCLCPP_INFO(this->get_logger(), "采集到数据:%s", msg_s.data.c_str());

            //***************发送加密请求 */
            // 构造认证请求
            auto request = std::make_shared<sm4enc_service::srv::Sm4enc::Request>();

            request->node = node_name;
            if (SM4_key != buf_key)
            {
                request->upkey = 1;
                buf_key = SM4_key;
            }
            else
            {

                request->upkey = 0;
            }
            request->sm4key = buf_key;
            request->plaintext = msg_s.data;

            // 发送异步请求，然后等待返回，返回时调用回调函数
            Serial_Client->async_send_request(request, std::bind(&Serial_read_node::Serialenc_callback, this, _1));
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "串口:%s", msg_s.data.c_str());
            // ros_ser.write(message.data.c_str());
        }
    }

    void serial_Init()
    {

        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        serial::parity_t pt = serial::parity_t::parity_none;
        serial::bytesize_t bt = serial::bytesize_t::eightbits;
        serial::flowcontrol_t ft = serial::flowcontrol_t::flowcontrol_none;
        serial::stopbits_t st = serial::stopbits_t::stopbits_one;

        // ros_ser.setPort("/dev/ttyS0");
        ros_ser.setPort("/dev/pts/8"); // TEST
        ros_ser.setBaudrate(BAUDRATE);
        ros_ser.setTimeout(to);
        ros_ser.setParity(pt);
        ros_ser.setBytesize(bt);
        ros_ser.setFlowcontrol(ft);
        ros_ser.setStopbits(st);

        try
        {
            ros_ser.open();
        }
        catch (const std::exception &e)
        {
            RCLCPP_ERROR(this->get_logger(), "Failed to open the serial port.");
            // return 0;
        }
    }

    std::string SM4_key = "fedcba98765432100123456789abcdef";
    std::string buf_key = "fedcba98765432100123456789abcdef";
    std::string node_name = "Serial_read_node";
};

int main(int argc, char **argv)
{

    // auto logger = rclcpp::get_logger("my_logger");

    rclcpp::init(argc, argv);
    auto node = std::make_shared<Serial_read_node>("Serial_read_node");

    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}