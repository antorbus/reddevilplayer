#ifndef UTILS_H
#define UTILS_H

#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <AudioToolbox/AudioToolbox.h>

typedef enum{
    STATE_DONE = 0,
    STATE_PAUSED,
    STATE_PLAYING,
} audio_player_state;

extern struct metal_audio_player {
    ExtAudioFileRef     file;
    AudioQueueRef       queue;
    UInt32              bytes_per_frame;
    AudioQueueBufferRef buf;
    audio_player_state  state;
} audio_player;


int choose_directory(char *path, size_t max_len);

CGEventRef keypress_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *info);

#endif