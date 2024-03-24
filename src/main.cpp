#include <algorithm>
#include <condition_variable>
#include <csignal>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>

#include "capture/capture.h"
#include "circular-buffer/circular-buffer.h"
#include "mjpeg/mjpeg.h"
#include "processing/processing.h"

namespace cvc = capvicam;

volatile std::sig_atomic_t signal_num = -1;

std::mutex processing_mutex;
std::condition_variable processing_condvar;
cvc::CircularBuffer image_processing_queue(8);

std::mutex mjpeg_mutex;
std::condition_variable mjpeg_condvar;
cvc::CircularBuffer image_mjpeg_queue(8);

void siginthandler(int param) {
    signal_num = param;
    std::cout << "[INFO ]: Stopping application. Received signal SIGINT"
              << std::endl;
}

void callback(const cvc::Buffer buffer, size_t id) {
    std::lock_guard lk{processing_mutex};
    if (!image_processing_queue.full()) {
        image_processing_queue.push(buffer, id);
        processing_condvar.notify_one();
    }
}

int main() {
    signal(SIGINT, siginthandler);
    auto mjpeg_thread = std::thread([] {
        try {
            cvc::MjpegService service("0.0.0.0", 8000);
            service.run();
        } catch (std::exception &ex) {
            std::cout << "[ERROR]: " << ex.what() << std::endl;
        }
    });
    auto processing_thread = std::thread([] {
        try {
            cvc::Processing processing;
            processing.run();
        } catch (std::exception &ex) {
            std::cout << "[ERROR]: " << ex.what() << std::endl;
        }
    });
    try {
        cvc::Capture capture("/dev/video0", 1920, 1080);
        // cvc::Capture capture("/dev/video0", 640, 480);
        capture.run(callback);
    } catch (std::exception &ex) {
        std::cout << "[ERROR]: " << ex.what() << std::endl;
    }
    if (mjpeg_thread.joinable()) {
        mjpeg_thread.join();
    }
    if (processing_thread.joinable()) {
        processing_thread.join();
    }
    return 0;
}
