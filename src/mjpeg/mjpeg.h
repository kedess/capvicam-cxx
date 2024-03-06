#pragma once
#include <iostream>
#include <unistd.h>

namespace capvicam {
    class MjpegService {
        public:
            MjpegService(const char * address, unsigned port) {
                std::cout << "[INFO ]: Started MjpegService" << std::endl;
                create_socket(address, port);
            }
            ~MjpegService() {
                if (fd >= 0) {
                    close(fd);
                }
                std::cout << "[INFO ]: Stopped MjpegService" << std::endl;
            }
            void run();
        private:
            void create_socket(const char* address, unsigned port);
            int fd = -1;
    };
}