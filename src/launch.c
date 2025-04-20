#include "rd.h"

#define PIDFILE "/tmp/RedDevilPlayer"

struct player p = {
    .path_working_dir =                 {0},
    .song_path_current  =               {0},
    .song_path_next  =                  {0},
    .song_path_prev =                   {0},
    .master_command_execution_status =  MASTER_OK,
    .audio_command_execution_status =   AUDIO_DONE,
    .command =                          COMMAND_NONE,
    // .flags =        0, 
    .lock =                             PTHREAD_MUTEX_INITIALIZER,
    .cond_audio =                       PTHREAD_COND_INITIALIZER,
    .cond_command =                     PTHREAD_COND_INITIALIZER,
};

pthread_t audio_thread =        NULL;
pthread_t key_monitor_thread =  NULL;


int rd_master_daemon(void){

    if (is_already_running() != 0){
        fprintf(stderr, "ERROR: failed to launch rd master daemon.\n");
        return -1;
    }

    if (daemon(0, 0) != 0) {
        perror("Failed to launch rd master daemon.");
        return -1;
    }

    openlog("rd_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Red Devil Daemon started.");
    

    signal(SIGINT,   terminate);
    signal(SIGTERM, terminate);


    if (pthread_create(&audio_thread, NULL, rd_audio_thread, NULL) != 0) {
        syslog(LOG_ERR, "Pthread_create failed (audio thread).");
        return -1;
    }

    if (pthread_detach(audio_thread) != 0){
        syslog(LOG_ERR, "Pthread_detach failed (audio thread).");
        return -1;
    }

    if (pthread_create(&key_monitor_thread, NULL, rd_key_monitor_thread, NULL) != 0) {
        syslog(LOG_ERR, "Pthread_create failed (key monitor thread thread).");
        return -1;
    }
    
    if (pthread_detach(key_monitor_thread) != 0){
        syslog(LOG_ERR, "Pthread_detach failed (key monitor thread thread).");
        return -1;
    }

    for (;;) {
        pthread_mutex_lock(&p.lock);

        while (p.command == COMMAND_NONE){
            pthread_cond_wait(&p.cond_command, &p.lock);
        }

        syslog(LOG_INFO, "Command recieved.");
        p.master_command_execution_status = MASTER_OK; 

        if (master_command_handler[p.command]() != 0) //STATUS_FAILED will return 0, means the app can continue but audio will not run
            terminate(SIGTERM);                        // true failure will return -1;

        if (p.master_command_execution_status == MASTER_FAILED){
            syslog(LOG_ERR, "ERROR: master_command_execution_status failed. Audio command will not run.");
        } 
        if (p.master_command_execution_status == MASTER_OK){ //aka is master_command_execution_status wasnt changed by command 
            p.audio_command_execution_status = AUDIO_WAITING;
        }

        //audio thread will acquire lock, set status to Done once its done
        while (p.audio_command_execution_status == AUDIO_WAITING){
            pthread_cond_wait(&p.cond_audio, &p.lock);
        }

        if (p.audio_command_execution_status == AUDIO_TERMINAL_FAILURE)
            terminate(SIGTERM);

        p.command = COMMAND_NONE;
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
            return 1;           
        }
        perror("flock");
        return -1;               
    }

    ftruncate(pidfile_fd, 0);
    dprintf(pidfile_fd, "%ld\n", (long)getpid());
    return 0;
}