#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <cstdint>
#include <memory>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <signal.h>
extern "C" {
#define restrict __restrict
#include <neatvnc.h>
#include <drm_fourcc.h>
#include <pixman.h>
#include <pthread.h>
#define AML_UNSTABLE_API 1
#include <aml.h>

}

#define BYTES_PER_PIXEL 4

class VncServer {
public:
    VncServer(uint32_t width = 640, uint32_t height = 480, int port = 5900)
        : width(width), height(height), port(port), fb_pool(nullptr), server(nullptr), display(nullptr), aml(nullptr) {
        framebuffer = new uint32_t[width * height];  // Allocate framebuffer dynamically
    }

    ~VncServer() {
        if (eventLoopThread.joinable()) {
            eventLoopThread.join();
        }
        delete[] framebuffer;  // Free framebuffer memory
    }
     uint32_t* getFramebuffer() {
        return framebuffer;
    }
    static void on_sigint()
{
	aml_exit(aml_get_default());
}
    bool start() {
        aml = aml_new();
    aml_set_default(aml);
        server = nvnc_open("0.0.0.0", port);
        if (!server) {
            std::cerr << "Failed to open VNC server on port " << port << std::endl;
            return false;
        }

        display = nvnc_display_new(0, 0);
        if (!display) {
            std::cerr << "Failed to create VNC display." << std::endl;
            nvnc_close(server);
            return false;
        }

        nvnc_add_display(server, display);
        nvnc_set_name(server, "WebOS");

        fb_pool = nvnc_fb_pool_new(width, height, DRM_FORMAT_ABGR8888, width);
        if (!fb_pool) {
            std::cerr << "Failed to create framebuffer pool." << std::endl;
            nvnc_close(server);
            return false;
        }

        std::cout << "VNC server started on port " << port << std::endl;

        eventLoopThread = std::thread([this]() {
        aml_run(aml);
    });
        return true;
    }

    void updateFramebuffer(const uint32_t* data, size_t size) {
        if (!data || size != width * height * BYTES_PER_PIXEL) {
            std::cerr << "Framebuffer size mismatch!" << std::endl;
            return;
        }

        struct nvnc_fb* fb = nvnc_fb_pool_acquire(fb_pool);
        if (!fb) {
            std::cerr << "Failed to acquire framebuffer from pool" << std::endl;
            return;
        }

        try {
            if (data) {
                std::memcpy(nvnc_fb_get_addr(fb), data, size);  // Copy framebuffer data
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during memory copy: " << e.what() << std::endl;
        }

        struct pixman_region16 damage;
        pixman_region_init_rect(&damage, 0, 0, width, height);
        nvnc_display_feed_buffer(display, fb, &damage);

        pixman_region_fini(&damage);
        nvnc_fb_pool_release(fb_pool, fb);
    }

    void fillFramebuffer(uint32_t color) {
        for (size_t i = 0; i < width * height; i++) {
            framebuffer[i] = color;
        }
    }

private:
    uint32_t width;
    uint32_t height;
    int port;
    uint32_t* framebuffer;
    struct nvnc* server;
    struct nvnc_display* display;
    struct nvnc_fb_pool* fb_pool;
    struct aml* aml=nullptr;
    std::thread eventLoopThread;
};

int main() {
    VncServer vncServer(640, 480, 5900);

    if (!vncServer.start()) {
        return EXIT_FAILURE;
    }

    vncServer.fillFramebuffer(0x00FF00);  // Fill with green

    while (1) {
        usleep(100000);  // Simulate update interval
        vncServer.updateFramebuffer(vncServer.getFramebuffer(), 640 * 480 * BYTES_PER_PIXEL);
    }

    return EXIT_SUCCESS;
}
