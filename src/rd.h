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
    MODE_NORMAL = 0,     
    MODE_RANDOM,
    MODE_QUEUE,
} playback_mode_t;

#define STATUS_WAITING 0 
#define STATUS_DONE 1
#define STATUS_FAILED 2

// #define FLAG_ (1u<<0)

extern struct player{
    char                path_working_dir[PATH_MAX];
    char                song_path_current[PATH_MAX];
    char                song_path_next[PATH_MAX];
    char                song_path_prev[PATH_MAX];
    int                 audio_command_execution_status;
    playback_command_t  command;
    playback_mode_t     mode;
    // uint64_t            flags;
    pthread_mutex_t     lock;
    pthread_cond_t      cond_command;
    pthread_cond_t      cond_status;
} p;

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

#define SIGNAL_COMMAND(master_command, syslog_message)  \
                        do { \
                            syslog(LOG_INFO, syslog_message);\
                            pthread_mutex_lock(&p.lock);\
                            pthread_cond_signal(&p.cond_command);\
                            p.command = master_command;\
                            pthread_mutex_unlock(&p.lock); \
                        } while(0)

#endif
