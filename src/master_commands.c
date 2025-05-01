#include "rd.h"

command_handler_function master_command_handler[NUM_COMMANDS] = {
    [COMMAND_NONE]  = command_none,
    [COMMAND_CLOSE] = master_command_close,
    [COMMAND_PLAY_PAUSE] = command_none,        
    [COMMAND_NEXT] = command_none,      
    [COMMAND_PREV] = command_none,       
    [COMMAND_OPEN] = master_command_open, 
    [COMMAND_KILL] = master_command_kill,
    [COMMAND_HELP] = command_none,
    [COMMAND_TOGGLE_RANDOM] = command_none, 
    [COMMAND_TOGGLE_LOOP] = command_none,
    [COMMAND_SEEK] = command_none,
};

int command_none(){return 0;}

//TODO
int master_command_close(){

    return 0;
}

int master_command_open(){
    char prev_path_working_dir[PATH_MAX];
    memcpy(prev_path_working_dir, master_command_context.path_working_dir, PATH_MAX);
    if (choose_directory(master_command_context.path_working_dir, PATH_MAX) != 0){
        syslog(LOG_ERR, "ERROR: open failed.");
        memcpy(master_command_context.path_working_dir, prev_path_working_dir, PATH_MAX);
        master_command_context.master_command_execution_status = MASTER_FAILED;
    }
    syslog(LOG_INFO, "Set working dir to %s", master_command_context.path_working_dir);
    return 0;
}

int master_command_kill(){
    return -1;
}
