#include "rd.h"

int command_none(){return 0;}

//TODO
int master_command_play_pause(){
   
    return 0;
}

int audio_command_play_pause(){
    if (ap.state == STATE_UNINITIALIZED){
        syslog(LOG_ERR, "ERROR: state uninitialized, nothing to play.");
        return 0;
    }
   
    switch (ap.state){

        // case STATE_DONE:
        //     syslog(LOG_INFO, "Playing new sound.");
        //     break;

        case STATE_PLAYING:
            if (ma_sound_stop(&ap.sounds[ap.sound_curr_idx]) != 0){
                syslog(LOG_ERR, "ERROR: Could not pause.");
                return -1;
            }
            ap.state = STATE_PAUSED;
            syslog(LOG_INFO, "Paused %s.", &ap.names[PATH_MAX*ap.sound_curr_idx]);
            break;
        
        case STATE_INITIALIZED:
            syslog(LOG_INFO, "Playing after.");
        case STATE_PAUSED:
            if (ma_sound_start(&ap.sounds[ap.sound_curr_idx]) != 0){
                syslog(LOG_ERR, "ERROR: Could not play.");
                return -1;
            }
            syslog(LOG_INFO, "Playing %s.", &ap.names[PATH_MAX*ap.sound_curr_idx]);
            ap.state = STATE_PLAYING;
            break;
            
        default:
            syslog(LOG_ERR, "ERROR: Unrecognized audio player state.");
            return -1;
            break;
    }
    return 0;
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
    if (ap.state == STATE_UNINITIALIZED){
        syslog(LOG_ERR, "ERROR: state uninitialized, nothing to play.");
        return 0;
    }
    int loop = 0;
    if (ap.flags & FLAG_LOOP){ //Next overrides loop, but sets it back after
        loop = 1;
        ap.flags ^= FLAG_LOOP;
    }
    sound_end_callback(NULL, &ap.sounds[ap.sound_curr_idx]);
    if (loop){
        ap.flags ^= FLAG_LOOP;
    }
    return 0;
}//

//TODO
int master_command_prev(){
    
    return 0;
}

int audio_command_prev(){
    if (ap.state == STATE_UNINITIALIZED){
        syslog(LOG_ERR, "ERROR: state uninitialized, nothing to play.");
        return 0;
    }
    if (ap.sound_prev_idx[0] != -1){
        ma_sound_stop(&ap.sounds[ap.sound_curr_idx]);
        syslog(LOG_INFO, "Sound finished.");
        ap.sound_curr_idx = ap.sound_prev_idx[0];
        for (int i = 0; i< NUM_SOUND_PREV-1; i++){
            ap.sound_prev_idx[i] = ap.sound_prev_idx[i+1];
        }
        ap.sound_prev_idx[NUM_SOUND_PREV-1] = -1;
        ap.state = STATE_PLAYING;
        syslog(LOG_INFO, "Playing prev sound %s.", &ap.names[PATH_MAX*ap.sound_curr_idx]);
        ma_sound_start(&ap.sounds[ap.sound_curr_idx]);
    }
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
    DIR *dir;
    struct dirent *dp;
    if ((dir = opendir (p.path_working_dir)) == NULL) {
        syslog(LOG_ERR, "ERROR: Cannot open dir.");
        return -1;
    } 
    int sounds_found = 0;
    //TODO USE GLOB
    while ((dp = readdir(dir)) != NULL) { 
        if ((strstr(dp->d_name, ".mp3") != NULL) || 
            (strstr(dp->d_name, ".flac") != NULL) ||
            (strstr(dp->d_name, ".wav") != NULL)) {
            syslog(LOG_INFO, "Found sound %s", dp->d_name);
            sounds_found++;
        }
    }
    if (!sounds_found){
        syslog(LOG_ERR, "Did not find sound.");
        closedir(dir);
        return 0;
    }
    if (free_sounds() !=0 ){
        closedir(dir);
        return -1;
    }
    ap.sounds = (ma_sound *)malloc(sounds_found * sizeof(ma_sound));
    ap.names =  (char *)calloc(sounds_found, PATH_MAX);
    if (ap.sounds == NULL){
        syslog(LOG_ERR, "Malloc failed to allocate space for %d sounds.", ap.num_sounds);
        return -1;
    }
    ap.num_sounds = sounds_found;
    syslog(LOG_INFO, "Malloc'd space for %d sounds.", ap.num_sounds);
    int sounds_loaded = 0;
    rewinddir(dir);
    char sound_path[PATH_MAX];
    while ((dp = readdir(dir)) != NULL) {
        if ((strstr(dp->d_name, ".mp3") != NULL) || 
            (strstr(dp->d_name, ".wav") != NULL) ||
            (strstr(dp->d_name, ".flac") != NULL)){
            memcpy(&ap.names[PATH_MAX*sounds_loaded], dp->d_name, PATH_MAX);
            memcpy(sound_path, p.path_working_dir, PATH_MAX);
            strcat(sound_path, dp->d_name);
            if (ma_sound_init_from_file(&ap.engine, sound_path, 0, NULL, NULL, &ap.sounds[sounds_loaded]) != 0){
                syslog(LOG_ERR, "ERROR: Could not initialize sound %s.", sound_path);
                closedir(dir);
                return -1;
            }
            if (ma_sound_set_end_callback(&ap.sounds[sounds_loaded], sound_end_callback, NULL) != 0){
                syslog(LOG_ERR, "ERROR: Could not set ma end callback for %s.", sound_path);
                closedir(dir);
                return -1;
            }
            // usleep(100000);
            sounds_loaded++;
            syslog(LOG_INFO, "Loaded sound %s", sound_path);
        }
    }
    closedir(dir);
    if (sounds_loaded != ap.num_sounds){
        syslog(LOG_ERR, "ERROR: Could not load all sounds.");
        return -1;
    }
    ap.sound_curr_idx = 0;
    for (int i = 0; i < NUM_SOUND_PREV; i++){
        ap.sound_prev_idx[i] = -1;
    }
    ap.state = STATE_INITIALIZED;
    syslog(LOG_INFO, "Successfully loaded all %d sounds.", sounds_loaded);
    return 0;
}

int master_command_kill(){
    return -1;
}

int master_command_help(){
   //TODO raygui
    
    return 0;
}


int audio_command_toggle_random(){
    ap.flags ^= FLAG_RANDOM;
    return 0;
}

int audio_command_toggle_loop(void){
    ap.flags ^= FLAG_LOOP;
    return 0;
}


int audio_command_seek(void){
    float sound_length_seconds;
    ma_sound *psound = &ap.sounds[ap.sound_curr_idx];
    if (ma_sound_get_length_in_seconds(psound, &sound_length_seconds)!=0){
        syslog(LOG_ERR, "ERROR: Could not get sound length.");
        return 0;
    }

    float sound_playback_time_seconds;
    if (ma_sound_get_cursor_in_seconds(psound, &sound_playback_time_seconds)){
        syslog(LOG_ERR, "ERROR: Could not get sound cursor.");
        return 0;
    }
    
    switch (p.command_flag){ //TODO is this OK? we should not be looking at p from audio commands
        case COMMAND_FLAG_SEEK_0:
            ma_seek_seconds_with_error_log(psound, 0.0f);
            break;
        case COMMAND_FLAG_SEEK_10:
            ma_seek_seconds_with_error_log(psound, 0.1f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_20:
            ma_seek_seconds_with_error_log(psound, 0.2f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_30:
            ma_seek_seconds_with_error_log(psound, 0.3f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_40:
            ma_seek_seconds_with_error_log(psound, 0.4f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_50:
            ma_seek_seconds_with_error_log(psound, 0.5f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_60:
            ma_seek_seconds_with_error_log(psound, 0.6f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_70:
            ma_seek_seconds_with_error_log(psound, 0.7f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_80:
            ma_seek_seconds_with_error_log(psound, 0.8f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_90:
            ma_seek_seconds_with_error_log(psound, 0.9f*sound_length_seconds);
            break;
        case COMMAND_FLAG_SEEK_FORWARD:
            ma_seek_seconds_with_error_log(psound, sound_playback_time_seconds+5.0f);
            break;
        case COMMAND_FLAG_SEEK_BACKWARD:
            ma_seek_seconds_with_error_log(psound, sound_playback_time_seconds-5.0f);
            break;
        
        default:
            syslog(LOG_ERR, "ERROR: Did not recognize command flag (seek).");
            break;
    }
    return 0;
}

command_handler_function master_command_handler[NUM_COMMANDS] = {
    [COMMAND_NONE]  = command_none,
    [COMMAND_CLOSE] = master_command_close,
    [COMMAND_PLAY_PAUSE] = master_command_play_pause,        
    [COMMAND_NEXT] = master_command_next,      
    [COMMAND_PREV] = master_command_prev,       
    [COMMAND_OPEN] = master_command_open, 
    [COMMAND_KILL] = master_command_kill,
    [COMMAND_HELP] = master_command_help,
    [COMMAND_TOGGLE_RANDOM] = command_none, 
    [COMMAND_TOGGLE_LOOP] = command_none,
    [COMMAND_SEEK] = command_none,
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
    [COMMAND_TOGGLE_LOOP] = audio_command_toggle_loop,
    [COMMAND_SEEK] = audio_command_seek,
};