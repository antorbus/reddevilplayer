#include "rd.h"
#include "metal.h"

int command_none(){return 0;}

//TODO
int master_command_play_pause(){
   
    return 0;
}

int audio_command_play_pause(){
    return audio_player_play_pause();
}//

//TODO
int master_command_close(){

    return 0;
}

int audio_command_close(){

    return 0;
}//

//TODO
int master_command_next(){
    return 0;
}

int audio_command_next(){
    return 0;
}//

//TODO
int master_command_prev(){
    return 0;
}

int audio_command_prev(){
    return 0;
}//

int master_command_open(){
    char prev_path_working_dir[PATH_MAX];
    memcpy(prev_path_working_dir, p.path_working_dir, PATH_MAX);
    if (choose_directory(p.path_working_dir, PATH_MAX) != 0){
        syslog(LOG_ERR, "ERROR: open failed.");
        memcpy(p.path_working_dir, prev_path_working_dir, PATH_MAX);
        p.master_command_execution_status = MASTER_FAILED;
    }
    syslog(LOG_INFO, "Set working dir to %s", p.path_working_dir);
    return 0;
}

int audio_command_open(){
    if (p.song_path_next[0] == 0){
        //we must load song
        DIR *dir;
        struct dirent *dp;
        if ((dir = opendir (p.path_working_dir)) == NULL) {
            syslog(LOG_ERR, "ERROR: Cannot open dir.");
            return -1;
        } 
        int song_found = 0;
        while ((dp = readdir (dir)) != NULL) {
            if (strstr(dp->d_name, ".mp3") != NULL){
                memcpy(p.song_path_next, p.path_working_dir, PATH_MAX);
                strcat(p.song_path_next, dp->d_name);
                syslog(LOG_INFO, "Found song %s", p.song_path_next);
                closedir(dir);
                song_found = 1;
                break;
            }
        }
        if (!song_found){
            closedir(dir);
            return -1;
        }
    }

    if (audio_player_open(p.song_path_next) != 0)
        return -1;
    memcpy(p.song_path_prev, p.song_path_current, PATH_MAX);
    memcpy(p.song_path_current, p.song_path_next, PATH_MAX);
    memset(p.song_path_next, 0, PATH_MAX);
    return 0;
}

int master_command_kill(){
    return -1;
}

int master_command_help(){
   
    if (CFUserNotificationDisplayNotice(
        10,                    
        kCFUserNotificationPlainAlertLevel, 
        NULL,                               
        NULL,                              
        NULL,                              
        CFSTR("Red Devil Player"),                             
        CFSTR(HELP_STR),                            
        NULL) != 0){
        p.master_command_execution_status = MASTER_FAILED;
        syslog(LOG_ERR, "ERROR: Failed to open help menu.");
    }
    
    return 0;
}

//TODO
int master_command_toggle_random(){
    return 0;
}

int audio_command_toggle_random(){
    return 0;
}//


command_handler_function master_command_handler[NUM_COMMANDS] = {
    [COMMAND_NONE]  = command_none,
    [COMMAND_CLOSE] = master_command_close,
    [COMMAND_PLAY_PAUSE] = master_command_play_pause,        
    [COMMAND_NEXT] = master_command_next,      
    [COMMAND_PREV] = master_command_prev,       
    [COMMAND_OPEN] = master_command_open, 
    [COMMAND_KILL] = master_command_kill,
    [COMMAND_HELP] = master_command_help,
    [COMMAND_TOGGLE_RANDOM] = master_command_toggle_random, 
};


command_handler_function audio_command_handler[NUM_COMMANDS] = {
    [COMMAND_NONE]  = command_none,
    [COMMAND_CLOSE] = audio_command_close,
    [COMMAND_PLAY_PAUSE] = audio_command_play_pause,        
    [COMMAND_NEXT] = audio_command_next,      
    [COMMAND_PREV] = audio_command_prev,       
    [COMMAND_OPEN] = audio_command_open, 
    [COMMAND_KILL] = command_none,
    [COMMAND_HELP] = command_none,
    [COMMAND_TOGGLE_RANDOM] = audio_command_toggle_random, 
};