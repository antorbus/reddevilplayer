#include "audio.h"

struct audio_player_context audio_player = {  
    .engine                     = {},  
    .resource_manager           = {},
    .decoder_backends           = {0},
    .num_sounds                 = 0, 
    .names                      = NULL,
    .sound_prev_idx             = {[0 ... NUM_SOUND_PREV-1] = -1},
    .sound_curr_idx             = -1,
    .sounds                     = NULL,       
    .flags                      = PLAYER_FLAG_NONE, 
    .state                      = STATE_UNINITIALIZED,
    .command                    = COMMAND_NONE,
    .command_flag               = COMMAND_FLAG_NONE,
    .command_buffer             = { .commands =             {COMMAND_NONE},
                                    .command_flags =        {COMMAND_FLAG_NONE},
                                    .command_queue_len =    0,
                                    .lock =                 PTHREAD_MUTEX_INITIALIZER,
                                    .command_arrived =      PTHREAD_COND_INITIALIZER}
};

void data_callback(ma_device* p_device, void* p_output, const void* p_input, ma_uint32 frame_count){
    ma_engine_read_pcm_frames(&audio_player.engine, p_output, frame_count, NULL);
}

int initialize_ma(void){
    audio_player.decoder_backends[0] = ma_decoding_backend_libvorbis;

    ma_resource_manager_config resource_manager_config = ma_resource_manager_config_init();
    resource_manager_config.pCustomDecodingBackendUserData = NULL;
    resource_manager_config.ppCustomDecodingBackendVTables = audio_player.decoder_backends;
    resource_manager_config.customDecodingBackendCount = NUM_DECODER_BACKENDS;
    if (ma_resource_manager_init(&resource_manager_config, &audio_player.resource_manager) != 0) {
        syslog(LOG_ERR, "ERROR: Could not initialize resource manager.");
        return -1;
    }

    ma_engine_config engine_config = ma_engine_config_init();
    engine_config.pResourceManager = &audio_player.resource_manager;

    engine_config.dataCallback = data_callback;
    if(ma_engine_init(&engine_config, &audio_player.engine)!=0){
        syslog(LOG_ERR, "ERROR: Could not initialize miniaudio engine.");
        return -1;
    }

    return 0;
}

void * rd_audio_thread(void *arg) {
    pthread_setname_np("RDaudiothread");
    while (1) {
        syslog(LOG_INFO, "(AUDIO THREAD) Waiting for audio command.");
        while (audio_player.command == COMMAND_NONE){
            pthread_mutex_lock(&audio_player.command_buffer.lock);
            if (audio_player.command_buffer.command_queue_len != 0){
                audio_player.command = audio_player.command_buffer.commands[0];
                audio_player.command_flag = audio_player.command_buffer.command_flags[0];
                for (int i = 0; i < SIZE_COMMAND_QUEUE-1; i++){
                    audio_player.command_buffer.commands[i] = audio_player.command_buffer.commands[i+1];
                    audio_player.command_buffer.command_flags[i] = audio_player.command_buffer.command_flags[i+1];
                }
                audio_player.command_buffer.commands[SIZE_COMMAND_QUEUE-1] = COMMAND_NONE;
                audio_player.command_buffer.command_flags[SIZE_COMMAND_QUEUE-1] = COMMAND_FLAG_NONE;
                syslog(LOG_INFO, "(AUDIO THREAD) Audio command size is %llu.", audio_player.command_buffer.command_queue_len);
                audio_player.command_buffer.command_queue_len--;

            } else{
                syslog(LOG_INFO, "(AUDIO THREAD) No commands.");
                pthread_cond_wait(&audio_player.command_buffer.command_arrived, &audio_player.command_buffer.lock);
            }
            pthread_mutex_unlock(&audio_player.command_buffer.lock);
        }
        syslog(LOG_INFO, "(AUDIO THREAD) Audio command received.");
        if (audio_command_handler[audio_player.command]() != 0){
            syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Audio command handler failed.");
            terminate(SIGTERM);
            return NULL;
        }
        syslog(LOG_INFO, "(AUDIO THREAD) Audio command handler succeeded.");
        audio_player.command = COMMAND_NONE;
        audio_player.command_flag = COMMAND_FLAG_NONE;
    }

    return NULL;
}

void sound_end_callback(void *p_user_data, ma_sound *p_sound){
    ma_sound_stop(&audio_player.sounds[audio_player.sound_curr_idx]);
    syslog(LOG_INFO, "(AUDIO THREAD) Sound finished.");
    if (!(audio_player.flags & PLAYER_FLAG_LOOP)){
        for (int i = NUM_SOUND_PREV-1; i > 0; i--){
            audio_player.sound_prev_idx[i] = audio_player.sound_prev_idx[i-1];
        }
        audio_player.sound_prev_idx[0] = audio_player.sound_curr_idx;
        if (!(audio_player.flags & PLAYER_FLAG_RANDOM)){
            syslog(LOG_INFO, "(AUDIO THREAD) Playing next sound.");
            audio_player.sound_curr_idx = (audio_player.sound_curr_idx +1 ) % audio_player.num_sounds;
        } else {
            audio_player.sound_curr_idx = rand() % audio_player.num_sounds;
            syslog(LOG_INFO, "(AUDIO THREAD) Playing next random sound.");
        }
    } else {
        syslog(LOG_INFO, "(AUDIO THREAD) Playing in loop.");
    }
    audio_player.state = STATE_PLAYING;
    ma_sound_start(&audio_player.sounds[audio_player.sound_curr_idx]);
    syslog(LOG_INFO, "(AUDIO THREAD) Playing %s", &audio_player.names[audio_player.sound_curr_idx * PATH_MAX]);
}

int free_sounds(void){
    if ((audio_player.num_sounds != 0) ^ (audio_player.sounds != NULL)){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: One of num_sounds or sounds is zero while the other one is non zero.");
        return -1;
    }
    if ((audio_player.num_sounds != 0) && (audio_player.sounds != NULL)){
        syslog(LOG_INFO, "(AUDIO THREAD) Clearing sounds.");
        if (ma_sound_stop(&audio_player.sounds[audio_player.sound_curr_idx])!=0){
            syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not stop current sound (free_sounds).");
        }
        for (int i = 0; i<audio_player.num_sounds; i++){
            ma_sound_uninit(&audio_player.sounds[i]);
        }
        audio_player.num_sounds = 0;
        free(audio_player.sounds);
        audio_player.sounds = NULL;
    }
    if (audio_player.names != NULL){
        free(audio_player.names);
        audio_player.names = NULL;
    }
    audio_player.state = STATE_UNINITIALIZED;
    return 0;
}


void audio_player_destroy(void){
    free_sounds();
    if (ma_engine_stop(&audio_player.engine)!=0){
        syslog(LOG_ERR, "(AUDIO THREAD) ERROR: Could not stop engine (destroy).");
    }
    ma_engine_uninit(&audio_player.engine);
    audio_player.state = STATE_DONE;
    pthread_mutex_destroy(&audio_player.command_buffer.lock);
    pthread_cond_destroy(&audio_player.command_buffer.command_arrived);
}
