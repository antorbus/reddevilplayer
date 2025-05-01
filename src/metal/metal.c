#include "rd.h"
#include "metal.h"

CFMachPortRef       metal_tap;
CFRunLoopSourceRef  metal_g_run_loop_src;

void * rd_key_monitor_thread(void *arg){
    pthread_setname_np("RDkeymonitorthread");
    metal_tap = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        CGEventMaskBit(kCGEventKeyDown),
        keypress_callback, NULL);

    if (!metal_tap) {
        syslog(LOG_ERR, "ERROR: grant input monitoring and retry.\n");
        kill(getpid(), SIGTERM);
        return NULL;
    }

    metal_g_run_loop_src = CFMachPortCreateRunLoopSource(NULL, metal_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), metal_g_run_loop_src, kCFRunLoopCommonModes);
    CGEventTapEnable(metal_tap, 1);
    
    syslog(LOG_INFO, "Starting key monitor thread with event tap.");

    CFRunLoopRun();

    return NULL;
}

void platform_specific_destroy(void){
    if (metal_tap) {
        CGEventTapEnable(metal_tap, 0);
        CFMachPortInvalidate(metal_tap);
    }

    if (metal_g_run_loop_src) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), metal_g_run_loop_src, kCFRunLoopCommonModes);
        CFRelease(metal_g_run_loop_src);
    }

    CFRunLoopStop(CFRunLoopGetCurrent());
}


CGEventRef keypress_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *info) {
    if (type == kCGEventKeyDown) {
        CGKeyCode    kc    = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        CGEventFlags flags = CGEventGetFlags(event);
        if ((flags & (kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand)) ==
                (kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand)){
            syslog(LOG_INFO, "Event tap caught.");
            playback_command_t master_command = COMMAND_NONE;
            playback_command_flag_t command_flag = COMMAND_FLAG_NONE;
            char syslog_message[1024] = {0};
            switch (kc) {
                case kVK_ANSI_K: 
                    master_command = COMMAND_KILL;
                    strncpy(syslog_message, "User event: termination.", sizeof(syslog_message)-1);
                    break;
                
                case kVK_ANSI_C: 
                    master_command = COMMAND_CLOSE;
                    strncpy(syslog_message, "User event: close.", sizeof(syslog_message)-1);
                    break;

                case kVK_ANSI_H: 
                    master_command = COMMAND_HELP;
                    strncpy(syslog_message, "User event: help.", sizeof(syslog_message)-1);
                    break;

                case kVK_ANSI_O: 
                    master_command = COMMAND_OPEN;
                    strncpy(syslog_message, "User event: open.", sizeof(syslog_message)-1);
                    break;
                
                case kVK_ANSI_P: 
                    master_command = COMMAND_PLAY_PAUSE;
                    strncpy(syslog_message, "User event: toggle play/pause.", sizeof(syslog_message)-1);
                    break;

                case kVK_ANSI_V: 
                    master_command = COMMAND_PREV;
                    strncpy(syslog_message, "User event: previous sound.", sizeof(syslog_message)-1);
                    break;

                case kVK_ANSI_N: 
                    master_command = COMMAND_NEXT;
                    strncpy(syslog_message, "User event: next sound.", sizeof(syslog_message)-1);
                    break;

                case kVK_ANSI_R: 
                    master_command = COMMAND_TOGGLE_RANDOM;
                    strncpy(syslog_message, "User event: toggle random.", sizeof(syslog_message)-1);
                    break;
                
                case kVK_ANSI_L: 
                    master_command = COMMAND_TOGGLE_LOOP;
                    strncpy(syslog_message, "User event: toggle loop.", sizeof(syslog_message)-1);
                    break;   
                    
            
                case kVK_ANSI_F: 
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_FORWARD;
                    strncpy(syslog_message, "User event: seek (forward).", sizeof(syslog_message)-1);
                    break;
            
                case kVK_ANSI_B: 
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_BACKWARD;
                    strncpy(syslog_message, "User event: seek (backward).", sizeof(syslog_message)-1);
                    break;
                    // TODO
                // case kVK_Space: 
                //     syslog(LOG_INFO, "User event: search.");
                //     break;

                case kVK_ANSI_0:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_0;
                    strncpy(syslog_message, "User event: playback from 0%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_1:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_10;
                    strncpy(syslog_message, "User event: playback from 10%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_2:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_20;
                    strncpy(syslog_message, "User event: playback from 20%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_3:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_30;
                    strncpy(syslog_message, "User event: playback from 30%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_4:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_40;
                    strncpy(syslog_message, "User event: playback from 40%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_5:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_50;
                    strncpy(syslog_message, "User event: playback from 50%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_6:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_60;
                    strncpy(syslog_message, "User event: playback from 60%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_7:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_70;
                    strncpy(syslog_message, "User event: playback from 70%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_8:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_80;
                    strncpy(syslog_message, "User event: playback from 80%%.", sizeof(syslog_message)-1);
                    break;
                case kVK_ANSI_9:
                    master_command = COMMAND_SEEK;
                    command_flag = COMMAND_FLAG_SEEK_90;
                    strncpy(syslog_message, "User event: playback from 90%%.", sizeof(syslog_message)-1);
                    break;

                default:
                    syslog(LOG_ERR, "ERROR: Unknown hotkey id.");
                    break;
            }
            int rc = pthread_mutex_trylock(&master_command_context.lock);
            if (rc == 0){ 
                master_command_context.command = master_command;
                master_command_context.command_flag = command_flag;
                pthread_cond_signal(&master_command_context.command_arrived);
                syslog(LOG_INFO, "%s", syslog_message);
                pthread_mutex_unlock(&master_command_context.lock); 
            } else{ 
                syslog(LOG_ERR, "ERROR: Monitor thread could not acquire lock.");
            }
        }    
    }
    return event;
}

int choose_directory(char *path, size_t max_len) {
    syslog(LOG_INFO, "Choosing dir.");
    memset(path, 0, max_len);
    const char *script =
      "osascript -e 'try' "
      "-e 'POSIX path of (choose folder with prompt \"Select a directory:\")' "
      "-e 'on error' "
      "-e 'return \"\"' "
      "-e 'end try'";
    FILE *fp = popen(script, "r");
    if (!fp) 
        return -1;

    if (fgets(path, max_len, fp) == NULL) {
        pclose(fp);
        return -1;
    }
    pclose(fp);

    size_t len = strlen(path);
    if (len && path[len-1] == '\n') 
        path[len-1] = '\0';

    return path[0] == '\0';
}

void print_bindings(void){
    printf("MacOS binding control keys: control + option + command\n");
    return;
}
