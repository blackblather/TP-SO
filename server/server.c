#include <stdio.h>		//Used for scanf() and stuff
#include <string.h>		//Used for strcmp() and strtok()
#include <errno.h>		//Used for error handling in strtol()
#include <stdlib.h>		//Used for strtol()
#include <unistd.h>		//Used for access() and getopt()
#include <ncurses.h>	//Used for ncurses, duuuh
#include <sys/types.h>	//Used for mkfifo
#include <sys/stat.h>	//Used for mkfifo
#include <pthread.h>	//Used for creating threads
#include <sys/types.h>	//Used for open()
#include <sys/stat.h>	//Used for open()
#include <fcntl.h>		//Used for open()
#include "server-defaults.h"

int ServerIsRunningOnNamedPipe(char* mainNamedPipeName){
	if(access(mainNamedPipeName, F_OK) == 0)
		return 1;
	return 0;
}

WINDOW *create_newwin(int height, int width, int starty, int startx){	//Source: http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/windows.html#LETBEWINDOW
	WINDOW* window;

	window = newwin(height, width, starty, startx);
	box(window, '|' , '-');		/* 0, 0 gives default characters 
					 * for the vertical and horizontal
					 * lines			*/
	wborder(window, '|', '|', '-', '-', '+', '+', '+', '+');
	//wrefresh(window);		/* Show that box 		*/

	return window;
}

void RefresAllWindows(WINDOW* window[], int totalWindows){
	for(int i = 0; i < totalWindows; i++)
		wrefresh(window[i]);
}

void InitWindows(WINDOW* window[]){
	window[0] = create_newwin(12,50,0,0);	//Output Window
	mvwprintw(window[0], 1, 18, "Command Result:");
	window[1] = create_newwin(11,80,11,0);	//Thread Events Window
	mvwprintw(window[1], 1, 33, "Thread Events:");
	window[2] = create_newwin(12,31,0,49);	//Statistics Window
	mvwprintw(window[2], 1, 10, "Statistics:");
	window[3] = create_newwin(3,80,21,0);	//Command Window
	RefresAllWindows(window, 4);
}

void ClearWindow(WINDOW* window){
	werase(window);
	wborder(window, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(window);
}

//NOT NEEDED IN META2, NCURSES FLUSHES THE BUFFER ON ITS OWN
/*void FlushStdin(){
	//Source: https://stackoverflow.com/questions/2187474/i-am-not-able-to-flush-stdin
	//"Remove" os chars do buffer até chegar ao '\n' ou End Of File (EOF)
	int f;
	while((f = fgetc(stdin)) != '\n' && f != EOF);
}*/

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
		printw("File not found.\n");
	else
		printw("Loaded file: %s\n", filename);
}

//TODO: Actually save file (só valida os parametros)
void SaveFile(char* filename){
	char replace = 'y';
	if(FileExists(filename)){
		printw("File %s already exists. Do you want to replace it? [y/n] ", filename);
		scanf("%c", &replace);
	}
	if(replace == 'y' || replace == 'Y')
		printw("Data saved in file: %s\n", filename); //FOPEN(filename, 'W'); and do stuff to save on the file
	else if(replace != 'n' && replace != 'N')
		printw("Invalid command.\n");
}

//TOD: Acutally free line (só valida os parametros)
void FreeLine(char* nrStr, int maxLines){
	//Source: https://en.cppreference.com/w/c/string/byte/strtol
	//Helper source: https://stackoverflow.com/questions/11279767/how-do-i-make-sure-that-strtol-have-returned-successfully
	int lineNumber = StrToPositiveInt(nrStr);
    if((lineNumber >= 0) && (lineNumber <= maxLines-1))
    	printw("Line %d freed.\n", lineNumber);
    else
    	printw("Invalid line.\n");
}

void PrintSettings(WINDOW* outputWindow,int maxLines, int maxColumns, char* dbFilename, int maxUsers, int timeout, int nrOfInteractionNamedPipes, char* mainNamedPipeName){
	mvwprintw(outputWindow,2,1,">Max lines: %d", maxLines);
	mvwprintw(outputWindow,3,1,">Max columns: %d", maxColumns);
	mvwprintw(outputWindow,4,1,">Database filename: %s", dbFilename);
	mvwprintw(outputWindow,5,1,">Max users: %d", maxUsers);
	mvwprintw(outputWindow,6,1,">Timeout (seconds): %d", timeout);
	mvwprintw(outputWindow,7,1,">Number of interaction named pipes: %d", nrOfInteractionNamedPipes);
	mvwprintw(outputWindow,8,1,">Main named pipe name: %s", mainNamedPipeName);
	wrefresh(outputWindow);
}

int UserExists(char *filename, char *username){
	//Source: https://www.tutorialspoint.com/c_standard_library/c_function_fscanf.htm
	if(FileExists(filename)){
		FILE *fp;
		char tmpUser[9];

		fp = fopen(filename, "r");
		if (fp != NULL){
			while(fscanf(fp, "%8[^ \n]", tmpUser))			
				if(strcmp(tmpUser, username)==0) {
					fclose(fp);
					return 1;
				}
			fclose(fp);
		}
	}
	return 0;
}

void ProcessCommand(char* cmd, int* shutdown, CommonSettings commonSettings, ServerSettings serverSettings, WINDOW* outputWindow){
	ClearWindow(outputWindow);
	mvwprintw(outputWindow, 1, 18, "Command Result:");
	char *cmdPart1, *cmdPart2;

	if((cmdPart1 = strtok(cmd, " ")) != NULL){
		if(strcmp(cmdPart1, "load")==0 || strcmp(cmdPart1, "save")==0 || strcmp(cmdPart1, "free")==0){
			if((cmdPart2 = strtok(NULL, " ")) != NULL){
				if(strcmp(cmdPart1, "load")==0) LoadFile(cmdPart2);
				else if(strcmp(cmdPart1, "save")==0) SaveFile(cmdPart2);
				else if(strcmp(cmdPart1, "free")==0) FreeLine(cmdPart2, commonSettings.maxLines);
			}
			else
				printw("Missing second parameter.\n");
		}
		else{
			if(strcmp(cmdPart1, "settings")==0) PrintSettings(outputWindow, commonSettings.maxLines, commonSettings.maxColumns, serverSettings.dbFilename, serverSettings.maxUsers, serverSettings.timeout, serverSettings.nrOfInteractionNamedPipes, commonSettings.mainNamedPipeName);
			else if(strcmp(cmdPart1, "userExists")==0) {
				//FOR TEST PURPOSES ONLY
				if(UserExists("medit.db", "user1"))
					printw("Encontrou o utilizador\n");
				else
					printw("Não encontrou o utilizador\n");
			}
			else if(strcmp(cmdPart1, "statistics")==0){}
			else if(strcmp(cmdPart1, "users")==0){}
			else if(strcmp(cmdPart1, "text")==0){}
			else if(strcmp(cmdPart1, "shutdown")==0) *shutdown = 1;
			else
				mvwprintw(outputWindow, 2, 1, ">Invalid command.");
		}
	}
	wrefresh(outputWindow);
}

void AllocScreenMemory(Screen *screen, CommonSettings commonSettings){
	(*screen).line = (Line*) malloc(commonSettings.maxLines*sizeof(Line));
	for(int i = 0; i < commonSettings.maxLines; i++){
		(*screen).line[i].lineNumber = i;
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

void InitFromEnvs(int* maxLines, int* maxColumns, int* maxUsers, int* timeout){
	char* envVal;
	if((envVal = getenv("MEDIT_MAXLINES")) != NULL) UpdateMaxLines(maxLines, envVal);
	if((envVal = getenv("MEDIT_MAXCOLUMNS")) != NULL) UpdateMaxColumns(maxColumns, envVal);
	if((envVal = getenv("MEDIT_MAXUSERS")) != NULL) UpdateMaxUsers(maxUsers, envVal);
	if((envVal = getenv("MEDIT_TIMEOUT")) != NULL) UpdateTimeout(timeout, envVal);
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

void FreeAllocatedMemory(Screen *screen, CommonSettings commonSettings){
	for(int i = 0; i < commonSettings.maxLines; i++)
		free((*screen).line[i].column);
	free((*screen).line);
}

void* MainNamedPipeThread(void* tArgs){
	ThreadMainNamedPipeArgs* args;
	args = (ThreadMainNamedPipeArgs*) tArgs;
	mkfifo(args->mainNamedPipeName, 0600);
	mvwprintw(args->threadEventsWindow,2,1, "Main namedpipe created.");
	wrefresh(args->threadEventsWindow);
	mvwprintw(args->threadEventsWindow, 3,1, "Waiting for clients...");
	wrefresh(args->threadEventsWindow);
	open(args->mainNamedPipeName, O_RDONLY);
	mvwprintw(args->threadEventsWindow, 3,1, "yolo i guess");
	/*while(1){

	}*/
}

void StartMainNamedPipeThread(WINDOW* threadEventsWindow, char* mainNamedPipeName){
	ThreadMainNamedPipeArgs args;
	args.threadEventsWindow = threadEventsWindow;
	args.mainNamedPipeName = mainNamedPipeName;

	pthread_t thread;

	int rc = pthread_create(&thread, NULL, MainNamedPipeThread, (void *)&args);
  	if (rc){
    	printf("ERROR; return code from pthread_create() is %d\n", rc);
    	exit(-1);
  	}
  	//pthread_exit(NULL);
}

int main(int argc, char* const argv[]){
	/* 
	 * Algumas funções têm muitos parametros, para evitar enviar o '&' de uma struct,
	 * quando esta nao precisa de aceder a todos os itens da struct. Mesmo sendo sendo eu
	 * quem fez a função, perfiro que a função nao tenha acesso a esses dados
	 */
	
	//TODO: Tratar aqui dos sinais:
	// -> Ctrl+C apaga os named pipes e isso
	// -> O sinal quando o servidor feixa (enviar msg aos clientes)

	char cmd[300] = {'\0'};
	int shutdown = 0;

	CommonSettings commonSettings;
	ServerSettings serverSettings;
	Screen screen;					//Exemplo de utilização: screen.line[0].column[0] = 'F';
	Line *occupiedLine;				//Linha(s) em edição	(UNUSED NA META 1)

	InitSettingsStructs(&commonSettings, &serverSettings);
	InitFromEnvs(&commonSettings.maxLines, &commonSettings.maxColumns, &serverSettings.maxUsers ,&serverSettings.timeout);
	InitFromOpts(argc, argv, &serverSettings.dbFilename, &commonSettings.mainNamedPipeName, &serverSettings.nrOfInteractionNamedPipes);
	
	if(ServerIsRunningOnNamedPipe(commonSettings.mainNamedPipeName) == 0){
		AllocScreenMemory(&screen, commonSettings);

		//Initialize ncurses
		initscr();
		refresh();
		WINDOW* window[4];
		InitWindows(window);
		
		StartMainNamedPipeThread(window[1], commonSettings.mainNamedPipeName);

		do{
			mvwprintw(window[3], 1, 1, "Command: ");
			wscanw(window[3], " %299[^\n]", cmd);//---> O scanw já faz wrefresh
			//ncurses flushes the "input buffer" automatically
			//FlushStdin();
			ProcessCommand(cmd, &shutdown, commonSettings, serverSettings, window[0]);
			ClearWindow(window[3]);
		}while(shutdown == 0);

		//Shutdown logic here
		unlink(commonSettings.mainNamedPipeName);
		endwin();
		FreeAllocatedMemory(&screen, commonSettings);
	} else
	printf("Found another server running on namedpipe: %s\nExiting...\n", commonSettings.mainNamedPipeName);
	return 0;
}