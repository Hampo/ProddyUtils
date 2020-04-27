# ProddyUtils

Util functions for Lua 5.3. Specifically created for 2Take1Menu.

- [Have a look at the example](Example.lua)

## Clipboard

The Clipboard functions are used to interact with the system's clipboard. Only supports text.

### *string* `Clipboard.GetText()`
### *bool* `Clipboard.SetText(string Text)`



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



## IO

The IO functions are used to interact with the user's filesystem.

### *bool* `IO.CreateDirectory(string Path)`
### *bool, bool* `IO.Exists(string Path)`
### *bool* `IO.IterateDirectory(string Path, function Callback)`
