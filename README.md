Mini Shell (C)

Mini Shell is a highly robust, memory-safe UNIX-like command line interpreter written in C. It demonstrates advanced operating system concepts including low-level process management, inter-process communication, asynchronous signal handling, and dynamic memory allocation. This program avoids wrapper functions and directly interacts with the Linux kernel via system calls.

Core Architecture and Concepts

Advanced Lexical Parsing
The shell replaces standard string tokenization with a custom Lexical State Machine. It accurately parses arguments wrapped in quotes, preserving inner whitespaces and automatically stripping the quotes. It uses dynamic memory to safely allocate arguments on the fly, backed by a strict cleanup protocol (The Janitor Loop) to prevent memory leaks.

Process Creation and Execution
It spawns a perfect clone of the parent process to execute commands via the system PATH, safely transforming process memory spaces without crashing the parent shell.

Inter-Process Communication
The shell implements dynamic piping, allowing the output of one command to flow directly into the input of the next. It manages an array of file descriptors, carefully wiring standard input and output streams and closing unused ends to prevent deadlocks.

File Stream Redirection
It detects redirection symbols and dynamically rewires standard input and output streams. It securely detaches standard input and output from the terminal and points them to local files on the disk.

Asynchronous Signal Handling
The main shell ignores Ctrl+C interrupts, ensuring the shell never accidentally dies. Child processes are explicitly reset so foreground tasks can still be killed. It also utilizes an asynchronous signal handler to silently clean up background processes the exact millisecond they terminate, without freezing the main shell.

Advanced Features

Interactive Prompt: Real-time command interface.
Built-in Commands: Native support for changing directories and exiting the shell.
Background Processing: Allows long-running tasks to execute in the background asynchronously while immediately returning shell control to the user.
Environment Variable Expansion: Dynamically parses and injects system environment variables (like your username or home directory) into commands at runtime.

Built-in Protections

Consecutive Space Shield: Ignores massive blocks of accidental whitespace.
Memory Leak Prevention: Guarantees full heap memory cleanup after every command cycle, even when argument arrays are intentionally severed for pipes.
Empty Command Handling: Safely handles empty Enter presses without crashing.

Example Usage

Standard Execution:
ls -la

Piping and Redirection:
cat input.txt | grep error > output.txt

Background Tasks:
sleep 100 &

Environment Variables:
echo Hello there, $USER

Build and Run Instructions

To compile the shell, use the following command in your terminal:
gcc -o Mini_Shell main.c

To run the shell:
./Mini_Shell
