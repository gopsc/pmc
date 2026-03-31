#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
//#include <CImg.h>
#include "hd/Camera.h"
#include "nn/ImageLoader.hpp"
#include "nn/ConvLayer.hpp"
#include "nn/PoolingLayer.hpp"
#include "nn/FlattenLayer.hpp"
#include "nn/CNNBuilder.hpp"
#include "nn/NNBuilder.hpp"
#include "nn/NeuralNetwork.hpp"

using namespace qing;


// 定义颜色用于不同的人脸框
const unsigned char colors[][3] = {
    {255, 0, 0},    // 红色
    {0, 255, 0},    // 绿色
    {0, 0, 255},    // 蓝色
    {255, 255, 0},  // 黄色
    {255, 0, 255}   // 紫色
};

void take_a_photo(const char* path);
int run_facerec(const char* in, const char* out);

// 主函数
int main(int argc, char* argv[]) {

    ImageLoader image_loader(3, 480, 640, true, false);

    ConvLayer conv1(3, 16, 3, 1, 1);
    PoolingLayer pool1(2, 2, 2, 0, true);
    ConvLayer conv2(16, 32, 3, 1, 1);
    PoolingLayer pool2(2, 2, 2, 0, true);
    ConvLayer conv3(32, 64, 3, 1, 1);
    PoolingLayer pool3(2, 2, 2, 0, true);
    ConvLayer conv4(64, 128, 3, 1, 1);
    PoolingLayer pool4(2, 2, 2, 0, true);
    ConvLayer conv5(128, 256, 3, 1, 1);
    PoolingLayer pool5(2, 2, 2, 0, true);
    ConvLayer conv6(256, 512, 3, 1, 1);
    PoolingLayer pool6(2, 2, 2, 0, true);
    FlattenLayer flatten;

    CNNBuilder cnn;
    int in_channels = 3;
    int in_h = 480;
    int in_w = 640;
    int batch_size = 1;

    cnn.add_conv_layer(conv1, in_channels, in_h, in_w);
    cnn.add_pool_layer(pool1, 16, in_h, in_w);
    cnn.add_conv_layer(conv2, 16, in_h/2, in_w/2);
    cnn.add_pool_layer(pool2, 32, in_h/2, in_w/2);
    cnn.add_conv_layer(conv3, 32, in_h/4, in_w/4);
    cnn.add_pool_layer(pool3, 64, in_h/4, in_w/4);
    cnn.add_conv_layer(conv4, 64, in_h/8, in_w/8);
    cnn.add_pool_layer(pool4, 128, in_h/8, in_w/8);
    cnn.add_conv_layer(conv5, 128, in_h/16, in_w/16);
    cnn.add_pool_layer(pool5, 256, in_h/16, in_w/16);
    cnn.add_conv_layer(conv6, 256, in_h/32, in_w/32);
    cnn.add_pool_layer(pool6, 512, in_h/32, in_w/32);
    cnn.add_flatten_layer(flatten, 512, 7, 10);

    cnn.print_architecture();

    NNBuilder nn;
    using nnl = NeuralNetwork;

    auto layer0 = nnl::Create_in_Factory(15360, 6500,0.5, nnl::ActivationFunc::Leaky_ReLU);
    auto layer1 = nnl::Create_in_Factory(6500, 1500, 0.45, nnl::ActivationFunc::Leaky_ReLU);
    auto layer2 = nnl::Create_in_Factory(1500, 300,  0.40, nnl::ActivationFunc::Leaky_ReLU);
    auto layer3 = nnl::Create_in_Factory(300,  70,  0.35, nnl::ActivationFunc::Leaky_ReLU);
    auto layer4 = nnl::Create_in_Factory(70,   5,   0.30, nnl::ActivationFunc::Sigmoid);

    nn.add(layer0);
    nn.add(layer1);
    nn.add(layer2);
    nn.add(layer3);
    nn.add(layer4);

    nn.print_shape();


    //Camera_init(640, 480);
    bool flag = true;
    while (flag) {


        //std::string imgpath = std::string("./dataSet/" + std::to_string(time(NULL)) + ".jpeg");
        //std::string txtpath  =std::string("./dataSet/" + std::to_string(time(NULL)) + ".txt");

        //bool flag;
        //auto img = get_frame_from_camera(flag);
        //img.save(imgpath.c_str());

        //if (run_facerec(imgpath.c_str(), txtpath.c_str()))
        //    throw std::runtime_error("facerec");
            
        std::string data_root = "./dataSet";
        namespace fs  = std::filesystem;
        for (const auto& entry : fs::recursive_directory_iterator(data_root)) {
            if (!fs::is_regular_file(entry.status()))
                continue;
            
            std::string basename = entry.path().stem().string();
            std::string txtpath = data_root + "/" + basename + ".txt";
            std::string imgpath = data_root + "/" + basename+".jpeg";
            auto img = cimg_library::CImg<unsigned char>(imgpath.c_str());

            auto image_data = image_loader.load_from_cimg(img, true);
            if (image_data.empty()) {
                std::cerr << "Failed to load image!" << std::endl;
                break;
            }

            /* 前向传播 */
            auto output = cnn.forward(image_data, batch_size, true);
            if (!output.empty()) {
                std::cout << "Forward pass successful!" << std::endl;
                std::cout << "Input size: " << image_data.size() << std::endl;
                std::cout << "Output size: " << output.size() << std::endl;
            
                // 计算输出统计信息
                float sum = 0.0f, max_val = output[0], min_val = output[0];
                for (float val : output) {
                    sum += val;
                    if (val > max_val) max_val = val;
                    if (val < min_val) min_val = val;
                }
            
                std::cout << "Output statistics:" << std::endl;
                std::cout << "  Mean: " << sum / output.size() << std::endl;
                std::cout << "  Min: " << min_val << std::endl;
                std::cout << "  Max: " << max_val << std::endl;
            }

            std::vector<float> t(5);
            std::ifstream file(txtpath);
            if (!file) t[0] = 0;
            else {
                t[0] = 1;
                for (int i=0; i<5; ++i) {
                    file >> t[i+1];
                    std::cout << t[i+1] << " ";
                }
            }
            std::cout<< std::endl;
            auto r = nn.forward(output);
            auto y = nn.cal_err(r, t);

            /* 不存在人脸时，位置信息不计入 */
            if (t[0] == 0)
                for (int i =1; i< 5; i++)
                    y[i]= 0;
            else {
                for (auto i: y) {
                    std::cout << i << " ";
                }
                std::cout << std::endl;

                auto fix_img = img.draw_rectangle(t[4]*640,t[1]*480, t[2]*640,t[3]*480, colors[1], 1, ~0U); /* 抽象为函数 */
                fix_img = fix_img.draw_rectangle(r[4]*640,r[1]*480, r[2]*640,r[3]*480, colors[0], 1, ~0U);
                std::string respath = "./resSet/" + basename+".jpeg";
                fix_img.save(respath.c_str());
            }
            y = nn.backward(y);
            y = cnn.backward(y);
        }
    }

    //Camera_close();
}

int run_facerec(const char* in, const char* out) {
    auto cmd = std::string("python tools/facerec.py detect ");
    cmd += in;
    cmd += " --output ";
    cmd += out;
    return system(cmd.c_str());
}