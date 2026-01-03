
#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

volatile sig_atomic_t interrupted;

/* Numeric PID comparison for qsort */
int compare(const void *a, const void *b)
{
    const char * const *sa = a;
    const char * const *sb = b;

    int ia = atoi(*sa);
    int ib = atoi(*sb);

    if (ia > ib) return 1;
    if (ia < ib) return -1;
    return 0;
}

/* Signal handler */
void sign(int sig)
{
    printf("\n");
    interrupted = 1;
}

int main(int argc, char *argv[])
{
    DIR *dp;
    struct dirent *dirp;
    char buffer[256];
    int exitCount = 0;

    struct sigaction action = {0};
    action.sa_handler = sign;

    if (sigaction(SIGINT, &action, NULL) == -1) {
        fprintf(stderr,
            "Error: Cannot register signal handler. %s.\n",
            strerror(errno));
    }

    while (exitCount == 0)
    {
        interrupted = 0;

        char pathaName[100];
        if (!getcwd(pathaName, 100)) {
            fprintf(stderr,
                "Error: Cannot get current working directory. %s.\n",
                strerror(errno));
        }

        printf("%s[%s]%s> ", BLUE, pathaName, DEFAULT);

        if ((fgets(buffer, sizeof(buffer), stdin)) == NULL) {
            continue;
        }

        /* removed buffer == NULL (buffer is an array, never NULL) */
        if (interrupted) {
            continue;
        }

        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }

        char *argList[200] = {NULL};
        int argCount = 0;

        char *token = strtok(buffer, " ");
        while (token)
        {
            argList[argCount] = token;
            token = strtok(NULL, " ");
            argCount++;
        }

        if (argList[0] == NULL) {
            continue;
        }

        /* --------------------- cd --------------------- */
        if (strcmp(argList[0], "cd") == 0)
        {
            if (argCount == 1)
            {
                struct passwd *pw = getpwuid(getuid());
                if (pw)
                {
                    char *path = pw->pw_dir;
                    if (chdir(path) != 0) {
                        fprintf(stderr,
                            "Error: Cannot change directory to %s. %s.\n",
                            path, strerror(errno));
                    }
                }
                else {
                    fprintf(stderr,
                        "Error: Cannot get passwd entry. %s.\n",
                        strerror(errno));
                }
            }
            else if (argCount == 2 && strcmp(argList[1], "~") == 0)
            {
                struct passwd *pw = getpwuid(getuid());
                if (pw)
                {
                    char *path = pw->pw_dir;
                    if (chdir(path) != 0) {
                        fprintf(stderr,
                            "Error: Cannot change directory to %s. %s.\n",
                            path, strerror(errno));
                    }
                }
                else {
                    fprintf(stderr,
                        "Error: Cannot get passwd entry. %s.\n",
                        strerror(errno));
                }
            }
            else if (argCount == 2)
            {
                if (argList[1][0] == '~')
                {
                    struct passwd *pw = getpwuid(getuid());
                    char *path = pw->pw_dir;

                    if (chdir(path) != 0) {
                        fprintf(stderr,
                            "Error: Cannot change directory to %s. %s.\n",
                            path, strerror(errno));
                    }

                    char pathExan[PATH_MAX];
                    strcpy(pathExan, pw->pw_dir);
                    strcat(pathExan, &argList[1][1]);

                    if (chdir(pathExan) != 0) {
                        fprintf(stderr,
                            "Error: Cannot change to directory. %s.\n",
                            strerror(errno));
                    }

                    continue;
                }
                else if (chdir(argList[1]) != 0)
                {
                    fprintf(stderr,
                        "Error: Cannot change directory to %s. %s.\n",
                        argList[1], strerror(errno));
                }
            }
            else
            {
                fprintf(stderr,
                    "Error: Too many arguments to cd. %s.\n",
                    strerror(errno));
            }

            continue;
        }

        /* --------------------- exit --------------------- */
        else if (strcmp(argList[0], "exit") == 0)
        {
            exitCount++;
            exit(EXIT_SUCCESS);
        }

        /* --------------------- pwd --------------------- */
        else if (strcmp(argList[0], "pwd") == 0)
        {
            printf("%s\n", pathaName);
            continue;
        }

        /* --------------------- lf --------------------- */
        if (strcmp(argList[0], "lf") == 0)
        {
            dp = opendir(pathaName);
            if (dp == NULL)
            {
                fprintf(stderr, "Cannot open %s\n", pathaName);
                continue;
            }

            while ((dirp = readdir(dp)) != NULL)
            {
                if (strcmp(dirp->d_name, ".") != 0 &&
                    strcmp(dirp->d_name, "..") != 0)
                {
                    printf("%s\n", dirp->d_name);
                }
            }

            closedir(dp);
            continue;
        }

        /* --------------------- lp --------------------- */
        if (strcmp(argList[0], "lp") == 0)
        {
            DIR *proc_dir = opendir("/proc");
            if (proc_dir == NULL)
            {
                fprintf(stderr, "Cannot open %s\n", strerror(errno));
                continue;
            }

            size_t pidCount = 0;
            char *pidList[1000];

            while ((dirp = readdir(proc_dir)) != NULL)
            {
                int is_pid = 1;

                for (int i = 0; dirp->d_name[i] != '\0'; i++)
                {
                    if (!isdigit(dirp->d_name[i]))
                    {
                        is_pid = 0;
                        break;
                    }
                }

                if (is_pid && pidCount < 1000)
                {
                    pidList[pidCount] = dirp->d_name;
                    pidCount++;
                }
            }

            qsort(pidList, pidCount, sizeof(char *), compare);

            int max_dig = 0;
            for (int i = 0; i < pidCount; i++)
            {
                int pid_len = strlen(pidList[i]);
                if (pid_len > max_dig)
                    max_dig = pid_len;
            }

            for (int i = 0; i < pidCount; i++)
            {
                char comdLine[256];
                struct passwd *pw = getpwuid(getuid());

                if (!pw)
                {
                    fprintf(stderr,
                        "Error: Cannot get passwd entry. %s.\n",
                        strerror(errno));
                    continue;
                }

                char path[PATH_MAX];
                strcpy(path, "/proc/");
                strcat(path, pidList[i]);
                strcat(path, "/cmdline");

                FILE *command_file = fopen(path, "r");

                if (command_file == NULL)
                {
                    fprintf(stderr,
                        "Cannot open %s: %s\n",
                        path, strerror(errno));
                    continue;
                }

                if (fgets(comdLine, sizeof(comdLine), command_file) != NULL)
                {
                    printf("%*s %s %s\n",
                        max_dig, pidList[i],
                        pw->pw_name,
                        comdLine);
                }
                else
                {
                    fprintf(stderr,
                        "Cannot open %s: %s\n",
                        path, strerror(errno));
                }

                fclose(command_file);
            }

            closedir(proc_dir);
            continue;
        }

        /* ---------------- exec external command ---------------- */
        pid_t pid;
        int stat;

        if ((pid = fork()) < 0)
        {
            fprintf(stderr,
                "Error: fork() failed. %s.\n",
                strerror(errno));
            continue;
        }
        else if (pid == 0)
        {
            execvp(argList[0], argList);
            fprintf(stderr,
                "Error: exec() failed. %s.\n",
                strerror(errno));
            return 0;
        }
        else
        {
            if (wait(&stat) < 0 && interrupted == 0)
            {
                fprintf(stderr,
                    "Error: wait() failed. %s.\n",
                    strerror(errno));
            }
        }
    }

    return 0;
}
