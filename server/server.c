#include <stdio.h>	//Used for scanf() and stuff
#include <string.h>	//Used for strcmp() and strtok()
#include <errno.h>	//Used for error handling in strtol()
#include <stdlib.h>	//Used for strtol()
#include <unistd.h>	//Used for access() and getopt()
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

//TODO: Actually load file (só valida os parametros)
void LoadFile(char* filename){
	if(!FileExists(filename))
		printf("File not found.\n");
	else
		printf("Loaded file: %s\n", filename);
}

//TODO: Actually save file (só valida os parametros)
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

//TOD: Acutally free line (só valida os parametros)
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

void Settings(int maxLines, int maxColumns, char* dbFilename, int maxUsers, int nrOfInteractionNamedPipes, char* mainNamedPipeName){
	printf("Max lines: %d\n", maxLines);
	printf("Max columns: %d\n", maxColumns);
	printf("Database filename: %s\n", dbFilename);
	printf("Max users: %d\n", maxUsers);
	printf("Number of interaction named pipes: %d\n", nrOfInteractionNamedPipes);
	printf("Main named pipe name: %s\n", mainNamedPipeName);
}

int UserExists(char *filename, char *username){
	if(FileExists(filename)){
		FILE *fp;
		char tmpUser[9];

		fp = fopen(filename, "r");
		if (fp != NULL){
			while(fgets(tmpUser, 8, fp) != NULL)
				if(strcmp(tmpUser, username)==0) {
					fclose(fp);
					return 1;
				}
			fclose(fp);
		}
	}
	return 0;
}

void ProcessCommand(char* cmd, int* shutdown, int maxLines, int maxColumns, char* dbFilename){
	char *cmdPart1, *cmdPart2,
		 *mainNamedPipeName = MEDIT_MAIN_NAMED_PIPE_NAME;

	int maxUsers = MEDIT_MAXUSERS,
		nrOfInteractionNamedPipes = MEDIT_NR_OF_INTERACTION_NAMED_PIPES;

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
			if(strcmp(cmdPart1, "settings")==0) Settings(maxLines, maxColumns, dbFilename, maxUsers, nrOfInteractionNamedPipes, mainNamedPipeName);
			else if(strcmp(cmdPart1, "userExists")==0) {
				if(UserExists("medit.db", "user1"))
					printf("Encontrou o utilizador\n");
				else
					printf("Não encontrou o utilizador\n");
			}
			else if(strcmp(cmdPart1, "statistics")==0){}
			else if(strcmp(cmdPart1, "users")==0){}
			else if(strcmp(cmdPart1, "text")==0){}
			else if(strcmp(cmdPart1, "shutdown")==0) *shutdown = 1;
			else
				printf("Invalid command.\n");
		}
	}
}

void AllocScreenMemory(Screen *screen, int maxLines, int maxColumns){
	(*screen).line = (Line*) malloc(maxLines*sizeof(Line));
	for(int i = 0; i < maxLines; i++){
		(*screen).line[i].column = (char*) malloc(maxColumns*sizeof(char));
		(*screen).line[i].username = NULL;	//Limpa o lixo que vem por default quando se cria um ponteiro.
	}
}

void InitFromOpts(int argc, char* const argv[], char *dbFilename){
	//Source: https://www.gnu.org/software/libc/manual/html_node/Getopt.html
	int c;
	while ((c = getopt (argc, argv, "f:p:n:")) != -1){
	    switch (c){
	      case 'f': break;
	      case 'p':
	        //cvalue = optarg;
	        break;
	      case 'n': break;
	      case '?':
	        /*if (optopt == 'c')
	          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
	        else if (isprint (optopt))
	          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
	        else
	          fprintf (stderr,
	                   "Unknown option character `\\x%x'.\n",
	                   optopt);*/
	        break;
	      default:
	        abort ();
	    }
	}
}

void InitFromEnvs(Screen *screen, int *maxLines, int *maxColumns){
	//Le var de ambiente aqui
	AllocScreenMemory(screen, (*maxLines), (*maxColumns));
}

void FreeAllocatedMemory(Screen *screen, int maxLines, int maxColumns){
	for(int i = 0; i < maxLines; i++)
		free((*screen).line[i].column);
	free((*screen).line);
	//free(OccupiedLine);
}

int main(int argc, char* const argv[]){
	//scanf(" %14[^ \n]%*[^ \n]%*[ \n]%255[^\n]",cmdPart1, cmdPart2);
	//printf("CMD1->%s\nCMD2->%s\n", cmdPart1, cmdPart2);
	
	char cmd[300] = {'\0'},
		 *dbFilename = MEDIT_DB_NAME;
	int maxLines = MEDIT_MAXLINES,
		maxColumns = MEDIT_MAXCOLUMNS;
	Screen screen;		//Exemplo de utilização: screen.line[0].column[0] = 'F';
	Line *OccupiedLine;	//Linha(s) em edição

	InitFromOpts(argc, argv, dbFilename);
	InitFromEnvs(&screen, &maxLines, &maxColumns);
	int shutdown = 0;
	do{
		printf("Command: ");
		scanf(" %299[^\n]", cmd);
		FlushStdin();
		ProcessCommand(cmd, &shutdown, maxLines, maxColumns, dbFilename);
	}while(shutdown == 0);
	//DO SHUTDOWN LOGIC HERE
	FreeAllocatedMemory(&screen, maxLines, maxColumns);
	return 0;
}