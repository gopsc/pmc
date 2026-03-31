#include <iostream>
#include "hd/Camera.h"

int main() {

    Camera_init(640, 480);

    std::string imgpath = std::to_string(time(NULL)) + ".jpeg";
    std::string txtpath  =std::to_string(time(NULL)) + ".txt";

    bool flag;
    auto img = get_frame_from_camera(flag);
    img.save(imgpath.c_str());
    
    Camera_close();
}