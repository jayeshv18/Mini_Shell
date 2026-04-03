#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
void sigchld_handler(int sig) { //This is an interrupt handler. It pauses the shell, cleans up the zombie, and resumes the shell perfectly
    // WNOHANG means "Clean up dead children, but DO NOT freeze if none are dead"
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


int main() {
    struct sigaction putearplug;// sa is a name variable
    //sa_handler=Signal Action Handler
    putearplug.sa_handler=SIG_IGN;//ignore exit from the shell, and instead exit from current process.
    /*sigemptyset= Clearing the Garbage Memory, Remember how C works.
     *When we typed struct sigaction sa,
     *C did not give a clean, empty structure. It gave us a block of RAM that is full of random garbage memory from whatever program was using it 5 minutes ago.*/
    sigemptyset(&putearplug.sa_mask); //A Mask is simply a list of signals you want the kernel to temporarily block while your sa_handler is currently busy doing its job. It acts like a "Do Not Disturb" sign.

    /*sigemptyset needs to physically alter the memory of our struct to wipe it clean.
     *If we just pass sa.sa_mask, C makes a copy of the mask, wipes the copy, and throws it in the trash,
     *leaving our real struct full of garbage. By passing the memory address (&),
     *we tell the function exactly where on your RAM stick it needs to go to wipe the real thing.*/

    putearplug.sa_flags =0;
    if (sigaction(SIGINT, &putearplug, NULL) == -1) { //SIGINT (Signal Interrupt)
        perror("sigaction failed");
        exit(1);
    }

    /*When a Cook (a child process) finishes their job or gets killed, they don’t just vanish. The Linux kernel turns them into a Zombie.
     *Why? Because the Cook is holding a "Timesheet" (an Exit Status). The kernel keeps the Cook's dead body in the kitchen just in case the Manager (the Parent Shell) wants to know if the Cook finished successfully or if they burned the food and crashed.
     *The only way to make the Zombie vanish is for the Manager to take the Timesheet. In C, we do this by calling waitpid().*/

    /*The Linux kernel has a built-in alarm bell specifically for this. Whenever any child process dies, the kernel rings a bell called SIGCHLD (Signal Child).
    Right now, your Manager ignores that bell.
    Here is what we want to happen:
    1. The Cook dies in the basement.
    2. The Kernel rings the SIGCHLD bell.
    3. The Manager hears the bell, yells "Hold on a second!" to the customer at the front desk, runs to the basement, grabs the Timesheet (waitpid), and immediately runs back to the front desk to finish taking the order.*/

    struct sigaction blockingsigchld;
    blockingsigchld.sa_handler=sigchld_handler;
    sigemptyset(&blockingsigchld.sa_mask);
    //SA_RESTART is critical! If the Manager is listening to the keyboard (fgets)
    // when the bell rings, this tells fgets to automatically resume after the cleanup.
    blockingsigchld.sa_flags=SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &blockingsigchld, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }

    while (1) {
        printf("MiniShell> ");
        fflush(stdout); //print this prompt RIGHT NOW, because NOT guarantee immediate display cause OS might delay or buffer fills.
        char buffer[1024];
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {//fgets is used to get input from keyboard or an io source. whereas scanf will choke at whitespace.
            perror("fgets failed");
            break;
        }
        char *args[64]; //a char ptr array to store the shell commands as string.
        int i=0;
        args[i]=strtok(buffer," \t\n"); //Split whenever you see ANY ONE of these characters ie ' ' (space) or '\n' (newline)
        while (args[i]!=NULL) {
            i++;
            args[i]=strtok(NULL," \t\n");
            // Null in strtok means, continue from where you left last time cause otherwise it'll always give the first element and keep stoping there.
        }
        if (args[0] == NULL) continue; //If user presses enter, crash handling

        int is_background=0; //background processsing variable dec
        if (strcmp(args[i-1],"&")==0) {
            is_background = 1; //flag
            args[i-1]=NULL; //overwrite that & with NULL, When this eventually hits execvp(), the sleep program will literally receive & as an argument, get confused, and throw an error.
        }

        if (strcmp(args[0], "cd")==0) {

            if (args[1]!=NULL) {
                int status=chdir(args[1]); //store result
                if (status==-1) { //handle error
                    perror("Change directory failed");
                }
            }
            else {
                char* home=getenv("HOME"); //getenv is used to get the environmental variables. else case if the user does cd and enter without arguments.
                if (home == NULL) {
                    fprintf(stderr,"cd: HOME not set.\n"); // If home doesn't exists or the env isn't set
                }else {
                    int res=chdir(home);
                    if (res == -1) {
                        perror("Change directory failed");
                    }
                }
            }
            continue; //skip the current iteration once cd is called cause cd doesn't requires fork() or execvp. It acts on parent process and not on child.
        }
        if (strcmp(args[0],"exit")==0) {
            exit(0);
        }

        char **commands[16]; //an array of 16 string arrays
        int num_commands=0; //counter
        commands[num_commands]=&args[0]; //Before we loop, our very first command starts at args[0]
        num_commands++;
        int l=0;

        while (args[l]!=NULL) {
            if (strcmp(args[l],"|")==0) {
                args[l]=NULL; // Sever the array here
                commands[num_commands]=&args[l+1]; //word immediately after the pipe (args[i+1]) is the start of the next command.
                num_commands++;
                /*commands[0] points to ["ls", "-l", NULL]
                commands[1] points to ["grep", "txt", NULL]
                commands[2] points to ["wc", "-l", NULL]*/
            }
            l++;
        }
        int prev_read_fd = 0; // Starts with keyboard input
        pid_t pids[16]; // Store the PIDs of our pipeline
        for (int j=0;j<num_commands;j++) {
            int fd[2];// fd[0] is read, fd[1] is write
            if (j<num_commands-1) {
                if (pipe(fd)==-1) {
                    perror("pipe failed");
                    break;
                }
            }

            pid_t pipe_id=fork();
            if (pipe_id==-1) {
                perror("fork failed");
                continue;
            }
            if (pipe_id==0) {//child process
                if (prev_read_fd!=0) {// Wire input from previous pipe
                    dup2(prev_read_fd,STDIN_FILENO);
                    close(prev_read_fd);
                }
                if (j<num_commands-1) {// Wire output to current pipe
                    dup2(fd[1],STDOUT_FILENO);
                    close(fd[1]);
                    close(fd[0]);// Child doesn't read from the pipe it just wrote to
                }
                // Take away the earplugs so the child can be killed by Ctrl+C
                struct sigaction removearplug;
                removearplug.sa_handler=SIG_DFL; //Tell this struct to use the Default behavior
                sigemptyset(&removearplug.sa_mask);
                removearplug.sa_flags=0;
                sigaction(SIGINT, &removearplug, NULL);

                /* I have used here a metaphor to make things understand easily, suppose there's a restraunt,
                * Your MiniShell (Parent): The Restaurant Manager, The ping command (Child): A Cook you just hired, Ctrl+C (SIGINT): The Fire Alarm.
                * right now, restaurant is using the default rules. When the Fire Alarm (Ctrl+C) goes off, everyone panics. The Cook runs out of the building. The Manager runs out of the building. The restaurant shuts down completely.
                * We want an immortal Manager. We want to give the Manager earplugs. When the Fire Alarm goes off, the Manager ignores it, stays at the desk, and waits.
                * We want the Cook to still hear the alarm, drop what they are doing, and run out of the building.
                * The Manager sees the Cook run out, shrugs, and asks for the next order (the MiniShell> prompt).
                *
                * When you call fork(), the Linux kernel makes a 100% exact clone of the parent process to create the child.
                * Because you put the earplugs on the Manager before you called fork(), the Cook was born wearing earplugs too!
                * If you type ping google.com, the Cook will ignore Ctrl+C. You will never be able to stop the ping.
                * We need to tell the Cook to go back to the standard, default behavior (which is to panic and drop dead when the Fire Alarm goes off).
                *
                * The Fire Alarm is still: SIGINT
                * The Default Behavior is called: SIG_DFL (Signal Default)
                * This applies only to the Cook. Therefore, it must go inside the child process block.
                 */

                //Redirection < >

                int m=0;
                // We use this to remember the exact array index where we need to cut the command.
                // We initialize it to -1 (meaning "no redirection found yet").
                int truncate_index = -1; // Track where to cut the array
                while (commands[j][m]!=NULL) { //Iterate through every single word in the current command array.
                    if (strcmp(commands[j][m],"<")==0) {
                        if (commands[j][m+1]==NULL) { // Error Handling: Did the user type "<" but forget the filename
                            fprintf(stderr,"<: No file specified.\n");
                            _exit(1); // Kill this child process immediately
                        }

                        // The kernel opens it and assigns it the lowest available
                        // file descriptor slot (usually Slot 3, since 0, 1, 2 are taken).

                        int file_fd=open(commands[j][m+1],O_RDONLY);
                        if (file_fd==-1) {
                            perror("open failed");
                            exit(1);
                        }

                        // STDIN_FILENO is Slot 0 (wired to the keyboard by default).
                        // This unplugs the keyboard, and plugs our file's wire into Slot 0.
                        // Now, when the program asks for user input, it reads the file instead!

                        dup2(file_fd,STDIN_FILENO);
                        close(file_fd);// Slot 0 is now reading the file, so we don't need Slot 3 anymore.
                        // Close it to prevent memory leaks in the kernel.

                        // Save the index of the first symbol we find
                        if (truncate_index == -1) truncate_index = m;
                    }
                    else if (strcmp(commands[j][m],">")==0) {
                        if (commands[j][m+1]==NULL) {
                            fprintf(stderr,">: No file specified.\n");
                            _exit(1);
                        }

                        int file_fd=open(commands[j][m+1],O_WRONLY|O_CREAT|O_TRUNC,0644);
                        if (file_fd==-1) {
                            perror("open failed");
                            _exit(1);
                        }

                        // STDOUT_FILENO is Slot 1 (wired to the monitor by default).
                        // This unplugs the monitor, and plugs our file's wire into Slot 1.
                        // Now, when the program runs printf(), the text goes straight into the file!

                        dup2(file_fd,STDOUT_FILENO);
                        close(file_fd); //Cleanup
                        // Save the index of the first symbol we find
                        if (truncate_index == -1) truncate_index = m;
                    }
                    m++; // Move to the next word in the command array
                }
                //Hide the redirection from execvp
                // If we found ANY redirection symbols (truncate_index is no longer -1),
                // we must cut the array right where the very FIRST symbol appeared.

                // Example: ["cat", "<", "input.txt", ">", "out.txt", NULL]
                // Becomes: ["cat", NULL, "input.txt", ">", "out.txt", NULL]

                // Why? execvp() stops reading arguments the moment it sees the first NULL.
                // So the program just sees "cat", but the kernel has already secretly
                // rewired its input and output behind the scenes!
                if (truncate_index != -1) {
                    commands[j][truncate_index] = NULL;
                }

                execvp(commands[j][0],commands[j] );
                perror("Process execution failed");
                _exit(1);

            }
            else {// parent process
                pids[j] = pipe_id; //so the parent remembers the child it just created:
                if (prev_read_fd!=0) { //Close the previous read descriptor (if it's not STDIN)
                    // Close the old read end
                    close(prev_read_fd);
                }// Close the write end of the new pipe, save the read end
                if (j<num_commands-1) { //If not the last command, close the write end of the NEW pipe, and save the read end for the next iteration!
                    close(fd[1]);
                    prev_read_fd=fd[0];
                }
            }
        }// The parent waits for ALL children to finish before showing the prompt again.
        //If is_background is 1, the parent should not run this wait loop.

        if (is_background==0) {//Background processing is 100% managed by the Parent Process
            for (int j=0;j<num_commands;j++) {
                // Wait specifically for the PIDs we just spawned! in the else {} block to track the pids and overcome race conditions
                waitpid(pids[j], NULL, 0);
            }
        }
    }
    return 0;
}