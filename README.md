MINI SHELL (C)

Welcome to the Mini Shell. This is a highly robust and memory safe command line interpreter, similar to UNIX, written entirely in C. It is designed to demonstrate advanced operating system concepts in a way that is easy to follow for anyone. These concepts include managing low level processes, communicating between different programs, handling system signals asynchronously, and safely allocating memory on the fly. Instead of using prepackaged wrapper functions, this program talks directly to the Linux kernel using native system calls.

CORE ARCHITECTURE AND CONCEPTS

-Advanced Lexical Parsing
Our shell replaces the standard way of chopping up text strings with a custom built Lexical State Machine. This means it accurately reads arguments that are wrapped in quotes, perfectly preserving any spaces inside those quotes, and then automatically removes the quotes for you. It uses dynamic memory to safely create space for these arguments as needed. To make sure your computer memory does not fill up with garbage, it is backed by a strict cleanup protocol called The Janitor Loop, which actively prevents memory leaks.

-Process Creation and Execution
When you type a command, the shell creates a perfect clone of itself. This clone then transforms into the program you requested by searching your system pathways. This safely changes the memory space of the process without ever crashing the main shell you are typing in.

-Inter Process Communication
This shell understands how to pipe data. This allows the output of one command to flow directly into the input of the next command. It does this by managing an array of system file descriptors, carefully wiring the standard input and output streams together, and strictly closing any unused connections to prevent the system from freezing up or deadlocking.

-File Stream Redirection
If you want to save your output or read from a file, the shell detects redirection symbols. It dynamically unplugs the standard input and output streams from your visual terminal and securely reroutes them to point directly at local files sitting on your hard drive.

-Asynchronous Signal Handling
We built the main shell to ignore standard interrupt signals like pressing Control C. This ensures the main shell never accidentally dies while you are working. However, any child processes you launch are explicitly reset, meaning you can still kill a program that gets stuck. It also uses a background signal handler to silently clean up background processes the exact millisecond they finish running, all without freezing the main shell you are interacting with.

ADVANCED FEATURES

-Interactive Prompt
You get a real time command interface to type into.

-Built In Commands
There is native support for changing directories and cleanly exiting the shell without needing external programs.

-Background Processing
You can allow long running tasks to execute out of sight in the background. The shell will immediately return control to you so you can keep typing new commands.

-Environment Variable Expansion
The shell dynamically reads and injects system environment variables right into your commands at runtime. For example, it knows how to replace a variable with your actual username or home directory path.

BUILT IN PROTECTIONS

-Consecutive Space Shield
If you accidentally type a massive block of empty spaces, the shell simply ignores them instead of breaking.

-Memory Leak Prevention
The program guarantees a full cleanup of your heap memory after every single command cycle. This works flawlessly even when command arrays are intentionally chopped up to create pipes.

-Empty Command Handling
If you accidentally press Enter on a blank line, the shell safely handles it and gives you a new prompt without crashing.

EXAMPLE USAGE

For standard execution, you can type basic commands like:
ls -la

For piping and redirection, you can chain commands together like:
cat input.txt | grep error > output.txt

For running tasks in the background, just add an ampersand like:
sleep 100 &

For printing environment variables, you can type something like:
echo Hello there, $USER

BUILD AND RUN INSTRUCTIONS

To compile the shell, open your terminal and run the following command:
gcc -o Mini_Shell main.c

Once it is compiled, you can run the shell by typing:
./Mini_Shell

The architecture of the Mini Shell
<img width="5452" height="7487" alt="Mini_Shell_Architecture" src="https://github.com/user-attachments/assets/7d611a90-c84a-4ec8-849f-0addb4bc92fd" />
