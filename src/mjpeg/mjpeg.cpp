#include "mjpeg.h"
#include "../circular-buffer/circular-buffer.h"
#include <arpa/inet.h>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <string.h>
#include <sys/epoll.h>
#include <thread>

extern volatile std::sig_atomic_t signal_num;

extern std::mutex mjpeg_mutex;
extern std::condition_variable mjpeg_condvar;
extern capvicam::CircularBuffer image_mjpeg_queue;

const int MAX_CLIENTS = 100;
const char *MAIN_HEADER =
    "HTTP/1.0 200 OK\r\nContent-Type: "
    "multipart/x-mixed-replace;boundary=mjpegstream\r\n\r\n";

static int set_nonblocking_socket(int fd) {
    int flags;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void send_data(int socket, Session &session) {
    ssize_t res =
        write(socket, session.buffer + session.pos, session.len - session.pos);
    if (res == -1) {
        session.pos = 0;
        session.len = 0;
        return;
    }
    session.pos += static_cast<size_t>(res);
}

namespace capvicam {
    void MjpegService::run() {
        while (-1 == signal_num) {
            std::unique_lock lk{mjpeg_mutex};
            mjpeg_condvar.wait_for(lk, std::chrono::milliseconds(10),
                                   [] { return !image_mjpeg_queue.empty(); });
            if (!image_mjpeg_queue.empty()) {
                ImageBuffer &value = image_mjpeg_queue.front();
                for (auto &session : sessions) {
                    if (session.second.pos == session.second.len) {
                        std::stringstream ss;
                        if (session.second.is_start) {
                            session.second.is_start = false;
                            ss << MAIN_HEADER;
                        }
                        ss << "--mjpegstream\r\nContent-Type: "
                              "image/jpeg\r\nContent-Length: "
                           << value.len << "\r\n\r\n";
                        auto str = ss.str();
                        std::memcpy(session.second.buffer, str.data(),
                                    str.size());
                        session.second.len = value.len + str.size();
                        session.second.pos = 0;
                        std::memcpy(session.second.buffer + str.size(),
                                    value.data, value.len);
                    }
                }
                image_mjpeg_queue.pop();
            }
            lk.unlock();
            struct sockaddr_in peer_addr;
            int addr_len = sizeof(peer_addr);
            struct epoll_event events[MAX_CLIENTS];
            struct epoll_event event;
            int fds = epoll_wait(epoll_fd, events, MAX_CLIENTS, 10);

            for (int i = 0; i < fds; i++) {
                if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (events[i].events & EPOLLRDHUP)) {
                    close(events[i].data.fd);
                    auto it = sessions.find(events[i].data.fd);
                    if (it != sessions.end()) {
                        // std::cout << "[DEBUG]: closed client connection" <<
                        // std::endl;
                        close(events[i].data.fd);
                        sessions.erase(it);
                    }
                    continue;
                }
                if (events[i].data.fd == fd) {
                    // std::cout << "[DEBUG]: accept new client" << std::endl;
                    int conn_sock = accept(
                        fd, reinterpret_cast<struct sockaddr *>(&peer_addr),
                        reinterpret_cast<socklen_t *>(&addr_len));
                    if (conn_sock == -1) {
                        close(conn_sock);
                        continue;
                    }
                    if (set_nonblocking_socket(conn_sock) == -1) {
                        close(conn_sock);
                        continue;
                    }
                    event.events = EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
                    event.data.fd = conn_sock;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &event) ==
                        -1) {
                        close(conn_sock);
                        continue;
                    }
                    Session session;
                    sessions.insert(
                        std::make_pair(conn_sock, std::move(session)));
                } else {
                    if (events[i].events & EPOLLOUT) {
                        auto it = sessions.find(events[i].data.fd);
                        if (it != sessions.end()) {
                            if (it->second.pos != it->second.len) {
                                send_data(events[i].data.fd, it->second);
                            }
                        }
                    }
                }
            }
        }
    }
    void MjpegService::create_socket(const char *address, unsigned port) {
        struct sockaddr_in server_addr;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw std::runtime_error(
                "Could not create socket for MjpegService. " +
                std::string(strerror(errno)));
        }
        int enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
            0) {
            throw std::runtime_error(
                "Could not create socket for MjpegService. " +
                std::string(strerror(errno)));
        }
        if (set_nonblocking_socket(fd) == -1) {
            throw std::runtime_error(
                "Could not create socket for MjpegService. " +
                std::string(strerror(errno)));
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(address);
        server_addr.sin_port = htons(static_cast<__uint16_t>(port));

        if (bind(fd, reinterpret_cast<struct sockaddr *>(&server_addr),
                 sizeof(server_addr)) < 0) {
            throw std::runtime_error(
                "Could not create socket for MjpegService. " +
                std::string(strerror(errno)));
        }

        if (listen(fd, SOMAXCONN) < 0) {
            throw std::runtime_error(
                "Could not create socket for MjpegService. " +
                std::string(strerror(errno)));
        };
    }
    void MjpegService::create_epoll() {
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw std::runtime_error("Could not create epoll fd");
        }
        struct epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    }
} // namespace capvicam