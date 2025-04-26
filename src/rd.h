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
    COMMAND_TOGGLE_LOOP, 
    COMMAND_SEEK,
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

#define COMMAND_FLAG_NONE 0
#define COMMAND_FLAG_SEEK_0 1
#define COMMAND_FLAG_SEEK_10 2
#define COMMAND_FLAG_SEEK_20 3
#define COMMAND_FLAG_SEEK_30 4
#define COMMAND_FLAG_SEEK_40 5
#define COMMAND_FLAG_SEEK_50 6
#define COMMAND_FLAG_SEEK_60 7
#define COMMAND_FLAG_SEEK_70 8
#define COMMAND_FLAG_SEEK_80 9
#define COMMAND_FLAG_SEEK_90 10
#define COMMAND_FLAG_SEEK_FORWARD 11
#define COMMAND_FLAG_SEEK_BACKWARD 12

extern struct player{
    char                path_working_dir[PATH_MAX];  
    master_status_t     master_command_execution_status; //set by the master command in question
    audio_status_t      audio_command_execution_status; //set by the audio thread NOT by the command TODO: make this better
    playback_command_t  command;
    uint64_t            command_flag;
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

#define FLAG_LOOP   (1<<0u)
#define FLAG_RANDOM (1<<1u)
 
#define NUM_SOUND_PREV 1024

extern struct audio_player {
    ma_engine           engine;
    int                 num_sounds;
    char                *names; //PATH_MAX x num_sounds
    int                 sound_prev_idx[NUM_SOUND_PREV];
    int                 sound_curr_idx;
    ma_sound            *sounds;
    uint64_t            flags;
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

int audio_command_toggle_random(void);

int audio_command_toggle_loop(void);

int master_command_close(void);
int audio_command_close(void);

int audio_command_seek(void);


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
void audio_player_destroy(void);
void platform_specific_destroy(void);
void sound_end_callback(void *p_user_data, ma_sound *p_sound);
int free_sounds(void);

#define TRY_SIGNAL_COMMAND(master_command, syslog_message)  \
                        do { \
                            int rc = pthread_mutex_trylock(&p.lock);\
                            if (rc == 0){ \
                                pthread_cond_signal(&p.cond_command);\
                                p.command = master_command;\
                                p.command_flag = COMMAND_FLAG_NONE;\
                                syslog(LOG_INFO, syslog_message);\
                                pthread_mutex_unlock(&p.lock); \
                            } else{ \
                                syslog(LOG_ERR, "ERROR: Monitor thread could not acquire lock.");\
                            }\
                        } while(0)

#define TRY_SIGNAL_COMMAND_WITH_FLAG(master_command, syslog_message, flag)  \
                        do { \
                            int rc = pthread_mutex_trylock(&p.lock);\
                            if (rc == 0){ \
                                pthread_cond_signal(&p.cond_command);\
                                p.command = master_command;\
                                p.command_flag = flag;\
                                syslog(LOG_INFO, syslog_message);\
                                pthread_mutex_unlock(&p.lock); \
                            } else{ \
                                syslog(LOG_ERR, "ERROR: Monitor thread could not acquire lock.");\
                            }\
                        } while(0)

#define ma_seek_seconds_with_error_log(psound, seconds) \
                        do {\
                            if (ma_sound_seek_to_second(psound, seconds)!=0){\
                                syslog(LOG_ERR, "ERROR: Could not seek to %f seconds.", seconds);\
                            } else{\
                                syslog(LOG_INFO, "Seeked to %f seconds.", seconds);\
                            }\
                        } while(0)\

#endif
