#include "rd.h"
#include "metal.h"

void audio_queue_callback(void *audio_p_ref, AudioQueueRef a, AudioQueueBufferRef b){

    struct metal_audio_player *audio_p = (struct metal_audio_player *)audio_p_ref;
    //frame is one sample across all channels (for stereo 1 frame  = 1 left + right sample)
    UInt32 frames = audio_p->buf->mAudioDataBytesCapacity / audio_p->bytes_per_frame;
    
    //abl is needed only to pass data into ExtAudioFileRead, so we can create it in the stack
    //it tells ExtAudioFileRead where to place data
    AudioBufferList abl = {1, {{0, audio_p->buf->mAudioDataBytesCapacity, audio_p->buf->mAudioData}}};

    //reads audio data from file and updates frames with number of frames read
    if ((ExtAudioFileRead(audio_p->file, &frames, &abl) == 0) && frames) {
        //update buffer to match actual number of buffers read 
        audio_p->buf->mAudioDataByteSize = frames * audio_p->bytes_per_frame;
        //put the buffe into the queue for playback
        AudioQueueEnqueueBuffer(audio_p->queue, audio_p->buf, 0, NULL);
    } else {                         
        //no frames read (end of fie) or an error then we stop
        audio_p->state = STATE_DONE;
        AudioQueueStop(audio_p->queue, false);
    }
}

int audio_player_open(char *path){
    
    if  (audio_player.queue && (
        (AudioQueueStop(audio_player.queue, true) != 0) ||
        (AudioQueueDispose(audio_player.queue, true) != 0))){
            audio_player_destroy();
            return -1;
    }
    if (audio_player.file && (ExtAudioFileDispose(audio_player.file) != 0)){ 
        audio_player_destroy();
        return -1;
    }
    
    memset(&audio_player, 0, sizeof(audio_player)); //sets state to done

    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
                       NULL, (const UInt8*)path, (CFIndex)strlen(path), false);
    int rc = ExtAudioFileOpenURL(url, &audio_player.file);
    CFRelease(url);

    if (rc != 0){
        syslog(LOG_ERR, "ERROR: Could not open audio file.");
        audio_player_destroy();
        return -1;
    }

    //pulse code modulation
    AudioStreamBasicDescription pcm = {0};
    UInt32 sz = sizeof(pcm);
    
    //populates pcm with info specified by kExtAudioFileProperty_FileDataFormat
    //stores info such as num channels, sampling rate  
    if (ExtAudioFileGetProperty(audio_player.file, kExtAudioFileProperty_FileDataFormat,
                            &sz, &pcm)!= 0){
        syslog(LOG_ERR, "Error: Could not read audio file. File might be corrupted or format unsupported.");
        audio_player_destroy();
        return -1;
    }
    //override for hi fi (32bit) linear pcm
    pcm.mFormatID         = kAudioFormatLinearPCM;
    pcm.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    pcm.mBitsPerChannel   = 32;
    UInt32 bytesPerSample = (pcm.mBitsPerChannel + 8-1) / 8;
    pcm.mChannelsPerFrame = 2; 
    pcm.mFramesPerPacket  = 1; //1 for uncompressed audio, compressed contains more, must be overriden for linear pcm
    pcm.mBytesPerFrame = pcm.mChannelsPerFrame * bytesPerSample;
    pcm.mBytesPerPacket = pcm.mBytesPerFrame * pcm.mFramesPerPacket;

    audio_player.bytes_per_frame = pcm.mBytesPerFrame;

    //set up conversion pipeline to pcm for being able to play data in certain formats
    if (ExtAudioFileSetProperty(audio_player.file, kExtAudioFileProperty_ClientDataFormat,
                            sizeof(pcm), &pcm)){
        syslog(LOG_ERR, "Error: Could not set up conversion pipeline.");
        audio_player_destroy();
        return -1;
    }
    //creates audio queue for playback
    if (AudioQueueNewOutput(&pcm, audio_queue_callback, (void*) &audio_player, NULL, NULL, 0, &audio_player.queue)!=0){
        syslog(LOG_ERR, "Error: Could not create audio queue.");
        audio_player_destroy();
        return -1;
    }

    //if sample rate is 44.1k samples per second, this specifies size of buffer that stores 0.25s of audio
    float buf_size_seconds = 0.25;
    UInt32 buf_bytes = (UInt32)(pcm.mSampleRate * pcm.mBytesPerFrame * buf_size_seconds);

    if (buf_bytes < 4096) 
        buf_bytes = 4096;

    //allocate memory for audio buffer and store it in .buf
    if (AudioQueueAllocateBuffer(audio_player.queue, buf_bytes, &audio_player.buf) != 0){
        syslog(LOG_ERR, "Error: Could not allocate audio buffer.");
        audio_player_destroy();
        return -1;
    }

    return 0;
}


int audio_player_play_pause(){
    if (!audio_player.queue){
        syslog(LOG_ERR, "Error: Audio queue not present (play_pause).");
        return -1;
    }
    switch (audio_player.state){

        case STATE_DONE:
            audio_queue_callback((void *)&audio_player, audio_player.queue, audio_player.buf); 
            //no break here, we want the next part to run
            syslog(LOG_INFO, "Priming from play_pause.");
        case STATE_PAUSED:

            if (AudioQueueStart(audio_player.queue, NULL) != 0){
                syslog(LOG_ERR, "ERROR: Could not play.");
                return -1;
            }
            syslog(LOG_INFO, "Playing.");
            audio_player.state = STATE_PLAYING;
            break;

        case STATE_PLAYING:
            if (AudioQueuePause (audio_player.queue) != 0){
                syslog(LOG_ERR, "ERROR: Could not pause.");
                return -1;
            }
            audio_player.state = STATE_PAUSED;
            syslog(LOG_INFO, "Paused.");
            break;
            
        default:
            syslog(LOG_ERR, "ERROR: Unrecognized audio player state.");
            return -1;
            break;
    }
    return 0;
}

void audio_player_destroy(void){
    if (audio_player.queue){   
        if (audio_player.state == STATE_PLAYING){
            audio_player.state = STATE_DONE;
            if (AudioQueueStop(audio_player.queue, true) !=0){
                syslog(LOG_ERR, "ERROR: Could not stop audio player (destroy).");
            }
        }
        if (AudioQueueDispose(audio_player.queue, true)!= 0){ //also diposes of buffer
            syslog(LOG_ERR, "ERROR: Could not dispose of audio queue (destroy).");
        } 
    }
    if (audio_player.file && (ExtAudioFileDispose(audio_player.file) != 0)){ //close decoder
        syslog(LOG_ERR, "ERROR: Could not close decoder (destroy).");
    }
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
                SIGNAL_COMMAND(COMMAND_PREV, "User event: previous song.");
                break;

            case kVK_ANSI_N: 
                SIGNAL_COMMAND(COMMAND_NEXT, "User event: next song.");
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
