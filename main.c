#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
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
        
        pid_t pid = fork(); // pid_t is a specialized data type used to represent Process IDs
        if (pid<0) {
            perror("Process creation failed");
            continue;
        }
        if (pid == 0) { //child process
            execvp(args[0],args); //in unix args[0] = program name. And the rest are arguments.
            //args[0] is the program name (ls), execvp asks the kernel to load that program and replace the current process with it.
            perror("Process execution failed"); // only runs if exec fails
            _exit(1); // terminate current process immediately, Child dies instantly, No side effects, No duplicate flushing
        }else{ //parent process
            wait(NULL);
        }
    }
    return 0;
}