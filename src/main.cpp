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
uint8_t *registered_rgb;

void depth_cb(freenect_device* dev, void* data, uint32_t timestamp)
{
    // Here we must change the buffer a little bit. Cause depth and video have
    // different sizes.
    printf("depth_cb\n");
    memcpy(depth_buf, data, sizeof(uint16_t) * VID_HEIGHT * VID_WIDTH);
}

void vid_cb(freenect_device* dev, void* data, uint32_t timestamp)
{
    printf("vid_cb\n");
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

//printf("D: %p RGB: %p RRGB: %p\n", depth, rgb, registered_rgb);
int process_frame()
{
    // Stereo correction.
    freenect_map_rgb_to_depth(fn_dev, depth_buf, vid_buf, registered_rgb);
    //for now, just a memcpy
    //memcpy(registered_rgb, vid_buf, 3 * VID_HEIGHT * VID_WIDTH);

    //Applies the threshold onto the frame.
    for (int i = 0; i < VID_HEIGHT; ++i)
    {
        for (int j = 0; j < VID_WIDTH; ++j)
        {
            if(depth_buf[VID_WIDTH * i + j] > 700)
            {
                registered_rgb[(VID_WIDTH * i + j) * 3] = 0;
                registered_rgb[(VID_WIDTH * i + j) * 3 + 1] = 0xFF;
                registered_rgb[(VID_WIDTH * i + j) * 3 + 2] = 0;
            }
        }
    }

    //cv::Mat frame(VID_HEIGHT, VID_WIDTH, CV_8UC3, registered_rgb);
    size_t written = write(output, registered_rgb, 3 * VID_HEIGHT * VID_WIDTH);
    if (written < 0) {
        std::cerr << "ERROR: could not write to output device!\n";
        close(output);
    }

    printf("Processed frame!\n");
    return 0;
}

int main(int argc, char* argv[])
{
    int rc = 0;

    /***OUTPUT STREAM CONFIG********/
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
    // Init libfreenect
    rc = freenect_init(&fn_ctx, NULL);
    if (rc < 0) return rc;

    // Use only cam for now
    freenect_set_log_level(fn_ctx, FREENECT_LOG_DEBUG);
    freenect_select_subdevices(fn_ctx, FREENECT_DEVICE_CAMERA);

    int num_devices = rc = freenect_num_devices(fn_ctx);
    if (rc < 0) return rc;
    if (num_devices == 0)
    {
	printf("No devices!\n");
	freenect_shutdown(fn_ctx);
	return 1;
    }

    rc = freenect_open_device(fn_ctx, &fn_dev, 0);
    if (rc < 0)
    {
	freenect_shutdown(fn_ctx);
	return rc;
    }

    rc = freenect_set_video_mode(fn_dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
    if (rc < 0)
    {
	freenect_shutdown(fn_ctx);
	return rc;
    }

    rc = freenect_set_depth_mode(fn_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM));
    if (rc < 0)
    {
	freenect_shutdown(fn_ctx);
	return rc;
    }

    // Callbacks
    freenect_set_video_callback(fn_dev, vid_cb);
    freenect_set_depth_callback(fn_dev, depth_cb);

    // Buffers
    vid_buf = (uint8_t*) malloc(3 * VID_HEIGHT * VID_WIDTH);
    registered_rgb = (uint8_t*) malloc(3 * VID_HEIGHT * VID_WIDTH);
    depth_buf = (uint16_t*) malloc(sizeof(uint16_t) * VID_HEIGHT * VID_WIDTH);

    // Start reading.
    rc = freenect_start_video(fn_dev);
    if (rc < 0)
    {
	freenect_shutdown(fn_ctx);

        free(depth_buf);
        free(vid_buf);
        free(registered_rgb);

	return rc;
    }

    rc = freenect_start_depth(fn_dev);
    if (rc < 0)
    {
	freenect_shutdown(fn_ctx);

        free(depth_buf);
        free(vid_buf);
        free(registered_rgb);

	return rc;
    }

    /***END KINECT CONFIGURATION***/

    while (should_run && freenect_process_events(fn_ctx) >= 0)
    {
	process_frame();
    }

    printf("Shutdown sequence commencing\n");

    freenect_stop_depth(fn_dev);
    freenect_stop_video(fn_dev);
    freenect_close_device(fn_dev);
    freenect_shutdown(fn_ctx);

    free(depth_buf);
    free(vid_buf);
    free(registered_rgb);

    printf("See ya!\n");

    return 0;
}
