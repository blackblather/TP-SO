#include <stdio.h>		//Used for scanf() and stuff
#include <string.h>		//Used for strcmp() and strtok()
#include <errno.h>		//Used for error handling in strtol()
#include <stdlib.h>		//Used for strtol()
#include <unistd.h>		//Used for access() and getopt()
#include <ncurses.h>	//Used for ncurses, duuuh
#include <sys/types.h>	//Used for mkfifo and open
#include <sys/stat.h>	//Used for mkfifo, open and stat
#include <pthread.h>	//Used for creating threads
#include <fcntl.h>		//Used for open()
#include <semaphore.h>	//Used for sem_open(), sem_wait() and sem_post()
#include <signal.h>		//Used for signal
#include "server-defaults.h"

CommonSettings commonSettings;	//Global por causa da função Shutdown(), usada ao chamar o SIGINT
ServerSettings serverSettings;	//Global por causa da função Shutdown(), usada ao chamar o SIGINT
Screen screen;					//Exemplo de utilização: screen.line[0].column[0] = 'F';
Line *occupiedLine;				//Linha(s) em edição	(UNUSED NA META 1)
MainNamedPipeThreadArgs mArgs;
InteractionNamedPipeThreadArgs iArgs;
ClientInfo* loggedInUsers;
int usersCount = 0;
pthread_mutex_t mutex_loggedInUsers = PTHREAD_MUTEX_INITIALIZER;
sem_t* interprocMutex;
/* VAR: serverSpecificInteractionNamedPipeDirName
 * tamanho do caminho (14) +
 * nr máximo de digitos que um inteiro pode ter (10) +
 * '/' (1)
 * '\0' (1) = 26*/
char serverSpecificInteractionNamedPipeDirName[26];
interactionNamedPipeInfo respServ;

void PrintLogo(){
	//Source: http://patorjk.com/software/taag/#p=display&h=1&v=2&f=Ogre&t=Medit%20Server%20
	//Font: ogre
	mvprintw(8,9,"--------------------------------------------------------------");
	mvprintw(9,9,"	            _  _  _     __                                ");
	mvprintw(10,9,"  /\\/\\    ___   __| |(_)| |_  / _\\  ___  _ _ __   __ ___  _ _  ");
	mvprintw(11,9," /    \\  / _ \\ / _` || || __| \\ \\  / _ \\| '_|\\ \\ / // _ \\| '_| ");
	mvprintw(12,9,"/ /\\/\\ \\|  __/| (_| || || |_  _\\ \\|  __/| |   \\ V /|  __/| |    ");
	mvprintw(13,9,"\\/    \\/ \\___| \\__,_||_| \\__| \\__/ \\___||_|    \\_/  \\___||_|    ");
	mvprintw(14,9,"--------------------------------------------------------------");
}

int DirectoryExists(char *path) {
	//Source #1: https://codeforwin.org/2018/03/c-program-check-file-or-directory-exists-not.html
	//Source #2: http://man7.org/linux/man-pages/man2/stat.2.html

    struct stat stats;

    stat(path, &stats);

    // Check for file existence
    if (S_ISDIR(stats.st_mode))
        return 1;

    return 0;
}

void InteractWithNamedPipe(int action, char* mainNamedPipeName, void* val, int size){
	int fd;
	if(action == O_RDONLY || action == O_WRONLY)
		if((fd = open(mainNamedPipeName, action)) >= 0){
			switch(action){
				case O_RDONLY: read(fd, val, size); break;
				case O_WRONLY: write(fd, val, size); break;
			}
			close(fd);
		}
}

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

//TODO: Acutally free line (só valida os parametros)
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

int UsernameHasValidLenght(char* username){
	int usernameLenght = strlen(username);
	if(usernameLenght > 0 && usernameLenght <= MEDIT_USERNAME_MAXLENGHT)
		return 1;
	return 0;
}

int UserExists(char *dbFilename, char *username){
	//Source: https://www.tutorialspoint.com/c_standard_library/c_function_fscanf.htm
	if(FileExists(dbFilename)){
		FILE* fp;
		char tmpUser[MEDIT_USERNAME_MAXLENGHT+1];
		fp = fopen(dbFilename, "r");
		if (fp != NULL){
			int fRet;
			do{
				memset(tmpUser, 0, sizeof(tmpUser));
				fRet = fscanf(fp, " %8[^ \n]", tmpUser);
				tmpUser[MEDIT_USERNAME_MAXLENGHT] = '\0';
				if(strcmp(tmpUser, username)==0) {
					fclose(fp);
					return 1;
				}
			}while(fRet > 0);
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
					mvwprintw(outputWindow, 2, 1, ">Encontrou o utilizador.");
				else
					mvwprintw(outputWindow, 2, 1, ">Não encontrou o utilizador.");
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

void AllocScreenMemory(){
	screen.line = (Line*) malloc(commonSettings.maxLines*sizeof(Line));
	for(int i = 0; i < commonSettings.maxLines; i++){
		screen.line[i].lineNumber = i;
		screen.line[i].column = (char*) malloc(commonSettings.maxColumns*sizeof(char));
		screen.line[i].username = NULL;	//Limpa o lixo que vem por default quando se cria um ponteiro.
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

void UpdateNumberOfInteractionNamedPipes(int *oldNrOfInteractionNamedPipes, char *newNrOfInteractionNamedPipesSTR){
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
	      case 'n': UpdateNumberOfInteractionNamedPipes(nrOfInteractionNamedPipes, optarg); break;
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

void InitCommonSettingsStruct(){
	commonSettings.maxLines = MEDIT_MAXLINES;
	commonSettings.maxColumns = MEDIT_MAXCOLUMNS;
	commonSettings.mainNamedPipeName = MEDIT_MAIN_NAMED_PIPE_NAME;
}

void InitServerSettingsStruct(){
	serverSettings.nrOfInteractionNamedPipes = MEDIT_NR_OF_INTERACTION_NAMED_PIPES;
	serverSettings.timeout = MEDIT_TIMEOUT;
	serverSettings.maxUsers = MEDIT_MAXUSERS;
	serverSettings.dbFilename = MEDIT_DB_NAME;
}

void InitSettingsStructs(){
	InitCommonSettingsStruct();
	InitServerSettingsStruct();
}

void InitEmptyLoggedInUsersArray(int maxUsers){
	loggedInUsers = (ClientInfo*) malloc(maxUsers*sizeof(ClientInfo));
	for(int i = 0; i < maxUsers; i++){
		memset(loggedInUsers[i].username, 0, sizeof(loggedInUsers[i].username));
		//loggedInUsers[i].PID = -1; //-> pid_t is an unsigned int, default value is 0, but im affrain 0 is a real PID
		loggedInUsers[i].isUsed = 0;
		loggedInUsers[i].interactionNamedPipeIndex = -1;
		loggedInUsers[i].clientNamedPipeIndex = -1;
	}
}

int UserIsLoggedIn(char* username, int maxUsers){
	for(int i = 0; i < maxUsers; i++)
		if(strcmp(loggedInUsers[i].username, username) == 0)
			return 1;
	return 0;
}

void LogUserInPOS(ClientInfo newClientInfo, int pos, int interactionNamedPipeIndex){
	strcpy(loggedInUsers[pos].username, newClientInfo.username);
	loggedInUsers[pos].PID = newClientInfo.PID;
	loggedInUsers[pos].isUsed = 1;
	loggedInUsers[pos].interactionNamedPipeIndex = interactionNamedPipeIndex;
}

int LogUserIn(ClientInfo newClientInfo, int maxUsers, int interactionNamedPipeIndex){
	for(int i = 0; i < maxUsers; i++)
		if(loggedInUsers[i].username[0] == 0 && loggedInUsers[i].isUsed == 0){
			LogUserInPOS(newClientInfo, i, interactionNamedPipeIndex);
			return i;
		}
	return -1;
}

int GetLoggedInUserPositionByPID(int PID, int maxUsers){
	for(int i = 0; i < maxUsers; i++)
		if(loggedInUsers[i].PID == PID)
			return i;
	return -1;
}

void ReserveLoggedInUserSlot(int PID, int maxUsers){
	for(int i = 0; i < maxUsers; i++)
		if(loggedInUsers[i].username[0] == 0 && loggedInUsers[i].isUsed == 0){
			memset(loggedInUsers[i].username, 0, sizeof(loggedInUsers[i].username));
			loggedInUsers[i].PID = PID;
			loggedInUsers[i].isUsed = 1;
			return;
		}
}

void InitEmptyNamedPipeUsersCountArray(int** namedPipeUsersCount, int nrOfInteractionNamedPipes){
	(*namedPipeUsersCount) = (int*) malloc(nrOfInteractionNamedPipes*sizeof(int));
	for(int i = 0; i < nrOfInteractionNamedPipes; i++)
		(*namedPipeUsersCount)[i] = 0;
}

int GetBestInteractionNamedPipeIndex(int nrOfInteractionNamedPipes, int maxUsers){
	//Array com numero de clientes por namedpipe de interação
	int minIndex = 0;
	int* namedPipeUsersCount;

	InitEmptyNamedPipeUsersCountArray(&namedPipeUsersCount, nrOfInteractionNamedPipes);

	for(int i = 0; i < maxUsers; i++)
		if(loggedInUsers[i].interactionNamedPipeIndex > -1)
			namedPipeUsersCount[loggedInUsers[i].interactionNamedPipeIndex]++;

	for(int i = 0; i < nrOfInteractionNamedPipes - 1; i++)
		if(namedPipeUsersCount[i+1]<namedPipeUsersCount[minIndex])
			minIndex = i+1;

	free(namedPipeUsersCount);
	return minIndex;
}

/*
Validates client info, and places new clients on the list to save their PIDs
No processo de validação, mais nenhuma thread pode mexer no array loggedInUsers
*/
int ValidateClientInfo(ClientInfo newClientInfo, int nrOfInteractionNamedPipes, int maxUsers, char* dbFilename){
	pthread_mutex_lock(&mutex_loggedInUsers);
	//Critical section
	int pos = GetLoggedInUserPositionByPID(newClientInfo.PID, maxUsers);
	if(usersCount < maxUsers &&
			(newClientInfo.username[0] == 0 ||
			 UsernameHasValidLenght(newClientInfo.username) == 0 ||
			 UserExists(dbFilename, newClientInfo.username) == 0 ||
			 UserIsLoggedIn(newClientInfo.username, maxUsers) == 1) &&
		pos == -1){
		/*
		 * If there's space AND
		 *			(the username is empty OR
		 *			 has an invalid lenght OR
		 *			 does not exist in the DB OR
		 *			 the inserted username is already logged in) AND
		 * the client's PID is not registered, THEN reseve it
		 */
		ReserveLoggedInUserSlot(newClientInfo.PID, maxUsers);
		usersCount++;
	} else if(UsernameHasValidLenght(newClientInfo.username) && UserExists(dbFilename, newClientInfo.username) && UserIsLoggedIn(newClientInfo.username, maxUsers) == 0){
		/*
		 * ELSE If the username has a valid lenght AND
		 * exists in the DB AND
		 * is not currently logged in
		 */
		if(pos >= 0){
			//If the user DID register his PID before (no need to increment maxusers for that user)
			respServ.INPIndex = GetBestInteractionNamedPipeIndex(nrOfInteractionNamedPipes, maxUsers);
			LogUserInPOS(newClientInfo, pos, respServ.INPIndex);
		} else if(usersCount < maxUsers){
			//If the user DIDN'T register his PID before (ex: ./client -u [username])
			respServ.INPIndex = GetBestInteractionNamedPipeIndex(nrOfInteractionNamedPipes, maxUsers);
			pos = LogUserIn(newClientInfo, maxUsers, respServ.INPIndex);
			usersCount++;
		}
	}
	//End critical section
	pthread_mutex_unlock(&mutex_loggedInUsers);
	return pos;
}

void* MainNamedPipeThread(void* tArgs){

	MainNamedPipeThreadArgs tMArgs;	//tMArgs = thread "Main" Arguments
	tMArgs = *(MainNamedPipeThreadArgs*) tArgs;

	interprocMutex = sem_open(MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME, O_CREAT, 0600, 1);
	mkfifo(tMArgs.mainNamedPipeName, 0600);

	mvwprintw(tMArgs.threadEventsWindow,2,1, "Main namedpipe created: %s", tMArgs.mainNamedPipeName);
	mvwprintw(tMArgs.threadEventsWindow, 3,1, "Waiting for clients...");
	//mvwprintw(tMArgs.threadEventsWindow, 4,1, "%s", serverSpecificInteractionNamedPipeDirName);
	wrefresh(tMArgs.threadEventsWindow);

	int userPos, clientNamedPipeIndex;
	ClientInfo newClientInfo;

	while(1){
		respServ.INPIndex = -1;

		InteractWithNamedPipe(O_RDONLY, tMArgs.mainNamedPipeName, &newClientInfo, sizeof(newClientInfo));

		userPos = ValidateClientInfo(newClientInfo, tMArgs.nrOfInteractionNamedPipes, tMArgs.maxUsers, tMArgs.dbFilename);

		InteractWithNamedPipe(O_WRONLY, tMArgs.mainNamedPipeName, &respServ, sizeof(interactionNamedPipeInfo));
		
		if(respServ.INPIndex != -1 && userPos != -1){
			InteractWithNamedPipe(O_RDONLY, tMArgs.mainNamedPipeName, &clientNamedPipeIndex, sizeof(int));
			loggedInUsers[userPos].clientNamedPipeIndex = clientNamedPipeIndex;
		}

		/* -> FOR TEST PURPOSES ONLY <- */
		for(int i = 0; i < 3; i++)
			mvwprintw(tMArgs.threadEventsWindow, 5+i,1, "[%d] username: %s, PID: %d, isUsed: %d, indexINP: %d, clientNPIndex: %d", i, loggedInUsers[i].username, loggedInUsers[i].PID, loggedInUsers[i].isUsed, loggedInUsers[i].interactionNamedPipeIndex, loggedInUsers[i].clientNamedPipeIndex);
		wrefresh(tMArgs.threadEventsWindow);
		/*
		mvwprintw(tMArgs.threadEventsWindow, 4,1, "Username: %s", username);
		mvwprintw(tMArgs.threadEventsWindow, 5,1, "respServ: %c", respServ);
		mvwprintw(tMArgs.threadEventsWindow, 6,1, "dbFilename: %s", tMArgs.dbFilename);
		mvwprintw(tMArgs.threadEventsWindow, 7,1, "strlen(Username): %d", strlen(username));
		wrefresh(tMArgs.threadEventsWindow);
		*/
		/* -> FOR TEST PURPOSES ONLY <- */
	}
}

void StartMainNamedPipeThread(WINDOW* threadEventsWindow, char* mainNamedPipeName, char* dbFilename, int maxUsers, int nrOfInteractionNamedPipes){
	//mArgs -> global var (para não ser criada na thread "principal")
	mArgs.mainNamedPipeName = mainNamedPipeName;
	mArgs.dbFilename = dbFilename;
	mArgs.maxUsers = maxUsers;
	mArgs.nrOfInteractionNamedPipes = nrOfInteractionNamedPipes;
	mArgs.threadEventsWindow = threadEventsWindow;

	pthread_t mainNamedPipeThread;

	int rc = pthread_create(&mainNamedPipeThread, NULL, MainNamedPipeThread, (void *)&mArgs);
  	if (rc){
  		endwin();
    	printf("ERROR; return code from pthread_create(&mainNamedPipeThread, [...]) is %d\n", rc);
    	exit(-1);
  	}
  	//pthread_exit(NULL);
}

void* InteractionNamedPipeThread(void* tArgs){
	InteractionNamedPipeThreadArgs tIArgs;	//tIArgs = thread "Interaction" Arguments
	tIArgs = *(InteractionNamedPipeThreadArgs*) tArgs;
	char clientNamedPipe[37];
	int UmChar;
	int i = 0;
	while(1){
		mvwprintw(tIArgs.threadEventsWindow, 8, 1, "Waiting input #%d", i);
		wrefresh(tIArgs.threadEventsWindow);
		InteractWithNamedPipe(O_RDONLY, "../inp/server_0/s0", &UmChar, sizeof(int));
		mvwprintw(tIArgs.threadEventsWindow, 9, 1, "Li o char: %c", UmChar);
		wrefresh(tIArgs.threadEventsWindow);
		i++;
		
		pthread_mutex_lock(&mutex_loggedInUsers);
		//Critical section
		for(int i = 0; i != tIArgs.maxUsers; i++)
			if(loggedInUsers[i].clientNamedPipeIndex != -1){
				sprintf(clientNamedPipe, "%sc%d", serverSpecificInteractionNamedPipeDirName, 
												  loggedInUsers[i].clientNamedPipeIndex);
				InteractWithNamedPipe(O_WRONLY, clientNamedPipe, &UmChar, sizeof(char));
				//mvwprintw(tIArgs.threadEventsWindow, 9, 1+i, "%d", i);
				//wrefresh(tIArgs.threadEventsWindow);
			}
		//End critical section
		pthread_mutex_unlock(&mutex_loggedInUsers);
	}
	mvwprintw(tIArgs.threadEventsWindow, 10, 1, "Saiu do while");
	wrefresh(tIArgs.threadEventsWindow);
}

void StartInteractionNamedPipeThread(WINDOW* threadEventsWindow, int maxUsers){
	//iArgs -> global var (para não ser criada na thread "principal")
	iArgs.maxUsers = maxUsers;
	iArgs.threadEventsWindow = threadEventsWindow;

	pthread_t interactionNamedPipeThread;

	int rc = pthread_create(&interactionNamedPipeThread, NULL, InteractionNamedPipeThread, (void *)&iArgs);
  	if (rc){
  		endwin();
    	printf("ERROR; return code from pthread_create(&interactionNamedPipeThread, [...]) is %d\n", rc);
    	exit(-1);
  	}
  	//pthread_exit(NULL);
}

void StartThreads(WINDOW* threadEventsWindow, char* mainNamedPipeName, char* dbFilename, int maxUsers, int nrOfInteractionNamedPipes){
	StartMainNamedPipeThread(threadEventsWindow, mainNamedPipeName, dbFilename, maxUsers, nrOfInteractionNamedPipes);
	StartInteractionNamedPipeThread(threadEventsWindow, maxUsers);
}

void SignalAllLoggedInUsers(int signum, int maxUsers){
	for(int i = 0; i < maxUsers; i++)
		if(loggedInUsers[i].username[0] != 0 || loggedInUsers[i].isUsed == 1)
			kill(loggedInUsers[i].PID, signum);
}

void CreateInteractionNamedPipeDir(){
	//Interaction namedpipe dir name starts from "server_1" to "server_2" and "server_3" and so on
	if(DirectoryExists(MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH) == 0)
		mkdir(MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH, 0700);

	//global var, used in comunication through main namedpipe
	respServ.INPServerSpecificFolderIndex = -1;
	do{
		respServ.INPServerSpecificFolderIndex++;
		sprintf(serverSpecificInteractionNamedPipeDirName, "%s%s%d/", MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH, 
															MEDIT_SERVER_SPECIFIC_INTERACTION_NAMED_PIPE_PATH,
															respServ.INPServerSpecificFolderIndex);
	}while(DirectoryExists(serverSpecificInteractionNamedPipeDirName) == 1);
	mkdir(serverSpecificInteractionNamedPipeDirName, 0700);
}

void InitInteractionNamedPipes(int nrOfInteractionNamedPipes){
	//Source: https://www.tutorialspoint.com/c_standard_library/limits_h.htm

	/* VAR: name:
	 *   GLOBAL VAR: serverSpecificInteractionNamedPipeDirName (25 -> ignorando '\0') +
	 * 	 's' (1) +
	 *   nr máximo de digitos que um inteiro pode ter (10) +
	 *   '\0' (1) = 37 */

	//(OLD) char name[18]; //int max value = +2147483647 (10 digitos)

	char name[37];
	
	CreateInteractionNamedPipeDir();

	for(int i = 0; i < nrOfInteractionNamedPipes; i++){
		memset(name, 0, sizeof(name));
		//namedpipes started with "s" are the "interaction namedpipes" in the enunciado cyka blyat
		//After meeting with the teacher, i decided to use those namedpipes to recieve the chars from the clients (and the logout requests)
		sprintf(name, "%ss%d", serverSpecificInteractionNamedPipeDirName, i);

		//(OLD) sprintf(name, "%s%d", MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH, i);
		mkfifo(name, 0600);
	}
}

void FreeAllocatedScreenMemory(Screen *screen, CommonSettings commonSettings){
	for(int i = 0; i < commonSettings.maxLines; i++)
		free((*screen).line[i].column);
	free((*screen).line);
}

void FreeAllocatedMemory(Screen *screen, CommonSettings commonSettings){
	FreeAllocatedScreenMemory(screen, commonSettings);
	//insert other dynamic array "free(...)'s" here
}

void DeleteInteractionNamedPipes(int nrOfInteractionNamedPipes){
	//Ver comment em InitInteractionNamedPipes(...)
	char name[36];
	for(int i = 0; i < nrOfInteractionNamedPipes; i++){
		memset(name, 0, sizeof(name));
		sprintf(name, "%ss%d", serverSpecificInteractionNamedPipeDirName, i);
		unlink(name);
	}
	//remove server specific "../inp/server_x" folder
	rmdir(serverSpecificInteractionNamedPipeDirName);
	//if "../inp/" is empty, delete it (only deletes it after the last server shutsdown)
	rmdir(MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH);
}

void Shutdown(){
	unlink(commonSettings.mainNamedPipeName);
	endwin();
	FreeAllocatedMemory(&screen, commonSettings);
	SignalAllLoggedInUsers(SIGUSR1, serverSettings.maxUsers);
	sem_unlink(MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME);
	DeleteInteractionNamedPipes(serverSettings.nrOfInteractionNamedPipes);
	exit(1);
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

	InitSettingsStructs();
	InitFromEnvs(&commonSettings.maxLines, &commonSettings.maxColumns, &serverSettings.maxUsers ,&serverSettings.timeout);
	InitFromOpts(argc, argv, &serverSettings.dbFilename, &commonSettings.mainNamedPipeName, &serverSettings.nrOfInteractionNamedPipes);

	if(ServerIsRunningOnNamedPipe(commonSettings.mainNamedPipeName) == 0){
		//Initialize ncurses
		initscr();
		PrintLogo();
		refresh();
		sleep(3);
		clear();
		WINDOW* window[4];
		InitWindows(window);

		signal(SIGINT, Shutdown);
		AllocScreenMemory();
		
		InitEmptyLoggedInUsersArray(serverSettings.maxUsers);
		InitInteractionNamedPipes(serverSettings.nrOfInteractionNamedPipes);
		StartThreads(window[1], commonSettings.mainNamedPipeName, serverSettings.dbFilename, serverSettings.maxUsers, serverSettings.nrOfInteractionNamedPipes);

		do{
			mvwprintw(window[3], 1, 1, "Command: ");
			//wscanw() already calls: wrefresh()
			wscanw(window[3], " %299[^\n]", cmd);
			//ncurses flushes the "input buffer" automatically, so there's no need to call: FlushStdin();
			ProcessCommand(cmd, &shutdown, commonSettings, serverSettings, window[0]);
			ClearWindow(window[3]);
		}while(shutdown == 0);

		//Shutdown logic here
		Shutdown();
	} else
	printf("Found another server running on namedpipe: %s\nExiting...\n", commonSettings.mainNamedPipeName);
	return 0;
}
//COMPILE WITH: gcc server.c -o server -lncurses -lpthread -lrt

//NOTAS PARA INCLUIR NO RELATÓRIO DA META 3:
/*
 * -> O login tinha um bug. Quando se iniciava um cliente com o parâmetro -p e um username que ja tivesse login feito,
 *    esse cliente não ficava com um espaço reservado no servidor. (fixed)
 *
 * -> o namedpipe principal pode ser criado em qqr localização do pc, visto
 *    ser o utilizador quem define o caminho do mesmo, o mesmo não se verifica
 *    nos namedpipes de interação, uma vez que são apenas definidos pelo servidor
 */