# RedDevil Player 

RedDevilPlayer is lightweight and invisible audio player that runs as a daemon. It is vi-like with minimal visible interface and is controlled entirely through keyboard shortcuts.

## Setup and Running
Requires accessibility privileges to capture keyboard shortcuts. To run simply execute `make run` from the application directory.

## Monitoring Commands
To view application logs do
`log stream --predicate 'process == "RedDevilPlayer"' --info`   


## Keyboard Shortcuts
On MacOs all shortcuts use the combination: **⌃ ⌥ ⌘** (Control + Option + Command) plus:

| Key | Function |
|-----|----------|
| c | Close player (removes song from buffer) |
| h | Help (opens keyboard bindings) |
| o | Open file explorer to select music folder |
| p | Play/Pause |
| l | toggle Loop |
| 0-9 | jump to 0%-90% of song playback |
| f | Forward 5 seconds |
| b | Backward 5 seconds |
| k | Kill (close player completely) |
| v | preVious song |
| n | Next song |
| r | toggle Random playback |
| spacebar | Search mode (type song name + enter to play closest match) |

## Features in Development
- **q**: Toggle play from queue (reads from queue.txt file)
- **w**: Write to queue (enter songs one at a time, finish with two enters)

Developer Andres Torrubia