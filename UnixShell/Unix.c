#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wchar.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_COMMANDS 10
#define MAX_CHARS 1024

void parseInput(char *pointerDelim, char* array[MAX_COMMANDS]);
void decision(char *commands[MAX_COMMANDS]);
void copyCommands(char *history[MAX_COMMANDS], char *commands[MAX_COMMANDS]);
int hasRedirect(char *commands[MAX_COMMANDS]);
int indexOfFile(char *commands[MAX_COMMANDS]);
void handleRedirect(char *commands[MAX_COMMANDS],int type);
int indexOfFile(char *commands[MAX_COMMANDS]);
int hasPipe(char *commands[MAX_COMMANDS]);
void parsePipe(char *commands[MAX_COMMANDS], char *parsed[MAX_COMMANDS],int start, int end);
void handlePipe(char *commands[MAX_COMMANDS], int pipeIndx);
int hasHistory = 0;
/*
 * parseInput takes the user input and stores the input into an array separated by space.
 */
void parseInput(char *pointerDelim, char* array[10]){

	int i = 0;

	while(pointerDelim != NULL){
		array[i] = pointerDelim;
		i++;
		pointerDelim = strtok(NULL, " \n");
	}
	array[i] = NULL;
}

void handleRedirect(char *commands[MAX_COMMANDS],int redirect){
	pid_t childpid;

	childpid = fork();

	char *args[MAX_COMMANDS];

	if(childpid<0){
		printf("Failed to create child pid: redirect.");
	}
	else if(childpid == 0){

		int fd, indexRed;

		printf("redirect: %d\n",redirect);
		if(redirect == 0){ //IN
			indexRed = indexOfFile(commands);

			if(indexRed>-1){
				commands[indexRed] = NULL;
			}
			else{
				perror("No file found: redirect.");
			}
			fd = open(commands[indexRed+1],O_RDONLY);
			if(fd <0){
				perror("Cannot open file: redirect.");
			}
			dup2(fd,0);
			close(fd);

		}
		else if(redirect == 1){ //OUT
			indexRed = indexOfFile(commands);

			if(indexRed >-1){
				commands[indexRed] = NULL;
			}
			else{
				perror("No file found: redirect.");
			}

			fd = open(commands[indexRed+1],O_CREAT|O_WRONLY|O_TRUNC);
			if(fd <0){
				perror("Cannot open file: redirect.");
			}
			dup2(fd,1);
			close(fd);

		}

		if(execvp(commands[0],commands) == -1){
			perror("Command does not exist");
		}
		//exit(0);
	}
	else{
			waitpid(childpid,NULL,0);
	}
}

void decision(char * commands[MAX_COMMANDS]){
	pid_t childpid;

	childpid = fork();


	if(childpid<0){
		printf("Failed to create child pid");
	}
	else if(childpid == 0){

		printf("child");

		if(execvp(commands[0],commands) == -1){
			perror("Command does not exist");
		}
		perror("error execvp decision");
	}
	else{
		waitpid(childpid,NULL,0);
	}
}

void copyCommands(char *history[MAX_COMMANDS], char *commands[MAX_COMMANDS]){
	int i = 0;
	char *temp = commands[i];

	while(temp!=NULL){

		//free(history[i]);
		//memcpy(history[i],commands[i],sizeof(char *));

		history[i] = temp;
		i++;
		temp = commands[i];
	}
	history[i] = temp;
}

int indexOfFile(char *commands[MAX_COMMANDS]){
	int i = 0;
	char *temp = commands[i];
	while(temp!=NULL){
		if(strcmp(temp,">")==0 || strcmp(temp,"<")==0){
			return i;
		}

		i++;
		temp = commands[i];
	}
	return -1;
}

int hasRedirect(char *commands[MAX_COMMANDS]){
	int i = 0;
	char *temp = commands[i];
	while(temp!=NULL){
		if(strcmp(temp,">")==0){
			return 1;
		}
		else if(strcmp(temp,"<")==0){
			return 0;
		}
		i++;
		temp = commands[i];
	}
	return -1;
}

int hasPipe(char *commands[MAX_COMMANDS]){
	int i = 0;
	char *temp = commands[i];
	while(temp != NULL){
		if(strcmp(temp,"|")==0){
			return i;
		}
		i++;
		temp = commands[i];
	}
	return -1;
}

void parsePipe(char *commands[MAX_COMMANDS],char *parsed[MAX_COMMANDS],int start,int end){
	int i;
	int j = 0;
	for(i = start; i<end; i++){
		if(commands[i] == NULL){
			parsed[j] = NULL;
			break;
		}
		parsed[j] = commands[i];
		j++;
	}
	parsed[j] = NULL;
}

void handlePipe(char *commands[MAX_COMMANDS],int pipeIndx){
	int fd[2];
	pid_t child1,child2;

	char *parsed1[MAX_COMMANDS];
	char *parsed2[MAX_COMMANDS];

	parsePipe(commands,parsed1,0,pipeIndx);
	parsePipe(commands,parsed2,pipeIndx+1,MAX_COMMANDS);

	if(pipe(fd) < 0){
		perror("Failed to pipe.");
	}
	child1 = fork();
	if(child1<0){
		perror("Failed to create first thread for pipe.");
	}
	if(child1 == 0){


		child2 = fork();
		if(child2 < 0){
			perror("Failed to create second pipe");
		}
		else if(child2 == 0){
			close(fd[1]);
			dup2(fd[0],0);
			close(fd[0]);
			if(execvp(parsed2[0],parsed2)<0){
				perror("could not execute command for pipe.");
			}
		}
		else{
			close(fd[0]);
			dup2(fd[1],1);
			if (execvp(parsed1[0], parsed1) < 0) {
			    perror("Could not execute command for pipe.");
			}
			close(fd[1]);
		}


	}
	else{
		close(fd[0]);
		close(fd[1]);
		waitpid(child2,NULL,0);
		waitpid(child1,NULL,0);
	}
}

int main(){
	char *input;
	char *his;//[MAX_CHARS];
	char *temp;
	char *history[MAX_COMMANDS];// = malloc(MAX_COMMANDS * sizeof(char *));
	//allocateHistory(history);
	//int hasHistory = 0;
	char *pDelim;

	char* pointerDelim;

	char* commands[MAX_COMMANDS];

	int should_run = 1;

	int flag = 0;

	while(should_run == 1){
		//free(commands);
		printf("osh>");
		fflush(stdout);
		//printf("OOOOOOOOOOOOF2");
		fgets(input,MAX_CHARS,stdin);
		//strcpy(his,input);
		//free(his);
		his = (char *)malloc(MAX_CHARS);
		if(flag == 0){
			//memcpy(his,input,MAX_CHARS);
			//strncpy(his,input,MAX_CHARS);
			strcpy(his,input);
//			pDelim = strtok(his," \n");
//			parseInput(pDelim,history);
		}
		printf("%s\n",his);
		pointerDelim = strtok(input, " \n");
		//printf("OOOOOOOOOOOOF1");
		parseInput(pointerDelim,commands);
		//printf("OOOOOOOOOOOOF");
		if(strcmp(commands[0],"exit") == 0 || strcmp(commands[0],"EXIT")==0){
			printf("Goodbye...\n");
			should_run = 0;
		}
		else if(strcmp(commands[0],"!!")==0){
			if(hasHistory == 0){
				printf("oof%s\n",his);
				printf("No commands in history.\n");
			}
			else{

				flag = 1;

				int redirect = hasRedirect(history);
				int pipe = hasPipe(history);

				if(redirect >= 0){
					handleRedirect(history,redirect);
				}
				else if(pipe >=0){
					handlePipe(history,pipe);
				}
				else{
					decision(history);
				}

			}
		}
		else{

			int redirect = hasRedirect(commands);
			int pipe = hasPipe(commands);

			if(redirect >= 0){
				handleRedirect(commands,redirect);
			}
			else if(pipe >=0){
				handlePipe(commands,pipe);
			}
			else{
				decision(commands);
			}
			flag = 0;
		}
		if(flag == 0){
			temp = (char *)malloc(MAX_CHARS);
			strcpy(temp,his);
			hasHistory = 1;
			pDelim = strtok(temp," \n");
			parseInput(pDelim,history);
		}
	}
	return 0;
}
