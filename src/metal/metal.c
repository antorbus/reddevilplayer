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


CGEventRef keypress_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *info) {
    if (type == kCGEventKeyDown) {
        CGKeyCode    kc    = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        CGEventFlags flags = CGEventGetFlags(event);
        if ((flags & (kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand)) ==
                (kCGEventFlagMaskControl|kCGEventFlagMaskAlternate|kCGEventFlagMaskCommand)){
            syslog(LOG_INFO, "Event tap caught.");
            switch (kc) {

            case kVK_ANSI_K: 
                SIGNAL_COMMAND(COMMAND_KILL, "User event: termination.");
                break;
            
            case kVK_ANSI_C: 
                SIGNAL_COMMAND(COMMAND_CLOSE, "User event: close.");
                break;

            case kVK_ANSI_H: 
                SIGNAL_COMMAND(COMMAND_HELP, "User event: help.");
                break;

            case kVK_ANSI_O: 
                SIGNAL_COMMAND(COMMAND_OPEN, "User event: open.");
                break;
            
            case kVK_ANSI_P: 
                SIGNAL_COMMAND(COMMAND_PLAY_PAUSE, "User event: toggle play/pause.");
                break;

            case kVK_ANSI_V: 
                SIGNAL_COMMAND(COMMAND_PREV, "User event: previous sound.");
                break;

            case kVK_ANSI_N: 
                SIGNAL_COMMAND(COMMAND_NEXT, "User event: next sound.");
                break;

            case kVK_ANSI_R: 
                SIGNAL_COMMAND(COMMAND_TOGGLE_RANDOM, "User event: toggle random.");
                break;
                
            // TODO
            // case kVK_ANSI_F: 
            //     SIGNAL_COMMAND(COMMAND_SEEK, "User event: seek (forward).")
            //     break;
        
            // case kVK_ANSI_B: 
            //     SIGNAL_COMMAND(COMMAND_SEEK, "User event: seek (backward).")
            //     break;
            // case kVK_Space: 
            //     syslog(LOG_INFO, "User event: search.");
            //     break;

            // case kVK_ANSI_0:
            //     syslog(LOG_INFO, "User event: playback from 0%%.");
            //     break;
            // case kVK_ANSI_1:
            //     syslog(LOG_INFO, "User event: playback from 10%%.");
            //     break;
            // case kVK_ANSI_2:
            //     syslog(LOG_INFO, "User event: playback from 20%%.");
            //     break;
            // case kVK_ANSI_3:
            //     syslog(LOG_INFO, "User event: playback from 30%%.");
            //     break;
            // case kVK_ANSI_4:
            //     syslog(LOG_INFO, "User event: playback from 4%%.");
            //     break;
            // case kVK_ANSI_5:
            //     syslog(LOG_INFO, "User event: playback from 50%%.");
            //     break;
            // case kVK_ANSI_6:
            //     syslog(LOG_INFO, "User event: playback from 60%%.");
            //     break;
            // case kVK_ANSI_7:
            //     syslog(LOG_INFO, "User event: playback from 70%%.");
            //     break;
            // case kVK_ANSI_8:
            //     syslog(LOG_INFO, "User event: playback from 80%%.");
            //     break;
            // case kVK_ANSI_9:
            //     syslog(LOG_INFO, "User event: playback from 90%%.");
            //     break;

            default:
                syslog(LOG_ERR, "ERROR: Unknown hotkey id.");
                break;
            }
        }    
    }
    return event;
}

int choose_directory(char *path, size_t max_len) {
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