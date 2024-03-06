#include <iostream>
#include <csignal>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#include "capture/capture.h"
#include "mjpeg/mjpeg.h"

namespace cvc = capvicam;

volatile std::sig_atomic_t signal_num = -1;

void siginthandler(int param) {
    signal_num = param;
    std::cout << "[INFO ]: Stopping application. Received signal SIGINT" << std::endl;
}

void callback([[maybe_unused]] const cvc::Buffer buffer, [[maybe_unused]] size_t id) {
    // std::cout << "[DEBUG]: received buffer, id = " << id << std::endl;
}

int main() {
    signal(SIGINT, siginthandler);
    auto th = std::thread([]{
        cvc::MjpegService service;
        service.run();
    });
    try {
        cvc::Capture capture("/dev/video0", 1920, 1080);
        capture.run(callback);
    } catch ( std::exception & ex) {
        std::cout << "[ERROR]: " << ex.what() << std::endl;
    }
    if(th.joinable()) {
        th.join();
    }
    return 0;
}
