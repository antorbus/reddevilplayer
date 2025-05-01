#include "rd.h"

void * rd_audio_thread(void *arg) {
    pthread_setname_np("RDaudiothread");
    while (1) {
        syslog(LOG_INFO, "(AUDIO THREAD) Waiting for audio command.");
        while (ap.command == COMMAND_NONE){
            pthread_mutex_lock(&ap.command_buffer.lock);
            if (ap.command_buffer.command_queue_len != 0){
                ap.command = ap.command_buffer.commands[0];
                ap.command_flag = ap.command_buffer.command_flags[0];
                for (int i = 0; i < SIZE_COMMAND_QUEUE-1; i++){
                    ap.command_buffer.commands[i] = ap.command_buffer.commands[i+1];
                    ap.command_buffer.command_flags[i] = ap.command_buffer.command_flags[i+1];
                }
                ap.command_buffer.commands[SIZE_COMMAND_QUEUE-1] = COMMAND_NONE;
                ap.command_buffer.command_flags[SIZE_COMMAND_QUEUE-1] = COMMAND_FLAG_NONE;
                ap.command_buffer.command_queue_len--;
                syslog(LOG_INFO, "(AUDIO THREAD) Audio command size is %llu.", ap.command_buffer.command_queue_len);

            } else{
                pthread_cond_wait(&ap.command_buffer.command_arrived, &ap.command_buffer.lock);
            }
            pthread_mutex_unlock(&ap.command_buffer.lock);
        }
        syslog(LOG_INFO, "(AUDIO THREAD) Audio command recieved.");
        if (audio_command_handler[ap.command]() != 0){
            syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Audio command handler failed.");
            terminate(SIGTERM);
            return NULL;
        }
        syslog(LOG_INFO, "(AUDIO THREAD) Audio command handler succeeded.");
        ap.command = COMMAND_NONE;
        ap.command_flag = COMMAND_FLAG_NONE;
    }

    return NULL;
}

void terminate(int sig){
    if (!((sig == SIGINT)||(sig == SIGTERM)))
        return;
    syslog(LOG_INFO, "Terminating.");

    audio_player_destroy();

    platform_specific_destroy();

    pthread_cond_destroy(&p.command_arrived);
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
    syslog(LOG_INFO, "(AUDIO THREAD) Sound finished.");
    if (!(ap.flags & PLAYER_FLAG_LOOP)){
        for (int i = NUM_SOUND_PREV-1; i > 0; i--){
            ap.sound_prev_idx[i] = ap.sound_prev_idx[i-1];
        }
        ap.sound_prev_idx[0] = ap.sound_curr_idx;
        if (!(ap.flags & PLAYER_FLAG_RANDOM)){
            syslog(LOG_INFO, "(AUDIO THREAD) Playing next sound.");
            ap.sound_curr_idx = (ap.sound_curr_idx +1 ) % ap.num_sounds;
        } else {
            ap.sound_curr_idx = rand() % ap.num_sounds;
            syslog(LOG_INFO, "(AUDIO THREAD) Playing next random sound.");
        }
    } else {
        syslog(LOG_INFO, "(AUDIO THREAD) Playing in loop.");
    }
    ap.state = STATE_PLAYING;
    ma_sound_start(&ap.sounds[ap.sound_curr_idx]);
    syslog(LOG_INFO, "(AUDIO THREAD) Playing %s", &ap.names[ap.sound_curr_idx * PATH_MAX]);
}

int free_sounds(void){
    if ((ap.num_sounds != 0) ^ (ap.sounds != NULL)){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: One of num_sounds or sounds is zero while the other one is non zero.");
        return -1;
    }
    if ((ap.num_sounds != 0) && (ap.sounds != NULL)){
        syslog(LOG_INFO, "(AUDIO THREAD) Clearing sounds.");
        if (ma_sound_stop(&ap.sounds[ap.sound_curr_idx])!=0){
            syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not stop current sound (free_sounds).");
        }
        for (int i = 0; i<ap.num_sounds; i++){
            ma_sound_uninit(&ap.sounds[i]);
        }
        ap.num_sounds = 0;
        free(ap.sounds);
        ap.sounds = NULL;
    }
    if (ap.names != NULL){
        free(ap.names);
        ap.names = NULL;
    }
    ap.state = STATE_UNINITIALIZED;
    return 0;
}


void audio_player_destroy(void){
    free_sounds();
    if (ma_engine_stop(&ap.engine)!=0){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not stop engine (destroy).");
    }
    ma_engine_uninit(&ap.engine);
    ap.state = STATE_DONE;
    pthread_mutex_destroy(&ap.command_buffer.lock);
    pthread_cond_destroy(&ap.command_buffer.command_arrived);
}
