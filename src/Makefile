
CHROME           = $(shell \
	which chromium google-chrome | head -n1)
V4L2LOOPBACK_VER = $(shell \
	apt-cache policy v4l2loopback-dkms | sed -n 2p | cut -d" " -f 4)

VID_WIDTH  = 640
VID_HEIGHT = 480
CXXFLAGS   = `pkg-config --cflags opencv4` \
			  -DVID_WIDTH=$(VID_WIDTH) -DVID_HEIGHT=$(VID_HEIGHT)\
			  -I./include
LDLIBS     = `pkg-config --libs opencv4`
TARGET     = main


all: $(TARGET)

setup-devices:
	@echo "- Using v4l2loopback version: $(V4L2LOOPBACK_VER)"
	@echo "- Inserting module..."
	@if (lsmod | grep v4l2loopback > /dev/null); then sudo rmmod v4l2loopback; fi
	@sudo modprobe v4l2loopback \
		devices=1 exclusive_caps=1 video_nr=6 \
	    card_label="OpenCV Camera"
	@sleep 1
	@echo "- Ready. Available devices:\n"
	@v4l2-ctl --list-devices -d5

.PHONY: clean
clean:
	$(RM) $(TARGET)
