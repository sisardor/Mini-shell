#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include "minishell.h"


// print welcome message
void welcomeMessage() {
	printf("\nWelcome to mini-shell\n");
}

// print prompt
void printPrompt() {
	printf("mshell > ");
}

// looks up path using environment variable PATH
// function looks up is command is actually in path
// if it finds command in that path, it will return path
char * lookupPath(char **argv, char **dir) {
	char *result;
	char pName[MAX_PATH_LEN];
	if( *argv[0] == '/') {
		return argv[0];
	}
	else if( *argv[0] == '.') {
		if( *++argv[0] == '.') {
			 if(getcwd(pName,sizeof(pName)) == NULL)
				 perror("getcwd(): error\n");
			 else {
				 *--argv[0];
				 asprintf(&result,"%s%s%s",pName,"/",argv[0]);
			 }
			 return result;
		}
		*--argv[0];
		if( *++argv[0] == '/') {
			if(getcwd(pName,sizeof(pName)) == NULL)
				perror("getcwd(): error\n");
			else {
				asprintf(&result,"%s%s",pName,argv[0]);
			}
			return result;
		}
	}

	// look in PAH directories, use access() to see if the
	// file is in the dir
	int i;
	for(i = 0 ; i < MAX_PATHS ; i++ ) {
		if(dir[i] != NULL) {
			asprintf(&result,"%s%s%s",  dir[i],"/",argv[0]);
			if(access(result, X_OK) == 0) {
				return result;
			}
		}
		else continue;
	}

	fprintf(stderr, "%s: command not found\n", argv[0]);
	return NULL;
}

// this function populates "pathv" with environment variable PATH
int parsePath(char* dirs[]){
	int debug = 0;
	char* pathEnvVar;
	char* thePath;
	int i;

	for(i=0 ; i < MAX_ARGS ; i++ )
		dirs[i] = NULL;
	pathEnvVar = (char*) getenv("PATH");
	thePath = (char*) malloc(strlen(pathEnvVar) + 1);
	strcpy(thePath, pathEnvVar);

	char* pch;
	pch = strtok(thePath, ":");
	int j=0;
	// loop through the thePath for ':' delimiter between each path name
	while(pch != NULL) {
		pch = strtok(NULL, ":");
		dirs[j] = pch;
		j++;
	}

	//===================== debug ===============
	// print the directories if debugging
	if(debug){
		printf("Directories in PATH variable\n");
		for(i=0; i<MAX_PATHS; i++)
			if(dirs[i] != '\0')
				printf("    Directory[%d]: %s\n", i, dirs[i]);
	}
}

// this function parses commandLine into command.argv and command.argc
int parseCommand(char * commandLine, struct command_t * command) {
	int debug = 0;

	char * pch;
	pch = strtok (commandLine," ");
	int i=0;
	while (pch != NULL) {
		command->argv[i] = pch;
		pch = strtok (NULL, " ");
		i++;
	}
	command->argc = i;
	command->argv[i++] = NULL;

	// pay no attention to this
	//===================== debug ===============
	if(debug) {
		printf("Stub: parseCommand(char, struct);\n");
		printf("Array size: %i\n", sizeof(*command->argv));
		int j;
		for(j=0; j<i; j++) {
			printf("command->argv[%i] = %s\n", j, command->argv[j]);
		}
		printf("\ncommand->argc = %i\n", command->argc);

		if(command->argv[0] != NULL) {
			//printf("*command->argv[%i] = %c\n", j, *command->argv[0]);
			char **p;
			for(p = &command->argv[1]; *p != NULL; p++) {
				printf("%s\n", *p);
			}
		}
	}
	return 0;
}

// this function read user input and save to commandLine
int readCommand(char * buffer, char * commandInput) {
	int debug = 0;
	buf_chars = 0;


	while((*commandInput != '\n') && (buf_chars < LINE_LEN)) {
		buffer[buf_chars++] = *commandInput;
		*commandInput = getchar();
	}
	buffer[buf_chars] = '\0';

	//===================== debug ===============
	if(debug){
		printf("Stub: readCommand(char *)\n");

		int i;
		for(i=0; i<buf_chars; i++) {
			printf("buffer[%i] = %c\n", i, buffer[i]);
		}
		printf("\nlength: %i\n", buf_chars-1);
		printf("\n1. buffer %s\n", buffer);
		printf("2. buffer[%i] = %c\n", buf_chars-2, buffer[buf_chars-2]);
		if(buffer[buf_chars-1] == '\n')
			printf("3. buffer[%i] = '\\n'\n", buf_chars-1);
		if(buffer[buf_chars] == '\0')
			printf("4. buffer[%i] = '\\0'\n", buf_chars);
	}
	return 0;
}

// this function is called from processFileInCommand(int)
// this function executes command with "<"
int executeFileInCommand(char * commandName, char * argv[], char * filename) {
	int pipefd[2];

	if(pipe(pipefd)) {
		perror("pipe");
		exit(127);
	}

	switch(fork()) {
	case -1:
		perror("fork()");
		exit(127);
	case 0:
        close(pipefd[0]);  /* the other side of the pipe */
        dup2(pipefd[1], 1);  /* automatically closes previous fd 1 */
        close(pipefd[1]);  /* cleanup */
 	   FILE * pFile;
 	   char mystring;

 	   pFile = fopen (filename , "r");
 	   if (pFile == NULL) perror ("Error opening file");
 	   else {

 		while ((mystring=fgetc(pFile)) != EOF) {
 				putchar(mystring); /* print the character */
 			}
 	     fclose (pFile);
 	   }
 	  exit(EXIT_SUCCESS);

    default:

        close(pipefd[1]);  /* the other side of the pipe */
        dup2(pipefd[0], 0);  /* automatically closes previous fd 0 */
        close(pipefd[0]);  /* cleanup */

        execve(commandName, argv, 0);
        perror(commandName);
        exit(125);

	}

	return 0;
}

// this function is called from processFileOutCommand(int)
// this function executes command with ">"
int executeFileOutCommand(char * commandName, char * argv[], char * filename) {
	int def = dup(1);

    //First, we're going to open a file
    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if(file < 0)    return 1;

    //Now we redirect standard output to the file using dup2
    if(dup2(file,1) < 0)    return 1;
    int pid;
    if( pid = fork() == 0) {
    	close(file);
    	close(def);
    	execve(commandName, argv,0);
    	return 0;
    }
    dup2(def, 1);
    close(file);
    close(def);
    wait(NULL);
 	close(file);
	return 0;
}

// this function is called from processPipedCommand(int)
// this function executes command with "|"
void executePipedCommand(char *argvA[], char  *argvB[], char * nameA, char * nameB) {
	int pipefd[2];

	if(pipe(pipefd)) {
		perror("pipe");
		exit(127);
	}

	switch(fork()) {
	case -1:
		perror("fork()");
		exit(127);
	case 0:
        close(pipefd[0]);  /* the other side of the pipe */
        dup2(pipefd[1], 1);  /* automatically closes previous fd 1 */
        close(pipefd[1]);  /* cleanup */
        /* exec ls */
        execve(nameA, argvA, 0);
        /* return value from execl() can be ignored because if execl returns
         * at all, the return value must have been -1, meaning error; and the
         * reason for the error is stashed in errno *///
        perror(nameA);
        exit(126);
    default:
        /* parent */
        /*
         * It is important that the last command in the pipeline is execd
         * by the parent, because that is the process we want the shell to
         * wait on.  That is, the shell should not loop and print the next
         * prompt, etc, until the LAST process in the pipeline terminates.
         * Normally this will mean that the other ones have terminated as
         * well, because otherwise their sides of the pipes won't be closed
         * so the later-on processes will be waiting for more input still.
         */
        /* do redirections and close the wrong side of the pipe */
        close(pipefd[1]);  /* the other side of the pipe */
        dup2(pipefd[0], 0);  /* automatically closes previous fd 0 */
        close(pipefd[0]);  /* cleanup */
        /* exec tr */
        execve(nameB, argvB, 0);
        perror(nameB);
        exit(125);

	}
}
// VOID

//void xxprint(char **argv,int argc) {
//	char line[13];
//	long lineno = 0;
//	int c, except = 0, number = 0, found = 0;
//	while (--argc > 0 && (*++argv)[0] == '-')
//		while (c = *++argv[0])
//			switch (c) {
//			case 'x':
//				printf("find: 1 %c\n", c);
//						 except = 1;
//	                   break;
//	               case 'n':
//	            	   printf("find: 2 %c\n", c);
//	                   number = 1;
//	                   break;
//	               default:
//	                   printf("find: illegal option %c\n", c);
//	                   argc = 0;
//	                   found = -1;
//	                   break;
//	               }
//}
