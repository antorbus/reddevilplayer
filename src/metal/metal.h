#ifndef UTILS_H
#define UTILS_H

#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <AudioToolbox/AudioToolbox.h>

typedef enum{
    STATE_UNINITIALIZED = 0,
    STATE_INITIALIZED,
    STATE_DONE,
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
int audio_player_open(char *path);
int audio_player_play_pause();
void audio_player_destroy(void);
CGEventRef keypress_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *info);

#endif