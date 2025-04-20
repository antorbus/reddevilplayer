#include "rd.h"
#include "metal.h"


static CFMachPortRef        metal_tap    = NULL;
static CFRunLoopSourceRef   metal_g_run_loop_src  = NULL;
struct metal_audio_player   audio_player = {0};

void * rd_audio_thread(void *arg) {
    
    while (1) {
        //try the lock once, if it is in use, then continue normally playing audio
        int rc = pthread_mutex_trylock(&p.lock);
        if (rc == 0){
            //we have aquired the lock, probably command will be none so we will continue singaling the master thread
            //the master thread might not even be waiting, in this case this is not a problem since command will be none 
            if (audio_command_handler[p.command]() != 0){
                syslog(LOG_ERR, "ERROR: Audio command handler failed.");
                p.audio_command_execution_status = STATUS_FAILED;
                pthread_cond_signal(&p.cond_status);
                pthread_mutex_unlock(&p.lock);
                return NULL;
            }
                
            p.audio_command_execution_status = STATUS_DONE;
            pthread_cond_signal(&p.cond_status);
            pthread_mutex_unlock(&p.lock);
        }
        //Audio player stuff
        //...
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

void terminate(int sig){
    if ((sig != SIGINT)||(sig != SIGTERM))
        return;
    syslog(LOG_INFO, "Terminating.");

    if (metal_tap) {
        CGEventTapEnable(metal_tap, false);
        CFMachPortInvalidate(metal_tap);
    }

    if (metal_g_run_loop_src) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), metal_g_run_loop_src, kCFRunLoopCommonModes);
        CFRelease(metal_g_run_loop_src);
    }

    CFRunLoopStop(CFRunLoopGetCurrent());

    pthread_cond_destroy(&p.cond_command);
    pthread_cond_destroy(&p.cond_status);
    pthread_mutex_destroy(&p.lock);

    closelog();

    exit(0);
}

void print_bindings(void){
    printf("MacOS binding control keys: control + option + command\n");
    return;
}



