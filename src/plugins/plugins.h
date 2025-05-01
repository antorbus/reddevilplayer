#ifndef PLUGINS_H
#define PLUGINS_H

#include "../rd.h"

//GUI
#if PLUGIN_GUI == 1

#include <SDL2/SDL.h>
int gui_show_help_menu(void);

#endif

#endif