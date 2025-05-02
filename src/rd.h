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

#define PLUGIN_GUI 1

int     rd_master_daemon(void);
void*   rd_audio_thread(void *arg);
void*   rd_key_monitor_thread(void *arg);
void    terminate(int sig);
void    print_bindings(void);
int     is_already_running(void);
int     initialize_ma(void);

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
    COMMAND_FLAG_NONE = 0,     
    COMMAND_FLAG_SEEK_0,
    COMMAND_FLAG_SEEK_10,
    COMMAND_FLAG_SEEK_20,
    COMMAND_FLAG_SEEK_30,
    COMMAND_FLAG_SEEK_40,
    COMMAND_FLAG_SEEK_50,
    COMMAND_FLAG_SEEK_60,
    COMMAND_FLAG_SEEK_70,
    COMMAND_FLAG_SEEK_80,
    COMMAND_FLAG_SEEK_90,
    COMMAND_FLAG_SEEK_FORWARD,
    COMMAND_FLAG_SEEK_BACKWARD,
} playback_command_flag_t;

typedef enum {
    MASTER_OK = 0,
    MASTER_FAILED,
} master_status_t;

extern struct playback_command_context{
    char                    path_working_dir[PATH_MAX];  
    master_status_t         master_command_execution_status; //set by the master command in question
    playback_command_t      command;
    playback_command_flag_t command_flag;
    pthread_cond_t          command_arrived;
    pthread_mutex_t         lock;
} master_command_context;


#define PLAYER_FLAG_NONE   0
#define PLAYER_FLAG_LOOP   (1<<0u)
#define PLAYER_FLAG_RANDOM (1<<1u)
 
#define SIZE_COMMAND_QUEUE 1024
struct command_queue{
    playback_command_t      commands[SIZE_COMMAND_QUEUE];
    playback_command_flag_t command_flags[SIZE_COMMAND_QUEUE];
    uint64_t                command_queue_len;
    pthread_cond_t          command_arrived;
    pthread_mutex_t         lock;
};


typedef int (*command_handler_function)(void);
extern command_handler_function master_command_handler[NUM_COMMANDS];
extern command_handler_function audio_command_handler[NUM_COMMANDS];

int command_none(void);

int master_command_open(void);
int master_command_kill(void);
int master_command_close(void);


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
void platform_specific_destroy(void);

#endif
