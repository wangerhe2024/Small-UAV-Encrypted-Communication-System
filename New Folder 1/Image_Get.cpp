// #include <iostream>
// #include "rclcpp/rclcpp.hpp"
// // #include <ros/ros.h>

// #include <cv_bridge/cv_bridge.h>               // 将ROS下的sensor_msgs/Image消息类型转化为cv::Mat数据类型
// #include <sensor_msgs/image_encodings.hpp>     // ROS下对图像进行处理
// #include <image_transport/image_transport.hpp> // 用来发布和订阅图像信息
// #include "sensor_msgs/msg/compressed_image.hpp"

// #include <opencv2/core/core.hpp>
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgproc/imgproc.hpp>
// #include <opencv2/videoio.hpp>

// int main(int argc, char **argv)
// {
//     // rclcpp::Node("imageGet_node");
//     rclcpp::init(argc, argv); // ros初始化，定义节点名为imageGet_node
//     // rclcpp::Handle nh;                      // 定义ros句柄
//     rclcpp::NodeOptions options;
//     rclcpp::Node::SharedPtr node = rclcpp::Node::make_shared("Image_Pub", options);
//     image_transport::ImageTransport it(node);                               //  类似ROS句柄
//     image_transport::ImageTransport it(node);
//     image_transport::Publisher image_pub = it.advertise("/cameraImage", 1); // 发布话题名/cameraImage

//     rclcpp::Rate loop_rate(200); // 设置刷新频率，Hz

//     cv::Mat imageRaw;            // 原始图像保存
//     cv::VideoCapture capture(0); // 创建摄像头捕获，并打开摄像头0(一般是0,2....)

//     if (capture.isOpened() == 0) // 如果摄像头没有打开
//     {
//         std::cout << "Read camera failed!" << std::endl;
//         return -1;
//     }

//     cv_bridge::CvImage img_bridge;
//     sensor_msgs::msg::Image msg;
//     sensor_msgs::msg::CompressedImage compressed_msg;
//     std_msgs::msg::Header header;

//     while (rclcpp::ok())
//     {
//         capture.read(imageRaw);                                    // 读取当前图像到imageRaw
//         cv::imshow("veiwer", imageRaw);                            // 将图像输出到窗口
//         img_bridge = cv_bridge::CvImage(header, "bgr8", imageRaw); // 图像格式转换
//         // img_bridge.toImageMsg(msg);
//         img_bridge.toCompressedImageMsg(compressed_msg);
//         image_pub.publish(msg); // 发布图像信息
//         // rclcpp::spin(node);                  // 没什么卵用，格式像样
//         loop_rate.sleep();       // 照应上面设置的频率
//         if (cv::waitKey(2) >= 0) // 延时ms,按下任何键退出(必须要有waitKey，不然是看不到图像的)
//             break;
//     }
// }

// ros2 run image_transport republish compressed in/compressed:=compressed_image raw out:=image_raw
// #include <iostream>
// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// using namespace std::chrono_literals;

// class Image_Get : public rclcpp::Node
// {
// public:
//     Image_Get(int fps)
//         : Node("Image_Get"), count_(0), fps_(fps)
//     {
//         publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/cameraImage", 10);
//         timer_ = this->create_wall_timer(100ms, std::bind(&Image_Get::publishImage, this));

//         set_timer(fps_);

//         cap_ = cv::VideoCapture(0); // Open default camera

//         printf("record compressed image!\n");

//         if (!cap_.isOpened())
//         {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//         }
//     }

//     // 设置定时器
//     void set_timer(int fps)
//     {
//         auto period = std::chrono::milliseconds(1000 / fps); // 根据fps设置定时器周期
//         timer_ = this->create_wall_timer(period, std::bind(&Image_Get::publishImage, this));
//     }

// private:
//     void publishImage()
//     {
//         cv::Mat frame;
//         cap_ >> frame; // Capture a frame from the camera

//         if (frame.empty())
//         {
//             RCLCPP_ERROR(this->get_logger(), "Failed to capture frame");
//             return;
//         }
//         cv::Mat resized_frame;
//         cv::resize(frame, resized_frame, cv::Size(640, 480), cv::INTER_LINEAR);

//         std::vector<uchar> buf;
//         cv::imencode(".jpg", resized_frame, buf, {cv::IMWRITE_JPEG_QUALITY, 80}); // Adjust JPEG quality (0-100 scale)

//         sensor_msgs::msg::CompressedImage msg;
//         msg.format = "jpeg";
//         msg.data = buf;

//         publisher_->publish(msg);

//         count_++;
//         printf("record compressed image: %d\r", count_);
//     }

//     rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;
//     rclcpp::TimerBase::SharedPtr timer_;

//     cv::VideoCapture cap_;
//     int count_;
//     int fps_; // 帧率
// };

// int main(int argc, char **argv)
// {
//     rclcpp::init(argc, argv);

//     int fps = 30;
//     if (argc > 1)
//     {
//         fps = std::stoi(argv[1]); // 从命令行读取fps参数
//     }

//     auto node = std::make_shared<Image_Get>(fps);
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }

// #include <iostream>
// #include <thread>
// #include <mutex>
// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// class Image_Get : public rclcpp::Node {
// public:
//     Image_Get(int fps)
//         : Node("Image_Get"), stop_capture_(false), count_(0), fps_(fps) {
//         publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/cameraImage", 10);

//         // 打开摄像头
//         cap_ = cv::VideoCapture(0);
//         if (!cap_.isOpened()) {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//             throw std::runtime_error("Failed to open camera");
//         }

//         // 启动捕获线程
//         capture_thread_ = std::thread(&Image_Get::captureFrames, this);
//         // 启动发布线程
//         publish_thread_ = std::thread(&Image_Get::publishFrames, this);
//     }

//     ~Image_Get() {
//         // 停止捕获
//         stop_capture_ = true;

//         if (capture_thread_.joinable()) {
//             capture_thread_.join(); // 等待捕获线程结束
//         }
//         if (publish_thread_.joinable()) {
//             publish_thread_.join(); // 等待发布线程结束
//         }
//     }

// private:
//     // 捕获线程：捕获摄像头帧并存储在缓冲区中
//     void captureFrames() {
//         cv::Mat frame;
//         while (!stop_capture_) {
//             cap_ >> frame; // 从摄像头捕获一帧
//             if (frame.empty()) {
//                 RCLCPP_ERROR(this->get_logger(), "Failed to capture frame");
//                 continue;
//             }

//             // 锁住缓冲区，防止发布线程访问时数据不一致
//             std::lock_guard<std::mutex> lock(buffer_mutex_);
//             frame_buffer_ = frame.clone(); // 将当前帧复制到缓冲区中
//         }
//     }

//     // 发布线程：将缓冲区中的图像进行压缩并发布
//     void publishFrames() {
//         cv::Mat frame;
//         rclcpp::Rate loop_rate(fps_); // 根据fps设定发布频率

//         while (rclcpp::ok() && !stop_capture_) {
//             {
//                 // 加锁读取缓冲区
//                 std::lock_guard<std::mutex> lock(buffer_mutex_);
//                 if (frame_buffer_.empty()) {
//                     continue; // 缓冲区中没有数据，跳过
//                 }
//                 frame = frame_buffer_.clone(); // 将缓冲区中的数据拷贝出来
//             }

//             // 调整图像大小
//             //cv::Mat resized_frame;
//             //cv::resize(frame, resized_frame, cv::Size(640, 480), cv::INTER_LINEAR);

//             // 将图像压缩为JPEG格式
//             std::vector<uchar> buf;
//             //cv::imencode(".jpg", resized_frame, buf, {cv::IMWRITE_JPEG_QUALITY, 60});
//             cv::imencode(".jpg", frame, buf, {cv::IMWRITE_JPEG_QUALITY, 60});

//             // 创建压缩图像消息并发布
//             sensor_msgs::msg::CompressedImage msg;
//             msg.format = "jpeg";
//             msg.data = buf;

//             publisher_->publish(msg);

//             //count_++;
//             //RCLCPP_INFO(this->get_logger(), "Published frame: %d", count_);

//             loop_rate.sleep(); // 根据fps控制发布速率
//         }
//     }

//     rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;
//     cv::VideoCapture cap_;      // 摄像头
//     cv::Mat frame_buffer_;      // 共享缓冲区，保存最新的图像帧
//     std::mutex buffer_mutex_;   // 保护缓冲区的互斥锁
//     std::thread capture_thread_; // 捕获线程
//     std::thread publish_thread_; // 发布线程
//     bool stop_capture_;         // 用于停止捕获的标志
//     int count_; // 发布的帧计数
//     int fps_;   // 发布的帧率
// };

// int main(int argc, char **argv) {
//     rclcpp::init(argc, argv);

//     // 默认帧率设置为30，如果命令行参数提供了fps，则用用户提供的帧率
//     int fps = 30;
//     if (argc > 1) {
//         fps = std::stoi(argv[1]);  // 从命令行读取fps参数
//     }

//     auto node = std::make_shared<Image_Get>(fps);  // 将fps传递给节点
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }

// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <vector>
// #include <condition_variable>
// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// class Image_Get : public rclcpp::Node {
// public:
//     Image_Get(int fps)
//         : Node("Image_Get"), stop_capture_(false), count_(0), fps_(fps), max_queue_size_(10) {
//         publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/cameraImage", 10);

//         // 打开摄像头
//         cap_ = cv::VideoCapture(0);
//         if (!cap_.isOpened()) {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//             throw std::runtime_error("Failed to open camera");
//         }

//         // 启动三个线程：捕获、压缩、发布
//         capture_thread_ = std::thread(&Image_Get::captureFrames, this);
//         compress_thread_ = std::thread(&Image_Get::compressFrames, this);
//         publish_thread_ = std::thread(&Image_Get::publishFrames, this);
//     }

//     ~Image_Get() {
//         stop_capture_ = true; // 停止所有线程

//         // 唤醒所有线程防止死锁
//         cond_var_.notify_all();

//         if (capture_thread_.joinable()) {
//             capture_thread_.join(); // 等待捕获线程结束
//         }
//         if (compress_thread_.joinable()) {
//             compress_thread_.join(); // 等待压缩线程结束
//         }
//         if (publish_thread_.joinable()) {
//             publish_thread_.join(); // 等待发布线程结束
//         }
//     }

// private:
//     // 捕获线程：捕获摄像头帧并存储在未压缩队列中
//     void captureFrames() {
//         cv::Mat frame;
//         rclcpp::Rate capture_rate(fps_*3); // 控制捕获频率与FPS一致
//         while (!stop_capture_) {
//             cap_ >> frame; // 从摄像头捕获一帧
//             if (frame.empty()) {
//                 RCLCPP_ERROR(this->get_logger(), "Failed to capture frame");
//                 continue;
//             }

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 if (uncompressed_queue_.size() >= max_queue_size_) {
//                     cond_var_.wait(lock, [this]() { return uncompressed_queue_.size() < max_queue_size_ || stop_capture_; });
//                 }
//                 uncompressed_queue_.push(frame.clone()); // 将帧压入未压缩队列
//             }
//             cond_var_.notify_one(); // 通知压缩线程
//             capture_rate.sleep(); // 限制捕获频率
//         }
//     }

//     // 压缩线程：从未压缩队列中读取图像，压缩为JPEG并存储在压缩队列中
//     void compressFrames() {
//         while (!stop_capture_) {
//             cv::Mat frame;

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 cond_var_.wait(lock, [this]() { return !uncompressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && uncompressed_queue_.empty()) return; // 退出线程

//                 // 从未压缩队列中取出一帧
//                 frame = uncompressed_queue_.front();
//                 uncompressed_queue_.pop();
//             }

//             // 压缩为JPEG

//             cv::Mat resized_frame;
//             cv::resize(frame, resized_frame, cv::Size(640, 480), cv::INTER_LINEAR);
//             std::vector<uchar> buf;
//             cv::imencode(".jpg", resized_frame, buf, {cv::IMWRITE_JPEG_QUALITY, 60});

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 if (compressed_queue_.size() >= max_queue_size_) {
//                     cond_var_.wait(lock, [this]() { return compressed_queue_.size() < max_queue_size_ || stop_capture_; });
//                 }
//                 compressed_queue_.push(buf); // 将压缩后的图像压入压缩队列
//             }
//             cond_var_.notify_one(); // 通知发布线程
//         }
//     }

//     // 发布线程：从压缩队列中读取压缩图像并发布
//     void publishFrames() {
//         rclcpp::Rate loop_rate(fps_); // 根据fps设定发布频率

//         while (rclcpp::ok() && !stop_capture_) {
//             std::vector<uchar> buf;

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 cond_var_.wait(lock, [this]() { return !compressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && compressed_queue_.empty()) return; // 退出线程

//                 buf = compressed_queue_.front();
//                 compressed_queue_.pop(); // 从压缩队列中取出一帧
//             }

//             // 创建压缩图像消息并发布
//             sensor_msgs::msg::CompressedImage msg;
//             msg.format = "jpeg";
//             msg.data = buf;

//             publisher_->publish(msg);
//             //count_++;
//             //RCLCPP_INFO(this->get_logger(), "Published frame: %d", count_);

//             loop_rate.sleep(); // 根据fps控制发布速率
//         }
//     }

//     rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;
//     cv::VideoCapture cap_;       // 摄像头

//     // 未压缩图像队列
//     std::queue<cv::Mat> uncompressed_queue_;
//     std::mutex uncompressed_queue_mutex_;

//     // 压缩图像队列
//     std::queue<std::vector<uchar>> compressed_queue_;
//     std::mutex compressed_queue_mutex_;

//     std::condition_variable cond_var_; // 用于线程同步

//     std::thread capture_thread_;  // 捕获线程
//     std::thread compress_thread_; // 压缩线程
//     std::thread publish_thread_;  // 发布线程

//     bool stop_capture_; // 用于停止线程的标志
//     int count_;         // 发布的帧计数
//     int fps_;           // 发布的帧率

//     // 最大队列大小，防止过度积压导致内存问题
//     const size_t max_queue_size_;
// };

// int main(int argc, char **argv) {
//     rclcpp::init(argc, argv);

//     // 默认帧率设置为30，如果命令行参数提供了fps，则用用户提供的帧率
//     int fps = 30;
//     if (argc > 1) {
//         fps = std::stoi(argv[1]);  // 从命令行读取fps参数
//     }

//     auto node = std::make_shared<Image_Get>(fps);  // 将fps传递给节点
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }

// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <vector>
// #include <condition_variable>
// #include <chrono>
// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// class Image_Get : public rclcpp::Node {
// public:
//     Image_Get(int fps)
//         : Node("Image_Get"), stop_capture_(false), fps_(fps), max_queue_size_(10) {
//         publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/cameraImage", 10);

//         // 打开摄像头，并设置捕获的帧率
//         cap_ = cv::VideoCapture(0, cv::CAP_ANY);
//         cap_.set(cv::CAP_PROP_FPS, fps);  // 设置摄像头捕获帧率
//         if (!cap_.isOpened()) {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//             throw std::runtime_error("Failed to open camera");
//         }

//         // 启动线程：捕获、两个压缩、发布
//         capture_thread_ = std::thread(&Image_Get::captureFrames, this);
//         compress_thread_1_ = std::thread(&Image_Get::compressFrames, this);
//         compress_thread_2_ = std::thread(&Image_Get::compressFrames, this);
//         publish_thread_ = std::thread(&Image_Get::publishFrames, this);
//     }

//     ~Image_Get() {
//         stop_capture_ = true;
//         cond_var_.notify_all();

//         if (capture_thread_.joinable()) capture_thread_.join();
//         if (compress_thread_1_.joinable()) compress_thread_1_.join();
//         if (compress_thread_2_.joinable()) compress_thread_2_.join();
//         if (publish_thread_.joinable()) publish_thread_.join();
//     }

// private:
//     void captureFrames() {
//         cv::Mat frame;
//         while (!stop_capture_) {
//             auto start = std::chrono::high_resolution_clock::now();

//             if (!cap_.read(frame)) {
//                 RCLCPP_ERROR(this->get_logger(), "Failed to capture frame");
//                 continue;
//             }

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 if (uncompressed_queue_.size() >= max_queue_size_) {
//                     cond_var_.wait(lock, [this]() { return uncompressed_queue_.size() < max_queue_size_ || stop_capture_; });
//                 }
//                 uncompressed_queue_.push(frame.clone());
//             }
//             cond_var_.notify_all();

//             auto end = std::chrono::high_resolution_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Capture time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     // 压缩线程：从未压缩队列中读取图像，压缩为JPEG并存储在压缩队列中
//     void compressFrames() {
//         while (!stop_capture_) {
//             cv::Mat frame;

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 cond_var_.wait(lock, [this]() { return !uncompressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && uncompressed_queue_.empty()) return; // 退出线程

//                 // 从未压缩队列中取出一帧
//                 frame = uncompressed_queue_.front();
//                 uncompressed_queue_.pop();
//             }

//             auto start = std::chrono::high_resolution_clock::now();  // 开始计时

//             // 压缩为JPEG
//             cv::Mat resized_frame;
//             cv::resize(frame, resized_frame, cv::Size(640, 480), cv::INTER_LINEAR);
//             std::vector<uchar> buf;
//             cv::imencode(".jpg", resized_frame, buf, {cv::IMWRITE_JPEG_QUALITY, 60});

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 if (compressed_queue_.size() >= max_queue_size_) {
//                     cond_var_.wait(lock, [this]() { return compressed_queue_.size() < max_queue_size_ || stop_capture_; });
//                 }
//                 compressed_queue_.push(buf); // 将压缩后的图像压入压缩队列
//             }
//             cond_var_.notify_one(); // 通知发布线程

//             auto end = std::chrono::high_resolution_clock::now();  // 结束计时
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Compress thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     // 发布线程：从压缩队列中读取压缩图像并发布
//     void publishFrames() {
//         rclcpp::Rate loop_rate(fps_); // 根据fps设定发布频率

//         while (rclcpp::ok() && !stop_capture_) {
//             std::vector<uchar> buf;

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 cond_var_.wait(lock, [this]() { return !compressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && compressed_queue_.empty()) return; // 退出线程

//                 buf = compressed_queue_.front();
//                 compressed_queue_.pop(); // 从压缩队列中取出一帧
//             }

//             auto start = std::chrono::high_resolution_clock::now();  // 开始计时

//             // 创建压缩图像消息并发布
//             sensor_msgs::msg::CompressedImage msg;
//             msg.format = "jpeg";
//             msg.data = buf;

//             publisher_->publish(msg);
//             loop_rate.sleep(); // 根据fps控制发布速率

//             auto end = std::chrono::high_resolution_clock::now();  // 结束计时
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Publish thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;
//     cv::VideoCapture cap_;
//     std::queue<cv::Mat> uncompressed_queue_;
//     std::mutex uncompressed_queue_mutex_;
//     std::queue<std::vector<uchar>> compressed_queue_;
//     std::mutex compressed_queue_mutex_;
//     std::condition_variable cond_var_;

//     std::thread capture_thread_;
//     std::thread compress_thread_1_;
//     std::thread compress_thread_2_;
//     std::thread publish_thread_;

//     bool stop_capture_;
//     int fps_;
//     const size_t max_queue_size_;
// };

// int main(int argc, char **argv) {
//     rclcpp::init(argc, argv);

//     // 默认帧率设置为30，如果命令行参数提供了fps，则用用户提供的帧率
//     int fps = 30;
//     if (argc > 1) {
//         fps = std::stoi(argv[1]);  // 从命令行读取fps参数
//     }

//     auto node = std::make_shared<Image_Get>(fps);  // 将fps传递给节点
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }

//*************************以下代码已经通过测试*************************** */
// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <vector>
// #include <condition_variable>
// #include <chrono>
// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// class Image_Get : public rclcpp::Node {
// public:
//     Image_Get(int fps, int width, int height)
//         : Node("Image_Get"), stop_capture_(false), count_(0), fps_(fps), width_(width),height_(height), max_uncompressed_queue_size_(10), max_compressed_queue_size_(10) {
//         publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/cameraImage", 10);

//         // 打开摄像头，并设置捕获的帧率和分辨率
//         cap_ = cv::VideoCapture(0, cv::CAP_ANY);
//         if (!cap_.isOpened()) {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//             throw std::runtime_error("Failed to open camera");
//         }
//         cap_.set(cv::CAP_PROP_FRAME_WIDTH, width);
//         cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height);
//         cap_.set(cv::CAP_PROP_FPS, fps);

//         // 启动四个线程：捕获、两个压缩、发布
//         capture_thread_ = std::thread(&Image_Get::captureFrames, this);
//         compress_thread_1_ = std::thread(&Image_Get::compressFrames, this);
//         compress_thread_2_ = std::thread(&Image_Get::compressFrames, this);
//         publish_thread_ = std::thread(&Image_Get::publishFrames, this);
//     }

//     ~Image_Get() {
//         stop_capture_ = true; // 停止所有线程

//         // 唤醒所有线程防止死锁
//         cond_var_uncompressed_.notify_all();
//         cond_var_compressed_.notify_all();

//         if (capture_thread_.joinable()) {
//             capture_thread_.join(); // 等待捕获线程结束
//         }
//         if (compress_thread_1_.joinable()) {
//             compress_thread_1_.join(); // 等待第一个压缩线程结束
//         }
//         if (compress_thread_2_.joinable()) {
//             compress_thread_2_.join(); // 等待第二个压缩线程结束
//         }
//         if (publish_thread_.joinable()) {
//             publish_thread_.join(); // 等待发布线程结束
//         }
//     }

// private:
//     // 捕获线程：捕获摄像头帧并存储在未压缩队列中
//     void captureFrames() {
//         cv::Mat frame;
//         while (!stop_capture_) {
//             auto start = std::chrono::high_resolution_clock::now();

//             // 异步抓取帧
//             if (!cap_.grab()) {
//                 RCLCPP_ERROR(this->get_logger(), "Failed to grab frame");
//                 continue;
//             }

//             // retrieve() 只在grab成功后获取解码的图像
//             cap_.retrieve(frame);
//             if (frame.empty()) {
//                 RCLCPP_ERROR(this->get_logger(), "Failed to retrieve frame");
//                 continue;
//             }

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 if (uncompressed_queue_.size() >= max_uncompressed_queue_size_) {
//                     // 如果队列满，则丢弃当前帧
//                     std::cout << "Uncompressed queue full, dropping frame" << std::endl;
//                     continue;
//                 }
//                 uncompressed_queue_.push(frame.clone());
//             }
//             cond_var_uncompressed_.notify_one();

//             auto end = std::chrono::high_resolution_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Capture thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     // 压缩线程：从未压缩队列中读取图像，压缩为JPEG并存储在压缩队列中
//     void compressFrames() {
//         while (!stop_capture_) {
//             cv::Mat frame;

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 cond_var_uncompressed_.wait(lock, [this]() { return !uncompressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && uncompressed_queue_.empty()) return;

//                 frame = uncompressed_queue_.front();
//                 uncompressed_queue_.pop();
//             }

//             auto start = std::chrono::high_resolution_clock::now();

//             // 压缩为JPEG
//             std::vector<uchar> buf;
//             cv::imencode(".jpg", frame, buf, {cv::IMWRITE_JPEG_QUALITY, 50}); // 降低JPEG质量以加快压缩速度

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 if (compressed_queue_.size() >= max_compressed_queue_size_) {
//                     cond_var_compressed_.wait(lock, [this]() { return compressed_queue_.size() < max_compressed_queue_size_ || stop_capture_; });
//                 }
//                 compressed_queue_.push(buf);
//             }
//             cond_var_compressed_.notify_one();

//             auto end = std::chrono::high_resolution_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Compress thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     // 发布线程：从压缩队列中读取压缩图像并发布
//     void publishFrames() {
//         rclcpp::Rate loop_rate(fps_);
//         while (rclcpp::ok() && !stop_capture_) {
//             std::vector<uchar> buf;

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 cond_var_compressed_.wait(lock, [this]() { return !compressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && compressed_queue_.empty()) return;

//                 buf = compressed_queue_.front();
//                 compressed_queue_.pop();
//             }

//             auto start = std::chrono::high_resolution_clock::now();

//             // 创建压缩图像消息并发布
//             sensor_msgs::msg::CompressedImage msg;
//             msg.format = "jpeg";
//             msg.data = buf;

//             /////**************使用SM4加密****************** */

//             /////********************************************* */

//             publisher_->publish(msg);
//             loop_rate.sleep(); // 根据fps控制发布速率

//             auto end = std::chrono::high_resolution_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Publish thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;
//     cv::VideoCapture cap_;  // 摄像头

//     // 未压缩图像队列
//     std::queue<cv::Mat> uncompressed_queue_;
//     std::mutex uncompressed_queue_mutex_;
//     std::condition_variable cond_var_uncompressed_;

//     // 压缩图像队列
//     std::queue<std::vector<uchar>> compressed_queue_;
//     std::mutex compressed_queue_mutex_;
//     std::condition_variable cond_var_compressed_;

//     std::thread capture_thread_;
//     std::thread compress_thread_1_;
//     std::thread compress_thread_2_;
//     std::thread publish_thread_;

//     bool stop_capture_;
//     int count_;
//     int fps_;
//     int width_;
//     int height_;
//     const size_t max_uncompressed_queue_size_;
//     const size_t max_compressed_queue_size_;
// };

// int main(int argc, char **argv) {
//     rclcpp::init(argc, argv);

//     int fps = 30;
//     int width = 640;
//     int height = 480;
//     if (argc > 1) {
//         fps = std::stoi(argv[1]);
//         width = std::stoi(argv[2]);
//         height = std::stoi(argv[3]);
//     }

//     auto node = std::make_shared<Image_Get>(fps, width, height);
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }

//************************以上代码已经通过测试****************************** */

// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <vector>
// #include <condition_variable>
// #include <chrono>
// #include "rclcpp/rclcpp.hpp"
// #include "std_msgs/msg/byte_multi_array.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// // 引入了SM4加密库
// #include "gmssl/sm4.h"
// #include "gmssl/hex.h"

// class Image_Get : public rclcpp::Node
// {
// public:
//     Image_Get(int fps, int width, int height)
//         : Node("Image_Get"), stop_capture_(false), count_(0), fps_(fps), width_(width), height_(height),
//           max_uncompressed_queue_size_(10), max_compressed_queue_size_(10)
//     {
//         publisher_ = this->create_publisher<std_msgs::msg::ByteMultiArray>("/cameraImageEncrypted", 10);

//         // 打开摄像头，并设置捕获的帧率和分辨率
//         cap_ = cv::VideoCapture(0, cv::CAP_ANY);
//         if (!cap_.isOpened())
//         {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//             throw std::runtime_error("Failed to open camera");
//         }
//         cap_.set(cv::CAP_PROP_FRAME_WIDTH, width);
//         cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height);
//         cap_.set(cv::CAP_PROP_FPS, fps);

//         // 启动四个线程：捕获、两个压缩、发布
//         capture_thread_ = std::thread(&Image_Get::captureFrames, this);
//         compress_thread_1_ = std::thread(&Image_Get::compressFrames, this);
//         compress_thread_2_ = std::thread(&Image_Get::compressFrames, this);
//         publish_thread_ = std::thread(&Image_Get::publishFrames, this);
//     }

//     ~Image_Get()
//     {
//         stop_capture_ = true; // 停止所有线程

//         // 唤醒所有线程防止死锁
//         cond_var_uncompressed_.notify_all();
//         cond_var_compressed_.notify_all();

//         if (capture_thread_.joinable())
//         {
//             capture_thread_.join(); // 等待捕获线程结束
//         }
//         if (compress_thread_1_.joinable())
//         {
//             compress_thread_1_.join(); // 等待第一个压缩线程结束
//         }
//         if (compress_thread_2_.joinable())
//         {
//             compress_thread_2_.join(); // 等待第二个压缩线程结束
//         }
//         if (publish_thread_.joinable())
//         {
//             publish_thread_.join(); // 等待发布线程结束
//         }
//     }

//     void SM4encrypt(std::vector<uchar> *plaindata, char *SM4_key, std::vector<uchar> *cipherdata)
//     {
//         SM4_KEY sm4_key;

//         uint8_t key[16] = {0};
//         size_t key_len;

//         uint8_t ctr[16];
//         uint8_t *buf1 = plaindata->data();
//         uint8_t *buf2 = cipherdata->data();
//         //uint8_t *buf3 = ;

//         hex_to_bytes(SM4_key, strlen(SM4_key), key, &key_len);

//         sm4_set_encrypt_key(&sm4_key, key);

//         memset(ctr, 0, sizeof(ctr));
//         sm4_ctr_encrypt(&sm4_key, ctr, buf1, sizeof(buf1), buf2);
//     }

// private:
//     // 捕获线程：捕获摄像头帧并存储在未压缩队列中
//     void captureFrames()
//     {
//         cv::Mat frame;
//         while (!stop_capture_)
//         {
//             auto start = std::chrono::high_resolution_clock::now();

//             // 异步抓取帧
//             if (!cap_.grab())
//             {
//                 RCLCPP_ERROR(this->get_logger(), "Failed to grab frame");
//                 continue;
//             }

//             // retrieve() 只在grab成功后获取解码的图像
//             cap_.retrieve(frame);
//             if (frame.empty())
//             {
//                 RCLCPP_ERROR(this->get_logger(), "Failed to retrieve frame");
//                 continue;
//             }

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 if (uncompressed_queue_.size() >= max_uncompressed_queue_size_)
//                 {
//                     std::cout << "Uncompressed queue full, dropping frame" << std::endl;
//                     continue;
//                 }
//                 uncompressed_queue_.push(frame.clone());
//             }
//             cond_var_uncompressed_.notify_one();

//             auto end = std::chrono::high_resolution_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Capture thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     // 压缩线程：从未压缩队列中读取图像，压缩为JPEG并存储在压缩队列中
//     void compressFrames()
//     {
//         while (!stop_capture_)
//         {
//             cv::Mat frame;

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 cond_var_uncompressed_.wait(lock, [this]()
//                                             { return !uncompressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && uncompressed_queue_.empty())
//                     return;

//                 frame = uncompressed_queue_.front();
//                 uncompressed_queue_.pop();
//             }

//             auto start = std::chrono::high_resolution_clock::now();

//             // 压缩为JPEG
//             std::vector<uchar> buf;
//             cv::imencode(".jpg", frame, buf, {cv::IMWRITE_JPEG_QUALITY, 50}); // 降低JPEG质量以加快压缩速度

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 if (compressed_queue_.size() >= max_compressed_queue_size_)
//                 {
//                     cond_var_compressed_.wait(lock, [this]()
//                                               { return compressed_queue_.size() < max_compressed_queue_size_ || stop_capture_; });
//                 }
//                 compressed_queue_.push(buf);
//             }
//             cond_var_compressed_.notify_one();

//             auto end = std::chrono::high_resolution_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Compress thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     // 发布线程：从压缩队列中读取压缩图像并发布
//     void publishFrames()
//     {
//         rclcpp::Rate loop_rate(fps_);
//         while (rclcpp::ok() && !stop_capture_)
//         {
//             std::vector<uchar> buf;

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 cond_var_compressed_.wait(lock, [this]()
//                                           { return !compressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && compressed_queue_.empty())
//                     return;

//                 buf = compressed_queue_.front();
//                 compressed_queue_.pop();
//             }

//             auto start = std::chrono::high_resolution_clock::now();

//             // 使用SM4加密
//             std::vector<uchar> encrypted_data;
//             SM4encrypt(&buf, SM4_key.data(), &encrypted_data); // SM4encrypt函数

//             // 创建加密后的消息并发布
//             std_msgs::msg::ByteMultiArray msg;
//             // msg.data = encrypted_data;
//             msg.data = encrypted_data;

//             publisher_->publish(msg);
//             loop_rate.sleep(); // 根据fps控制发布速率

//             auto end = std::chrono::high_resolution_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//             std::cout << "Publish thread time: " << duration.count() << " ms" << std::endl;
//         }
//     }

//     rclcpp::Publisher<std_msgs::msg::ByteMultiArray>::SharedPtr publisher_;
//     cv::VideoCapture cap_; // 摄像头

//     // 未压缩图像队列
//     std::queue<cv::Mat> uncompressed_queue_;
//     std::mutex uncompressed_queue_mutex_;
//     std::condition_variable cond_var_uncompressed_;

//     // 压缩图像队列
//     std::queue<std::vector<uchar>> compressed_queue_;
//     std::mutex compressed_queue_mutex_;
//     std::condition_variable cond_var_compressed_;

//     std::thread capture_thread_;
//     std::thread compress_thread_1_;
//     std::thread compress_thread_2_;
//     std::thread publish_thread_;

//     bool stop_capture_;
//     int count_;
//     int fps_;
//     int width_;
//     int height_;
//     const size_t max_uncompressed_queue_size_;
//     const size_t max_compressed_queue_size_;

//     // 初始化SM4加密
//     std::string SM4_key = "fedcba98765432100123456789abcdef"; // 使用SM4密钥，字符串格式
// };

// int main(int argc, char **argv)
// {
//     rclcpp::init(argc, argv);

//     int fps = 30;
//     int width = 640;
//     int height = 480;
//     if (argc > 1)
//     {
//         fps = std::stoi(argv[1]);
//         width = std::stoi(argv[2]);
//         height = std::stoi(argv[3]);
//     }

//     auto node = std::make_shared<Image_Get>(fps, width, height);
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }

// ***********以上代码通过编译，未通过测试 */




// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <vector>
// #include <condition_variable>
// #include <chrono>
// #include "rclcpp/rclcpp.hpp"
// #include "std_msgs/msg/byte_multi_array.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// // 引入SM4加密库
// #include "gmssl/sm4.h"
// #include "gmssl/hex.h"

// class Image_Get : public rclcpp::Node
// {
// public:
//     Image_Get(int fps, int width, int height)
//         : Node("Image_Get"), stop_capture_(false), fps_(fps), width_(width), height_(height),
//           max_uncompressed_queue_size_(10), max_compressed_queue_size_(10), max_encrypted_queue_size_(10)
//     {
//         publisher_ = this->create_publisher<std_msgs::msg::ByteMultiArray>("/cameraImageEncrypted", 10);

//         // 打开摄像头
//         cap_ = cv::VideoCapture(0, cv::CAP_ANY);
//         if (!cap_.isOpened())
//         {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//             throw std::runtime_error("Failed to open camera");
//         }
//         cap_.set(cv::CAP_PROP_FRAME_WIDTH, width);
//         cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height);
//         cap_.set(cv::CAP_PROP_FPS, fps);

//         // 启动线程
//         capture_thread_ = std::thread(&Image_Get::captureFrames, this);
//         compress_thread_1_ = std::thread(&Image_Get::compressFrames, this);
//         compress_thread_2_ = std::thread(&Image_Get::compressFrames, this);
//         encrypt_thread_ = std::thread(&Image_Get::encryptFrames, this);
//         publish_thread_ = std::thread(&Image_Get::publishFrames, this);
//     }

//     ~Image_Get()
//     {
//         stop_capture_ = true; // 停止所有线程

//         // 唤醒所有线程以避免死锁
//         cond_var_uncompressed_.notify_all();
//         cond_var_compressed_.notify_all();
//         cond_var_encrypted_.notify_all();

//         if (capture_thread_.joinable())
//             capture_thread_.join();
//         if (compress_thread_1_.joinable())
//             compress_thread_1_.join();
//         if (compress_thread_2_.joinable())
//             compress_thread_2_.join();
//         if (encrypt_thread_.joinable())
//             encrypt_thread_.join();
//         if (publish_thread_.joinable())
//             publish_thread_.join();
//     }

//     void SM4encrypt(const std::vector<uchar> &plain_data, const std::string &key, std::vector<uchar> &cipher_data)
//     {
//         SM4_KEY sm4_key;
//         uint8_t key_bytes[16] = {0};
//         size_t key_len;

//         hex_to_bytes(key.c_str(), key.length(), key_bytes, &key_len);
//         sm4_set_encrypt_key(&sm4_key, key_bytes);

//         uint8_t ctr[16] = {0};
//         cipher_data.resize(plain_data.size());
//         sm4_ctr_encrypt(&sm4_key, ctr, plain_data.data(), plain_data.size(), cipher_data.data());
//     }

// private:
//     // 捕获线程
//     void captureFrames()
//     {
//         cv::Mat frame;
//         while (!stop_capture_)
//         {
//             if (!cap_.grab())
//                 continue;

//             cap_.retrieve(frame);
//             if (frame.empty())
//                 continue;

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 if (uncompressed_queue_.size() < max_uncompressed_queue_size_)
//                 {
//                     uncompressed_queue_.push(frame.clone());
//                 }
//             }
//             cond_var_uncompressed_.notify_one();
//         }
//     }

//     // 压缩线程
//     void compressFrames()
//     {
//         while (!stop_capture_)
//         {
//             cv::Mat frame;
//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 cond_var_uncompressed_.wait(lock, [this]()
//                                             { return !uncompressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && uncompressed_queue_.empty())
//                     return;

//                 frame = uncompressed_queue_.front();
//                 uncompressed_queue_.pop();
//             }

//             std::vector<uchar> buf;
//             cv::imencode(".jpg", frame, buf);

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 if (compressed_queue_.size() < max_compressed_queue_size_)
//                 {
//                     compressed_queue_.push(buf);
//                 }
//             }
//             cond_var_compressed_.notify_one();
//         }
//     }

//     // 加密线程
//     void encryptFrames()
//     {
//         while (!stop_capture_)
//         {
//             std::vector<uchar> compressed_data;
//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 cond_var_compressed_.wait(lock, [this]()
//                                           { return !compressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && compressed_queue_.empty())
//                     return;

//                 compressed_data = compressed_queue_.front();
//                 compressed_queue_.pop();
//             }

//             std::vector<uchar> encrypted_data;
//             SM4encrypt(compressed_data, SM4_key, encrypted_data);

//             {
//                 std::unique_lock<std::mutex> lock(encrypted_queue_mutex_);
//                 if (encrypted_queue_.size() < max_encrypted_queue_size_)
//                 {
//                     encrypted_queue_.push(encrypted_data);
//                 }
//             }
//             cond_var_encrypted_.notify_one();
//         }
//     }

//     // 发布线程
//     void publishFrames()
//     {
//         rclcpp::Rate loop_rate(fps_);
//         while (rclcpp::ok() && !stop_capture_)
//         {
//             std::vector<uchar> encrypted_data;
//             {
//                 std::unique_lock<std::mutex> lock(encrypted_queue_mutex_);
//                 cond_var_encrypted_.wait(lock, [this]()
//                                          { return !encrypted_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && encrypted_queue_.empty())
//                     return;

//                 encrypted_data = encrypted_queue_.front();
//                 encrypted_queue_.pop();
//             }

//             std_msgs::msg::ByteMultiArray msg;
//             msg.data = encrypted_data;
//             publisher_->publish(msg);

//             loop_rate.sleep();
//         }
//     }

//     rclcpp::Publisher<std_msgs::msg::ByteMultiArray>::SharedPtr publisher_;
//     cv::VideoCapture cap_;

//     // 队列和锁
//     std::queue<cv::Mat> uncompressed_queue_;
//     std::mutex uncompressed_queue_mutex_;
//     std::condition_variable cond_var_uncompressed_;

//     std::queue<std::vector<uchar>> compressed_queue_;
//     std::mutex compressed_queue_mutex_;
//     std::condition_variable cond_var_compressed_;

//     std::queue<std::vector<uchar>> encrypted_queue_;
//     std::mutex encrypted_queue_mutex_;
//     std::condition_variable cond_var_encrypted_;

//     std::thread capture_thread_;
//     std::thread compress_thread_1_;
//     std::thread compress_thread_2_;
//     std::thread encrypt_thread_;
//     std::thread publish_thread_;

//     bool stop_capture_;
//     int fps_;
//     int width_;
//     int height_;
//     const size_t max_uncompressed_queue_size_;
//     const size_t max_compressed_queue_size_;
//     const size_t max_encrypted_queue_size_;

//     std::string SM4_key = "fedcba98765432100123456789abcdef";
// };

// int main(int argc, char **argv)
// {
//     rclcpp::init(argc, argv);

//     auto node = std::make_shared<Image_Get>(30, 640, 480);
//     rclcpp::spin(node);
//     rclcpp::shutdown();
//     return 0;
// }
//***********以上代码通过测试,是可用的 */
// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <queue>
// #include <vector>
// #include <condition_variable>
// #include <chrono>
// #include "rclcpp/rclcpp.hpp"
// #include "std_msgs/msg/byte_multi_array.hpp"
// #include "opencv2/opencv.hpp"
// #include "cv_bridge/cv_bridge.h"

// // 引入SM4加密库
// #include "gmssl/sm4.h"
// #include "gmssl/hex.h"

// class Image_Get : public rclcpp::Node
// {
// public:
//     Image_Get(int fps, int width, int height)
//         : Node("Image_Get"), stop_capture_(false), fps_(fps), width_(width), height_(height),
//           max_uncompressed_queue_size_(10), max_compressed_queue_size_(10), max_encrypted_queue_size_(10)
//     {
//         publisher_ = this->create_publisher<std_msgs::msg::ByteMultiArray>("/cameraImageEncrypted", 10);

//         // 打开摄像头
//         cap_ = cv::VideoCapture(0, cv::CAP_ANY);
//         if (!cap_.isOpened())
//         {
//             RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
//             throw std::runtime_error("Failed to open camera");
//         }
//         cap_.set(cv::CAP_PROP_FRAME_WIDTH, width);
//         cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height);
//         cap_.set(cv::CAP_PROP_FPS, fps);

//         // 启动线程
//         capture_thread_ = std::thread(&Image_Get::captureFrames, this);
//         compress_thread_1_ = std::thread(&Image_Get::compressFrames, this);
//         //compress_thread_2_ = std::thread(&Image_Get::compressFrames, this);
//         encrypt_thread_ = std::thread(&Image_Get::encryptFrames, this);
//         publish_thread_ = std::thread(&Image_Get::publishFrames, this);
//     }

//     ~Image_Get()
//     {
//         stop_capture_ = true; // 停止所有线程

//         // 唤醒所有线程以避免死锁
//         cond_var_uncompressed_.notify_all();
//         cond_var_compressed_.notify_all();
//         cond_var_encrypted_.notify_all();

//         if (capture_thread_.joinable())
//             capture_thread_.join();
//         if (compress_thread_1_.joinable())
//             compress_thread_1_.join();
//         // if (compress_thread_2_.joinable())
//         //     compress_thread_2_.join();
//         if (encrypt_thread_.joinable())
//             encrypt_thread_.join();
//         if (publish_thread_.joinable())
//             publish_thread_.join();
//     }

//     void SM4encrypt(const std::vector<uchar> &plain_data, const std::string &key, std::vector<uchar> &cipher_data)
//     {
//         SM4_KEY sm4_key;
//         uint8_t key_bytes[16] = {0};
//         size_t key_len;

//         hex_to_bytes(key.c_str(), key.length(), key_bytes, &key_len);
//         sm4_set_encrypt_key(&sm4_key, key_bytes);

//         uint8_t ctr[16] = {0};
//         cipher_data.resize(plain_data.size());
//         sm4_ctr_encrypt(&sm4_key, ctr, plain_data.data(), plain_data.size(), cipher_data.data());
//     }

// private:
//     void logTime(const std::string &stage, const std::chrono::steady_clock::time_point &start_time)
//     {
//         auto end_time = std::chrono::steady_clock::now();
//         double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();
//         RCLCPP_INFO(this->get_logger(), "%s: %.2f ms", stage.c_str(), duration);
//     }

//     // 捕获线程
//     void captureFrames()
//     {
//         cv::Mat frame;
//         while (!stop_capture_)
//         {
//             auto start_time = std::chrono::steady_clock::now();
//             if (!cap_.grab())
//                 continue;

//             cap_.retrieve(frame);
//             if (frame.empty())
//                 continue;

//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 if (uncompressed_queue_.size() < max_uncompressed_queue_size_)
//                 {
//                     uncompressed_queue_.push(frame.clone());
//                 }
//             }
//             cond_var_uncompressed_.notify_one();
//             logTime("Capture", start_time);
//         }
//     }

//     // 压缩线程
//     void compressFrames()
//     {
//         while (!stop_capture_)
//         {
//             auto start_time = std::chrono::steady_clock::now();
//             cv::Mat frame;
//             {
//                 std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
//                 cond_var_uncompressed_.wait(lock, [this]()
//                                             { return !uncompressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && uncompressed_queue_.empty())
//                     return;

//                 frame = uncompressed_queue_.front();
//                 uncompressed_queue_.pop();
//             }

//             std::vector<uchar> buf;
//             cv::imencode(".jpg", frame, buf);

//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 if (compressed_queue_.size() < max_compressed_queue_size_)
//                 {
//                     compressed_queue_.push(buf);
//                 }
//             }
//             cond_var_compressed_.notify_one();
//             logTime("Compress", start_time);
//         }
//     }

//     // 加密线程
//     void encryptFrames()
//     {
//         while (!stop_capture_)
//         {
//             auto start_time = std::chrono::steady_clock::now();
//             std::vector<uchar> compressed_data;
//             {
//                 std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
//                 cond_var_compressed_.wait(lock, [this]()
//                                           { return !compressed_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && compressed_queue_.empty())
//                     return;

//                 compressed_data = compressed_queue_.front();
//                 compressed_queue_.pop();
//             }

//             std::vector<uchar> encrypted_data;
//             SM4encrypt(compressed_data, SM4_key, encrypted_data);

//             {
//                 std::unique_lock<std::mutex> lock(encrypted_queue_mutex_);
//                 if (encrypted_queue_.size() < max_encrypted_queue_size_)
//                 {
//                     encrypted_queue_.push(encrypted_data);
//                 }
//             }
//             cond_var_encrypted_.notify_one();
//             logTime("Encrypt", start_time);
//         }
//     }

//     // 发布线程
//     void publishFrames()
//     {
//         rclcpp::Rate loop_rate(fps_);
//         while (rclcpp::ok() && !stop_capture_)
//         {
//             auto start_time = std::chrono::steady_clock::now();
//             std::vector<uchar> encrypted_data;
//             {
//                 std::unique_lock<std::mutex> lock(encrypted_queue_mutex_);
//                 cond_var_encrypted_.wait(lock, [this]()
//                                          { return !encrypted_queue_.empty() || stop_capture_; });

//                 if (stop_capture_ && encrypted_queue_.empty())
//                     return;

//                 encrypted_data = encrypted_queue_.front();
//                 encrypted_queue_.pop();
//             }

//             std_msgs::msg::ByteMultiArray msg;
//             msg.data = encrypted_data;
//             publisher_->publish(msg);
//             logTime("Publish", start_time);
//             loop_rate.sleep();
//         }
//     }

//     rclcpp::Publisher<std_msgs::msg::ByteMultiArray>::SharedPtr publisher_;
//     cv::VideoCapture cap_;

//     std::queue<cv::Mat> uncompressed_queue_;
//     std::mutex uncompressed_queue_mutex_;
//     std::condition_variable cond_var_uncompressed_;

//     std::queue<std::vector<uchar>> compressed_queue_;
//     std::mutex compressed_queue_mutex_;
//     std::condition_variable cond_var_compressed_;

//     std::queue<std::vector<uchar>> encrypted_queue_;
//     std::mutex encrypted_queue_mutex_;
//     std::condition_variable cond_var_encrypted_;

//     std::thread capture_thread_;
//     std::thread compress_thread_1_;
//     //std::thread compress_thread_2_;
//     std::thread encrypt_thread_;
//     std::thread publish_thread_;

//     bool stop_capture_;
//     int fps_;
//     int width_;
//     int height_;
//     const size_t max_uncompressed_queue_size_;
//     const size_t max_compressed_queue_size_;
//     const size_t max_encrypted_queue_size_;

//     std::string SM4_key = "fedcba98765432100123456789abcdef";
// };

// int main(int argc, char **argv)
// {
//     rclcpp::init(argc, argv);

//     auto node = std::make_shared<Image_Get>(30, 640, 480);
//     rclcpp::spin(node);
//     rclcpp::shutdown();               
//     return 0;
// }


//***********以上代码通过测试,是可用的 */
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/byte_multi_array.hpp"
#include "opencv2/opencv.hpp"
#include "cv_bridge/cv_bridge.h"

// 引入SM4加密库
#include "gmssl/sm4.h"
#include "gmssl/hex.h"

class Image_Get : public rclcpp::Node
{
public:
    Image_Get(int fps, int width, int height)
        : Node("Image_Get"), stop_capture_(false), fps_(fps), width_(width), height_(height),
          max_uncompressed_queue_size_(10), max_compressed_queue_size_(10), max_encrypted_queue_size_(10)
    {
        publisher_ = this->create_publisher<std_msgs::msg::ByteMultiArray>("/cameraImageEncrypted", 10);

        // 打开摄像头
        cap_ = cv::VideoCapture(0, cv::CAP_ANY);
        if (!cap_.isOpened())
        {
            RCLCPP_ERROR(this->get_logger(), "Failed to open camera");
            throw std::runtime_error("Failed to open camera");
        }
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        cap_.set(cv::CAP_PROP_FPS, fps);

        // 启动线程
        capture_thread_ = std::thread(&Image_Get::captureFrames, this);
        compress_thread_1_ = std::thread(&Image_Get::compressFrames, this);
        //compress_thread_2_ = std::thread(&Image_Get::compressFrames, this);
        encrypt_thread_ = std::thread(&Image_Get::encryptFrames, this);
        publish_thread_ = std::thread(&Image_Get::publishFrames, this);
    }

    ~Image_Get()
    {
        stop_capture_ = true; // 停止所有线程

        // 唤醒所有线程以避免死锁
        cond_var_uncompressed_.notify_all();
        cond_var_compressed_.notify_all();
        cond_var_encrypted_.notify_all();

        if (capture_thread_.joinable())
            capture_thread_.join();
        if (compress_thread_1_.joinable())
            compress_thread_1_.join();
        // if (compress_thread_2_.joinable())
        //     compress_thread_2_.join();
        if (encrypt_thread_.joinable())
            encrypt_thread_.join();
        if (publish_thread_.joinable())
            publish_thread_.join();
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
    void logTime(const std::string &stage, const std::chrono::steady_clock::time_point &start_time)
    {
        auto end_time = std::chrono::steady_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        RCLCPP_INFO(this->get_logger(), "%s: %.2f ms", stage.c_str(), duration);
    }

    // 捕获线程
    void captureFrames()
    {
        cv::Mat frame;
        while (!stop_capture_)
        {
            auto start_time = std::chrono::steady_clock::now();
            if (!cap_.grab())
                continue;

            cap_.retrieve(frame);
            if (frame.empty())
                continue;

            {
                std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
                if (uncompressed_queue_.size() < max_uncompressed_queue_size_)
                {
                    uncompressed_queue_.push(frame.clone());
                }
            }
            cond_var_uncompressed_.notify_one();
            logTime("Capture", start_time);
        }
    }

    // 压缩线程
    void compressFrames()
    {
        while (!stop_capture_)
        {
            auto start_time = std::chrono::steady_clock::now();
            cv::Mat frame;
            {
                std::unique_lock<std::mutex> lock(uncompressed_queue_mutex_);
                cond_var_uncompressed_.wait(lock, [this]()
                                            { return !uncompressed_queue_.empty() || stop_capture_; });

                if (stop_capture_ && uncompressed_queue_.empty())
                    return;

                frame = uncompressed_queue_.front();
                uncompressed_queue_.pop();
            }

            std::vector<uchar> buf;
            cv::imencode(".jpg", frame, buf);

            {
                std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
                if (compressed_queue_.size() < max_compressed_queue_size_)
                {
                    compressed_queue_.push(buf);
                }
            }
            cond_var_compressed_.notify_one();
            logTime("Compress", start_time);
        }
    }

    // 加密线程
    void encryptFrames()
    {
        while (!stop_capture_)
        {
            auto start_time = std::chrono::steady_clock::now();
            std::vector<uchar> compressed_data;
            {
                std::unique_lock<std::mutex> lock(compressed_queue_mutex_);
                cond_var_compressed_.wait(lock, [this]()
                                          { return !compressed_queue_.empty() || stop_capture_; });

                if (stop_capture_ && compressed_queue_.empty())
                    return;

                compressed_data = compressed_queue_.front();
                compressed_queue_.pop();
            }

            std::vector<uchar> encrypted_data;
            SM4encrypt(compressed_data, SM4_key, encrypted_data);

            {
                std::unique_lock<std::mutex> lock(encrypted_queue_mutex_);
                if (encrypted_queue_.size() < max_encrypted_queue_size_)
                {
                    encrypted_queue_.push(encrypted_data);
                }
            }
            cond_var_encrypted_.notify_one();
            logTime("Encrypt", start_time);
        }
    }

    // 发布线程
    void publishFrames()
    {
        rclcpp::Rate loop_rate(fps_);
        while (rclcpp::ok() && !stop_capture_)
        {
            auto start_time = std::chrono::steady_clock::now();
            std::vector<uchar> encrypted_data;
            {
                std::unique_lock<std::mutex> lock(encrypted_queue_mutex_);
                cond_var_encrypted_.wait(lock, [this]()
                                         { return !encrypted_queue_.empty() || stop_capture_; });

                if (stop_capture_ && encrypted_queue_.empty())
                    return;

                encrypted_data = encrypted_queue_.front();
                encrypted_queue_.pop();
            }

            std_msgs::msg::ByteMultiArray msg;
            msg.data = encrypted_data;
            publisher_->publish(msg);
            logTime("Publish", start_time);
            loop_rate.sleep();
        }
    }

    rclcpp::Publisher<std_msgs::msg::ByteMultiArray>::SharedPtr publisher_;
    cv::VideoCapture cap_;

    std::queue<cv::Mat> uncompressed_queue_;
    std::mutex uncompressed_queue_mutex_;
    std::condition_variable cond_var_uncompressed_;

    std::queue<std::vector<uchar>> compressed_queue_;
    std::mutex compressed_queue_mutex_;
    std::condition_variable cond_var_compressed_;

    std::queue<std::vector<uchar>> encrypted_queue_;
    std::mutex encrypted_queue_mutex_;
    std::condition_variable cond_var_encrypted_;

    std::thread capture_thread_;
    std::thread compress_thread_1_;
    //std::thread compress_thread_2_;
    std::thread encrypt_thread_;
    std::thread publish_thread_;

    bool stop_capture_;
    int fps_;
    int width_;
    int height_;
    const size_t max_uncompressed_queue_size_;
    const size_t max_compressed_queue_size_;
    const size_t max_encrypted_queue_size_;

    std::string SM4_key = "fedcba98765432100123456789abcdef";
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<Image_Get>(30, 640, 480);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}



