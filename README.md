# NTW_Shell
Α simple unix shell made for the Operating Systems Class 7th Semester, ECE AUTH 2017-2018

To use this shell firstly download the files *shell.c* and *Makefile* from this repo.
Then execute the commands:
  - `cd path_to_files`
  - `make` or `make all`
  - `./ntw_shell` or `./fe_shell`

TODO: continue this readme file.

## Program's and source code explanation
The aim of the exercise was to make a **unix shell program** that would execute commands by forking new processes which would use the
`int execvp(const char *file, char *const argv[]);` function.

The shell supports two modes:
- The `Interactive mode`
- and the `Batch file mode`

Both modes support lines with:
- Single commands with/without arguments
- Multiple commands separated by `;` or `&&`
  - Separating two commands with `;` will result in the execution of both commands
  whether or not the first command executed successfully.
  - Separating two commands with `&&` will result in the execution of both commands
  **ONLY** if the first command executed successfully.
- The "Special" commands: `quit` and `cd` which are executed by the main process as executing them on a forked process will not achieve the desired behavior.

### New data types
In order to execute the commands using the `execvp()` function the commands have to be in a *string array* format where each element is a pointer to a command's argument and the last element points to `NULL`.

Therefore, the program needs to make that *array string* and make the last element point to `NULL`. To do that the number of arguments of each command is needed. The program also needs to know the separator (`;` or `&&`) used before and after each command to choose whether or not will one command be executed. Finally, the program should know if a command is "special" in order to execute it at the main process without forking.

So, to store all the above information and have them nicely organized, the `struct command` datatype was created and used.

The enumerators `E_delimiter_type` and `E_special_type` were used for storing the delimiters and the special type of each command for better readability of the program.

### Program's main loop
The program starts by entering into the `interactive mode` or the `batch file mode` based on the number of arguments provided at the execution. If there are two arguments it enters into the `batch file mode` only if the second argument (the first is always the program's name) is a valid file's name, if it isn't or there is no second argument it enters the `interactive mode` by setting the input data file pointer to `stdin`.

The differences between the two modes (except that the input data stream is not the same) are that in the `batch file mode`
- there is no prompt message printed after the execution of every line and 
- there is no need for the command `quit` to be included in the file's end in order to terminate the program. That happens automatically after all the commands are parsed and executed (if needed).

In any case the program enters a `while` loop where on every iteration a line of data is read. That line is then parsed by the `parseBuffer()` function which will fill all the commands' information it found into the `struct command commands` array. It will also fill the number of the commands it found in the `command_num` variable.

After that, the commands found will be executed sequentially one-by-one by either the main process (if the command is "special") or by a forked process with the `execvp()` function. 

In the case where a command fails to execute, based on the separator before the next command, it will be determined if the next command will be skipped or executed. A message informing the user -that the command failed to execute- will be printed on the `stderr` as well.

There is also a stricter behavior programmed, where a command-fail will result in the program's termination (as long as there is not a ';' separator after the command). To enable this behavior, compile the program with the `-D __FORCE_EXIT_ON_ERROR` or run the `fe_shell` program which is automatically made by compiling with `make all`.

#### Method: read_line()
It case the program run in the `interactive mode` it prints a prompt message. Then it reads a line pointed be the fd. The **max line length** it reads is defined and has on-default- the value **512**. If the file pointer has reached `EOF` or a `NULL` value, the function return -1. Otherwise, it returns 0.

#### Method: parse_buffer()
This function parses the buffer (one line of input) and extracts the commands. It works by searching the separators (`&&` and `;`) in the line. Then, whatever is before, after or between two separators is assumed to be a possible command. If there are no separators found, the whole line is assumed to be one command.

Then, it parses each possible command to extract its arguments. The arguments extracted, their number and the delimiters (separators) before and after each command are all stored to the `struct command** commands` pointed array.

Any whitespace between the arguments or the commands is ignored and there are also checks in order to not overflow the commands' array (max command's, argument's number and max argument's size are all defined to default values that usually suffice)
