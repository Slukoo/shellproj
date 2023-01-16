#include<string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>



#include "global.h"

int debug = 0;
int original_ls = 0;
//char** wdir;

// This is the file that you should work on.

// declaration
int execute (struct cmd *cmd);

// name of the program, to be printed in several places
#define NAME "myshell"



// Some helpful functions

void errmsg (char *msg)
{
	fprintf(stderr,"error: %s\n",msg);
}







// apply_redirects() should modify the file descriptors for standard
// input/output/error (0/1/2) of the current process to the files
// whose names are given in cmd->input/output/error.
// append is like output but the file should be extended rather
// than overwritten.

void apply_redirects (struct cmd *cmd){
	if (cmd->output != NULL) {
		int fdesc = open(cmd->output, O_WRONLY | O_TRUNC | O_CREAT, 420);
		dup2(fdesc, 1);
	}
	if (cmd->append != NULL) {
		int fdesc = open(cmd->append, O_WRONLY | O_APPEND | O_CREAT, 420);
		dup2(fdesc, 1);
	}
	if (cmd->input != NULL) {
		int fdesc = open(cmd->input, O_RDONLY);
		dup2(fdesc, 0);
	}
	if (cmd->error != NULL) {
		int fdesc = open(cmd->error, O_WRONLY | O_TRUNC | O_CREAT, 420);
		dup2(fdesc, 2);
	}
}

// Functions for handling cd, cat and ls:

int cd(struct cmd *cmd){
	 		int in = dup(0);
			int out = dup(1);
			int err = dup(2);
			apply_redirects(cmd);
				if(cmd->args[2] != NULL) {
					fprintf(stderr,"cd : too many arguments\n");
					return -1;
				}
				char* dir = cmd->args[1];
				if(dir == NULL) {	//redirects to ~ if no argument is given
					dir = getenv("HOME");
				}
				if(chdir(dir)) { //error while trying to change directory
					fprintf(stderr, "cd : unknown directory %s\n", cmd->args[1]);
					return -1;
				}
				//wdir = &dir;
				fprintf(stdout,"changed directory to %s\n", dir);

			dup2(in,0);
			dup2(out,1);
			dup2(err,2);
			return 0;

}



int cat(struct cmd *cmd){
		
		if(cmd->args[1] == NULL){
			int pid = fork();
			if(!pid){
				char* prompt = "";
				while(1){
					char *line = readline(prompt);
					if (!line) break;
					if (!*line) continue;
					int in = dup(0);
					int out = dup(1);
					int err = dup(2);
					apply_redirects(cmd);
					fprintf(stdout,"%s\n",line);
					free(line);
					dup2(in,0);
					dup2(out,1);
					dup2(err,2);
				}
				
				exit(0);
			}
			wait(NULL);
			return 0;
			}
			
		
		else{
			int in = dup(0);
			int out = dup(1);
			int err = dup(2);
			apply_redirects(cmd);
		
			if(cmd->args[2] != NULL) {
			fprintf(stderr,"cat: too many arguments\n");
			return -1;
		}
		char* filename = cmd->args[1];
		if(filename == NULL) {
			return -1;
		}
		FILE* file = fopen(filename,"r");
		if(file == NULL){
			fprintf(stderr,"cat: can't open file %s\n", filename);
			return -1;
		}
		char ch = getc(file);
		while(ch != EOF){
			fprintf(stdout,"%c",ch);
			ch = getc(file);
		}
		fprintf(stdout,"\n");
		fclose(file);
		dup2(in,0);
		dup2(out,1);
		dup2(err,2);
		return 0;
		}
}


int ls(struct cmd* cmd){
	int in = dup(0);
	int out = dup(1);
	int err = dup(2);
	apply_redirects(cmd);
	char* dir;
	int n;
	struct dirent** namelist;
	int exclude_hidden(const struct dirent* dirent){
		return (dirent->d_name[0] != '.');
	}
	if(cmd->args[1] == NULL){				//ls
		dir = ".";
		n = scandir(dir,&namelist,exclude_hidden,alphasort);
	}
	else{if(cmd->args[2] == NULL){
		if(strcmp(cmd->args[1],"-a")){		//ls directory
			dir = cmd->args[1];
			n = scandir(dir,&namelist,exclude_hidden,alphasort);
		}
		else{								//ls -a
			dir = ".";
			n = scandir(dir,&namelist,NULL,alphasort);
		}
	}
	else{if(cmd->args[3] == NULL){
		if(!(strcmp(cmd->args[1],"-a"))){	//ls -a directory
			dir = cmd->args[2];
			n = scandir(dir,&namelist,NULL,alphasort);
		}
		else{
			if (cmd->args[1][0] == '-')
			{
				fprintf(stderr,"ls: only valid option is -a (switch to original ls command by typing \"original_ls\"\n");
			}
			else{
				fprintf(stderr,"ls: too many arguments\n");
			}
			return -1;
		}
	}
	else{
		fprintf(stderr,"ls: too many arguments\n");
		return -1;
	}}}
	if(n<0){
		fprintf(stderr,"ls: unknown directory %s\n",dir);
		return -1;
		
	}
	while(n){
		n = n-1;
		fprintf(stdout,"%s ",namelist[n]->d_name);
		free(namelist[n]);
	}
	free(namelist);
	fprintf(stdout,"\n");
	dup2(in,0);
	dup2(out,1);
	dup2(err,2);
	return 0;

}


// The function execute() takes a command parsed at the command line.
// The structure of the command is explained in output.c.
// Returns the exit code of the command in question.

void execution (struct cmd *cmd, int* status){
		if (cmd->type == C_PLAIN) {
			if(debug){
				printf("executing command %s \n", cmd->args[0]);
			}
			if (!(strcmp(cmd->args[0], "cd"))){
				*status = cd(cmd);
			}
			else{ if (!(strcmp(cmd->args[0], "cat"))){
					*status = cat(cmd);
				}
			
			else{ if (!original_ls && !(strcmp(cmd->args[0], "ls"))){
					*status = ls(cmd);
				
			}
			else{
				int pid = fork();
				if (pid == 0) {
					apply_redirects(cmd);
					execvp(cmd->args[0],cmd->args);
					exit(-1);
				}
			}
			}
			}
		}
		else{
		*status = execute(cmd);
		}
		wait(status);
}



int execute (struct cmd *cmd)
{
	//output(cmd,0);
	int status;
	switch (cmd->type)
	{
	    case C_PLAIN: 
			execution(cmd,&status);
			return status;
		
	    case C_SEQ: 
			execution(cmd->left,&status);
			execution(cmd->right,&status);
			return status;
	    case C_AND:
			execution(cmd->left,&status);
			if(!status){
				execution(cmd->right,&status);
			}
			return status;
	    case C_OR:
			execution(cmd->left,&status);
			if(status){
				execution(cmd->right,&status);
			}
			return status;
	    case C_PIPE:
			int p[2];
			pipe(p);
			int pid = fork();
			if(pid){
				close(p[1]);
				wait(NULL);
				int pid2 = fork();
				if (pid2){
					close(p[0]);
					wait(&status);
					return status;
				}
				else{
					dup2(p[0], 0);
					exit(execute(cmd->right));
				}
			}
			else{
				dup2(p[1], 1);
				exit(execute(cmd->left));
			}
			return 0;
	    case C_VOID:
			int in = dup(0);
			int out = dup(1);
			int err = dup(2);
			apply_redirects(cmd);
			execute(cmd->left);
			dup2(in,0);
			dup2(out,1);
			dup2(err,2);
			return status;



		errmsg("I do not know how to do this, please help me!");
		return -1;
	}

	// Just to satisfy the compiler
	errmsg("This cannot happen!");
	return -1;
		
}

int main (int argc, char **argv)
{
	char* prompt = malloc(strlen(NAME)+3);
	//char* buf = getcwd(NULL,0);
	//wdir = &buf;
	printf("welcome to %s!\n", NAME);
	sprintf(prompt,"%s> ", NAME);
	if (argc > 1){
		if (!strcmp(argv[1],"debug")){
			debug = 1;
		}
	}
	while (1)
	{
		signal(SIGINT,SIG_IGN);
		//sprintf(prompt,"%s: %s> ", NAME, *wdir);
		char *line = readline(prompt);
		if (!line) break;	// user pressed Ctrl+D; quit shell
		if(!strcmp(line,"debug")){
			printf("enabling debug mode\n");
			debug = 1;
			continue;
		}
		if(!strcmp(line,"debugstop")){
			printf("disabling debug mode\n");
			debug = 0;
			continue;
		}
		if(!strcmp(line,"original_ls")){
			printf("switching to original ls command\n");
			original_ls = 1;
			continue;
		}
		if(!strcmp(line,"own_ls")){
			printf("switching to reimplemented ls command (only -a option is working)\n");
			original_ls = 0;
			continue;
		}
		if (!*line) continue;	// empty line

		add_history (line);	// add line to history

		struct cmd *cmd = parser(line);
		if (!cmd) continue;	// some parse error occurred; ignore
		if(debug){
			output(cmd,0);
		}	
		execute(cmd);
	}
	printf("goodbye!\n");
	return 0;
}
