# ProddyUtils

Util functions for Lua 5.3. Specifically created for 2Take1Menu.

- [Have a look at the example](Example.lua)

## ProddyUtils

These functions are in the root of the library.

### *bool* `CheckVersion(int Major, int Minor, int Build)`
### *table* `GetVersion()`
### *metatable* `GetMetatable(string TypeName)`
### *int* `GetTop()`



## Clipboard

The Clipboard functions are used to interact with the system's clipboard. Only supports text.

### *string* `Clipboard.GetText()`
### *bool* `Clipboard.SetText(string Text)`



## IO

The IO functions are used to interact with the user's filesystem.

### *bool* `IO.CreateDirectory(string Path)`
### *bool* `IO.DirExists(string Path)`
### *bool, bool* `IO.Exists(string Path)`
### *bool* `IO.FileExists(string Path)`
### *table* `IO.GetFiles(string Path, string... Extensions)`
### *bool* `IO.IterateDirectory(string Path, function Callback)`



## Keyboard

The Keyboard functions interact with the user's keyboard.

### *bool* `Keyboard.IsKeyPressed(Keyboard.Keys... Keys)`
### *void* `Keyboard.KeyDown(Keyboard.DXKeys... Keys)`
### *void* `Keyboard.KeyUp(Keyboard.DXKeys... Keys)`
### `Keyboard.Keys` - Table containing valid keys.
### `Keyboard.DXKeys` - Table containing valid  DirectInput keys.



## MessageBox

The MessageBox function is used to display a windows MessageBox to the user and receive the selection.

### MessageBox.Buttons
| Name        | Value |
| ----------- | ----- |
| OK          | 1     |
| OKCancel    | 2     |
| RetryCancel | 3     |
| YesNo       | 4     |
| YesNoCancel | 5     |

### MessageBox.DialogResult
| Name        | Value |
| ----------- | ----- |
| OK          | 1     |
| Cancel      | 2     |
| Retry       | 3     |
| Yes         | 4     |
| No          | 5     |

### *MessageBox.DialogResult* `MessageBox.Show(string Text, string Caption, MessageBox.Buttons Buttons)`



## Net

The Net functions are used to access things on the network.

### *bool*, *string|int* `Net.DownloadString(string Host, string Page)`



## OS

The OS functions are used to get system information.

### *int* `OS.GetTimeNano()`
### *int* `OS.GetTimeMicro()`
### *int* `OS.GetTimeMillis()`