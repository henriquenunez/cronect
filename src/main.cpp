#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <libfreenect_sync.h>

#ifndef VID_WIDTH
#define VID_WIDTH 640
#endif

#ifndef VID_HEIGHT
#define VID_HEIGHT 480
#endif

#define VIDEO_IN  "/dev/video0"
#define VIDEO_OUT "/dev/video6"

//output device.
int output;

size_t framesize = VID_WIDTH * VID_HEIGHT * 3;

void quit_func()
{
    printf("deu ruim\n");
    exit(1);
}

int my_looop()
{
    short *depth = 0;
    char *rgb = 0;
    uint32_t ts;

    while(1)
    {
        printf("afes\n");
        if (freenect_sync_get_depth((void**)&depth, &ts, 0, FREENECT_DEPTH_11BIT) < 0)
            quit_func();
        
        if (freenect_sync_get_video((void**)&rgb, &ts, 0, FREENECT_VIDEO_RGB) < 0)
            quit_func();

        printf("[%d][%d][%d][%d][%d][%d][%d][%d]\n[%d][%d][%d][%d][%d][%d][%d][%d]\n", depth[100], depth[200], depth[300], depth[400], depth[500], depth[600], depth[700], depth[800], depth[900], depth[1000], depth[1100], depth[1200], depth[1300], depth[1400], depth[1500], depth[1600]);

        for (int i = 0; i < VID_HEIGHT; ++i)
        {
            for (int j = 0; j < VID_WIDTH; ++j)
            {
                
                if(depth[VID_WIDTH * i + j] > 700)
                {
                    rgb[(VID_WIDTH * i + j) * 3] = 0;
                    rgb[(VID_WIDTH * i + j) * 3 + 1] = 0xFF;
                    rgb[(VID_WIDTH * i + j) * 3 + 2] = 0;
                }
            }
        }

        //generate opencv mat here.
        // grab frame
     
        cv::Mat frame(VID_HEIGHT, VID_WIDTH, CV_8UC3, rgb);
        /*
        if (not cam.grab()) {
            std::cerr << "ERROR: could not read from camera!\n";
            break;
        }
        cam.retrieve(frame);
        */

        // apply simple filter (NOTE: result should be as defined PIXEL FORMAT)
        // convert twice because we need RGB24
        //cv::Mat result;
        //cv::cvtColor(frame, result, cv::COLOR_RGB2GRAY);
        //cv::cvtColor(result, result, cv::COLOR_GRAY2RGB);

        // show frame
        // cv::imshow("gui", frame);

        // write frame to output device
        size_t written = write(output, frame.data, framesize);
        if (written < 0) {
            std::cerr << "ERROR: could not write to output device!\n";
            close(output);
            break;
        }

        // wait for user to finish program pressing ESC
        //if (cv::waitKey(10) == 27)
            //break;
    }

    return 0;
}

int
main(int argc, char* argv[]) {

    // open and configure input camera (/dev/video0)
    /*cv::VideoCapture cam(VIDEO_IN);
    if (not cam.isOpened()) {
        std::cerr << "ERROR: could not open camera!\n";
        return -1;
    }
    cam.set(cv::CAP_PROP_FRAME_WIDTH, VID_WIDTH);
    cam.set(cv::CAP_PROP_FRAME_HEIGHT, VID_HEIGHT);
    */

    // open output device
    output = open(VIDEO_OUT, O_RDWR);
    if(output < 0) {
        std::cerr << "ERROR: could not open output device!\n" << strerror(errno);
        return -2;
    }

    // configure params for output device
    struct v4l2_format vid_format;
    memset(&vid_format, 0, sizeof(vid_format));
    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(output, VIDIOC_G_FMT, &vid_format) < 0) {
        std::cerr << "ERROR: unable to get video format!\n" << strerror(errno);
        return -1;
    }

    //vid_format.fmt.pix.width = cam.get(cv::CAP_PROP_FRAME_WIDTH);
    //vid_format.fmt.pix.height = cam.get(cv::CAP_PROP_FRAME_HEIGHT);

    // NOTE: change this according to below filters...
    // Chose one from the supported formats on Chrome:
    // - V4L2_PIX_FMT_YUV420,
    // - V4L2_PIX_FMT_Y16,
    // - V4L2_PIX_FMT_Z16,
    // - V4L2_PIX_FMT_INVZ,
    // - V4L2_PIX_FMT_YUYV,
    // - V4L2_PIX_FMT_RGB24,
    // - V4L2_PIX_FMT_MJPEG,
    // - V4L2_PIX_FMT_JPEG
    vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;

    vid_format.fmt.pix.sizeimage = framesize;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(output, VIDIOC_S_FMT, &vid_format) < 0) {
        std::cerr << "ERROR: unable to set video format!\n" << strerror(errno);
        return -1;
    }

    // loop over these actions:
    my_looop();

    std::cout << "\n\nFinish, bye!\n";
}
