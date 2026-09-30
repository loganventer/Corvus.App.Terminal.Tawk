#ifndef APP_INFRASTRUCTURE_FFMPEG_VIDEO_POSTER_H
#define APP_INFRASTRUCTURE_FFMPEG_VIDEO_POSTER_H

#include "contracts/i_video_poster.h"

/* Grabs a frame half a second into the video with ffmpeg, in the
 * background, saved beside the video as <video>.poster.jpg. Reports no
 * poster when ffmpeg is not installed. */
IVideoPoster *ffmpeg_video_poster_create(void);

#endif
