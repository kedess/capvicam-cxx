#include "processing.h"
#include "../circular-buffer/circular-buffer.h"
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>

extern volatile std::sig_atomic_t signal_num;
extern std::mutex processing_mutex;
extern std::condition_variable processing_condvar;
extern capvicam::CircularBuffer image_processing_queue;

extern std::mutex mjpeg_mutex;
extern std::condition_variable mjpeg_condvar;
extern capvicam::CircularBuffer image_mjpeg_queue;

namespace capvicam {
    void Processing::run() {
        std::unique_ptr<ImageBuffer> buffer = std::make_unique<ImageBuffer>();
        while (-1 == signal_num) {
            std::unique_lock lk{processing_mutex};
            processing_condvar.wait_for(lk, std::chrono::seconds(1), [] {
                return !image_processing_queue.empty();
            });
            bool update = false;
            if (!image_processing_queue.empty()) {
                update = true;
                ImageBuffer &value = image_processing_queue.front();
                std::memcpy(buffer->data, value.data, value.len);
                buffer->len = value.len;
                buffer->id = value.id;
                // std::cout << "[DEBUG]: received buffer (processing), id = "
                // << value.id << std::endl;
                image_processing_queue.pop();
            }
            lk.unlock();
            if (update) {
                // Поступило новое изображение. Обрабатываем
                {
                    std::lock_guard lk_{mjpeg_mutex};
                    if (!image_mjpeg_queue.full()) {
                        image_mjpeg_queue.push(buffer->data, buffer->len,
                                               buffer->id);
                        mjpeg_condvar.notify_one();
                    }
                }
            }
        }
    }
} // namespace capvicam