#include <csignal>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "capture.h"

namespace cvc = capvicam;

extern volatile std::sig_atomic_t signal_num;

capvicam::Capture::Capture(const char *path, uint16_t width, uint16_t height) {
    this->fd = open(path, O_RDWR);
    if (-1 == this->fd) {
        throw std::runtime_error("Could not open device (" + std::string(path) +
                                 "). " + std::string(strerror(errno)));
    }
    std::cout << "[INFO ]: Open device (" << path << "). successfully"
              << std::endl;
    struct v4l2_capability *caps = new struct v4l2_capability;
    if (-1 == ioctl(fd, VIDIOC_QUERYCAP, caps)) {
        throw std::runtime_error("Could not read device capability. " +
                                 std::string(strerror(errno)));
    }
    std::cout << "[INFO ]: Device capability driver = " << caps->driver
              << ", card = " << caps->card << ", bus_info = " << caps->bus_info
              << ", version = " << ((caps->version >> 16) & 0xFF) << "."
              << ((caps->version >> 8) & 0xFF) << "." << (caps->version & 0xFF)
              << std::endl;
    if (!(caps->device_caps & V4L2_CAP_STREAMING) &&
        (caps->device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
        throw std::runtime_error("Not support video streaming");
    }
    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt)) {
        throw std::runtime_error("Could not set format params. " +
                                 std::string(strerror(errno)));
    }
    this->create_buffers();
}

capvicam::Capture::~Capture() {
    for (unsigned int idx = 0; idx < BUFFERS_NUM; idx++) {
        munmap(this->buffers[idx].start, this->buffers[idx].len);
    }

    enum v4l2_buf_type kind = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_STREAMOFF, &kind) == -1) {
        std::cout << "[ERROR]: an error occurred during the stop streaming. "
                  << strerror(errno) << std::endl;
        return;
    }
    std::cout << "[INFO ]: Stopped streaming successfully" << std::endl;
    if (this->fd != -1) {
        close(this->fd);
        std::cout << "[INFO ]: Closed device successfully" << std::endl;
    }
    delete this->caps;
}

void capvicam::Capture::run(OnImageReceived callback) {
    static size_t frame_idx = {};
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_STREAMON, &type)) {
        throw std::runtime_error("Could not start streaming. " +
                                 std::string(strerror(errno)));
    }
    std::cout << "[INFO ]: Started streaming successfully" << std::endl;
    while (-1 == signal_num) {
        struct v4l2_buffer buf;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        int ff = ioctl(fd, VIDIOC_DQBUF, &buf);
        if (ff == EAGAIN) {
            continue;
        }
        if (ff == -1) {
            throw std::runtime_error("An error occurred during streaming. " +
                                     std::string(strerror(errno)));
        }
        if (-1 == ioctl(fd, VIDIOC_QBUF, &buf)) {
            throw std::runtime_error("An error occurred during streaming. " +
                                     std::string(strerror(errno)));
        }
        frame_idx++;
        callback(this->buffers[buf.index], frame_idx);
    }
}

void capvicam::Capture::create_buffers() {
    struct v4l2_requestbuffers req = {};
    req.count = BUFFERS_NUM;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == ioctl(this->fd, VIDIOC_REQBUFS, &req)) {
        throw std::runtime_error("Could not request buffers. " +
                                 std::string(strerror(errno)));
    }
    for (unsigned int idx = 0; idx < req.count; idx++) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = idx;
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            throw std::runtime_error("Could not request buffers. " +
                                     std::string(strerror(errno)));
        }
        Buffer buffer;
        buffer.len = buf.length;
        buffer.start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, buf.m.offset);
        if (MAP_FAILED == buffer.start) {
            throw std::runtime_error("Could not request buffers. " +
                                     std::string(strerror(errno)));
        }
        this->buffers[idx] = buffer;
    }
    for (unsigned int idx = 0; idx < req.count; idx++) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = idx;
        if (-1 == ioctl(fd, VIDIOC_QBUF, &buf)) {
            throw std::runtime_error("Could not request buffers. " +
                                     std::string(strerror(errno)));
        }
    }
}