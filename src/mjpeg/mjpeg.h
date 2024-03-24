#pragma once
#include "../constants.h"
#include <iostream>
#include <unistd.h>
#include <unordered_map>

class Session {
public:
    Session() {
        buffer = new std::byte[ImageBufferMaxSize];
    }
    ~Session() {
        if (buffer) {
            delete[] buffer;
        }
    }
    Session(const Session &p) = delete;
    Session &operator=(const Session &) = delete;
    Session(Session &&p) {
        std::swap(this->buffer, p.buffer);
        std::swap(this->pos, p.pos);
        std::swap(this->len, p.len);
    }
    Session &operator=(Session &&) = delete;
    std::byte *buffer = nullptr;
    size_t pos = 0;
    size_t len = 0;
    bool is_start = true;
};

namespace capvicam {
    class MjpegService {
    public:
        MjpegService(const char *address, unsigned port) {
            std::cout << "[INFO ]: Started MjpegService" << std::endl;
            create_socket(address, port);
            create_epoll();
        }
        ~MjpegService() {
            if (fd >= 0) {
                close(fd);
            }
            if (epoll_fd >= 0) {
                close(epoll_fd);
            }
            for (auto &session : sessions) {
                close(session.first);
            }
            std::cout << "[INFO ]: Stopped MjpegService" << std::endl;
        }
        void run();

    private:
        std::unordered_map<int, Session> sessions;
        void create_socket(const char *address, unsigned port);
        void create_epoll();
        int fd = -1;
        int epoll_fd = -1;
    };
} // namespace capvicam