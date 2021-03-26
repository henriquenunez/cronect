CXX = clang++
VID_WIDTH  = 640
VID_HEIGHT = 480
CXXFLAGS   = `pkg-config --cflags opencv4` \
	     `pkg-config --cflags libfreenect` \
			  -DVID_WIDTH=$(VID_WIDTH) -DVID_HEIGHT=$(VID_HEIGHT)
LDLIBS     = `pkg-config --libs opencv4` \
	     `pkg-config --libs libfreenect` \
	     -lpthread
TARGET     = main
BUILD_DIR = ../build

all: build_dir $(TARGET)

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

$(TARGET):
	${CXX} main.cpp ../libs/libfreenect_sync.c ${CXXFLAGS} -o ${BUILD_DIR}/${TARGET} ${LDLIBS}

build_dir:
	mkdir ${BUILD_DIR} || true

run:
	${BUILD_DIR}/${TARGET}

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR} || true
