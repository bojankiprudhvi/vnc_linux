-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs \
    -lX11 -lXext -lXshm -lpthread

     gcc -I/usr/include/pixman-1 -I/usr/include/libdrm -I/usr/include/opencv4/ sample_vnc_server.cpp -o simple_vnc_server -L/usr/local/lib -lneatvnc -lpixman-1 -laml -lpthread -lstdc++ -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs  -lopencv_videoio -lX11 -lXext 