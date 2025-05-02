#ifndef AUDIO_H
#define AUDIO_H

#include "rd.h"
#include "../external/miniaudio/miniaudio.h"
#include "../external/miniaudio/extras/decoders/libvorbis/miniaudio_libvorbis.h"

typedef enum{
    STATE_UNINITIALIZED = 0,
    STATE_INITIALIZED,
    STATE_DONE,
    STATE_PAUSED,
    STATE_PLAYING,
} audio_player_state;

#define NUM_SOUND_PREV 1024
#define NUM_DECODER_BACKENDS 1
extern struct audio_player_context {
    ma_engine                   engine;
    ma_resource_manager         resource_manager;    
    ma_decoding_backend_vtable* decoder_backends[NUM_DECODER_BACKENDS];
    int                         num_sounds;
    char                        *names; //PATH_MAX x num_sounds
    int                         sound_prev_idx[NUM_SOUND_PREV];
    int                         sound_curr_idx;
    ma_sound                    *sounds;
    playback_command_flag_t     flags;
    audio_player_state          state;
    playback_command_t          command;
    playback_command_flag_t     command_flag;
    struct command_queue        command_buffer;
} audio_player;

int audio_command_play_pause(void);
int audio_command_next(void);
int audio_command_prev(void);
int audio_command_close(void);
int audio_command_seek(void);
int audio_command_toggle_random(void);
int audio_command_toggle_loop(void);
int audio_command_open(void);


void audio_player_destroy(void);
void sound_end_callback(void *p_user_data, ma_sound *p_sound);
int free_sounds(void);

#endif