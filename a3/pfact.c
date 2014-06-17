#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>

pid_t Waitpid(pid_t pid, int *status, int option) {
    pid_t result = waitpid(pid, status, option);
    if (result == -1) {
        perror("Wait Error");
        exit(-1);
    } else {
        return result;
    }
}

int Read(int fd, void *buffer, size_t size) {
    int result = read(fd, buffer, size);
    if (result == -1) {
        perror("Write to pipe error");
        exit(-1);
    } else {
        return result;
    }
}

int Write(int fd, void *buffer, size_t size) {
    int result = write(fd, buffer, size);
    if (result == -1) {
        perror("Write to pipe error");
        exit(-1);
    } else {
        return result;
    }
}

void Pipe(int fd[2]) {
    if ((pipe(fd)) == -1) {
        perror("Error making pipe");
        exit(-1);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: pfact NUMBER");
    } else {
        pid_t pid;
        // Create a master "incoming" file descriptor fd.
        int fd[2];
        // Create a file descriptor to send data to chained child processes.
        int fd_minions[2];
        // Make a Master pipe
        Pipe(fd);
        // Find out the number to be factored
        int n = atoi(argv[1]);
        int max_k = ceil(sqrt(n));
        int current_k = 1;
        int m = 2;
        int factor1 = 0;
        int child_status = 0;
        int result;

        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
            perror("signal");
            exit(-1);
        }

        /*
         * Fork into: 1. Master process which feeds the numbers
         * and 2. Child proccess(es) for factoring.
        */
        pid = fork();
        if (pid == -1) {
            // Error
            perror("Error forking master process");
            exit(-1);
        } else if (pid > 0) {
            /*
             * Master parent process which feeds the numbers begins.
             */

            //Don't need to read from the child - close the pipe.
            close(fd[0]);

            /* Send all the numbers up to sqrt(n)/max_k to child process(es).
             * We don't bother sending numbers above sqrt(n) because it's wasteful
             * as we know that there can't be any prime factors for n between sqrt(n) and n.
             */
            int k;
            for (k = 2; k <= max_k; k++) {
                // Write a number to the pipe.
                Write(fd[1], &k, sizeof(int));

                /*
                 * Check for the termination of the child process.
                 * If the child process has terminated and we haven't sent
                 * all the numbers through the pipe, two prime numbers have been found
                 * and we can terminate without having to write all the remaining
                 * numbers so we can break.
                 */
                result = Waitpid(pid, &child_status, WNOHANG);
                if (result) {
                    break;
                }
            }

            /*
             * If the child proccess(es) haven't terminated yet, there is no prime factor for n
             * between 2 and sqrt(n). The only possibility left to check is if n itself is prime.
             * So we write n to the pipeline.
             */
            if (!result) {
                k = n;
                Write(fd[1], &k, sizeof(int));

                /*
                 * Here we use -1 as a termination signal for the child process(es) to let them know
                 * that all the numbers have been sent and if prime(s) haven't been found yet, they should
                 * terminate.
                 */
                k = -1;
                Write(fd[1], &k, sizeof(int));
            }

            // Wait for child process(es) to exit.
            Waitpid(pid, &child_status, 0);


            // If any of the child processe(es) exited with an error (-1), exit the main process with an -1.
            if (WEXITSTATUS(child_status) == -1) {
                exit(-1);
            } else {
                fprintf(stderr, "Number of stages = %d\n", WEXITSTATUS(child_status));
            }

        } else {
            // Child or "worker processes"
            // No need to write to the pipe (to the partent), therefore we can close it.
            close(fd[1]);
            // Keep running till the parent process is sending numbers.
            while (Read(fd[0], &current_k, sizeof(int)) > 0) {
                // If there are any children, check if they exited, and exit accordingly
                if (pid) {
                    if (Waitpid(pid, &child_status, WNOHANG)) {
                        exit(WEXITSTATUS(child_status) + 1);
                    }
                }

                // Check for termination signal.
                if (current_k == -1) {
                    // If there is a child process, forward the termination signal to it and wait for it to exit.
                    if (pid) {
                        Write(fd_minions[1], &current_k, sizeof(int));
                        Waitpid(pid, &child_status, 0);
                        // If child process exited with error, exited with error.
                        if (WEXITSTATUS(child_status) == -1) {
                            exit(-1);
                        } else {
                            // If child process exited successfully, exit w/ child's exit code + 1.
                            exit(WEXITSTATUS(child_status) + 1);
                        }

                    } else {
                        /* If there are no child processes, and no more numbers to check,
                         * the number isn't prime or product of prime so we exit.
                         */
                        printf("%d is not the product of two primes\n", n);
                        exit(1);
                    }
                } else {
                    /*
                     * If the number coming into the pipe isnt divisible by the current process's m, pass it onto
                     * the next child in the pipe. If the next child does not exist, create it.
                     */
                    if ((current_k % m) != 0) {
                        if (!pid) {
                            // If there isn't a child for this process, create one w/ a pipe to it.
                            Pipe(fd_minions);

                            pid = fork();
                            if (pid == -1) {
                                perror("Error forking minion process");
                                exit(-1);
                            }
                        }
                        if (pid == 0) {
                            // A new child in the pipe is created.
                            fd[0] = fd_minions[0];
                            // Close unused write file descriptor to the parent.
                            close(fd_minions[1]);
                            // First number to make it past to here is m for this process.
                            Read(fd[0], &m, sizeof(int));
                        } else {
                            // Parent process in the chain of processes.
                            // Pass the number that is the new child's m to the new child via fd_minions pipe.
                            Write(fd_minions[1], &current_k, sizeof(int));
                        }
                    }
                    // If the number that the process just got from its parent divides n, you have a prime factor.
                    if (n % m == 0) {
                        /*
                         * If a factor previously exists, check if the the current prime factor
                         * and the previous one multiply to the number n
                         */
                        if (factor1) {
                            if ((factor1 * m) == n) {
                                printf("%d %d %d\n", n, factor1, m);
                                exit(1);
                            } else if (factor1 != m) {
                                printf("%d is not the product of two primes\n", n);
                                exit(1);
                            }
                        } else if ((m * m) == n) {
                            // Check if the number n is square of the current prime factor.
                            printf("%d %d %d\n", n, m, m);
                            exit(1);
                        } else if (n == m) {
                            // Check if the number itself is prime.
                            printf("%d is prime\n", n);
                            exit(0);
                        } else {
                            /*
                             * If neither of the above scenerios happen, we continue looking
                             * and store the current prime factor for later.
                             */
                            factor1 = m;
                        }
                    }
                }
            }
        }

    }
    return 0;
}