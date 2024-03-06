#include "mjpeg.h"
#include <thread>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

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
            // std::cout << "Hello MjpegService" << std::endl;
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