#pragma once
#include "../capture/capture.h"
#include "../constants.h"
#include <iostream>

namespace capvicam {
    struct ImageBuffer {
        std::byte data[ImageBufferMaxSize];
        size_t len;
        size_t id;
    };

    class CircularBuffer {
    public:
        explicit CircularBuffer(size_t capacity) : capacity(capacity) {
            data = new ImageBuffer[capacity];
        }
        ~CircularBuffer() {
            if (data) {
                delete[] data;
            }
        }
        CircularBuffer(const CircularBuffer &p) = delete;
        CircularBuffer &operator=(const CircularBuffer &) = delete;
        CircularBuffer(CircularBuffer &&p) = delete;
        CircularBuffer &operator=(CircularBuffer &&) = delete;
        ImageBuffer &front();
        void pop();
        void push(const Buffer &buffer, size_t id);
        void push(std::byte *buffer, size_t len, size_t id);
        bool empty() const {
            return head == tail && cnt == 0;
        }
        bool full() const {
            return cnt == capacity;
        }

    private:
        ImageBuffer *data = nullptr;
        size_t capacity;
        size_t head = 0;
        size_t tail = 0;
        size_t cnt = 0;
    };
} // namespace capvicam
