#pragma once
#include <cstddef>
#include <cstdint>

const unsigned int BUFFERS_NUM = 8;

namespace capvicam {
    struct Buffer {
        void *start = nullptr;
        size_t len;
    };
} // namespace capvicam

using OnImageReceived = void (*)(const capvicam::Buffer, size_t);

namespace capvicam {

    class Capture final {
    public:
        Capture(const char *path, uint16_t width, uint16_t height);
        ~Capture();
        void run(OnImageReceived callback);

    private:
        void create_buffers();
        int fd = -1;
        struct v4l2_capability *caps = nullptr;
        Buffer buffers[BUFFERS_NUM] = {};
    };
} // namespace capvicam