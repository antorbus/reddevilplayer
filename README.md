
RedDevil Player 
Andres Torrubia

use:
pgrep -fl RedDevilPlayer
to view PID

use:
log stream --predicate 'process == "RedDevilPlayer"' --info
to view syslog

To open bindings press control (⌃ ⌥ ⌘) + option (⌥) + command (⌘) + [one of]:
- c (close player NOT KILL, removes song from buffer)
- h (Help, opens bindings)
- o (Open, opens file explorer so you can select folder with songs)
- p (Play/Pause)     
- l (Loop)
- 0-9 (goes to 0% to 90% of song playback)
- f (Forwards 5 seconds)
- b (Backwards 5 seconds)
- k (Kill, closes player)
- v (preVious song)
- n (Next song)
- r (toggle Random)
- spacebar + [song name] + enter (Searches song and plays closest match) 


Needs to be fletched out
- q (toggle play from Queue, from file queue.txt, uses closest match)
- w (Writes queue one song at a time, separated by an enter until two enters are pressed)



TODO: 
- clear error handling in metal_handlers
- add error checks around pthread_mutex_lock because they can return error codes
- switch to launchd
- icon 
- make it so that users cannot open help more than once
- make text for help better
- add queue files that can be created "playlists"
- make open recursive
- multiple queue things buttons to add songs to queue
- better status bar logo
- cool terminal ascii logo
- show song name + playtime + random + algorithmic + liked
- support for SID etc
- maybe start playback from prev song
