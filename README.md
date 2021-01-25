# cronect

Using Kinect v1 camera as a chromakey.

That's it: no need for a messy green screen behind you to record videos!

## Usage:

- Install [libfreenect](https://github.com/OpenKinect/libfreenect), which are the drivers needed for interfacing with the Kinect (we also need the "c_sync" wrapper).
- Run `make setup-devices`, which configures a virtual camera, using v4l2-loopback (video4linux2).
- Run `make && make run`

## Issues:

- Perspective is wrong, so there's a significant offset in the captured image and the depth values.
- Some noise appears on the image.

## Thanks:

- Thanks to [Oscar Aceña](https://bitbucket.org/OscarAcena/) for his work on [ocv-virtual-cam](https://bitbucket.org/OscarAcena/ocv-virtual-cam)
