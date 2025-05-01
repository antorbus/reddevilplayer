#include "rd.h"
#include "audio.h"
#define PIDFILE "/tmp/RedDevilPlayer"

struct playback_command_context master_command_context = {
    .path_working_dir =                 {0},
    .master_command_execution_status =  MASTER_OK,
    .command =                          COMMAND_NONE,
    .command_flag =                     COMMAND_FLAG_NONE,
    .lock =                             PTHREAD_MUTEX_INITIALIZER,
    .command_arrived =                  PTHREAD_COND_INITIALIZER,
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

    if(ma_engine_init(NULL, &audio_player.engine)!=0){
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
        pthread_mutex_lock(&master_command_context.lock);

        while (master_command_context.command == COMMAND_NONE){
            syslog(LOG_INFO, "Waiting for command.");
            pthread_cond_wait(&master_command_context.command_arrived, &master_command_context.lock); //wait until we receive a command
        }

        syslog(LOG_INFO, "Command received.");
        master_command_context.master_command_execution_status = MASTER_OK; 

        if (master_command_handler[master_command_context.command]() != 0){   //STATUS_FAILED will return 0, means the app can continue but audio will not run
            terminate(SIGTERM);                          // true failure will return -1;
        }                       

        if (master_command_context.master_command_execution_status == MASTER_FAILED){
            syslog(LOG_ERR, "ERROR: master_command_execution_status failed. Audio command will not run.");
        } else if (master_command_context.master_command_execution_status == MASTER_OK){ //aka is master_command_execution_status wasnt changed by command TODO we could remove this
            if (pthread_mutex_trylock(&audio_player.command_buffer.lock) == 0){
                if (audio_player.command_buffer.command_queue_len < SIZE_COMMAND_QUEUE -1){
                     //fill the commandbuffer of the audio thread
                    audio_player.command_buffer.commands[audio_player.command_buffer.command_queue_len] = master_command_context.command;
                    audio_player.command_buffer.command_flags[audio_player.command_buffer.command_queue_len] = master_command_context.command_flag;
                    if (audio_player.command_buffer.command_queue_len < SIZE_COMMAND_QUEUE -1){
                        audio_player.command_buffer.command_queue_len++;
                    }
                } else {
                    syslog(LOG_ERR, "ERROR: audio buffer full.");
                }
                pthread_cond_signal(&audio_player.command_buffer.command_arrived);
                pthread_mutex_unlock(&audio_player.command_buffer.lock);
            } else{
                syslog(LOG_ERR, "ERROR: could not aquire audio command buffer lock.");
            }
        }

        master_command_context.command = COMMAND_NONE;
        master_command_context.command_flag = COMMAND_FLAG_NONE;

        pthread_mutex_unlock(&master_command_context.lock);
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
    printf("If GUI plugin is on, press the binding control keys + h to open the help menu.\n\n");
    printf("Activated plugs:\n\tGUI plugin: %d\n\n", PLUGIN_GUI);
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

void terminate(int sig){
    if (!((sig == SIGINT)||(sig == SIGTERM)))
        return;
    syslog(LOG_INFO, "Terminating.");

    audio_player_destroy();

    platform_specific_destroy();

    pthread_cond_destroy(&master_command_context.command_arrived);
    pthread_mutex_destroy(&master_command_context.lock);

    closelog();

    exit(0);
}
