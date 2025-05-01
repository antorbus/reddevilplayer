#include "rd.h"

#define PIDFILE "/tmp/RedDevilPlayer"

struct player p = {
    .path_working_dir =                 {0},
    .master_command_execution_status =  MASTER_OK,
    .command =                          COMMAND_NONE,
    .command_flag =                     COMMAND_FLAG_NONE,
    .lock =                             PTHREAD_MUTEX_INITIALIZER,
    .command_arrived =                  PTHREAD_COND_INITIALIZER,
};

struct audio_player ap = {  
    .engine                     = {},  
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

pthread_t audio_thread          = NULL;
pthread_t key_monitor_thread    = NULL;

int initialize(void){
    srand(time(NULL));

    if (is_already_running() != 0){
        fprintf(stderr, "ERROR: Failed to launch rd master daemon.\n");
        return -1;
    }

    if (daemon(0, 0) != 0) {
        perror("Failed to launch rd master daemon.");
        return -1;
    }

    openlog("rd_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Red Devil Daemon started.");
    

    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);

    if(ma_engine_init(NULL, &ap.engine)!=0){
        syslog(LOG_ERR, "ERROR: Could not initialize miniaudio engine.");
        return -1;
    }

    if (pthread_create(&audio_thread, NULL, rd_audio_thread, NULL) != 0) {
        syslog(LOG_ERR, "ERROR: Pthread_create failed (audio thread).");
        return -1;
    }

    if (pthread_detach(audio_thread) != 0){
        syslog(LOG_ERR, "ERROR: Pthread_detach failed (audio thread).");
        return -1;
    }

    if (pthread_create(&key_monitor_thread, NULL, rd_key_monitor_thread, NULL) != 0) {
        syslog(LOG_ERR, "ERROR: Pthread_create failed (key monitor thread thread).");
        return -1;
    }
    
    if (pthread_detach(key_monitor_thread) != 0){
        syslog(LOG_ERR, "ERROR: Pthread_detach failed (key monitor thread thread).");
        return -1;
    }

    return 0;
}

int rd_master_daemon(void){

    if (initialize() != 0){
        syslog(LOG_ERR, "ERROR: could not initialize master thread.");
        terminate(SIGTERM);
        return -1;
    }
    

    syslog(LOG_INFO, "Starting master thread loop.");
    for (;;) {        
        pthread_mutex_lock(&p.lock);

        while (p.command == COMMAND_NONE){
            syslog(LOG_INFO, "Waiting for command.");
            pthread_cond_wait(&p.command_arrived, &p.lock); //wait until we receive a command
        }

        syslog(LOG_INFO, "Command received.");
        p.master_command_execution_status = MASTER_OK; 

        if (master_command_handler[p.command]() != 0){   //STATUS_FAILED will return 0, means the app can continue but audio will not run
            terminate(SIGTERM);                          // true failure will return -1;
        }                       

        if (p.master_command_execution_status == MASTER_FAILED){
            syslog(LOG_ERR, "ERROR: master_command_execution_status failed. Audio command will not run.");
        } else if (p.master_command_execution_status == MASTER_OK){ //aka is master_command_execution_status wasnt changed by command TODO we could remove this
            if (pthread_mutex_trylock(&ap.command_buffer.lock) == 0){
                if (ap.command_buffer.command_queue_len < SIZE_COMMAND_QUEUE -1){
                     //fill the commandbuffer of the audio thread
                    ap.command_buffer.commands[ap.command_buffer.command_queue_len] = p.command;
                    ap.command_buffer.command_flags[ap.command_buffer.command_queue_len] = p.command_flag;
                    if (ap.command_buffer.command_queue_len < SIZE_COMMAND_QUEUE -1){
                        ap.command_buffer.command_queue_len++;
                    }
                } else {
                    syslog(LOG_ERR, "ERROR: audio buffer full.");
                }
                pthread_cond_signal(&ap.command_buffer.command_arrived);
                pthread_mutex_unlock(&ap.command_buffer.lock);
            } else{
                syslog(LOG_ERR, "ERROR: could not aquire audio command buffer lock.");
            }
        }

        p.command = COMMAND_NONE;
        p.command_flag = COMMAND_FLAG_NONE;

        pthread_mutex_unlock(&p.lock);
    }

    return 0;
}

int main(){
    printf("\033[0;31m"); 
    printf("          _____                 \n");
    printf("         /\\    \\              \n");
    printf("        /::\\    \\             \n");
    printf("       /::::\\    \\            \n");
    printf("      /::::::\\    \\           \n");
    printf("     /:::/\\:::\\    \\         \n");
    printf("    /:::/__\\:::\\    \\        \n");
    printf("   /::::\\   \\:::\\    \\      \n");
    printf("  /::::::\\   \\:::\\    \\     \n");
    printf(" /:::/\\:::\\   \\:::\\____\\   \n");
    printf("/:::/  \\:::\\   \\:::|    |    \n");
    printf("\\::/   |::::\\  /:::|____|     \n");
    printf(" \\/____|:::::\\/:::/    /      \n");
    printf("       |:::::::::/    /         \n");
    printf("       |::|\\::::/    /         \n");
    printf("       |::| \\::/____/          \n");
    printf("       |::|  ~|                 \n");
    printf("       |::|   |                 \n");
    printf("       \\::|   |                \n");
    printf("        \\:|   |                \n");
    printf("         \\|___|");
    printf("\033[0m");
    printf("ed");
    printf("\033[0;31m");
    printf("D");
    printf("\033[0m");
    printf("evil");
    printf("\033[0;31m");
    printf("P");
    printf("\033[0m");
    printf("layer\n");
    printf("                         \n");
    printf("                         \n");
    print_bindings();
    printf("Press the binding control keys + h to open the help menu.\n\n");
    printf("(Developer info) To view syslog use: log stream --predicate 'process == \"RedDevilPlayer\"' --info\n\n");
    
    if (rd_master_daemon() != 0){
        fprintf(stderr, "ERROR: Could not launch Red Devil Player daemon.\n\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

int is_already_running(void){
    int pidfile_fd = open(PIDFILE, O_RDWR|O_CREAT, 0644);
    if (pidfile_fd == -1) {
        perror("open pidfile");
        return -1;
    }

    if (flock(pidfile_fd, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK) {
            fprintf(stderr, "ERROR: Another instance is already running.\n");
            return -1;           
        }
        perror("flock");
        return -1;               
    }

    ftruncate(pidfile_fd, 0);
    dprintf(pidfile_fd, "%ld\n", (long)getpid());
    return 0;
}