#include "rd.h"

void * rd_audio_thread(void *arg) {

    while (1) {
        pthread_mutex_lock(&p.lock);

        while (p.audio_command_execution_status != AUDIO_THREAD_WAITING){
            pthread_cond_wait(&p.cond_audio, &p.lock);
        }
        
        //we have aquired the lock, probably p.command will be none so we will continue singaling the master thread
        //the master thread might not even be waiting, in this case this is not a problem since command will be none
        //so the signal will be lost which is OK 
        if (audio_command_handler[p.command]() != 0){

            syslog(LOG_ERR, "ERROR: Audio command handler failed.");
            p.audio_command_execution_status = AUDIO_THREAD_TERMINAL_FAILURE; //this will kill the program
            pthread_cond_signal(&p.cond_audio);
            pthread_mutex_unlock(&p.lock);
            return NULL;
        }
            
        p.audio_command_execution_status = AUDIO_THREAD_DONE;
        pthread_cond_signal(&p.cond_audio);
        pthread_mutex_unlock(&p.lock);
    
    }

    return NULL;
}

void terminate(int sig){
    if (!((sig == SIGINT)||(sig == SIGTERM)))
        return;
    syslog(LOG_INFO, "Terminating.");

    audio_player_destroy();

    platform_specific_destroy();

    pthread_cond_destroy(&p.cond_command);
    pthread_cond_destroy(&p.cond_audio);
    pthread_mutex_destroy(&p.lock);

    closelog();

    exit(0);
}

void print_bindings(void){
    printf("MacOS binding control keys: control + option + command\n");
    return;
}

void sound_end_callback(void *p_user_data, ma_sound *p_sound){
    ma_sound_stop(&ap.sounds[ap.sound_curr_idx]);
    syslog(LOG_INFO, "Sound finished.");
    syslog(LOG_INFO, "Playing next sound.");
    ap.sound_prev_idx = ap.sound_curr_idx;
    ap.sound_curr_idx = (ap.sound_curr_idx +1 ) % ap.num_sounds;
    ap.state = STATE_PLAYING;
    ma_sound_start(&ap.sounds[ap.sound_curr_idx]);
}

int free_sounds(void){
    if ((ap.num_sounds != 0) ^ (ap.sounds != NULL)){
        syslog(LOG_ERR, "ERROR: One of num_sounds or sounds is zero while the other one is non zero.");
        return -1;
    }
    if ((ap.num_sounds != 0) && (ap.sounds != NULL)){
        syslog(LOG_INFO, "Clearing sounds.");
        if (ma_sound_stop(&ap.sounds[ap.sound_curr_idx])!=0){
            syslog(LOG_ERR, "ERROR: Could not stop current sound (free_sounds).");
        }
        for (int i = 0; i<ap.num_sounds; i++){
            ma_sound_uninit(&ap.sounds[i]);
        }
        ap.num_sounds = 0;
        free(ap.sounds);
        ap.sounds = NULL;
    }
    ap.state = STATE_UNINITIALIZED;
    return 0;
}


int audio_player_play_pause(){
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
            syslog(LOG_INFO, "Paused.");
            break;
        
        case STATE_INITIALIZED:
            syslog(LOG_INFO, "Playing after.");
        case STATE_PAUSED:
            if (ma_sound_start(&ap.sounds[ap.sound_curr_idx]) != 0){
                syslog(LOG_ERR, "ERROR: Could not play.");
                return -1;
            }
            syslog(LOG_INFO, "Playing.");
            ap.state = STATE_PLAYING;
            break;
            
        default:
            syslog(LOG_ERR, "ERROR: Unrecognized audio player state.");
            return -1;
            break;
    }
    return 0;
}

void audio_player_destroy(void){
    free_sounds();
    if (ma_engine_stop(&ap.engine)!=0){
        syslog(LOG_ERR, "ERROR: Could not stop engine (destroy).");
    }
    ma_engine_uninit(&ap.engine);
    ap.state = STATE_DONE;
}
