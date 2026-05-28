# typebox

A minimal floating input bar for screensharing while muted. Type what you want to say — press **Enter** to copy to clipboard and clear.

## Build

Requires `gtk4` and `ttf-jetbrains-mono`.

```bash
make
./build/typebox
```

## Shortcuts

| Key        | Action                          |
|------------|---------------------------------|
| Type       | Append text                     |
| Backspace  | Delete last character           |
| Enter      | Copy to clipboard, then clear   |
| Ctrl+C     | Copy selection                  |
| Ctrl+V     | Paste                           |
| Ctrl+X     | Cut                             |
| Ctrl+A     | Select all                      |
| Ctrl+Z     | Undo                            |
| Escape     | Quit                            |

## Hyprland

```ini
windowrule = float true, match:class dev.typebox

# optional keybind
bind = SUPER, SPACE, exec, path-to-executable
```
