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
#include <filesystem>
#include <chrono>  // For time management
#define Status MyStatus // Rename to avoid conflict

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/Xutil.h>
#include <sys/shm.h>  // For shmat and shmdt
 // Include OpenCV headers
#include <fstream> 
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
    VncServer(uint32_t width = 960, uint32_t height = 540, int port = 5900)
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
 /*void dumpFramebufferToFileWithTimeStamp(const void *fb_addr, size_t size)
        {
        static int frame_cnt = 0;
        if (frame_cnt > 120)
            return;
        std::string directory = "/home/prudhviraj/neatvnc_build/example/dump/";

        if (!std::filesystem::exists(directory))
        {
            if (!std::filesystem::create_directory(directory))
            {
                std::cerr << "Failed to create directory: " << directory << std::endl;
                return;
            }
        }
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << directory << "framebuffer_dump_" << std::put_time(std::localtime(&time_t_now), "%Y%m%d_%H%M%S")
            << '_' << std::setfill('0') << std::setw(3) << ms.count() << ".raw";
        std::string fileName = oss.str();

        // Open file and write framebuffer data
        std::ofstream file(fileName, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << fileName << std::endl;
            return;
        }

        file.write(reinterpret_cast<const char *>(fb_addr), size);
        if (!file) {
            std::cerr << "Failed to write data to file: " << fileName << std::endl;
            return;
        }

        std::cout << "Saved framebuffer to " << fileName << std::endl;
        frame_cnt++;
    }  
*/
   /* void dump_from_queue()
    {
        while()
        {
            //if queue not empyt
            //wriete the data and free fb
            //queue pop
            
        }


    } 
    void push_to_queue(const uint32_t* data, size_t size,time)
    {
        //push the data into queue
    }*/
    void updateFramebuffer(const uint32_t* data, size_t size) {
        if (!data || size != width * height * BYTES_PER_PIXEL) {
            std::cerr << "Framebuffer size mismatch!" << std::endl;
            return;
        }
        std::cout<<"Dumping frame "<<"\n";
      //  dumpFramebufferToFileWithTimeStamp(data,size);
      //push fb and time stamp into queue;
      //push_to_queue()
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
         std::cout << "nvnc_fb_unref \n"  << std::endl;
      // nvnc_fb_pool_release(fb_pool, fb);
      nvnc_fb_unref(fb);
    }


   void captureScreen() {
    std::string folderPath ="/home/lg/vnc/neat_vnc_local_server/temp_frames";
    static int frameCount = 1; // Keep track of the current frame
    char fileName[256];
    snprintf(fileName, sizeof(fileName), "%s/frame_%d.raw", folderPath.c_str(), frameCount);

    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open raw file: " << fileName << std::endl;
        return;
    }

    // Read raw file directly into the framebuffer
    file.read(reinterpret_cast<char*>(framebuffer), width * height * 4);
    if (!file) {
        std::cerr << "Error reading raw file: " << fileName << std::endl;
        return;
    }

    // Increment the frame count for the next call
    frameCount++;
    if(frameCount>824)
    frameCount=1;
}


public:
    uint32_t width;
    uint32_t height;
    int port;
    uint32_t* framebuffer; 
    struct nvnc* server; 
    struct nvnc_display* display; 
    struct nvnc_fb_pool* fb_pool; 
    struct aml* aml = nullptr; 
    std::thread eventLoopThread;

   
};

int main() {
    VncServer vncServer(960, 540, 5900);
   if (!vncServer.start()) {
        return EXIT_FAILURE;
    }
    constexpr int fpsPrintIntervalSec = 1;
    uint32_t m_fps=24;
    std::chrono::milliseconds frame_interval(1000 / m_fps);
    const std::chrono::milliseconds print_interval_ms(fpsPrintIntervalSec * 1000);

    auto start_time = std::chrono::steady_clock::now();
    auto next_frame_time = start_time + frame_interval;
    auto fps_interval_start = start_time;
    uint32_t frame_count = 0;
    int frames_in_interval = 0;
    //start dump thread



    while (true) {
        vncServer.captureScreen();

         // Update the framebuffer in the VNC server
         vncServer.updateFramebuffer(vncServer.getFramebuffer(), vncServer.width * vncServer.height * BYTES_PER_PIXEL);

        frames_in_interval++;
        frame_count++;

        auto now = std::chrono::steady_clock::now();

        if (now - fps_interval_start >= print_interval_ms) {
            double elapsed_seconds = std::chrono::duration<double>(now - fps_interval_start).count();
            double fps_actual = frames_in_interval / elapsed_seconds;
            fps_interval_start = now;
            frames_in_interval = 0;
        }

        std::this_thread::sleep_until(next_frame_time);

        next_frame_time += frame_interval;

        if (frame_count % (m_fps * 3600) == 0) {
            start_time = std::chrono::steady_clock::now();
            next_frame_time = start_time + frame_interval;
            fps_interval_start = start_time;
            frame_count = 0;
        }
    }
     return EXIT_SUCCESS;
}
