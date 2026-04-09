Mini_Shell (C) 

Mini Shell is a minimal Unix-like command-line interpreter written in C.
It demonstrates core operating system concepts such as process creation, program execution, and command parsing.

This program avoids using system() and directly interacts with the OS via system calls (fork, execvp, wait).

The shell follows a simple loop:
  -Display a prompt (MiniShell>)
  -Read user input using fgets
  -Parse the input into tokens using strtok
  -Create a new process using fork()
  -Execute the command in the child process using execvp()
  -Parent process waits for execution to complete using wait()

Core Concepts Implemented:

1. Command Parsing
  -Input is split into tokens based on delimiters (space, tab, newline)
  -Tokens are stored as an array of strings (char *args[])
  -This mimics how arguments (argv) are passed to programs in Unix

2. Process Creation (fork)
  -A new process is created for each command
  -The child process executes the command
  -The parent process continues running the shell

3. Program Execution (execvp)
  -Replaces the child process with the requested program
  -Uses system PATH to locate executables
   Example:
   ls -l
   
   becomes:
   ["ls", "-l", NULL]

4. Process Synchronization (wait)
  -Parent process waits for the child to finish
  -Prevents overlapping execution and messy output

5. Buffer Handling
  -fflush(stdout) ensures prompt is displayed immediately
  -Proper handling of newline characters from fgets
   
Features:
  -Interactive shell prompt
  -Executes system commands (ls, pwd, whoami, etc.)
  -Argument parsing
  -Process management using fork-exec model
  -Basic error handling

Limitations:
  -No built-in commands like cd
  -No piping (|)
  -No input/output redirection
  -No command history
  -No support for quoted strings

Example Usage:
  -MiniShell> ls
  -MiniShell> pwd
  -MiniShell> whoami

Build & Run:
gcc main.c -o Mini_Shell
./Mini_Shell
