#ifndef APP_INFRASTRUCTURE_FFMPEG_CAMERA_H
#define APP_INFRASTRUCTURE_FFMPEG_CAMERA_H

#include "contracts/i_camera.h"

/* The camera through ffmpeg (AVFoundation on macOS, Video4Linux on Linux):
 * a live preview of raw frames, and a snapped frame saved as a JPEG. */
ICamera *ffmpeg_camera_create(void);
/* The same with another ffmpeg input, such as "lavfi" "testsrc" in tests. */
ICamera *ffmpeg_camera_create_for(const char *input_format, const char *input);

#endif
