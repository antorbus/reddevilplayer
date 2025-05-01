#include "plugins.h"
// #include <SDL2/SDL.h>



// on hotkey + t will show timestamp
// audio timebar will look something like ====$-------------
// audio timestamp will look like         01:56:21 - 06:01:30

// on hotkey + s will search for text
// search bar will apear like this: TEXT THAt is autocompleted (upper case for usertext, lower for most similar)
// use Jaro–Winkler or Levenshtein distance for fuzzy matching.

// on hotkey + m will make a queue (playlist)
// queue will use same autocomplete format but user text will be added to a .txt 

// on hotkey + q will play from queue 

// on hoteky + o will have the option to also open a queue

// on hotkey + h will show the help menu

int gui_show_help_menu(void) {
    return 0;
}
