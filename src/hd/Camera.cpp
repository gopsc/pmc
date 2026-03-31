/* FIXME: 这个文件非混乱 */
/* FIXME: 抽象一个服务器类 */
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "hd/Camera.h"

#define cimg_display 0 /* 不显示图像，只保存 */
#include "CImg.h"
#include <jpeglib.h>
using namespace cimg_library;

/*-----------------------------------*/
/* Camera */
static const char* filename = "/dev/video0"; /* FIXME: 不一定 */
static int width = -1;
static int height = -1;

/* 为了在获取单帧中使用 */
static struct v4l2_buffer buf;
static enum v4l2_buf_type type  =  V4L2_BUF_TYPE_VIDEO_CAPTURE;

/*-----------------------------------*/
/* 静态全局线程对象及相关同步原语 */
static std::thread camera_thread;
static std::atomic<bool> camera_running{false};
static std::mutex camera_mutex;

/* device file */
static int fd = -1;


/*-----------------------------------*/
void Camera_check();/* check if this device support video capture (V4L2_CAP_VIDEO_CAPTURE) */
void Camera_setVideoFormat(int w, int h);/* Set Video Format */
void Camera_reqBuf();/* Request Camera Buffer */
void Camera_setup(); /* mapping buffer and data collection loop */
void Camera_unsetup();

/*-----------------------------------*/
bool Camera_is_open() {
    return fd >= 0;
}

/* FIXME: 如果在集成操作中出现异常，应该释放资源 */
void Camera_init(int w, int h) {

    if (Camera_is_open())
        throw std::runtime_error("Camera already opened.");

    fd = open(filename, O_RDWR);
    if (fd < 0)
        throw std::runtime_error(
            "Failed to open device: " + std::string(strerror(errno))
        );

    Camera_check();
    Camera_setVideoFormat(w, h);
    Camera_reqBuf();
    Camera_setup();
}

/* */
void Camera_close() {
    Camera_unsetup();
    close(fd);
    fd = -1;
}


void Camera_check() {
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        throw std::runtime_error(
            "VIDIOC_QUERYCAP failed: " + std::string(strerror(errno))
        );
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        throw std::runtime_error("Device does not support video capture");
    }
}


/* FIXME: 将Camera_init()中的参数放在这里 */
void Camera_setVideoFormat(int width, int height) {
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;  /* */
    fmt.fmt.pix.height = height; /* */
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  /* usually format: YUYV, MJPEG */ 
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
        throw std::runtime_error(
            "VIDIOC_S_FMT failed: " + std::string(strerror(errno)));
    ::width=  width;
    ::height= height;
}


void Camera_reqBuf() {
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;                           /* the number of buffer */
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;           /* the mode of memory mapping */

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
        throw std::runtime_error(
	    "VIDIOC_REQBUFS failed: " + std::string(strerror(errno)));

}

/*------------------------------------------------------------------------------*/

void Camera_setup() {
    /* mapping buffer (It is needed to handle every buffer in loop) */
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;  /* Index of the buffer */

    /* 查询缓冲区信息 */
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        throw std::runtime_error("VIDIOC_QUERYBUF failed: " + std::string(strerror(errno)));
    }

    /* 将缓冲区加入队列 */
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        throw std::runtime_error("VIDIOC_QBUF failed: " + std::string(strerror(errno)));
    }

    /* 开始采集 */
    if (ioctl(fd, VIDIOC_STREAMON, &buf.type) < 0) { /* start collection */
        throw std::runtime_error("VIDIOC_STREAMON failed: " + std::string(strerror(errno)));
    }

}

void Camera_unsetup()
{
    /* 停止采集流 */
    ioctl(fd, VIDIOC_STREAMOFF, &type);
}


/*------------------------------------------------------------------------------*/
/*
 * Convert YUYV frame to CImg object and return it
 *
 * translate out camera data to CImg format
 * 
 * new version
 * 
 * FIXME: 专门建个库
 */
CImg<unsigned char> YUYV_to_CImg(uint8_t* frameData, int width, int height) {
    // 参数检查
    if (!frameData || width <= 0 || height <= 0) {
        throw std::invalid_argument("Invalid parameters");
    }
    
    CImg<unsigned char> img(width, height, 1, 3);
    
    // 使用指针提高效率
    uint8_t* src = frameData;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 2) {
            uint8_t Y0 = src[0];
            uint8_t U  = src[1];
            uint8_t Y1 = src[2];
            uint8_t V  = src[3];
            src += 4;
            
            // 使用更精确的系数
            int R0 = Y0 + ((V - 128) * 1402) / 1000;
            int G0 = Y0 - ((U - 128) * 344) / 1000 - ((V - 128) * 714) / 1000;
            int B0 = Y0 + ((U - 128) * 1772) / 1000;
            
            int R1 = Y1 + ((V - 128) * 1402) / 1000;
            int G1 = Y1 - ((U - 128) * 344) / 1000 - ((V - 128) * 714) / 1000;
            int B1 = Y1 + ((U - 128) * 1772) / 1000;
            
            // 范围限制
            R0 = std::max(0, std::min(255, R0));
            G0 = std::max(0, std::min(255, G0));
            B0 = std::max(0, std::min(255, B0));
            
            R1 = std::max(0, std::min(255, R1));
            G1 = std::max(0, std::min(255, G1));
            B1 = std::max(0, std::min(255, B1));
            
            // 设置像素
            img(x, y, 0, 0) = static_cast<uint8_t>(R0);
            img(x, y, 0, 1) = static_cast<uint8_t>(G0);
            img(x, y, 0, 2) = static_cast<uint8_t>(B0);
            
            if (x + 1 < width) {
                img(x + 1, y, 0, 0) = static_cast<uint8_t>(R1);
                img(x + 1, y, 0, 1) = static_cast<uint8_t>(G1);
                img(x + 1, y, 0, 2) = static_cast<uint8_t>(B1);
            }
        }
    }
    
    return img;
}


CImg<unsigned char> get_frame_from_camera(bool& flag) {

    std::lock_guard<std::mutex> lock(camera_mutex);
    //if (!camera_running.load())
    //    throw std::runtime_error("Camera not running");
    
    /* Dequeue to get data */
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        perror("dequeue failed");
        throw std::runtime_error("dequeue failed");
    }

    /* 获取当前缓冲区的数据指针 */
    uint8_t *frameData = static_cast<uint8_t*>(mmap(NULL, buf.length, 
                                        PROT_READ | PROT_WRITE, MAP_SHARED, 
                                        fd, buf.m.offset));
    
    if (frameData == MAP_FAILED) {
        ioctl(fd, VIDIOC_QBUF, &buf);
        throw std::runtime_error("mmap failed in get_frame_from_camera");
    }

    /* process frameData */
    //std::cout << "Got length: " << buf.length << std::endl;
    auto img = YUYV_to_CImg(frameData, width, height);
    
    /* 解除内存映射 */
    munmap(frameData, buf.length);

    if (ioctl(fd, VIDIOC_QBUF, &buf)<0){  /* join the queue again */
        perror("requeue failed");
        throw std::runtime_error("requeue failed");
    }

    
    flag = camera_running.load(); /* 检查下一次是否需要退出 */
    return img; /* 返回图像 */
}



//CImg<unsigned char> YUYV_to_CImg(uint8_t* frameData, int width, int height) {
    // 创建CImg图像对象 (width, height, depth=1, spectrum=3 for RGB)
//    CImg<unsigned char> img(width, height, 1, 3);

    // 遍历图像的每一行和每一列
//    for (int y = 0; y < height; y++) {
//        for (int x = 0; x < width; x += 2) {
            // YUYV格式：每两个像素占用4个字节
//            int yuyv_index = (y * width + x) * 2;

            // 提取YUV值
//            uint8_t Y0 = frameData[yuyv_index];
//            uint8_t U = frameData[yuyv_index + 1];
//            uint8_t Y1 = frameData[yuyv_index + 2];
//            uint8_t V = frameData[yuyv_index + 3];

            // 转换第一个像素（x, y）
//            int R0 = Y0 + (V - 128) * 1.402;
//            int G0 = Y0 - (U - 128) * 0.34414 - (V - 128) * 0.71414;
//            int B0 = Y0 + (U - 128) * 1.772;

            // 转换第二个像素（x+1, y）
//            int R1 = Y1 + (V - 128) * 1.402;
//            int G1 = Y1 - (U - 128) * 0.34414 - (V - 128) * 0.71414;
//            int B1 = Y1 + (U - 128) * 1.772;

            // 限制范围到0-255
//            R0 = std::max(0, std::min(255, R0));
//            G0 = std::max(0, std::min(255, G0));
//            B0 = std::max(0, std::min(255, B0));

//            R1 = std::max(0, std::min(255, R1));
//            G1 = std::max(0, std::min(255, G1));
//            B1 = std::max(0, std::min(255, B1));

            // 设置第一个像素值
//            img(x, y, 0, 0) = static_cast<uint8_t>(R0);
//            img(x, y, 0, 1) = static_cast<uint8_t>(G0);
//            img(x, y, 0, 2) = static_cast<uint8_t>(B0);

            // 设置第二个像素值（如果在图像范围内）
//            if (x + 1 < width) {
//                img(x + 1, y, 0, 0) = static_cast<uint8_t>(R1);
//                img(x + 1, y, 0, 1) = static_cast<uint8_t>(G1);
//                img(x + 1, y, 0, 2) = static_cast<uint8_t>(B1);
//            }
//        }
//    }

//    return img;
//}



/* 
 * Convert YUYV pixel at position (x, y) in frame to RGB
 * YUYV format: [Y0 U0 Y1 V0] [Y2 U2 Y3 V2] ...
 * Each 4 bytes contain 2 pixels (Y0,U0,V0 for pixel0, Y1,U0,V0 for pixel1)
 */
void YUYV_to_RGB(uint8_t* frameData, int width, int height, int x, int y, 
                 uint8_t& r, uint8_t& g, uint8_t& b) {
    // 计算像素在YUYV数据中的索引
    int pixel_index = y * width + x;
    
    // YUYV中每2个像素共享4字节：Y0 U Y1 V
    int byte_offset = (pixel_index / 2) * 4;
    
    // 确定是第一个像素还是第二个像素
    bool is_first_pixel = (pixel_index % 2 == 0);
    
    uint8_t Y, U, V;
    
    if (is_first_pixel) {
        // 第一个像素：Y0, U, V
        Y = frameData[byte_offset];     // Y0
        U = frameData[byte_offset + 1]; // U
        V = frameData[byte_offset + 3]; // V
    } else {
        // 第二个像素：Y1, U, V (与第一个像素共享U和V)
        Y = frameData[byte_offset + 2]; // Y1
        U = frameData[byte_offset + 1]; // U (共享)
        V = frameData[byte_offset + 3]; // V (共享)
    }
    
    // YUV to RGB conversion formula (integer approximation)
    // 注意：YUV值通常范围是Y:16-235, UV:16-240
    // 这里先转换为0-255范围
    int Y_adj = Y - 16;
    int U_adj = U - 128;
    int V_adj = V - 128;
    
    // 计算RGB值
    int R = (298 * Y_adj + 409 * V_adj + 128) >> 8;
    int G = (298 * Y_adj - 100 * U_adj - 208 * V_adj + 128) >> 8;
    int B = (298 * Y_adj + 516 * U_adj + 128) >> 8;
    
    // 限制范围到0-255
    r = (R < 0) ? 0 : ((R > 255) ? 255 : static_cast<uint8_t>(R));
    g = (G < 0) ? 0 : ((G > 255) ? 255 : static_cast<uint8_t>(G));
    b = (B < 0) ? 0 : ((B > 255) ? 255 : static_cast<uint8_t>(B));
}

/*
 * Convert YUYV frame to CImg object and save as image
 *
 * FIXME(20251229): 每次处理一个像素都要调用YUYV_to_RGB函数，重新计算索引和转换，而YUYV格式中每两个相邻像素共享U和V值
 * FIXME(20260324): 效率太慢
 */
//bool save_YUYV_as_image(uint8_t* frameData, int width, int height, 
//                         const char* filename, int frame_num = 0) {
//    try {
        // 创建CImg图像对象 (width, height, depth=1, spectrum=3 for RGB)
//        CImg<unsigned char> img(width, height, 1, 3);
        
        // 转换为RGB并填充到CImg
//        for (int y = 0; y < height; y++) {
//            for (int x = 0; x < width; x++) {
//                uint8_t r, g, b;
//                YUYV_to_RGB(frameData, width, height, x, y, r, g, b);
                
                // CImg存储顺序是平面：首先所有R，然后所有G，最后所有B
                // 或者使用(x, y, 0, channel)访问
//                img(x, y, 0, 0) = r;  // Red channel
//                img(x, y, 0, 1) = g;  // Green channel
//                img(x, y, 0, 2) = b;  // Blue channel
//            }
//        }
        
        // 生成带帧编号的文件名
//        char full_filename[256];
//        if (frame_num > 0) {
//            snprintf(full_filename, sizeof(full_filename), 
//                    "%s_frame%03d.bmp", filename, frame_num);
//        } else {
//            snprintf(full_filename, sizeof(full_filename), "%s.bmp", filename);
//        }
        
        // 保存为BMP格式（也支持PNG、JPEG等）
//        img.save(full_filename);
//        std::cout << "Image saved as: " << full_filename << std::endl;
        
//        return true;
//    } catch (CImgException& e) {
//        std::cerr << "Error saving image: " << e.what() << std::endl;
//        return false;
//    }
//}



/*
 * 将CImg数据压缩成jpeg data
 *
 * FIXME: 具有性能问题
 */
std::vector<unsigned char> compress_to_jpeg(const CImg<unsigned char>& image, int quality = 75) {
    
    // 检查图像的有效性
    if (image.width() == 0 || image.height() == 0) {
        std::cerr << "Error: Invalid image dimensions!" << std::endl;
        return std::vector<unsigned char>();
    }
    
    // 检查频谱（通道数）
    int channels = image.spectrum();
    
    // CImg数据布局：默认是平面格式 [plane1, plane2, plane3]
    // 但我们需要交错的RGB格式用于JPEG压缩
    
    unsigned char* buffer = nullptr;
    unsigned long buffer_size = 0;
    
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &buffer, &buffer_size);
    
    int width = image.width();
    int height = image.height();
    
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;  // 总是使用3通道RGB
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    
    // 开始压缩
    jpeg_start_compress(&cinfo, TRUE);
    
    JSAMPROW row_pointer[1];
    std::vector<unsigned char> row_buffer(width * 3);  // 用于存储一行交错的RGB数据
    
    // 处理每一行
    for (int y = 0; y < height; y++) {
        // 将CImg的平面格式转换为交错的RGB格式
        if (channels == 3) {
            // RGB图像：CImg存储为平面格式 [R平面, G平面, B平面]
            for (int x = 0; x < width; x++) {
                row_buffer[x * 3] = image(x, y, 0, 0);     // R
                row_buffer[x * 3 + 1] = image(x, y, 0, 1); // G
                row_buffer[x * 3 + 2] = image(x, y, 0, 2); // B
            }
        }
        else if (channels == 1) {
            // 灰度图像：重复到三个通道
            for (int x = 0; x < width; x++) {
                unsigned char gray = image(x, y, 0, 0);
                row_buffer[x * 3] = gray;     // R
                row_buffer[x * 3 + 1] = gray; // G
                row_buffer[x * 3 + 2] = gray; // B
            }
        }
        else if (channels == 4) {
            // RGBA图像：忽略Alpha通道
            for (int x = 0; x < width; x++) {
                row_buffer[x * 3] = image(x, y, 0, 0);     // R
                row_buffer[x * 3 + 1] = image(x, y, 0, 1); // G
                row_buffer[x * 3 + 2] = image(x, y, 0, 2); // B
                // 忽略Alpha通道：image(x, y, 0, 3)
            }
        }
        else {
            // 其他通道数：使用前3个通道
            std::cout << "Warning: Using first 3 channels of " << channels << "-channel image" << std::endl;
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < 3 && c < channels; c++) {
                    row_buffer[x * 3 + c] = image(x, y, 0, c);
                }
                // 如果通道数不足3，用0填充
                for (int c = channels; c < 3; c++) {
                    row_buffer[x * 3 + c] = 0;
                }
            }
        }
        
        row_pointer[0] = row_buffer.data();
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
        
    }
    
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    
    // 将JPEG数据复制到vector
    std::vector<unsigned char> jpeg_data(buffer, buffer + buffer_size);
    
    // 释放libjpeg分配的内存
    free(buffer);
    
    // 验证JPEG数据
    if (jpeg_data.size() >= 2) {
        bool has_soi = (jpeg_data[0] == 0xFF && jpeg_data[1] == 0xD8);
        bool has_eoi = (jpeg_data.size() >= 2 && 
                       jpeg_data[jpeg_data.size()-2] == 0xFF && 
                       jpeg_data[jpeg_data.size()-1] == 0xD9);
        
        if (!has_soi || !has_eoi) {
            std::cerr << "Warning: Generated JPEG may be corrupted!" << std::endl;
        }
    }
    
    return jpeg_data;
}