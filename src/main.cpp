#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdint.h>

#include <linux/videodev2.h>

#include <libfreenect.h>
#include "../libs/libfreenect_sync.h"
#include <libfreenect_registration.h>

#include <opencv2/opencv.hpp>

#ifndef VID_WIDTH
#define VID_WIDTH 640
#endif

#ifndef VID_HEIGHT
#define VID_HEIGHT 480
#endif

#define VIDEO_IN  "/dev/video0"
#define VIDEO_OUT "/dev/video3" //TODO: make generic

//output device.
int output;
bool should_run = true;

size_t framesize = VID_WIDTH * VID_HEIGHT * 3;

freenect_context* fn_ctx;
freenect_device* fn_dev;

// Buffers
uint16_t *depth_buf;
uint8_t *vid_buf;

void depth_cb(freenect_device* dev, void* data, uint32_t timestamp)
{
    // Here we must change the buffer a little bit. Cause depth and video have
    // different sizes.
    memcpy(depth_buf, data, sizeof(uint16_t) * VID_HEIGHT * VID_WIDTH);
}

void vid_cb(freenect_device* dev, void* data, uint32_t timestamp)
{
    memcpy(vid_buf, data, 3 * sizeof(uint8_t) * VID_HEIGHT * VID_WIDTH);
}

void quit_func()
{
    printf("Something wrong happened!\n");
    exit(1);
}

void signal_handler(int signal)
{
    if (signal == SIGINT
    ||	signal == SIGTERM)
	should_run = false;
}

int get_frames()
{
    uint16_t *depth = 0;
    uint8_t *rgb = 0;			    //3 since it's RGB
    uint8_t *registered_rgb = (uint8_t*) malloc(3 * VID_HEIGHT * VID_WIDTH);
    uint32_t ts;
    printf("Working...\n");
    freenect_device* kinect;

    //Setting LED color. Also will start the loop.
    freenect_sync_set_led(LED_BLINK_RED_YELLOW , 0);
    
    //Getting reference after init.
    kinect = freenect_sync_get_device_ref(0);
    printf("Kinect device pointer: %p\n", kinect);

    
    while(should_run)
    {
        if (freenect_sync_get_depth((void**)&depth, &ts, 0, FREENECT_DEPTH_11BIT) < 0)
            quit_func();
        
        if (freenect_sync_get_video((void**)&rgb, &ts, 0, FREENECT_VIDEO_RGB) < 0)
            quit_func();

	printf("D: %p RGB: %p RRGB: %p\n", depth, rgb, registered_rgb);
	freenect_map_rgb_to_depth(kinect, depth, rgb, registered_rgb);

	//memcpy(registered_rgb, rgb, sizeof(uint8_t) * 3 * VID_HEIGHT * VID_WIDTH);
	// Stereo correction.
	printf("mapped\n");

	//Applies the threshold onto the frame.
        for (int i = 0; i < VID_HEIGHT; ++i)
        {
            for (int j = 0; j < VID_WIDTH; ++j)
            {        
                if(depth[VID_WIDTH * i + j] > 700)
                {
                    registered_rgb[(VID_WIDTH * i + j) * 3] = 0;
                    registered_rgb[(VID_WIDTH * i + j) * 3 + 1] = 0xFF;
                    registered_rgb[(VID_WIDTH * i + j) * 3 + 2] = 0;
                }
            }
        }

        //generate opencv mat here.
        // grab frame
     
        cv::Mat frame(VID_HEIGHT, VID_WIDTH, CV_8UC3, registered_rgb);
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

    free(registered_rgb);

    return 0;
}

int main(int argc, char* argv[])
{
    /***OUTPUT STREAM CONFIG********/
    // open output device
    output = open(VIDEO_OUT, O_RDWR);
    if(output < 0)
    {
        std::cerr << "ERROR: could not open output device!\n" << strerror(errno);
        return -2;
    }
 
    // configure params for output device
    struct v4l2_format vid_format;
    memset(&vid_format, 0, sizeof(vid_format));
    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(output, VIDIOC_G_FMT, &vid_format) < 0)
    {
        std::cerr << "ERROR: unable to get video format!\n" << strerror(errno);
        return -1;
    }

    // NOTE: Available pixel formats.
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
    /***END OUTPUT STREAM CONFIG***/

    /***KINECT CONFIGURATION*******/
    //freenect_context

    /***END KINECT CONFIGURATION***/

    // loop over these actions:
    get_frames();
    std::cout << "\n\nFinish, bye!\n";
    freenect_sync_stop();

}
