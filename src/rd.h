#ifndef RD_H
#define RD_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <syslog.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include "../external/miniaudio/miniaudio.h"

int     rd_master_daemon(void);

void*   rd_audio_thread(void *arg);
void*   rd_key_monitor_thread(void *arg);
void    terminate(int sig);
void    print_bindings(void);
int     is_already_running(void);

typedef enum {
    COMMAND_NONE = 0,     
    COMMAND_CLOSE,
    COMMAND_PLAY_PAUSE,        
    COMMAND_NEXT,      
    COMMAND_PREV,       
    COMMAND_OPEN, 
    COMMAND_KILL,
    COMMAND_HELP,
    COMMAND_TOGGLE_RANDOM, 
    // COMMAND_SEEK,
    // COMMAND_TOGGLE_QUEUE, 
    NUM_COMMANDS,
} playback_command_t;

typedef enum {
    MASTER_OK = 0,
    MASTER_FAILED,
} master_status_t;

typedef enum {
    AUDIO_THREAD_DONE = 0,
    AUDIO_THREAD_WAITING,     
    AUDIO_THREAD_TERMINAL_FAILURE,
} audio_status_t;

// #define FLAG_ (1u<<0)

extern struct player{
    char                path_working_dir[PATH_MAX];
    char                sound_path_curr[PATH_MAX];
    char                sound_path_next[PATH_MAX];
    char                sound_path_prev[PATH_MAX];       
    master_status_t     master_command_execution_status; //set by the master command in question
    audio_status_t      audio_command_execution_status; //set by the audio thread NOT by the command TODO: make this better
    playback_command_t  command;
    // uint64_t            flags;
    pthread_mutex_t     lock;
    pthread_cond_t      cond_command;
    pthread_cond_t      cond_audio;
} p;


typedef enum{
    STATE_UNINITIALIZED = 0,
    STATE_INITIALIZED,
    STATE_DONE,
    STATE_PAUSED,
    STATE_PLAYING,
} audio_player_state;

extern struct audio_player {
    ma_engine           engine;
    ma_sound            prev_sound;
    ma_sound            curr_sound;
    ma_sound            next_sound;
    audio_player_state  state;
} ap;

typedef int (*command_handler_function)(void);
extern command_handler_function master_command_handler[NUM_COMMANDS];
extern command_handler_function audio_command_handler[NUM_COMMANDS];

int command_none(void);

int master_command_play_pause(void);
int audio_command_play_pause(void);

int master_command_next(void);
int audio_command_next(void);

int master_command_prev(void);
int audio_command_prev(void);

int master_command_open(void);
int audio_command_open(void);

int master_command_kill(void);

int master_command_help(void);

int master_command_toggle_random(void);
int audio_command_toggle_random(void);

int master_command_close(void);
int audio_command_close(void);


#define HELP_STR \
"- h (Help, opens bindings)\n"\
"- o (Open, opens file explorer so you can select folder with songs)\n"\
"- p (Play/Pause)\n"\
"- l (Loop)\n"\
"- 0-9 (goes to 0% to 90% of song playback)\n"\
"- f (Forwards 5 seconds)\n"\
"- b (Backwards 5 seconds)\n"\
"- k (Kill, closes player)\n"\
"- v (preVious song)\n"\
"- n (Next song)\n"\
"- r (toggle Random)\n"\
// "- spacebar + [song name] + enter (Searches song and plays closest match) "
//  add the othere 


extern pthread_t        audio_thread;
extern pthread_t        key_monitor_thread;

int choose_directory(char *path, size_t max_len);
int audio_player_open(char *path);
int audio_player_play_pause();
void audio_player_destroy(void);
void platform_specific_destroy(void);

#define SIGNAL_COMMAND(master_command, syslog_message)  \
                        do { \
                            syslog(LOG_INFO, syslog_message);\
                            pthread_mutex_lock(&p.lock);\
                            pthread_cond_signal(&p.cond_command);\
                            p.command = master_command;\
                            pthread_mutex_unlock(&p.lock); \
                        } while(0)

#endif
