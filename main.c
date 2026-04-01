#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
int main() {
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
            if (pipe_id==0) {
                if (prev_read_fd!=0) {// Wire input from previous pipe
                    dup2(prev_read_fd,STDIN_FILENO);
                    close(prev_read_fd);
                }
                if (j<num_commands-1) {// Wire output to current pipe
                    dup2(fd[1],STDOUT_FILENO);
                    close(fd[1]);
                    close(fd[0]);// Child doesn't read from the pipe it just wrote to
                }
                // < and > logic
                execvp(commands[j][0],commands[j] );
                perror("Process execution failed");
                _exit(1);
            }else {// parent process
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
        for (int j=0;j<num_commands;j++) {
            wait(NULL);
        }
    }
    return 0;
}