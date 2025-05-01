#include "audio.h"

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

int audio_command_play_pause(){
    if (audio_player.state == STATE_UNINITIALIZED){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: state uninitialized, nothing to play.");
        return 0;
    }
   
    switch (audio_player.state){

        // case STATE_DONE:
        //     syslog(LOG_INFO, "Playing new sound.");
        //     break;

        case STATE_PLAYING:
            if (ma_sound_stop(&audio_player.sounds[audio_player.sound_curr_idx]) != 0){
                syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not pause.");
                return -1;
            }
            audio_player.state = STATE_PAUSED;
            syslog(LOG_INFO, "(AUDIO THREAD) Paused %s.", &audio_player.names[PATH_MAX*audio_player.sound_curr_idx]);
            break;
        
        case STATE_INITIALIZED:
            syslog(LOG_INFO, "(AUDIO THREAD) Playing after.");
        case STATE_PAUSED:
            if (ma_sound_start(&audio_player.sounds[audio_player.sound_curr_idx]) != 0){
                syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not play.");
                return -1;
            }
            syslog(LOG_INFO, "(AUDIO THREAD) Playing %s.", &audio_player.names[PATH_MAX*audio_player.sound_curr_idx]);
            audio_player.state = STATE_PLAYING;
            break;
            
        default:
            syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Unrecognized audio player state.");
            return -1;
            break;
    }
    return 0;
}//

//TODO
int audio_command_close(){

    return 0;
}//

int audio_command_next(){
    if (audio_player.state == STATE_UNINITIALIZED){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: state uninitialized, nothing to play.");
        return 0;
    }
    int loop = 0;
    if (audio_player.flags & PLAYER_FLAG_LOOP){ //Next overrides loop, but sets it back after
        loop = 1;
        audio_player.flags ^= PLAYER_FLAG_LOOP;
    }
    sound_end_callback(NULL, &audio_player.sounds[audio_player.sound_curr_idx]);
    if (loop){
        audio_player.flags ^= PLAYER_FLAG_LOOP;
    }
    return 0;
}//

int audio_command_prev(){
    if (audio_player.state == STATE_UNINITIALIZED){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: state uninitialized, nothing to play.");
        return 0;
    }
    if (audio_player.sound_prev_idx[0] != -1){
        ma_sound_stop(&audio_player.sounds[audio_player.sound_curr_idx]);
        syslog(LOG_INFO, "(AUDIO THREAD) Sound finished.");
        audio_player.sound_curr_idx = audio_player.sound_prev_idx[0];
        for (int i = 0; i< NUM_SOUND_PREV-1; i++){
            audio_player.sound_prev_idx[i] = audio_player.sound_prev_idx[i+1];
        }
        audio_player.sound_prev_idx[NUM_SOUND_PREV-1] = -1;
        audio_player.state = STATE_PLAYING;
        syslog(LOG_INFO, "(AUDIO THREAD) Playing prev sound %s.", &audio_player.names[PATH_MAX*audio_player.sound_curr_idx]);
        ma_sound_start(&audio_player.sounds[audio_player.sound_curr_idx]);
    }
    return 0;
}//


int audio_command_open(){
    DIR *dir;
    struct dirent *dp;
    if ((dir = opendir (master_command_context.path_working_dir)) == NULL) {
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Cannot open dir.");
        return -1;
    } 
    int sounds_found = 0;
    //TODO USE GLOB
    while ((dp = readdir(dir)) != NULL) { 
        if ((strstr(dp->d_name, ".mp3") != NULL) || 
            (strstr(dp->d_name, ".flac") != NULL) ||
            (strstr(dp->d_name, ".wav") != NULL)) {
            syslog(LOG_INFO, "(AUDIO THREAD) Found sound %s", dp->d_name);
            sounds_found++;
        }
    }
    if (!sounds_found){
        syslog(LOG_ERR, "(AUDIO THREAD) Did not find sound.");
        closedir(dir);
        return 0;
    }
    if (free_sounds() !=0 ){
        closedir(dir);
        return -1;
    }
    audio_player.sounds = (ma_sound *)malloc(sounds_found * sizeof(ma_sound));
    audio_player.names =  (char *)calloc(sounds_found, PATH_MAX);
    if (audio_player.sounds == NULL){
        syslog(LOG_ERR, "(AUDIO THREAD) Malloc failed to allocate space for %d sounds.", audio_player.num_sounds);
        return -1;
    }
    audio_player.num_sounds = sounds_found;
    syslog(LOG_INFO, "(AUDIO THREAD) Malloc'd space for %d sounds.", audio_player.num_sounds);
    int sounds_loaded = 0;
    rewinddir(dir);
    char sound_path[PATH_MAX];
    while ((dp = readdir(dir)) != NULL) {
        if ((strstr(dp->d_name, ".mp3") != NULL) || 
            (strstr(dp->d_name, ".wav") != NULL) ||
            (strstr(dp->d_name, ".flac") != NULL)){
            memcpy(&audio_player.names[PATH_MAX*sounds_loaded], dp->d_name, PATH_MAX);
            memcpy(sound_path, master_command_context.path_working_dir, PATH_MAX);
            strcat(sound_path, dp->d_name);
            //try MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_STREAM
            if (ma_sound_init_from_file(&audio_player.engine, sound_path,  0, NULL, NULL, &audio_player.sounds[sounds_loaded]) != 0){
                syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not initialize sound %s.", sound_path);
                closedir(dir);
                return -1;
            }
            if (ma_sound_set_end_callback(&audio_player.sounds[sounds_loaded], sound_end_callback, NULL) != 0){
                syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not set ma end callback for %s.", sound_path);
                closedir(dir);
                return -1;
            }
            // usleep(100000);
            sounds_loaded++;
            syslog(LOG_INFO, "(AUDIO THREAD) Loaded sound %s", sound_path);
        }
    }
    closedir(dir);
    if (sounds_loaded != audio_player.num_sounds){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not load all sounds.");
        return -1;
    }
    audio_player.sound_curr_idx = 0;
    for (int i = 0; i < NUM_SOUND_PREV; i++){
        audio_player.sound_prev_idx[i] = -1;
    }
    audio_player.state = STATE_INITIALIZED;
    syslog(LOG_INFO, "(AUDIO THREAD) Successfully loaded all %d sounds.", sounds_loaded);
    audio_command_play_pause();
    return 0;
}


int audio_command_toggle_random(){
    audio_player.flags ^= PLAYER_FLAG_RANDOM;
    syslog(LOG_INFO, "(AUDIO THREAD) Audio player random flag set to %d.", ((audio_player.flags & PLAYER_FLAG_RANDOM)!=0));
    return 0;
}

int audio_command_toggle_loop(void){
    audio_player.flags ^= PLAYER_FLAG_LOOP;
    syslog(LOG_INFO, "(AUDIO THREAD) Audio player loop flag set to %d.", ((audio_player.flags & PLAYER_FLAG_LOOP)!=0));
    return 0;
}


int audio_command_seek(void){
    float sound_length_seconds;
    ma_sound *psound = &audio_player.sounds[audio_player.sound_curr_idx];
    if (ma_sound_get_length_in_seconds(psound, &sound_length_seconds)!=0){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not get sound length.");
        return 0;
    }

    float sound_playback_time_seconds;
    if (ma_sound_get_cursor_in_seconds(psound, &sound_playback_time_seconds)){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not get sound cursor.");
        return 0;
    }
    float seconds = 0.0f;
    switch (audio_player.command_flag){
        case COMMAND_FLAG_SEEK_0:
            seconds = 0.0f;
            break;
        case COMMAND_FLAG_SEEK_10:
            seconds = 0.1f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_20:
            seconds = 0.2f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_30:
            seconds = 0.3f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_40:
            seconds = 0.4f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_50:
            seconds = 0.5f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_60:
            seconds = 0.6f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_70:
            seconds = 0.7f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_80:
            seconds = 0.8f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_90:
            seconds = 0.9f*sound_length_seconds;
            break;
        case COMMAND_FLAG_SEEK_FORWARD:
            seconds = sound_playback_time_seconds+5.0f;
            break;
        case COMMAND_FLAG_SEEK_BACKWARD:
            seconds = sound_playback_time_seconds-5.0f;
            break;
        
        default:
            syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Did not recognize command flag (seek).");
            break;
    }
    if (ma_sound_seek_to_second(psound, seconds)!=0){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not seek to %f seconds.", seconds);\
    } else{
        syslog(LOG_INFO, "(AUDIO THREAD) Seeked to %f seconds.", seconds);\
    }
    return 0;
}
