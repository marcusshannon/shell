#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/fcntl.h>

#define MAXLINE 200
#define MAXARGS 20
#define PIPE_READ 0
#define PIPE_WRITE 1

static char * getword(char * begin, char **end_ptr) {
    char * end = begin;

    while ( *begin == ' ' )
        begin++;  /* Get rid of leading spaces. */
    end = begin;
    while ( *end != '\0' && *end != '\n' && *end != ' ' )
        end++;  /* Keep going. */
    if ( end == begin )
        return NULL;  /* if no more words, return NULL */
    *end = '\0';  /* else put string terminator at end of this word. */
    *end_ptr = end;
    if (begin[0] == '$') { /* if this is a variable to be expanded */
        begin = getenv(begin+1); /* begin+1, to skip past '$' */
        if (begin == NULL) {
            perror("getenv");
            begin = "UNDEFINED";
        }
    }
    return begin; /* This word is now a null-terminated string.  return it. */
}

static void getargs(char cmd[], int *argcp, char *argv[])
{
    char *cmdp = cmd;
    char *end;
    int i = 0;

    if (fgets(cmd, MAXLINE, stdin) == NULL && feof(stdin)) {
        printf("Couldn't read from standard input. End of file? Exiting ...\n");
        exit(1);
    }
    while ( (cmdp = getword(cmdp, &end)) != NULL && (strcmp(cmdp, "#") != 0)) { /* end is output param */

        if ( (strcmp(cmdp, ">") == 0) ||  (strcmp(cmdp, "<") == 0) ||  (strcmp(cmdp, "|") == 0) ||  (strcmp(cmdp, "&") == 0) ) {
            argv[i++] = NULL;
        }
        argv[i++] = cmdp;
        cmdp = end + 1;
    }
    argv[i] = NULL;
    *argcp = i;
}

/*  Above this comment: Copyright (c) Gene Cooperman, 2006; May be freely copied as long as this
*   copyright notice remains.  There is no warranty.
*/

static void execute(int argc, char *argv[])
{
    int pipefd[2];
    int fdd;

    pid_t childpid, childpid2;

    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    if (pipe(pipefd) == -1) {
        perror("pipe");
    }

    childpid = fork();

    if (argc > 1 && argv[2] != NULL && (strcmp(argv[argc - 2], "|") == 0) && childpid > 0) childpid2 = fork();

    if (childpid == -1) {
        perror("fork");
        printf("  (failed to execute command)\n");
    }
    if (childpid == 0) {
        if (argc > 1) {
            if ((strcmp(argv[argc - 2], ">") == 0)) {
                close(STDOUT_FILENO);
                int fd = open(argv[argc - 1], O_WRONLY);
                if (fd == -1) perror("open for writing");
            }
            if ((strcmp(argv[argc - 2], "<") == 0)) {
                close(STDIN_FILENO);
                int fd = open(argv[argc - 1], O_RDONLY);
                if (fd == -1) perror("open for reading");
            }
            if ((strcmp(argv[argc - 2], "|") == 0)) {
                /* close  */
                if (-1 == close(STDOUT_FILENO)) {
                    perror("close");
                }

                fdd = dup(pipefd[PIPE_WRITE]); /* set up empty STDOUT to be pipe_fd[1] */
                if (-1 == fdd) {
                    perror("dup");
                }

                if (fdd != STDOUT_FILENO) {
                    fprintf(stderr, "Pipe output not at STDOUT.\n");
                }

                close(pipefd[PIPE_READ]); /* never used by child1 */
                close(pipefd[PIPE_WRITE]); /* not needed any more */
            }
        }
        if (-1 == execvp(argv[0], argv)) {
            perror("execvp");
            printf("  (couldn't find command)\n");
        }
        /* NOT REACHED unless error occurred */
        exit(1);
    } else if (argc > 1 && argv[2] != NULL && (strcmp(argv[argc - 2], "|") == 0) && childpid2 == 0) {
        /* close  */
        if (-1 == close(STDIN_FILENO)) {
            perror("close");
        }
        /* set up empty STDIN to be pipe_fd[0] */
        fdd = dup(pipefd[PIPE_READ]);
        if (-1 == fdd) {
            perror("dup");
        }

        if (fdd != STDIN_FILENO) {
            fprintf(stderr, "Pipe input not at STDIN.\n");
        }

        close(pipefd[PIPE_READ]); /* not needed any more */
        close(pipefd[PIPE_WRITE]); /* never used by child2 */

        argv[0] = argv[argc - 1];
        if (-1 == execvp(argv[0], argv)) {
            perror("execvp");
        }
    }
    else {
        close(pipefd[PIPE_READ]);
        close(pipefd[PIPE_WRITE]);

        if (argc > 1 && (strcmp(argv[argc - 1], "&") == 0)) return;

        waitpid(childpid, NULL, 0);  /* wait until child process finishes */

        if (argc > 1 && (strcmp(argv[argc - 2], "|") == 0)) {
            waitpid(childpid2, NULL, 0);
        }
        /* parent:  in parent, childpid was set to pid of child process */
    }
    return;
}

void interrupt_handler(int signum) {
    if (signum == SIGINT)
        printf("\nInterruption signal recognized\n");
}

int main(int argc, char *argv[])
{
    signal(SIGINT, interrupt_handler);
    char cmd[MAXLINE];
    char *childargv[MAXARGS];
    int childargc;

    if ( argc > 1 && strcmp(argv[1], "script.myshell") == 0 ) freopen(argv[1], "r", stdin);

    while (1) {
        printf("%% ");
        fflush(stdout);
        getargs(cmd, &childargc, childargv);
        if ( childargc > 0 && strcmp(childargv[0], "exit") == 0 )
            exit(0);
        else if ( childargc > 0 && strcmp(childargv[0], "logout") == 0 )
            exit(0);
        else
            execute(childargc, childargv);
    }
}
