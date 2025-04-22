#include "rd.h"
#include "metal.h"

CFMachPortRef       metal_tap;
CFRunLoopSourceRef  metal_g_run_loop_src;

void * rd_audio_thread(void *arg) {

    while (1) {
        //try the lock once, if it is in use, then continue normally playing audio
        pthread_mutex_lock(&p.lock);
        
        //we have aquired the lock, probably p.command will be none so we will continue singaling the master thread
        //the master thread might not even be waiting, in this case this is not a problem since command will be none
        //so the signal will be lost which is OK 
        if (p.audio_command_execution_status == AUDIO_THREAD_WAITING && 
            audio_command_handler[p.command]() != 0){

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

void * rd_key_monitor_thread(void *arg){

    metal_tap = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        CGEventMaskBit(kCGEventKeyDown),
        keypress_callback, NULL);

    if (!metal_tap) {
        syslog(LOG_ERR, "ERROR: grant input monitoring and retry.\n");
        SIGNAL_COMMAND(COMMAND_KILL, "ERROR: Key monitor thread could not intialize.");
        return NULL;
    }

    metal_g_run_loop_src = CFMachPortCreateRunLoopSource(NULL, metal_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), metal_g_run_loop_src, kCFRunLoopCommonModes);
    CGEventTapEnable(metal_tap, true);
    
    syslog(LOG_INFO, "Starting key monitor thread with event tap.");

    CFRunLoopRun();

    return NULL;
}

void platform_specific_destroy(void){
    if (metal_tap) {
        CGEventTapEnable(metal_tap, false);
        CFMachPortInvalidate(metal_tap);
    }

    if (metal_g_run_loop_src) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), metal_g_run_loop_src, kCFRunLoopCommonModes);
        CFRelease(metal_g_run_loop_src);
    }

    CFRunLoopStop(CFRunLoopGetCurrent());
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





void ma_sound_end_callback(void *p_user_data, ma_sound *p_sound){
    ap.state = STATE_DONE;
    //TODO...
}

int audio_player_open(char *path){
    
    ma_sound_uninit(&ap.prev_sound);
    ap.prev_sound = ap.curr_sound;
    if (ma_sound_init_from_file(&ap.engine, path, 0, NULL, NULL, &ap.curr_sound) != 0){
        syslog(LOG_ERR, "ERROR: Could not initialize sound.");
        return -1;
    }
    if (ma_sound_set_end_callback(&ap.curr_sound, ma_sound_end_callback, NULL) != 0){
        syslog(LOG_ERR, "ERROR: Could not set ma end callback.");
        return -1;
    }
    ap.state = STATE_INITIALIZED;
    return 0;
}


int audio_player_play_pause(){
    if (ap.state == STATE_UNINITIALIZED){
        syslog(LOG_ERR, "ERROR: state uninitialized, nothing to play.");
        return 0;
    }
   
    switch (ap.state){

        // case STATE_DONE:
        //     syslog(LOG_INFO, "Playing new song.");
        //     break;

        case STATE_PLAYING:
            if (ma_sound_stop(&ap.curr_sound) != 0){
                syslog(LOG_ERR, "ERROR: Could not pause.");
                return -1;
            }
            ap.state = STATE_PAUSED;
            syslog(LOG_INFO, "Paused.");
            break;
        
        case STATE_INITIALIZED:
            syslog(LOG_INFO, "Priming from play_pause.");
        case STATE_PAUSED:
            if (ma_sound_start(&ap.curr_sound) != 0){
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
    if (ma_engine_stop(&ap.engine)!=0){
        syslog(LOG_ERR, "ERROR: Could not stop engine (destroy).");
    }
    ma_sound_uninit(&ap.prev_sound);
    ma_sound_uninit(&ap.curr_sound);
    ma_sound_uninit(&ap.next_sound);
    ma_engine_uninit(&ap.engine);
    ap.state = STATE_DONE;
}
