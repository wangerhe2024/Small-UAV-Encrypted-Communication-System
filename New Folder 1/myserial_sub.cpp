#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include <serial/serial.h>

#define BAUDRATE 115200

class serial_sub_node : public rclcpp::Node
{
public:
    serial_sub_node(std::string name) : Node(name)
    {
        //serial_Init();
        serial_subscriber_ = this->create_subscription<std_msgs::msg::String>("serial_msg", 10, std::bind(&serial_sub_node::command_callback, this, std::placeholders::_1));
    }

private:
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr serial_subscriber_;

    void command_callback(const std_msgs::msg::String::SharedPtr msg_s)
    {
        //ros_ser.write(msg_s->data.c_str());
        RCLCPP_INFO(this->get_logger(), "Publisher:%s", msg_s->data.c_str());
    }

    void serial_Init()
    {
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        serial::parity_t pt = serial::parity_t::parity_none;
        serial::bytesize_t bt = serial::bytesize_t::eightbits;
        serial::flowcontrol_t ft = serial::flowcontrol_t::flowcontrol_none;
        serial::stopbits_t st = serial::stopbits_t::stopbits_one;

        // ros_ser.setPort("/dev/ttyS0");
        ros_ser.setPort("/dev/ttyS2"); // TEST
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
            // return -1;
            rclcpp::shutdown();
        }

        /**/
    }

    serial::Serial ros_ser;
};

int main(int argc, char **argv)
{

    // auto logger = rclcpp::get_logger("my_logger");

    rclcpp::init(argc, argv);
    auto node = std::make_shared<serial_sub_node>("tree_serial_sub_node");

    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}