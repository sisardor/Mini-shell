/*
 ============================================================================
 Name        : minishell.c
 Author      : Sardor Isakov
 Version     : v2.0
 Copyright   : All rights reserved
 Description : Simple Shell in C, Ansi-style
 ============================================================================
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include "minishell.h"
struct command_t command;

// internal command clears terminal screen
//
// @return void
void clearScreen() {
	printf("\033[2J\033[1;1H");
}

// not implemented yet
void self() {
	printf("self...\n");
}

// internal command to change diractory
//
// return void
void changeDir() {
	if (command.argv[1] == NULL) {
		chdir(getenv("HOME"));
	} else {
		if (chdir(command.argv[1]) == -1) {
			printf(" %s: no such directory\n", command.argv[1]);
		}
	}
}

// This function check is for built in commands
// and processes if there any
//
// @return boolean
int checkInternalCommand() {

	if(strcmp("cd", command.argv[0]) == 0) {
		changeDir();
		return 1;
	}
	if(strcmp("clear", command.argv[0]) == 0) {
		clearScreen();
		return 1;
	}
	if(strcmp("self", command.argv[0]) == 0) {
		clearScreen();
		return 1;
	}

	return 0;
}

// excuteCommand() executes regular command, commands which doesn't have > < |
// rediractions
//
// example: ls -il, cat filname
//
// @return 0 if exec if successful
int excuteCommand() {

	int child_pid;
	int status;
	pid_t thisChPID;


	child_pid = fork();
	if(child_pid < 0 ) {
		fprintf(stderr, "Fork fails: \n");
		return 1;
	}
	else if(child_pid==0) {
		/* CHILD */
		execve(command.name, command.argv,0);
		printf("Child process failed\n");
		return 1;
	}
	else if(child_pid > 0) {
		/* PARENT */

		do {
			thisChPID = waitpid(child_pid, &status, WUNTRACED | WCONTINUED);
            if (thisChPID == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(status)) {
                //printf("exited, status=%d\n", WEXITSTATUS(status));
                return 0;
            } else if (WIFSIGNALED(status)) {
                printf("killed by signal %d\n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d\n", WSTOPSIG(status));
            } else if (WIFCONTINUED(status)) {
                printf("continued\n");
            }
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		return 0;
	}

}

// this function process commands that have pipeline in it "|"
// this function is called from processCommand() function
// functin accepts indes location of | and processes accordingly
//
// @param	int		is index location of |
// @return	bool	return true on success
int processPipedCommand(int i) {
	char *argvA[5];
	char *argvB[5];
	char *nameA, *nameB;

	int ii;
	for(ii=0;ii<i;ii++) {
		argvA[ii] = command.argv[ii];
	}
	argvA[ii]=NULL;
	nameA = lookupPath(argvA, pathv);

	int j,jj=0;
	for(j=i+1; j<command.argc; j++) {
		argvB[jj] = command.argv[j];
		jj++;
	}
	argvB[jj]=NULL;
	nameB = lookupPath(argvB, pathv);

	int pid, status;
	fflush(stdout);

    switch ((pid = fork())) {
    case -1:
        perror("fork");
        break;
    case 0:
        /* child */
    	executePipedCommand(argvA, argvB, nameA, nameB);
        break;  /* not reached */
    default:
        /* parent; fork() return value is child pid */
        /* These two pids output below will be the same: the process we
         * forked will be the one which satisfies the wait().  This mightn't
         * be the case in a more complex situation, e.g. a shell which has
         * started several "background" processes. */
        pid = wait(&status);
        return 0;
    }
    return 1;
}

// this function process commands that have redirection ">"
// this function is called from processCommand() function
// functin accepts indes location of > and processes accordingly
//
// @param	int		is index location of > within *argv[]
// @return	bool	return true on success
int processFileOutCommand(int i) {

	char *argv[5];
	char *commandName;
	int j;
	for(j=0; j<i; j++) {
		argv[j] = command.argv[j];
	}
	argv[j] = NULL;
	commandName = lookupPath(argv, pathv);

	return executeFileOutCommand(commandName, argv, command.argv[i+1]);
}

// this function process commands that have redirection "<"
// this function is called from processCommand() function
// functin accepts indes location of < and processes accordingly
//
// @param	int		is index location of < within *argv[]
// @return	bool	return true on success
int processFileInCommand(int i) {
	char *argv[5];
	char *commandName;

	int j;
	for(j=0; j<i; j++) {
		argv[j] = command.argv[j];
	}
	argv[j] = NULL;
	commandName = lookupPath(argv, pathv);

	int pid, status;
	fflush(stdout);

    switch ((pid = fork())) {
    case -1:
        perror("fork");
        break;
    case 0:
        /* child */
    	executeFileInCommand(commandName, argv, command.argv[i+1]);
        break;  /* not reached */
    default:
        /* parent; fork() return value is child pid */
        /* These two pids output below will be the same: the process we
         * forked will be the one which satisfies the wait().  This mightn't
         * be the case in a more complex situation, e.g. a shell which has
         * started several "background" processes. */
        pid = wait(&status);
        return 0;
    }

	return 0;
}

// this function process commands and searches for < > |
// if there any redirection than processes accordingly
// if no rediraction than execute regular command
//
// @return	bool	return true on success
int processCommand() {

	int i;
	int infile=0, outfile=0, pipeLine=0;
	char *outFileName;
	char *inFileName;
	for(i=0;i<command.argc; i++) {
		if(strcmp(command.argv[i], ">") == 0) {
			return processFileOutCommand(i);
		}
		else if(strcmp(command.argv[i], "<") == 0) {
			return processFileInCommand(i);

		}
		else if(strcmp(command.argv[i], "|") == 0) {
		    return processPipedCommand(i);
		}
	}
	return excuteCommand();
}

/*
 ============================================================================

	Main method of this program

 ============================================================================
 */
int main(int argc, char* argv[]) {
	int i;
	int debug = 0;

	parsePath(pathv);
	welcomeMessage();

	// main loop
	while(TRUE) {
		printPrompt();

		commandInput = getchar(); //gets 1st char
		if(commandInput == '\n') { // if not input print prompt
			continue;
		}
		else {
			readCommand(commandLine, &commandInput); // read command

			if((strcmp(commandLine, "exit") == 0) || (strcmp(commandLine, "quit") == 0))
				break;

			parseCommand(commandLine, &command); //parses command into argv[], argc

			if(checkInternalCommand() == 0) {
				command.name = lookupPath(command.argv, pathv);

				if(command.name == NULL) {
					printf("Stub: error\n");
					continue;
				}

				processCommand();
			}
		}
	}

	printf("\n");
	exit(EXIT_SUCCESS);
}
