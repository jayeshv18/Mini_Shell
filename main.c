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
        /* THE WHITEBOARD METAPHOR: ADVANCED PARSING & MEMORY MANAGEMENT
 * CAST OF CHARACTERS & PROPS:
 * - The Dictator: The user typing the command (buffer).
 * - The Mini-Whiteboard: current_word (Our temporary character canvas).
 * - The "Literal" Sticky Note: in_quotes (Our State Machine flag).
 * - The Polaroid Camera: strdup() (Grabs fresh, permanent memory).
 * - The Photo Album: *args[] (Our array of argument pointers).
 *
 *  THE ASSEMBLY LINE
 * The Dictator reads the command one character at a time.
 * - If it's a normal letter, we write it on our Whiteboard (current_word).
 * - If the Dictator says "Quote!", we flip our Sticky Note (in_quotes = !in_quotes).
 *
 *   THE SPACE TRIGGER
 * If the Dictator says "Space!":
 * - IF the Sticky Note says we are IN QUOTES: We ignore the trigger and just
 * draw a space on the Whiteboard. It's part of a filename!
 * - IF the Sticky Note says we are NOT IN QUOTES: The word is officially done.
 * 1. We check the Whiteboard (c > 0) to make sure it's not completely blank
 * (this protects us from saving multiple spaces in a row).
 * 2. We draw a Stop Sign at the end of the letters ('\0') so C knows where
 * the word actually ends.
 * 3. We use our Polaroid Camera (strdup) to take a permanent photo of the
 * Whiteboard, and tape that photo into our Photo Album (args[i]).
 * 4. We wipe the Whiteboard completely clean (c = 0) to prepare for the next word.
 *
 * THE FINAL FLUSH (Leftover Paint)
 * The Dictator finishes the sentence (\n or \0) and walks out of the room.
 * Because they didn't say "Space" after the very last word, that word is still
 * sitting on our Whiteboard, un-photographed!
 * - We check the Whiteboard one last time (c > 0). If there is ink on it, we
 * draw a Stop Sign, take a Polaroid, and put it in the Album.
 * - FINALLY, we put a completely blank piece of paper (NULL) at the very back
 * of the Photo Album. If we don't do this, the blind robot (execvp) will keep
 * flipping pages past the end of the album into garbage memory and crash.
  */
        char *args[64]; //a char ptr array to store the shell commands as string.
        int i=0;
        int c=0;
        char current_word[1024];
        int in_quotes=0;
        for (int p=0;buffer[p]!='\0' && buffer[p]!='\n';p++) {
            char current_char=buffer[p];
            if (current_char=='"') {
                in_quotes=!in_quotes; // FLIP THE STATE: If 0 make it 1, if 1 make it 0
            }else if (current_char==' ' && in_quotes==0) { // space
                if (c>0) { //Only save if there's a real word.
                    current_word[c]='\0'; //once the string is capped with \0
                    args[i] = strdup(current_word); // we use strdup() to grab fresh memory for it
                    //point args[i] to that memory
                    i++; // Move the array index forward
                    c = 0; // then wipe the canvas by setting c = 0.
                }
            }else {
                current_word[c]=current_char;
                c++;
            }
        }
        if (c>0) {
            current_word[c]='\0';
            args[i] = strdup(current_word);
            i++;
        }
        args[i]=NULL;

        if (args[0] == NULL) continue; //If user presses enter, crash handling

        int is_background=0; //background processsing variable dec
        if (strcmp(args[i-1],"&")==0) {
            is_background = 1; //flag
            free(args[i-1]);// we need to free the memory in the last end .
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
                free(args[l]); // we need to free the mem, check at the end for more details.
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
        /* When we use strdup(), it allocates a block of memory on the heap. our array (args[index]) holds the pointer (the memory address) to that block.
         * If we write args[index] = NULL; first, we overwrite the memory address. The heap memory is still allocated, but our program has lost the only pointer to it.
         * we can no longer access or free it. This is a memory leak (specifically, an orphaned pointer).
         *
         * we must call free(args[index]); to release the heap memory before we overwrite the pointer with NULL.
         *
        * To make pipes (|) and backgrounding (&) work with execvp(), our code intentionally injects NULLs into the middle of the args array.
        * If we use a while (args[k] != NULL) loop to clean up memory, the loop will terminate the millisecond it hits the first injected NULL.
        * Any arguments that came after the pipe will be completely ignored and left in RAM, causing a memory leak.
        * Because the variable i tracked the exact number of words we originally allocated,
        * a for (int k = 0; k < i; k++) loop forces the program to check the entire length of the array, bypassing the injected NULLs and safely freeing every remaining pointer.
         */

        for (int k=0;k<i;k++) { // i is the exact total number of words we parsed
            if (args[k]!=NULL) { // Only free it if it isn't already a blank page
                free(args[k]);
            }
        }
    }
    return 0;
}