#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Read a user id and password from standard input, 
   - create a new process to run the validate program
   - use exec (probably execlp) to load the validate program.
   - send 'validate' the user id and password on a pipe, 
   - print a message 
        "Password verified" if the user id and password matched, 
        "Invalid password", or 
        "No such user"
     depending on the return value of 'validate'.

Setting the character arrays to have a capacity of 256 when we are only
expecting to get 10 bytes in each is a cheesy way of preventing most
overflow problems.
*/

#define MAXLINE 256
#define MAXPASSWD 10

void strip(char *str, int capacity) {
    char *ptr;
    if((ptr = strchr(str, '\n')) == NULL) {
        str[capacity - 1] = '\0';
    } else {
        *ptr = '\0';
    }
}


int main(void) {
    char userid[MAXLINE];
    char password[MAXLINE];
    pid_t pid;
    int fd[2];
    int exitStatus;

    /* Read a user id and password from stdin */
    printf("User id:\n");
    if((fgets(userid, MAXLINE, stdin)) == NULL) {
        fprintf(stderr, "Could not read from stdin\n"); 
        exit(1);
    }
    strip(userid, MAXPASSWD);

    printf("Password:\n");
    if((fgets(password, MAXLINE, stdin)) == NULL) {
        fprintf(stderr, "Could not read from stdin\n"); 
        exit(1);
    }
    strip(password, MAXPASSWD);

    /*Your code here*/

    pipe(fd);

    pid = fork();
        if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid > 0){
        /* Parent */
        

        write(fd[1],userid,10);
        write(fd[1],password,10);
        close(fd[1]);
        // Don't really need the pid anymore, just recycle that variable
        if ((wait(&pid)) == -1){
            perror("wait error");
        }else{
            exitStatus = WEXITSTATUS(pid);
            if(exitStatus == 0){
                printf("Password verified\n");
            }else if(exitStatus == 3){
                printf("No such user\n");
            }else if (exitStatus == 2){
                printf("Invalid password\n");
            }
        }
    } else{
        /* Child */
        if((dup2(fd[0], fileno(stdin))) == -1) {
            perror("dup2");
            exit(1);
        }else{
            close(fd[0]);
            close(fd[1]);
        }
        execlp("/Users/B-1P/Documents/CSC 209/tempCode/Ex5/validate", "validate", NULL);
        printf("Something went wrong");
        perror("execFail");
        exit(1);
    }
    
    return 0;
}
