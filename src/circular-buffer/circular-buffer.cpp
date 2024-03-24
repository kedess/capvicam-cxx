#include "circular-buffer.h"
#include <cstdint>
#include <cstring>

namespace capvicam {
    void CircularBuffer::push(const Buffer &buffer, size_t id) {
        if (buffer.len >= ImageBufferMaxSize) {
            throw std::runtime_error(
                "Could not add to buffer. Buffer size exceeded");
        }
        std::memcpy(data[head].data, buffer.start, buffer.len);
        data[head].len = buffer.len;
        data[head].id = id;
        head = (head + 1) % capacity;
        cnt++;
    }
    void CircularBuffer::push(std::byte *buffer, size_t len, size_t id) {
        if (len >= ImageBufferMaxSize) {
            throw std::runtime_error(
                "Could not add to buffer. Buffer size exceeded");
        }
        std::memcpy(data[head].data, buffer, len);
        data[head].len = len;
        data[head].id = id;
        head = (head + 1) % capacity;
        cnt++;
    }
    ImageBuffer &CircularBuffer::front() {
        return data[tail];
    }
    void CircularBuffer::pop() {
        tail = (tail + 1) % capacity;
        cnt--;
    }
} // namespace capvicam