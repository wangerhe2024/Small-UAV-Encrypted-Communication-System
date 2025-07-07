// // // #include <ros/ros.h>
// // #include "rclcpp/rclcpp.hpp"
// // #include <image_transport/image_transport.hpp>
// // #include "sensor_msgs/msg/compressed_image.hpp"
// // #include <opencv2/highgui/highgui.hpp>
// // #include <cv_bridge/cv_bridge.h>

// // void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& msg)
// // {
// //     try // 如果转换失败，则提跳转到catch语句
// //     {

// //         cv::imshow("view1", cv_bridge::toCvCopy(msg, "bgr8")->image); // 将图像转换openCV的格式，并输出到窗口
// //         cv::waitKey(1);                                               // 一定要有wiatKey(),要不然是黑框或者无窗口
// //     }
// //     catch (cv_bridge::Exception &e)
// //     {
// //         std::cout << "Could not convert for '%s' to 'bgr8'." <<  msg->encoding.c_str() << std::endl;

// //     }
// // }

// // int main(int argc, char **argv)
// // {
// //     rclcpp::init(argc, argv); // 注册节点名

// //     // ros::NodeHandle nh; // 注册句柄
// //     // image_transport::ImageTransport it(nh); // 注册句柄

// //     rclcpp::NodeOptions options;
// //     rclcpp::Node::SharedPtr node = rclcpp::Node::make_shared("Image_Sub", options);
// //     image_transport::ImageTransport it(node);                                              //  类似ROS句柄
// //     image_transport::Subscriber image_sub = it.subscribe("/cameraImage", 1, imageCallback); // 订阅/cameraImage话题，并添加回调函数
// //     rclcpp::spin(node);                                                                      // 循环等待回调函数触发
// // }

// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgcodecs.hpp>
// #include <iostream>

// // 回调函数，用于处理订阅到的压缩图像
// void imageCallback(const sensor_msgs::msg::CompressedImage::ConstSharedPtr& msg)
// {
//     try
//     {
//         // 将压缩图像数据转换为OpenCV支持的格式
//         cv::Mat rawData = cv::Mat(1, msg->data.size(), CV_8UC1, const_cast<unsigned char*>(msg->data.data()));

//         // 使用 imdecode 将 JPEG 数据解码为 OpenCV 的 Mat 对象
//         cv::Mat decodedImage = cv::imdecode(rawData, cv::IMREAD_COLOR);

//         if (!decodedImage.empty()) {
//             // 显示图像
//             cv::imshow("view1", decodedImage);
//             cv::waitKey(1);
//         } else {
//             std::cerr << "Failed to decode image." << std::endl;
//         }
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "Exception: " << e.what() << std::endl;
//     }
// }

// int main(int argc, char **argv)
// {
//     // 初始化节点
//     rclcpp::init(argc, argv);

//     // 创建 ROS 2 节点
//     auto node = rclcpp::Node::make_shared("Image_Sub");

//     // 使用标准的 ROS 2 订阅器来订阅压缩图像话题
//     auto subscription = node->create_subscription<sensor_msgs::msg::CompressedImage>(
//         "/cameraImage", 10, imageCallback);

//     // 循环等待回调函数触发
//     rclcpp::spin(node);

//     // 关闭节点
//     rclcpp::shutdown();
//     return 0;
// }

// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgcodecs.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <queue>
// #include <chrono>

// #include "gmssl/sm4.h"

// class ImageSubscriber : public rclcpp::Node {
// public:
//     ImageSubscriber()
//         : Node("Image_Sub"), stop_threads_(false), frame_count_(0), fps_(0) {
//         // 创建订阅器
//         subscription_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
//             "/cameraImage", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));

//         // 启动解码线程、显示线程、FPS 计算线程
//         decode_thread_ = std::thread(&ImageSubscriber::decodeImages, this);
//         display_thread_ = std::thread(&ImageSubscriber::displayImages, this);
//         fps_thread_ = std::thread(&ImageSubscriber::showFPS, this);
//     }

//     ~ImageSubscriber() {
//         stop_threads_ = true;

//         // 通知所有等待的线程
//         cond_var_.notify_all();

//         // 等待线程结束
//         if (decode_thread_.joinable()) decode_thread_.join();
//         if (display_thread_.joinable()) display_thread_.join();
//         if (fps_thread_.joinable()) fps_thread_.join();

//         cv::destroyAllWindows();  // 关闭所有OpenCV窗口
//     }

// private:
//     // 回调函数，用于接收压缩图像并推入队列
//     void imageCallback(const sensor_msgs::msg::CompressedImage::ConstSharedPtr& msg) {
//         std::lock_guard<std::mutex> lock(queue_mutex_);
//         compressed_queue_.push(msg);  // 将压缩图像推入队列
//         frame_count_++;               // 增加帧计数
//         cond_var_.notify_one();       // 通知解码线程
//     }

//     // 解码线程，用于从压缩图像队列中读取并解码
//     void decodeImages() {
//         while (!stop_threads_) {
//             sensor_msgs::msg::CompressedImage::ConstSharedPtr msg;

//             {
//                 std::unique_lock<std::mutex> lock(queue_mutex_);
//                 cond_var_.wait(lock, [this]() { return !compressed_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && compressed_queue_.empty()) return; // 退出线程

//                 msg = compressed_queue_.front();
//                 compressed_queue_.pop();  // 从队列中获取消息
//             }

//             try {
//                 // 将压缩图像数据转换为OpenCV支持的格式
//                 cv::Mat rawData = cv::Mat(1, msg->data.size(), CV_8UC1, const_cast<unsigned char*>(msg->data.data()));
//                 cv::Mat decodedImage = cv::imdecode(rawData, cv::IMREAD_COLOR);

//                 if (!decodedImage.empty()) {
//                     std::lock_guard<std::mutex> lock(image_mutex_);
//                     latest_image_ = decodedImage.clone();
//                     image_ready_ = true;
//                 } else {
//                     std::cerr << "Failed to decode image." << std::endl;
//                 }
//             } catch (const std::exception &e) {
//                 std::cerr << "Exception: " << e.what() << std::endl;
//             }
//         }
//     }

//     // 显示线程，用于显示解码后的图像，并叠加FPS
//     void displayImages() {
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 防止过快检查

//             std::lock_guard<std::mutex> lock(image_mutex_);
//             if (image_ready_) {
//                 // 将FPS叠加到图像左上角
//                 std::string fps_text = "FPS: " + std::to_string(fps_);
//                 cv::putText(latest_image_, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

//                 // 显示图像
//                 cv::imshow("view1", latest_image_);
//                 cv::waitKey(1);
//                 image_ready_ = false;
//             }
//         }
//     }

//     // FPS 线程，用于显示每秒的帧数
//     void showFPS() {
//         using namespace std::chrono;
//         auto start_time = steady_clock::now();
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::seconds(1));

//             auto end_time = steady_clock::now();
//             auto duration = duration_cast<seconds>(end_time - start_time).count();
//             if (duration >= 1) {
//                 fps_ = frame_count_ / duration;  // 计算帧率
//                 frame_count_ = 0;                // 重置帧计数
//                 start_time = end_time;           // 重置开始时间
//                 //RCLCPP_INFO(this->get_logger(), "Current FPS: %d", fps_);
//             }
//         }
//     }

//     rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr subscription_;
//     std::thread decode_thread_, display_thread_, fps_thread_;
//     std::mutex queue_mutex_, image_mutex_;
//     std::condition_variable cond_var_;

//     std::queue<sensor_msgs::msg::CompressedImage::ConstSharedPtr> compressed_queue_;  // 压缩图像队列
//     cv::Mat latest_image_;  // 解码后的最新图像
//     bool image_ready_ = false;
//     bool stop_threads_;  // 用于停止线程的标志

//     // 帧数计数
//     int frame_count_;
//     int fps_;  // 每秒帧率
// };

// int main(int argc, char **argv) {
//     // 初始化节点
//     rclcpp::init(argc, argv);

//     // 创建多线程执行器
//     rclcpp::executors::MultiThreadedExecutor executor;

//     // 创建订阅节点
//     auto node = std::make_shared<ImageSubscriber>();

//     // 将节点添加到执行器
//     executor.add_node(node);

//     // 开始执行回调
//     executor.spin();

//     // 关闭节点
//     rclcpp::shutdown();
//     return 0;
// }

//*************************以上代码已经通过测试 **************************/
// #include "rclcpp/rclcpp.hpp"
// //#include "sensor_msgs/msg/compressed_image.hpp"
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgcodecs.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <queue>
// #include <chrono>
// #include "std_msgs/msg/byte_multi_array.hpp"

// #include "gmssl/sm4.h"
// #include "gmssl/hex.h"

// class ImageSubscriber : public rclcpp::Node {
// public:
//     ImageSubscriber()
//         : Node("Image_Sub"), stop_threads_(false), frame_count_(0), fps_(0) {
//         // 创建订阅器
//         subscription_ = this->create_subscription<std_msgs::msg::ByteMultiArray>("/cameraImageEncrypted", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));

//         // 启动解码线程、显示线程、FPS 计算线程
//         decode_thread_ = std::thread(&ImageSubscriber::decodeImages, this);
//         display_thread_ = std::thread(&ImageSubscriber::displayImages, this);
//         fps_thread_ = std::thread(&ImageSubscriber::showFPS, this);
//     }

//     ~ImageSubscriber() {
//         stop_threads_ = true;

//         // 通知所有等待的线程
//         cond_var_.notify_all();

//         // 等待线程结束
//         if (decode_thread_.joinable()) decode_thread_.join();
//         if (display_thread_.joinable()) display_thread_.join();
//         if (fps_thread_.joinable()) fps_thread_.join();

//         cv::destroyAllWindows();  // 关闭所有OpenCV窗口
//     }

//     void SM4decrypt(std::vector< uchar> *cipherdata, char *SM4_key, std::vector< uchar> *plaindata)
//     {
//         SM4_KEY sm4_key;

//         uint8_t key[16] = {0};
//         size_t key_len;

//         uint8_t ctr[16];
//         uint8_t *buf1 = cipherdata->data();
//         uint8_t *buf2 = plaindata->data();
//         //uint8_t buf3[30] = {0};

//         hex_to_bytes(SM4_key, strlen(SM4_key), key, &key_len);

//         sm4_set_encrypt_key(&sm4_key, key);

//         memset(ctr, 0, sizeof(ctr));
//         sm4_ctr_encrypt(&sm4_key, ctr, buf1, sizeof(buf1), buf2);
//     }

// private:
//     // 回调函数，用于接收压缩图像并推入队列
//     void imageCallback(const std_msgs::msg::ByteMultiArray::ConstSharedPtr& msg) {
//         std::lock_guard<std::mutex> lock(queue_mutex_);
//         compressed_queue_.push(msg);  // 将压缩图像推入队列
//         frame_count_++;               // 增加帧计数
//         cond_var_.notify_one();       // 通知解码线程
//     }

//     // 解码线程，用于从压缩图像队列中读取并解码
//     void decodeImages() {
//         while (!stop_threads_) {
//             std_msgs::msg::ByteMultiArray::ConstSharedPtr msg;

//             {
//                 std::unique_lock<std::mutex> lock(queue_mutex_);
//                 cond_var_.wait(lock, [this]() { return !compressed_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && compressed_queue_.empty()) return; // 退出线程

//                 msg = compressed_queue_.front();
//                 compressed_queue_.pop();  // 从队列中获取消息
//             }

//             try {
//                 // 解密
//                 std::vector<uchar> data_vector(msg->data.begin(), msg->data.end());
//                 std::vector<uchar> decrypted_data; // 存储解密后的数据
//                 SM4decrypt(&data_vector, SM4_key.data(), &decrypted_data); // 解密操作

//                 // 将压缩图像数据转换为OpenCV支持的格式
//                 //cv::Mat rawData = cv::Mat(1, msg->data.size(), CV_8UC1, const_cast<unsigned char*>(msg->data.data()));
//                 cv::Mat rawData = cv::Mat(1, decrypted_data.size(), CV_8UC1, const_cast<unsigned char*>(decrypted_data.data()));
//                 cv::Mat decodedImage = cv::imdecode(rawData, cv::IMREAD_COLOR);

//                 if (!decodedImage.empty()) {
//                     std::lock_guard<std::mutex> lock(image_mutex_);
//                     latest_image_ = decodedImage.clone();
//                     image_ready_ = true;
//                 } else {
//                     std::cerr << "Failed to decode image." << std::endl;
//                 }
//             } catch (const std::exception &e) {
//                 std::cerr << "Exception: " << e.what() << std::endl;
//             }
//         }
//     }

//     // 显示线程，用于显示解码后的图像，并叠加FPS
//     void displayImages() {
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 防止过快检查

//             std::lock_guard<std::mutex> lock(image_mutex_);
//             if (image_ready_) {
//                 // 将FPS叠加到图像左上角
//                 std::string fps_text = "FPS: " + std::to_string(fps_);
//                 cv::putText(latest_image_, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

//                 // 显示图像
//                 cv::imshow("view1", latest_image_);
//                 cv::waitKey(1);
//                 image_ready_ = false;
//             }
//         }
//     }

//     // FPS 线程，用于显示每秒的帧数
//     void showFPS() {
//         using namespace std::chrono;
//         auto start_time = steady_clock::now();
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::seconds(1));

//             auto end_time = steady_clock::now();
//             auto duration = duration_cast<seconds>(end_time - start_time).count();
//             if (duration >= 1) {
//                 fps_ = frame_count_ / duration;  // 计算帧率
//                 frame_count_ = 0;                // 重置帧计数
//                 start_time = end_time;           // 重置开始时间
//                 //RCLCPP_INFO(this->get_logger(), "Current FPS: %d", fps_);
//             }
//         }
//     }

//     rclcpp::Subscription<std_msgs::msg::ByteMultiArray>::SharedPtr subscription_;
//     std::thread decode_thread_, display_thread_, fps_thread_;
//     std::mutex queue_mutex_, image_mutex_;
//     std::condition_variable cond_var_;

//     std::queue<std_msgs::msg::ByteMultiArray::ConstSharedPtr> compressed_queue_;  // 压缩图像队列
//     cv::Mat latest_image_;  // 解码后的最新图像
//     bool image_ready_ = false;
//     bool stop_threads_;  // 用于停止线程的标志

//     // 帧数计数
//     int frame_count_;
//     int fps_;  // 每秒帧率
//     // 初始化SM4加密
//     std::string SM4_key = "fedcba98765432100123456789abcdef"; // 使用SM4密钥，字符串格式
// };

// int main(int argc, char **argv) {
//     // 初始化节点
//     rclcpp::init(argc, argv);

//     // 创建多线程执行器
//     rclcpp::executors::MultiThreadedExecutor executor;

//     // 创建订阅节点
//     auto node = std::make_shared<ImageSubscriber>();

//     // 将节点添加到执行器
//     executor.add_node(node);

//     // 开始执行回调
//     executor.spin();

//     // 关闭节点
//     rclcpp::shutdown();
//     return 0;
// }

//***********以上代码通过编译，未通过测试 */
// #include "rclcpp/rclcpp.hpp"
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgcodecs.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <queue>
// #include <chrono>
// #include "std_msgs/msg/byte_multi_array.hpp"
// #include "gmssl/sm4.h"
// #include "gmssl/hex.h"

// class ImageSubscriber : public rclcpp::Node {
// public:
//     ImageSubscriber()
//         : Node("Image_Sub"), stop_threads_(false), frame_count_(0), fps_(0) {
//         // 创建订阅器
//         subscription_ = this->create_subscription<std_msgs::msg::ByteMultiArray>(
//             "/cameraImageEncrypted", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));

//         // 启动解密线程、解码线程、显示线程、FPS 计算线程
//         decrypt_thread_ = std::thread(&ImageSubscriber::decryptImages, this);
//         decode_thread_ = std::thread(&ImageSubscriber::decodeImages, this);
//         display_thread_ = std::thread(&ImageSubscriber::displayImages, this);
//         fps_thread_ = std::thread(&ImageSubscriber::showFPS, this);
//     }

//     ~ImageSubscriber() {
//         stop_threads_ = true;

//         // 通知所有等待的线程
//         decrypt_cond_var_.notify_all();
//         decode_cond_var_.notify_all();

//         // 等待线程结束
//         if (decrypt_thread_.joinable()) decrypt_thread_.join();
//         if (decode_thread_.joinable()) decode_thread_.join();
//         if (display_thread_.joinable()) display_thread_.join();
//         if (fps_thread_.joinable()) fps_thread_.join();

//         cv::destroyAllWindows(); // 关闭所有OpenCV窗口
//     }

//     void SM4decrypt(const std::vector<uchar>& cipherdata, const std::string& sm4_key, std::vector<uchar>& plaindata) {
//         SM4_KEY sm4_key_struct;
//         uint8_t key[16] = {0};
//         size_t key_len;

//         uint8_t ctr[16] = {0};
//         hex_to_bytes(sm4_key.c_str(), sm4_key.length(), key, &key_len);

//         sm4_set_encrypt_key(&sm4_key_struct, key);
//         plaindata.resize(cipherdata.size());
//         sm4_ctr_encrypt(&sm4_key_struct, ctr, cipherdata.data(), cipherdata.size(), plaindata.data());
//     }

// private:
//     // 回调函数，用于接收加密图像并推入解密队列
//     void imageCallback(const std_msgs::msg::ByteMultiArray::ConstSharedPtr& msg) {
//         std::lock_guard<std::mutex> lock(decrypt_queue_mutex_);
//         decrypt_queue_.push(msg); // 将加密图像推入解密队列
//         frame_count_++;           // 增加帧计数
//         decrypt_cond_var_.notify_one(); // 通知解密线程
//     }

//     // 解密线程，从加密队列读取并解密数据
//     void decryptImages() {
//         while (!stop_threads_) {
//             std_msgs::msg::ByteMultiArray::ConstSharedPtr msg;
//             {
//                 std::unique_lock<std::mutex> lock(decrypt_queue_mutex_);
//                 decrypt_cond_var_.wait(lock, [this]() { return !decrypt_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && decrypt_queue_.empty()) return;

//                 msg = decrypt_queue_.front();
//                 decrypt_queue_.pop();
//             }

//             try {
//                 std::vector<uchar> decrypted_data;
//                 SM4decrypt(msg->data, sm4_key_, decrypted_data);

//                 {
//                     std::lock_guard<std::mutex> lock(decode_queue_mutex_);
//                     decode_queue_.push(decrypted_data);
//                 }
//                 decode_cond_var_.notify_one(); // 通知解码线程
//             } catch (const std::exception& e) {
//                 std::cerr << "Decryption error: " << e.what() << std::endl;
//             }
//         }
//     }

//     // 解码线程，从解密队列读取数据并解码
//     void decodeImages() {
//         while (!stop_threads_) {
//             std::vector<uchar> decrypted_data;

//             {
//                 std::unique_lock<std::mutex> lock(decode_queue_mutex_);
//                 decode_cond_var_.wait(lock, [this]() { return !decode_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && decode_queue_.empty()) return;

//                 decrypted_data = decode_queue_.front();
//                 decode_queue_.pop();
//             }

//             try {
//                 cv::Mat rawData(1, decrypted_data.size(), CV_8UC1, const_cast<unsigned char*>(decrypted_data.data()));
//                 cv::Mat decodedImage = cv::imdecode(rawData, cv::IMREAD_COLOR);

//                 if (!decodedImage.empty()) {
//                     std::lock_guard<std::mutex> lock(image_mutex_);
//                     latest_image_ = decodedImage.clone();
//                     image_ready_ = true;
//                 } else {
//                     std::cerr << "Failed to decode image." << std::endl;
//                 }
//             } catch (const std::exception& e) {
//                 std::cerr << "Decoding error: " << e.what() << std::endl;
//             }
//         }
//     }

//     // 显示线程，显示解码后的图像
//     void displayImages() {
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));

//             std::lock_guard<std::mutex> lock(image_mutex_);
//             if (image_ready_) {
//                 std::string fps_text = "FPS: " + std::to_string(fps_);
//                 cv::putText(latest_image_, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

//                 cv::imshow("view1", latest_image_);
//                 cv::waitKey(1);
//                 image_ready_ = false;
//             }
//         }
//     }

//     // FPS 线程，计算帧率
//     void showFPS() {
//         using namespace std::chrono;
//         auto start_time = steady_clock::now();
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::seconds(1));

//             auto end_time = steady_clock::now();
//             auto duration = duration_cast<seconds>(end_time - start_time).count();
//             if (duration >= 1) {
//                 fps_ = frame_count_ / duration;
//                 frame_count_ = 0;
//                 start_time = end_time;
//             }
//         }
//     }

//     rclcpp::Subscription<std_msgs::msg::ByteMultiArray>::SharedPtr subscription_;
//     std::thread decrypt_thread_, decode_thread_, display_thread_, fps_thread_;

//     std::mutex decrypt_queue_mutex_, decode_queue_mutex_, image_mutex_;
//     std::condition_variable decrypt_cond_var_, decode_cond_var_;

//     std::queue<std_msgs::msg::ByteMultiArray::ConstSharedPtr> decrypt_queue_; // 解密队列
//     std::queue<std::vector<uchar>> decode_queue_;                             // 解码队列

//     cv::Mat latest_image_;
//     bool image_ready_ = false;
//     bool stop_threads_;

//     int frame_count_;
//     int fps_;
//     std::string sm4_key_ = "fedcba98765432100123456789abcdef";
//     //std::string sm4_key_ = "fedcba98765432100123456789abcdeg";
// };

// int main(int argc, char** argv) {
//     rclcpp::init(argc, argv);
//     rclcpp::executors::MultiThreadedExecutor executor;
//     auto node = std::make_shared<ImageSubscriber>();
//     executor.add_node(node);
//     executor.spin();
//     rclcpp::shutdown();
//     return 0;
// }
//***********以上代码通过测试,是可用的 */

// #include "rclcpp/rclcpp.hpp"
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgcodecs.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <queue>
// #include <chrono>
// #include "std_msgs/msg/byte_multi_array.hpp"
// #include "gmssl/sm4.h"
// #include "gmssl/hex.h"

// class ImageSubscriber : public rclcpp::Node {
// public:
//     ImageSubscriber()
//         : Node("Image_Sub"), stop_threads_(false), frame_count_(0), fps_(0),
//           decrypt_time_(0), decode_time_(0), display_time_(0) {
//         // 创建订阅器
//         subscription_ = this->create_subscription<std_msgs::msg::ByteMultiArray>(
//             "/cameraImageEncrypted", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));

//         // 启动解密线程、解码线程、显示线程、FPS 计算线程
//         decrypt_thread_ = std::thread(&ImageSubscriber::decryptImages, this);
//         decode_thread_ = std::thread(&ImageSubscriber::decodeImages, this);
//         display_thread_ = std::thread(&ImageSubscriber::displayImages, this);
//         fps_thread_ = std::thread(&ImageSubscriber::showFPS, this);
//         monitor_thread_ = std::thread(&ImageSubscriber::monitorThread, this);
//     }

//     ~ImageSubscriber() {
//         stop_threads_ = true;

//         // 通知所有等待的线程
//         decrypt_cond_var_.notify_all();
//         decode_cond_var_.notify_all();

//         // 等待线程结束
//         if (decrypt_thread_.joinable()) decrypt_thread_.join();
//         if (decode_thread_.joinable()) decode_thread_.join();
//         if (display_thread_.joinable()) display_thread_.join();
//         if (fps_thread_.joinable()) fps_thread_.join();
//         if (monitor_thread_.joinable()) monitor_thread_.join();

//         cv::destroyAllWindows(); // 关闭所有OpenCV窗口
//     }

//     void SM4decrypt(const std::vector<uchar>& cipherdata, const std::string& sm4_key, std::vector<uchar>& plaindata) {
//         SM4_KEY sm4_key_struct;
//         uint8_t key[16] = {0};
//         size_t key_len;

//         uint8_t ctr[16] = {0};
//         hex_to_bytes(sm4_key.c_str(), sm4_key.length(), key, &key_len);

//         sm4_set_encrypt_key(&sm4_key_struct, key);
//         plaindata.resize(cipherdata.size());
//         sm4_ctr_encrypt(&sm4_key_struct, ctr, cipherdata.data(), cipherdata.size(), plaindata.data());
//     }

// private:
//     void imageCallback(const std_msgs::msg::ByteMultiArray::ConstSharedPtr& msg) {
//         std::lock_guard<std::mutex> lock(decrypt_queue_mutex_);
//         decrypt_queue_.push(msg);
//         frame_count_++;
//         decrypt_cond_var_.notify_one();
//     }

//     void decryptImages() {
//         while (!stop_threads_) {
//             auto start_time = std::chrono::high_resolution_clock::now();

//             std_msgs::msg::ByteMultiArray::ConstSharedPtr msg;
//             {
//                 std::unique_lock<std::mutex> lock(decrypt_queue_mutex_);
//                 decrypt_cond_var_.wait(lock, [this]() { return !decrypt_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && decrypt_queue_.empty()) return;

//                 msg = decrypt_queue_.front();
//                 decrypt_queue_.pop();
//             }

//             try {
//                 std::vector<uchar> decrypted_data;
//                 SM4decrypt(msg->data, sm4_key_, decrypted_data);

//                 {
//                     std::lock_guard<std::mutex> lock(decode_queue_mutex_);
//                     decode_queue_.push(decrypted_data);
//                 }
//                 decode_cond_var_.notify_one();
//             } catch (const std::exception& e) {
//                 std::cerr << "Decryption error: " << e.what() << std::endl;
//             }

//             auto end_time = std::chrono::high_resolution_clock::now();
//             decrypt_time_ += std::chrono::duration<double>(end_time - start_time).count();
//         }
//     }

//     void decodeImages() {
//         while (!stop_threads_) {
//             auto start_time = std::chrono::high_resolution_clock::now();

//             std::vector<uchar> decrypted_data;

//             {
//                 std::unique_lock<std::mutex> lock(decode_queue_mutex_);
//                 decode_cond_var_.wait(lock, [this]() { return !decode_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && decode_queue_.empty()) return;

//                 decrypted_data = decode_queue_.front();
//                 decode_queue_.pop();
//             }

//             try {
//                 cv::Mat rawData(1, decrypted_data.size(), CV_8UC1, const_cast<unsigned char*>(decrypted_data.data()));
//                 cv::Mat decodedImage = cv::imdecode(rawData, cv::IMREAD_COLOR);

//                 if (!decodedImage.empty()) {
//                     std::lock_guard<std::mutex> lock(image_mutex_);
//                     latest_image_ = decodedImage.clone();
//                     image_ready_ = true;
//                 } else {
//                     std::cerr << "Failed to decode image." << std::endl;
//                 }
//             } catch (const std::exception& e) {
//                 std::cerr << "Decoding error: " << e.what() << std::endl;
//             }

//             auto end_time = std::chrono::high_resolution_clock::now();
//             decode_time_ += std::chrono::duration<double>(end_time - start_time).count();
//         }
//     }

//     void displayImages() {
//         while (!stop_threads_) {
//             auto start_time = std::chrono::high_resolution_clock::now();

//             std::this_thread::sleep_for(std::chrono::milliseconds(10));

//             std::lock_guard<std::mutex> lock(image_mutex_);
//             if (image_ready_) {
//                 std::string fps_text = "FPS: " + std::to_string(fps_);
//                 cv::putText(latest_image_, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

//                 cv::imshow("view1", latest_image_);
//                 cv::waitKey(1);
//                 image_ready_ = false;
//             }

//             auto end_time = std::chrono::high_resolution_clock::now();
//             display_time_ += std::chrono::duration<double>(end_time - start_time).count();
//         }
//     }

//     void showFPS() {
//         using namespace std::chrono;
//         auto start_time = steady_clock::now();
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::seconds(1));

//             auto end_time = steady_clock::now();
//             auto duration = duration_cast<seconds>(end_time - start_time).count();
//             if (duration >= 1) {
//                 fps_ = frame_count_ / duration;
//                 frame_count_ = 0;
//                 start_time = end_time;
//             }
//         }
//     }

//     void monitorThread() {
//         while (!stop_threads_) {
//             std::this_thread::sleep_for(std::chrono::seconds(5));
//             std::cout << "Thread Time Usage:\n"
//                       << "Decrypt Thread: " << decrypt_time_ << " seconds\n"
//                       << "Decode Thread: " << decode_time_ << " seconds\n"
//                       << "Display Thread: " << display_time_ << " seconds\n"
//                       << std::endl;
//         }
//     }

//     rclcpp::Subscription<std_msgs::msg::ByteMultiArray>::SharedPtr subscription_;
//     std::thread decrypt_thread_, decode_thread_, display_thread_, fps_thread_, monitor_thread_;

//     std::mutex decrypt_queue_mutex_, decode_queue_mutex_, image_mutex_;
//     std::condition_variable decrypt_cond_var_, decode_cond_var_;

//     std::queue<std_msgs::msg::ByteMultiArray::ConstSharedPtr> decrypt_queue_;
//     std::queue<std::vector<uchar>> decode_queue_;

//     cv::Mat latest_image_;
//     bool image_ready_ = false;
//     bool stop_threads_;

//     int frame_count_;
//     int fps_;

//     std::string sm4_key_ = "fedcba98765432100123456789abcdef";

//     // 计时器
//     double decrypt_time_, decode_time_, display_time_;
// };

// int main(int argc, char** argv) {
//     rclcpp::init(argc, argv);
//     rclcpp::executors::MultiThreadedExecutor executor;
//     auto node = std::make_shared<ImageSubscriber>();
//     executor.add_node(node);
//     executor.spin();
//     rclcpp::shutdown();
//     return 0;
// }

//****** 以上代码已经通过测试****************/

// #include "rclcpp/rclcpp.hpp"
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgcodecs.hpp>
// #include <opencv2/imgproc.hpp>
// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <queue>
// #include <chrono>
// #include "std_msgs/msg/byte_multi_array.hpp"
// #include "sensor_msgs/msg/compressed_image.hpp"
// #include "gmssl/sm4.h"
// #include "gmssl/hex.h"

// class ImageSubscriber : public rclcpp::Node
// {
// public:
//     ImageSubscriber()
//         : Node("Image_Sub"), stop_threads_(false), frame_count_(0), fps_(0),
//           decrypt_time_(0), decode_time_(0), display_time_(0)
//     {
//         // 创建订阅器
//         subscription_ = this->create_subscription<std_msgs::msg::ByteMultiArray>(
//             "/cameraImageEncrypted", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));

//         publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/cameraImageDecrypted", 10);

//         // 启动解密线程、解码线程、显示线程、FPS 计算线程
//         decrypt_thread_ = std::thread(&ImageSubscriber::decryptImages, this);
//         decode_thread_ = std::thread(&ImageSubscriber::decodeImages, this);
//         display_thread_ = std::thread(&ImageSubscriber::displayImages, this);
//         fps_thread_ = std::thread(&ImageSubscriber::showFPS, this);
//         monitor_thread_ = std::thread(&ImageSubscriber::monitorThread, this);
//         publish_thread_ = std::thread(&ImageSubscriber::publishImages, this);
//     }

//     ~ImageSubscriber()
//     {
//         stop_threads_ = true;

//         // 通知所有等待的线程
//         decrypt_cond_var_.notify_all();
//         decode_cond_var_.notify_all();
//         publish_cond_var_.notify_all();

//         // 等待线程结束
//         if (decrypt_thread_.joinable())
//             decrypt_thread_.join();
//         if (decode_thread_.joinable())
//             decode_thread_.join();
//         if (display_thread_.joinable())
//             display_thread_.join();
//         if (fps_thread_.joinable())
//             fps_thread_.join();
//         if (monitor_thread_.joinable())
//             monitor_thread_.join();
//         if (publish_thread_.joinable())
//             publish_thread_.join();

//         cv::destroyAllWindows(); // 关闭所有OpenCV窗口
//     }

//     void SM4decrypt(const std::vector<uchar> &cipherdata, const std::string &sm4_key, std::vector<uchar> &plaindata)
//     {
//         SM4_KEY sm4_key_struct;
//         uint8_t key[16] = {0};
//         size_t key_len;

//         uint8_t ctr[16] = {0};
//         hex_to_bytes(sm4_key.c_str(), sm4_key.length(), key, &key_len);

//         sm4_set_encrypt_key(&sm4_key_struct, key);
//         plaindata.resize(cipherdata.size());
//         sm4_ctr_encrypt(&sm4_key_struct, ctr, cipherdata.data(), cipherdata.size(), plaindata.data());
//     }

// private:
//     void logTime(const std::string &stage, const std::chrono::steady_clock::time_point &start_time)
//     {
//         auto end_time = std::chrono::steady_clock::now();
//         double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();
//         RCLCPP_INFO(this->get_logger(), "%s: %.2f ms", stage.c_str(), duration);
//     }

//     void imageCallback(const std_msgs::msg::ByteMultiArray::ConstSharedPtr &msg)
//     {
//         std::lock_guard<std::mutex> lock(decrypt_queue_mutex_);
//         decrypt_queue_.push(msg);
//         frame_count_++;
//         decrypt_cond_var_.notify_one();
//     }

//     void decryptImages()
//     {
//         while (!stop_threads_)
//         {
//             auto start_time = std::chrono::steady_clock::now();

//             std_msgs::msg::ByteMultiArray::ConstSharedPtr msg;
//             {
//                 std::unique_lock<std::mutex> lock(decrypt_queue_mutex_);
//                 decrypt_cond_var_.wait(lock, [this]()
//                                        { return !decrypt_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && decrypt_queue_.empty())
//                     return;

//                 msg = decrypt_queue_.front();
//                 decrypt_queue_.pop();
//             }

//             try
//             {
//                 std::vector<uchar> decrypted_data;
//                 SM4decrypt(msg->data, sm4_key_, decrypted_data);

//                 {
//                     std::lock_guard<std::mutex> lock(decode_queue_mutex_);
//                     decode_queue_.push(decrypted_data);
//                 }
//                 decode_cond_var_.notify_one();
//             }
//             catch (const std::exception &e)
//             {
//                 std::cerr << "Decryption error: " << e.what() << std::endl;
//             }

//             logTime("decrypt", start_time);
//         }
//     }

//     void decodeImages()
//     {
//         while (!stop_threads_)
//         {
//             auto start_time = std::chrono::steady_clock::now();

//             std::vector<uchar> decrypted_data;

//             {
//                 std::unique_lock<std::mutex> lock(decode_queue_mutex_);
//                 decode_cond_var_.wait(lock, [this]()
//                                       { return !decode_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && decode_queue_.empty())
//                     return;

//                 decrypted_data = decode_queue_.front();
//                 decode_queue_.pop();
//             }

//             try
//             {
//                 {
//                     std::lock_guard<std::mutex> lock(publish_queue_mutex_);
//                     publish_queue_.push(decrypted_data);
//                 }
//                 publish_cond_var_.notify_one();
//             }
//             catch (const std::exception &e)
//             {
//                 std::cerr << "Decoding error: " << e.what() << std::endl;
//             }

//             logTime("decode", start_time);
//         }
//     }

//     void publishImages()
//     {
//         while (!stop_threads_)
//         {
//             auto start_time = std::chrono::steady_clock::now();

//             std::vector<uchar> publish_data;

//             {
//                 std::unique_lock<std::mutex> lock(publish_queue_mutex_);
//                 publish_cond_var_.wait(lock, [this]()
//                                        { return !publish_queue_.empty() || stop_threads_; });

//                 if (stop_threads_ && publish_queue_.empty())
//                     return;

//                 publish_data = publish_queue_.front();
//                 publish_queue_.pop();
//             }

//             try
//             {
//                 auto msg = sensor_msgs::msg::CompressedImage();
//                 msg.format = "jpeg";
//                 msg.data = publish_data;
//                 publisher_->publish(msg);
//             }
//             catch (const std::exception &e)
//             {
//                 std::cerr << "Publishing error: " << e.what() << std::endl;
//             }

//              logTime("publish", start_time);
//         }
//     }

//     void displayImages()
//     {
//         while (!stop_threads_)
//         {


//             std::this_thread::sleep_for(std::chrono::milliseconds(10));

//             std::lock_guard<std::mutex> lock(image_mutex_);
//             if (image_ready_)
//             {
//                 std::string fps_text = "FPS: " + std::to_string(fps_);
//                 cv::putText(latest_image_, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

//                 cv::imshow("view1", latest_image_);
//                 cv::waitKey(1);
//                 image_ready_ = false;
//             }
//         }
//     }

//     void showFPS()
//     {
//         using namespace std::chrono;
//         auto start_time = steady_clock::now();
//         while (!stop_threads_)
//         {
//             std::this_thread::sleep_for(std::chrono::seconds(1));

//             auto end_time = steady_clock::now();
//             auto duration = duration_cast<seconds>(end_time - start_time).count();
//             if (duration >= 1)
//             {
//                 fps_ = frame_count_ / duration;
//                 frame_count_ = 0;
//                 start_time = end_time;
//             }
//         }
//     }



//     rclcpp::Subscription<std_msgs::msg::ByteMultiArray>::SharedPtr subscription_;

//     rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;

//     std::thread decrypt_thread_, decode_thread_, display_thread_, fps_thread_, monitor_thread_, publish_thread_;

//     std::mutex decrypt_queue_mutex_, decode_queue_mutex_, image_mutex_, publish_queue_mutex_;
//     std::condition_variable decrypt_cond_var_, decode_cond_var_, publish_cond_var_;

//     std::queue<std_msgs::msg::ByteMultiArray::ConstSharedPtr> decrypt_queue_;
//     std::queue<std::vector<uchar>> decode_queue_, publish_queue_;

//     cv::Mat latest_image_;
//     bool image_ready_ = false;
//     bool stop_threads_;

//     int frame_count_;
//     int fps_;

//     std::string sm4_key_ = "fedcba98765432100123456789abcdef";

//     // 计时器
//     double decrypt_time_, decode_time_, display_time_;
// };

// int main(int argc, char **argv)
// {
//     rclcpp::init(argc, argv);
//     rclcpp::executors::MultiThreadedExecutor executor;
//     auto node = std::make_shared<ImageSubscriber>();
//     executor.add_node(node);
//     executor.spin();
//     rclcpp::shutdown();
//     return 0;
// }


//******以上代码已经通过测试 */

#include "rclcpp/rclcpp.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include "std_msgs/msg/byte_multi_array.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "gmssl/sm4.h"
#include "gmssl/hex.h"

class ImageSubscriber : public rclcpp::Node {
public:
    ImageSubscriber()
        : Node("Image_Sub"), stop_threads_(false) {
        // 创建订阅器
        subscription_ = this->create_subscription<std_msgs::msg::ByteMultiArray>(
            "/cameraImageEncrypted", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));

        // 创建发布器
        publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("/cameraImageDecrypted", 10);

        // 启动解密线程和发布线程
        decrypt_thread_ = std::thread(&ImageSubscriber::decryptImages, this);
        publish_thread_ = std::thread(&ImageSubscriber::publishImages, this);
    }

    ~ImageSubscriber() {
        stop_threads_ = true;

        // 通知所有等待的线程
        decrypt_cond_var_.notify_all();
        publish_cond_var_.notify_all();

        // 等待线程结束
        if (decrypt_thread_.joinable()) decrypt_thread_.join();
        if (publish_thread_.joinable()) publish_thread_.join();
    }

    void SM4decrypt(const std::vector<uchar>& cipherdata, const std::string& sm4_key, std::vector<uchar>& plaindata) {
        SM4_KEY sm4_key_struct;
        uint8_t key[16] = {0};
        size_t key_len;

        uint8_t ctr[16] = {0};
        hex_to_bytes(sm4_key.c_str(), sm4_key.length(), key, &key_len);

        sm4_set_encrypt_key(&sm4_key_struct, key);
        plaindata.resize(cipherdata.size());
        sm4_ctr_encrypt(&sm4_key_struct, ctr, cipherdata.data(), cipherdata.size(), plaindata.data());
    }

private:
    void imageCallback(const std_msgs::msg::ByteMultiArray::ConstSharedPtr& msg) {
        std::lock_guard<std::mutex> lock(decrypt_queue_mutex_);
        decrypt_queue_.push(msg);
        decrypt_cond_var_.notify_one();
    }

    void decryptImages() {
        while (!stop_threads_) {
            std_msgs::msg::ByteMultiArray::ConstSharedPtr msg;
            {
                std::unique_lock<std::mutex> lock(decrypt_queue_mutex_);
                decrypt_cond_var_.wait(lock, [this]() { return !decrypt_queue_.empty() || stop_threads_; });

                if (stop_threads_ && decrypt_queue_.empty()) return;

                msg = decrypt_queue_.front();
                decrypt_queue_.pop();
            }

            try {
                std::vector<uchar> decrypted_data;
                SM4decrypt(msg->data, sm4_key_, decrypted_data);

                {
                    std::lock_guard<std::mutex> lock(publish_queue_mutex_);
                    publish_queue_.push(decrypted_data);
                }
                publish_cond_var_.notify_one();
            } catch (const std::exception& e) {
                std::cerr << "Decryption error: " << e.what() << std::endl;
            }
        }
    }

    void publishImages() {
        while (!stop_threads_) {
            std::vector<uchar> publish_data;

            {
                std::unique_lock<std::mutex> lock(publish_queue_mutex_);
                publish_cond_var_.wait(lock, [this]() { return !publish_queue_.empty() || stop_threads_; });

                if (stop_threads_ && publish_queue_.empty()) return;

                publish_data = publish_queue_.front();
                publish_queue_.pop();
            }

            try {
                auto msg = sensor_msgs::msg::CompressedImage();
                msg.format = "jpeg";
                msg.data = publish_data;
                publisher_->publish(msg);
            } catch (const std::exception& e) {
                std::cerr << "Publishing error: " << e.what() << std::endl;
            }
        }
    }

    rclcpp::Subscription<std_msgs::msg::ByteMultiArray>::SharedPtr subscription_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr publisher_;

    std::thread decrypt_thread_, publish_thread_;

    std::mutex decrypt_queue_mutex_, publish_queue_mutex_;
    std::condition_variable decrypt_cond_var_, publish_cond_var_;

    std::queue<std_msgs::msg::ByteMultiArray::ConstSharedPtr> decrypt_queue_;
    std::queue<std::vector<uchar>> publish_queue_;

    bool stop_threads_;

    std::string sm4_key_ = "fedcba98765432100123456789abcdef";
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::executors::MultiThreadedExecutor executor;
    auto node = std::make_shared<ImageSubscriber>();
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}

