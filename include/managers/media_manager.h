#ifndef APP_MANAGERS_MEDIA_MANAGER_H
#define APP_MANAGERS_MEDIA_MANAGER_H

#include <stddef.h>

#include "contracts/camera_result.h"
#include "core/message.h"
#include "utilities/rgb_image.h"
#include "managers/media_manager_deps.h"

/* Use cases for local media: opening files, playing and recording voice
 * notes, taking photos. Borrows its dependencies. */
typedef struct MediaManager MediaManager;

MediaManager *media_manager_create(const MediaManagerDeps *deps);
void          media_manager_destroy(MediaManager *mgr);

/* Voice notes toggle play/stop; other media open in the system viewer. */
int         media_manager_activate(MediaManager *mgr, const char *path, MessageType type);
/* Copies a received file into the save folder (Downloads by default) as
 * `name`, never overwriting; the new path goes to `out`. */
int         media_manager_save_copy(MediaManager *mgr, const char *path, const char *name, char *out, size_t size);
void        media_manager_stop_playback(MediaManager *mgr);
/* Path of the voice note playing now, or "". */
const char *media_manager_playing(MediaManager *mgr);
/* How far into the voice note playing now, in milliseconds (0 when idle). */
int64_t     media_manager_playing_ms(MediaManager *mgr);

int  media_manager_start_recording(MediaManager *mgr);
/* Finishes the recording; on success fills path and seconds. */
int  media_manager_finish_recording(MediaManager *mgr, char *path, size_t size, int *seconds);
void media_manager_cancel_recording(MediaManager *mgr);
int  media_manager_is_recording(MediaManager *mgr);
int  media_manager_recording_seconds(MediaManager *mgr);
/* True once the configured maximum length is reached (the client then sends). */
int  media_manager_recording_limit_reached(MediaManager *mgr);


/* Taking a photo: a live camera picture to aim with, then a snapped frame. */
/* True when a camera can take photos on this system. */
int          media_manager_camera_available(MediaManager *mgr);
/* Starts the live picture; returns 0 when the camera started. */
int          media_manager_start_camera(MediaManager *mgr);
/* Returns 1 and fills `frame` when a new picture arrived; it stays valid until the next call. */
int          media_manager_camera_frame(MediaManager *mgr, RgbImage *frame);
/* Saves the current picture into the outgoing media folder; its path goes to `out`. */
CameraResult media_manager_snap_photo(MediaManager *mgr, char *out, size_t size);
void         media_manager_stop_camera(MediaManager *mgr);
/* Recording a video from the live picture, with sound from the microphone. */
int          media_manager_start_video(MediaManager *mgr);
/* Finishes the video and stops the camera; on success fills path and seconds. */
int          media_manager_finish_video(MediaManager *mgr, char *path, size_t size, int *seconds);
int          media_manager_is_recording_video(MediaManager *mgr);
int          media_manager_video_seconds(MediaManager *mgr);
/* True once the longest video tawk records is reached (the client then stops). */
int          media_manager_video_limit_reached(MediaManager *mgr);

#endif
