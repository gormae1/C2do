# C2do
Command line TODO list application, keep track of your TODO items directly in your command line!

![Example usage screenshot](https://raw.githubusercontent.com/gormae1/C2do/refs/heads/main/c2do_usage_example.png)

# Requirements
- libsqlite3
- libreadline

# Usage
Use Make to build the executable, then run the executable. If you do not specify a previous database file as a command line argument, a new one will be created at startup (you will be prompted for the file name). Type 'h' to see the list of commands. Upon exit, all changes will be saved to the database.

Example:

| Command  | Explanation |
| ------------- | ------------- |
|  `./c2do 2do.sqlite3` | Run the program, opening the already-saved database 2do.sqlite3 |
|  `a` | Add an entry to the database |
| `example entry text` | The text for the new entry is 'example entry text' |
|  `work` | The new entry shall be stored under the 'work' tag |
| `med` | The entry will have medium urgency |




