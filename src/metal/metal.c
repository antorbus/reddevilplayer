#include "rd.h"
#include "metal.h"

CFMachPortRef       metal_tap;
CFRunLoopSourceRef  metal_g_run_loop_src;

void * rd_key_monitor_thread(void *arg){

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


CGEventRef keypress_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *info) {
    if (type == kCGEventKeyDown) {
        CGKeyCode    kc    = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        CGEventFlags flags = CGEventGetFlags(event);
        if ((flags & (kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand)) ==
                (kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand)){
            syslog(LOG_INFO, "Event tap caught.");
            switch (kc) {

            case kVK_ANSI_K: 
                TRY_SIGNAL_COMMAND(COMMAND_KILL, "User event: termination.");
                break;
            
            case kVK_ANSI_C: 
                TRY_SIGNAL_COMMAND(COMMAND_CLOSE, "User event: close.");
                break;

            case kVK_ANSI_H: 
                TRY_SIGNAL_COMMAND(COMMAND_HELP, "User event: help.");
                break;

            case kVK_ANSI_O: 
                TRY_SIGNAL_COMMAND(COMMAND_OPEN, "User event: open.");
                break;
            
            case kVK_ANSI_P: 
                TRY_SIGNAL_COMMAND(COMMAND_PLAY_PAUSE, "User event: toggle play/pause.");
                break;

            case kVK_ANSI_V: 
                TRY_SIGNAL_COMMAND(COMMAND_PREV, "User event: previous sound.");
                break;

            case kVK_ANSI_N: 
                TRY_SIGNAL_COMMAND(COMMAND_NEXT, "User event: next sound.");
                break;

            case kVK_ANSI_R: 
                TRY_SIGNAL_COMMAND(COMMAND_TOGGLE_RANDOM, "User event: toggle random.");
                break;
            
            case kVK_ANSI_L: 
                TRY_SIGNAL_COMMAND(COMMAND_TOGGLE_LOOP, "User event: toggle loop.");
                break;   
                
           
            case kVK_ANSI_F: 
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: seek (forward).", COMMAND_FLAG_SEEK_FORWARD);
                break;
        
            case kVK_ANSI_B: 
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: seek (backward).", COMMAND_FLAG_SEEK_BACKWARD);
                break;
                // TODO
            // case kVK_Space: 
            //     syslog(LOG_INFO, "User event: search.");
            //     break;

            case kVK_ANSI_0:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 0%%.", COMMAND_FLAG_SEEK_0);
                break;
            case kVK_ANSI_1:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 10%%.", COMMAND_FLAG_SEEK_10);
                break;
            case kVK_ANSI_2:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 20%%.", COMMAND_FLAG_SEEK_20);
                break;
            case kVK_ANSI_3:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 30%%.", COMMAND_FLAG_SEEK_30);
                break;
            case kVK_ANSI_4:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 40%%.", COMMAND_FLAG_SEEK_40);
                break;
            case kVK_ANSI_5:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 50%%.", COMMAND_FLAG_SEEK_50);
                break;
            case kVK_ANSI_6:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 60%%.", COMMAND_FLAG_SEEK_60);
                break;
            case kVK_ANSI_7:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 70%%.", COMMAND_FLAG_SEEK_70);
                break;
            case kVK_ANSI_8:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 80%%.", COMMAND_FLAG_SEEK_80);
                break;
            case kVK_ANSI_9:
                TRY_SIGNAL_COMMAND_WITH_FLAG(COMMAND_SEEK, "User event: playback from 90%%.", COMMAND_FLAG_SEEK_90);
                break;

            default:
                syslog(LOG_ERR, "ERROR: Unknown hotkey id.");
                break;
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