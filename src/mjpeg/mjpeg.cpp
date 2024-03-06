#include "mjpeg.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <fcntl.h>

extern volatile std::sig_atomic_t signal_num;

static int set_nonblocking_socket(int fd) {
    int flags;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

namespace capvicam {
    void MjpegService::run() {
        while (-1 == signal_num) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Hello MjpegService" << std::endl;
        }
    }
}