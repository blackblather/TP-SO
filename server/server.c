#include <stdio.h>	//Used for scanf() and stuff
#include <string.h>	//Used for strcmp() and strtok()
#include <errno.h>	//Used for error handling in strtol()
#include <stdlib.h>	//Used for strtol()
#include <unistd.h>	//Used for access()
#include "server-defaults.h"
#include "../medit-defaults.h"
void FlushStdin(){
	//Source: https://stackoverflow.com/questions/2187474/i-am-not-able-to-flush-stdin
	//"Remove" os chars do buffer até chegar ao '\n' ou End Of File (EOF)
	int f;
	while((f = fgetc(stdin)) != '\n' && f != EOF);
}

int FileExists(char* filename){
	if(access( filename, F_OK ) != -1)
		return 1;
	return 0;
}

void LoadFile(char* filename){
	if(!FileExists(filename))
		printf("File not found.\n");
	else
		printf("Loaded file: %s\n", filename);
}

void SaveFile(char* filename){
	char replace = 'y';
	if(FileExists(filename)){
		printf("File %s already exists. Do you want to replace it? [Y/n] ", filename);
		scanf("%c", &replace);
	}
	if(replace == 'y' || replace == 'Y')
		printf("Data saved in file: %s\n", filename); //FOPEN(filename, 'W'); and do stuff to save on the file
	else if(replace != 'n' && replace != 'N')
		printf("Invalid command.\n");
}

void FreeLine(char* nrStr, int maxLines){
	//Source: https://en.cppreference.com/w/c/string/byte/strtol
	//Helper source: https://stackoverflow.com/questions/11279767/how-do-i-make-sure-that-strtol-have-returned-successfully
	char* end;
	int lineNumber = (int)strtol(nrStr, &end, 10);
	if (errno == ERANGE || end == nrStr){
		lineNumber = -1;
        errno = 0;
    }
    if((lineNumber >= 0) && (lineNumber <= maxLines))
    	printf("Line %d freed.\n", lineNumber);
    else
    	printf("Invalid line.\n");
}

void Settings(int maxLines, int maxColumns){
	printf("Max lines: %d\n", maxLines);
	printf("Max columns: %d\n", maxColumns);
}

void ProcessCommand(char* cmd, int* shutdown){
	char *cmdPart1, *cmdPart2;
	int maxLines = MEDIT_MAXLINES, maxColumns = MEDIT_MAXCOLUMNS;
	if((cmdPart1 = strtok(cmd, " ")) != NULL){
		if(strcmp(cmdPart1, "load")==0 || strcmp(cmdPart1, "save")==0 || strcmp(cmdPart1, "free")==0){
			if((cmdPart2 = strtok(NULL, " ")) != NULL){
				if(strcmp(cmdPart1, "load")==0) LoadFile(cmdPart2);
				else if(strcmp(cmdPart1, "save")==0) SaveFile(cmdPart2);
				else if(strcmp(cmdPart1, "free")==0) FreeLine(cmdPart2, maxLines);
			}
			else
				printf("Missing second parameter.\n");
		}
		else{
			if(strcmp(cmdPart1, "settings")==0) Settings(maxLines, maxColumns);
			else if(strcmp(cmdPart1, "statistics")==0){}
			else if(strcmp(cmdPart1, "users")==0){}
			else if(strcmp(cmdPart1, "text")==0){}
			else if(strcmp(cmdPart1, "shutdown")==0) *shutdown = 1;
			else
				printf("Invalid command.\n");
		}
	}
}

int main(int argc, char const *argv[])
{
	//scanf(" %14[^ \n]%*[^ \n]%*[ \n]%255[^\n]",cmdPart1, cmdPart2);
	//printf("CMD1->%s\nCMD2->%s\n", cmdPart1, cmdPart2);
	char cmd[300] = {'\0'};
	int shutdown = 0;
	do{
		printf("Command: ");
		scanf(" %299[^\n]", cmd);
		FlushStdin();
		ProcessCommand(cmd, &shutdown);
	}while(shutdown == 0);
	//DO SHUTDOWN LOGIC HERE
	return 0;
}