#include "mjpeg.h"
#include <thread>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "../circular-buffer/circular-buffer.h"

extern volatile std::sig_atomic_t signal_num;

extern std::mutex mjpeg_mutex;
extern std::condition_variable mjpeg_condvar;
extern capvicam::CircularBuffer image_mjpeg_queue;

static int set_nonblocking_socket(int fd) {
    int flags;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

namespace capvicam {
    void MjpegService::run() {
        while (-1 == signal_num) {
            std::unique_lock lk{mjpeg_mutex};
            mjpeg_condvar.wait_for(lk, std::chrono::seconds(1), [] {
                return !image_mjpeg_queue.empty();
            });
            if (!image_mjpeg_queue.empty()) {
                ImageBuffer & value = image_mjpeg_queue.front();
                std::cout << "[DEBUG]: received buffer (mjpeg), id = " << value.id << std::endl;
                image_mjpeg_queue.pop();
            }
        }
    }
    void MjpegService::create_socket(const char* address, unsigned port) {
        struct sockaddr_in server_addr;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw std::runtime_error("Could not create socket for MjpegService. " + std::string(strerror(errno)));
        }
        int enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            throw std::runtime_error("Could not create socket for MjpegService. " + std::string(strerror(errno)));
        }
        if (set_nonblocking_socket(fd) == -1) {
            throw std::runtime_error("Could not create socket for MjpegService. " + std::string(strerror(errno)));
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(address);
        server_addr.sin_port = htons(static_cast<__uint16_t>(port));

        if (bind(fd, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
            throw std::runtime_error("Could not create socket for MjpegService. " + std::string(strerror(errno)));
        }

        if (listen(fd, SOMAXCONN) < 0) {
            throw std::runtime_error("Could not create socket for MjpegService. " + std::string(strerror(errno)));
        };
    }
}