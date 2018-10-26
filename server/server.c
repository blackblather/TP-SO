#include <stdio.h>	//Used for scanf() and stuff
#include <string.h>	//Used for strcmp() and strtok()
#include <errno.h>	//Used for error handling in strtol()
#include <stdlib.h>	//Used for strtol()
#include <unistd.h>	//Used for access() and getopt()
#include "server-defaults.h"
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

int StrToPositiveInt(char* nrStr){
	char* end;
	int nr = (int)strtol(nrStr, &end, 10);
	if (errno == ERANGE || end == nrStr){
		nr = -1;
        errno = 0;
    }
    return nr;
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
	int lineNumber = StrToPositiveInt(nrStr);
    if((lineNumber >= 0) && (lineNumber <= maxLines))
    	printf("Line %d freed.\n", lineNumber);
    else
    	printf("Invalid line.\n");
}

void PrintSettings(int maxLines, int maxColumns, char* dbFilename, int maxUsers, int timeout, int nrOfInteractionNamedPipes, char* mainNamedPipeName){
	printf("Max lines: %d\n", maxLines);
	printf("Max columns: %d\n", maxColumns);
	printf("Database filename: %s\n", dbFilename);
	printf("Max users: %d\n", maxUsers);
	printf("Timeout (seconds): %d\n", timeout);
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

void ProcessCommand(char* cmd, int* shutdown, CommonSettings commonSettings, ServerSettings serverSettings){
	char *cmdPart1, *cmdPart2;

	if((cmdPart1 = strtok(cmd, " ")) != NULL){
		if(strcmp(cmdPart1, "load")==0 || strcmp(cmdPart1, "save")==0 || strcmp(cmdPart1, "free")==0){
			if((cmdPart2 = strtok(NULL, " ")) != NULL){
				if(strcmp(cmdPart1, "load")==0) LoadFile(cmdPart2);
				else if(strcmp(cmdPart1, "save")==0) SaveFile(cmdPart2);
				else if(strcmp(cmdPart1, "free")==0) FreeLine(cmdPart2, commonSettings.maxLines);
			}
			else
				printf("Missing second parameter.\n");
		}
		else{
			if(strcmp(cmdPart1, "settings")==0) PrintSettings(commonSettings.maxLines, commonSettings.maxColumns, serverSettings.dbFilename, serverSettings.maxUsers, serverSettings.timeout, serverSettings.nrOfInteractionNamedPipes, commonSettings.mainNamedPipeName);
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

void AllocScreenMemory(Screen *screen, CommonSettings commonSettings){
	(*screen).line = (Line*) malloc(commonSettings.maxLines*sizeof(Line));
	for(int i = 0; i < commonSettings.maxLines; i++){
		(*screen).line[i].column = (char*) malloc(commonSettings.maxColumns*sizeof(char));
		(*screen).line[i].username = NULL;	//Limpa o lixo que vem por default quando se cria um ponteiro.
	}
}

void UpdateDbFilename(char **dbFilename, char *newDbFilename){
	if(FileExists(newDbFilename))
		(*dbFilename) = newDbFilename;
}

void UpdateMainNamedPipeName(char **oldMainNamedPipeName, char *newdMainNamedPipeName){
	(*oldMainNamedPipeName) = newdMainNamedPipeName;
}

void UpdateNumberThatCanOnlyBePositive(int *oldNr, char *newNrStr){
	int newNrTmp = StrToPositiveInt(newNrStr);
	if(newNrTmp > 0)
		(*oldNr) = newNrTmp;
}

void UpdateNumberOfInteractiveNamedPipes(int *oldNrOfInteractionNamedPipes, char *newNrOfInteractionNamedPipesSTR){
	UpdateNumberThatCanOnlyBePositive(oldNrOfInteractionNamedPipes, newNrOfInteractionNamedPipesSTR);
}

void InitFromOpts(int argc, char* const argv[], char **dbFilename, char **mainNamedPipeName, int* nrOfInteractionNamedPipes){
	//Source: https://www.gnu.org/software/libc/manual/html_node/Getopt.html
	int c;
	while ((c = getopt(argc, argv, "f:p:n:")) != -1){
	    switch (c){
	      case 'f': UpdateDbFilename(dbFilename, optarg); break;
	      case 'p':	UpdateMainNamedPipeName(mainNamedPipeName, optarg); break;
	      case 'n': UpdateNumberOfInteractiveNamedPipes(nrOfInteractionNamedPipes, optarg); break;
	    }
	}
}

void UpdateMaxLines(int *oldMaxLines, char* newMaxLinesStr){
	UpdateNumberThatCanOnlyBePositive(oldMaxLines, newMaxLinesStr);
}

void UpdateMaxColumns(int *oldMaxColumns, char* newMaxColumnsStr){
	UpdateNumberThatCanOnlyBePositive(oldMaxColumns, newMaxColumnsStr);
}

void UpdateMaxUsers(int *oldMaxUsers, char* newMaxUsersStr){
	UpdateNumberThatCanOnlyBePositive(oldMaxUsers, newMaxUsersStr);
}

void UpdateTimeout(int *oldTimeout, char* newTimeoutStr){
	UpdateNumberThatCanOnlyBePositive(oldTimeout, newTimeoutStr);
}

void InitFromEnvs(int* maxLines, int* maxColumns, int* maxUsers, int* timeout){
	char* envVal;
	if((envVal = getenv("MEDIT_MAXLINES")) != NULL) UpdateMaxLines(maxLines, envVal);
	if((envVal = getenv("MEDIT_MAXCOLUMNS")) != NULL) UpdateMaxColumns(maxColumns, envVal);
	if((envVal = getenv("MEDIT_MAXUSERS")) != NULL) UpdateMaxUsers(maxUsers, envVal);
	if((envVal = getenv("MEDIT_TIMEOUT")) != NULL) UpdateTimeout(timeout, envVal);
}

void FreeAllocatedMemory(Screen *screen, CommonSettings commonSettings){
	for(int i = 0; i < commonSettings.maxLines; i++)
		free((*screen).line[i].column);
	free((*screen).line);
}

void InitCommonSettingsStruct(CommonSettings* commonSettings){
	(*commonSettings).maxLines = MEDIT_MAXLINES;
	(*commonSettings).maxColumns = MEDIT_MAXCOLUMNS;
	(*commonSettings).mainNamedPipeName = MEDIT_MAIN_NAMED_PIPE_NAME;
}

void InitServerSettingsStruct(ServerSettings* serverSettings){
	(*serverSettings).nrOfInteractionNamedPipes = MEDIT_NR_OF_INTERACTION_NAMED_PIPES;
	(*serverSettings).timeout = MEDIT_TIMEOUT;
	(*serverSettings).maxUsers = MEDIT_MAXUSERS;
	(*serverSettings).dbFilename = MEDIT_DB_NAME;
}

void InitSettingsStructs(CommonSettings* commonSettings, ServerSettings* serverSettings){
	InitCommonSettingsStruct(commonSettings);
	InitServerSettingsStruct(serverSettings);
}

int main(int argc, char* const argv[]){
	/* 
	 * Algumas funções tê muitos parametros, para evitar enviar o '&' de uma struct,
	 * quando esta nao precisa de aceder a todos os itens da struct. Mesmo sendo sendo eu
	 * quem fez a função, perfiro que a função nao tenha acesso a esses dados
	 */

	char cmd[300] = {'\0'};
	int shutdown = 0;

	CommonSettings commonSettings;
	ServerSettings serverSettings;
	Screen screen;					//Exemplo de utilização: screen.line[0].column[0] = 'F';
	Line *occupiedLine;				//Linha(s) em edição	(UNUSED IETA 1)

	InitSettingsStructs(&commonSettings, &serverSettings);
	InitFromOpts(argc, argv, &serverSettings.dbFilename, &commonSettings.mainNamedPipeName, &serverSettings.nrOfInteractionNamedPipes);
	InitFromEnvs(&commonSettings.maxLines, &commonSettings.maxColumns, &serverSettings.maxUsers ,&serverSettings.timeout);
	
	AllocScreenMemory(&screen, commonSettings);

	do{
		printf("Command: ");
		scanf(" %299[^\n]", cmd);
		FlushStdin();
		ProcessCommand(cmd, &shutdown, commonSettings, serverSettings);
	}while(shutdown == 0);

	//DO SHUTDOWN LOGIC HERE
	FreeAllocatedMemory(&screen, commonSettings);
	return 0;
}