#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>		
#include <sys/types.h>	//Used for open()
#include <sys/stat.h>	//Used for open()
#include <fcntl.h>		//Used for open()
#include <pthread.h>	//Used for creating threads
#include <semaphore.h>	//Used for sem_open(), sem_wait() and sem_post()
#include <signal.h>		//Used for signal
#include "client-defaults.h"

sem_t* mainNamedPipeSem;
interactionNamedPipeInfo respServ;
char interactionNamedPipe[37];
/* VAR: clientNamedPipe:
 *   tamanho do caminho (14) +
 *   nr máximo de digitos que um inteiro pode ter (10) +
 *	 '/' (1) +
 *	 'c' (1) +
 *   nr máximo de digitos que um inteiro pode ter (10) +
 *   '\0' (1) = 37 */
char clientNamedPipe[37];
int clientNamedPipeIndex;
ClientNamedPipeInfo rArgs;
CharInfo chInfo, chInfoFIFO;
pthread_mutex_t mutex_chInfo = PTHREAD_MUTEX_INITIALIZER;

void PrintLogo(){
	//Source: http://patorjk.com/software/taag/#p=display&h=1&v=2&f=Ogre&t=Medit%20Client%20
	//Font: ogre
	mvprintw(4,9,"--------------------------------------------------------------");
	mvprintw(5,9,"                   _  _  _       ___  _  _               _    ");
	mvprintw(6,9,"  /\\/\\    ___   __| |(_)| |_    / __\\| |(_)  ___  _ __  | |_  ");
	mvprintw(7,9," /    \\  / _ \\ / _` || || __|  / /   | || | / _ \\| '_ \\ | __| ");
	mvprintw(8,9,"/ /\\/\\ \\|  __/| (_| || || |_  / /___ | || ||  __/| | | || |_  ");
	mvprintw(9,9,"\\/    \\/ \\___| \\__,_||_| \\__| \\____/ |_||_| \\___||_| |_| \\__| ");
	mvprintw(10,9,"--------------------------------------------------------------");
}

int FileExists(char* filename){
	if(access( filename, F_OK ) != -1)
		return 1;
	return 0;
}

int ServerIsRunningOnNamedPipe(char* mainNamedPipeName){
	if(access(mainNamedPipeName, F_OK) == 0)
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

/*void WriteReadNamedpipe(char* semName, int oflag, char* namedPipeName, void* writeVal, int writeValSize, void* readVal, int readValSize){
	//Este if + switch podiam ser ignorados, porque o sem_open() ignora os ultimos 2 params caso o semaforo ja exista
	//Apenas cá estão para o código ser um pouco mais legível
	if(oflag == O_EXCL || oflag == O_CREAT){
		switch(oflag){
			case O_EXCL: interprocMutex = sem_open(semName, oflag); break;
			case O_CREAT: interprocMutex = sem_open(semName, oflag, 0600, 1); break;
		}
		sem_wait(interprocMutex);
			//Critical section
			InteractWithNamedPipe(O_WRONLY, namedPipeName, writeVal, writeValSize);
			InteractWithNamedPipe(O_RDONLY, namedPipeName, readVal, readValSize);
			//End critical section
		sem_post(interprocMutex);
	}
}*/

void SetInteractionNamedPipePath(){
	sprintf(interactionNamedPipe, "%s%s%d/s%d", MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH, 
											 	MEDIT_SERVER_SPECIFIC_INTERACTION_NAMED_PIPE_PATH,
											 	respServ.INPServerSpecificFolderIndex,
											 	respServ.INPIndex);
}

void InitClientNamedPipe(){
	clientNamedPipeIndex = -1;
	do{
		clientNamedPipeIndex++;
		sprintf(clientNamedPipe, "%s%s%d/c%d", MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH, 
											   MEDIT_SERVER_SPECIFIC_INTERACTION_NAMED_PIPE_PATH,
											   respServ.INPServerSpecificFolderIndex,
											   clientNamedPipeIndex);
	}while(FileExists(clientNamedPipe) == 1);
	mkfifo(clientNamedPipe, 0600);
}

int CantLogin(char* mainNamedPipeName, ClientInfo* clientInfo){
	//int respServ = -1;
	respServ.INPIndex = -1;
	//WriteReadNamedpipe(MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME, O_EXCL, mainNamedPipeName, clientInfo, sizeof((*clientInfo)), &respServ, sizeof(interactionNamedPipeInfo));
	mainNamedPipeSem = sem_open(MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME, O_EXCL);
	sem_wait(mainNamedPipeSem);
		//Critical section
		InteractWithNamedPipe(O_WRONLY, mainNamedPipeName, clientInfo, sizeof(ClientInfo));
		InteractWithNamedPipe(O_RDONLY, mainNamedPipeName, &respServ, sizeof(interactionNamedPipeInfo));
		SetInteractionNamedPipePath();

	if(respServ.INPIndex == -1){
		//End critical section (fail login case)
		sem_post(mainNamedPipeSem);
		return 1;
	} else {
		InitClientNamedPipe();
		InteractWithNamedPipe(O_WRONLY, mainNamedPipeName, &clientNamedPipeIndex, sizeof(int));
		//End critical section (successful login case)
		sem_post(mainNamedPipeSem);
		return 0;
	}
}

void AskUsernameWhileNotLoggedIn(char* mainNamedPipeName, ClientInfo* clientInfo){
	while(CantLogin(mainNamedPipeName, clientInfo)){
		PrintLogo();
		mvprintw(15,28,"Insert your username: ");
		scanw(" %8[^ \n]", clientInfo->username);
		clientInfo->username[MEDIT_USERNAME_MAXLENGHT] = '\0';
		clear();
	}
}

void UpdateUsername(char *username, char* val) {
	strncpy(username, val, MEDIT_USERNAME_MAXLENGHT);
}

void UpdateMainNamedPipeName(char **oldMainNamedPipeName, char *newdMainNamedPipeName){
	(*oldMainNamedPipeName) = newdMainNamedPipeName;
}


void InitFromOpts(int argc, char* const argv[], char *username, char **mainNamedPipeName){
	//Source: https://www.gnu.org/software/libc/manual/html_node/Getopt.html
	int c;
	while ((c = getopt(argc, argv, "u:p:")) != -1){
	    switch (c){
	    	case 'u': UpdateUsername(username, optarg); break;
	      	case 'p': UpdateMainNamedPipeName(mainNamedPipeName, optarg); break;
	    }
	}
}

void PrintLineNrs(int maxLinesTMP){
	for(int i = 0; i < maxLinesTMP; i++)
		mvprintw(i, 0, "%2d:", i);
}


void UpdateCursorPosition(int posY, int posX){
	move(posY, posX);
	refresh();
}

void EnterLineEditMode(int posY, int maxColumns, char* mainNamedPipeName){
	/* SOURCES:
	 * Source #1: Exemplo NCURSES 1 -> Moodle
	 * Source #2: Exemplo NCURSES 2 -> Moodle
	 * Source #3: https://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html
	 * Source #4: https://invisible-island.net/ncurses/man/curs_getch.3x.html#h3-Keypad-mode
	 * Source #5: https://stackoverflow.com/questions/48274895/how-can-i-know-that-the-user-hit-the-esc-key-in-a-console-with-ncurses-linux?rq=1
	 * Source #6: http://invisible-island.net/ncurses/man/curs_getch.3x.html#h2-NOTES
	 * Source #7: http://pubs.opengroup.org/onlinepubs/7990989775/xcurses/inch.html
	 */
	/* NOTAS:
	 *
	 * Nota #1: 27 é o valor decimal da tabela ASCII correspondente à tecla 'Escape'.
	 * A biblioteca NCURSES define um timeout, que é usado ao premir esta tecla, para
	 * poder distinguir se o utilizador clicou na tecla 'Escape' ou se usou uma
	 * Input Escape Sequence. (VER: 'Source #4' e 'Source #5')
	 *
	 * Nota #2: Para evitar que o delay acima mencionado aconteça, é preciso
	 * chamar a função notimeout(win, TRUE). (VER: 'Source #5')
	 *
	 * Nota #3: KEY_ENTER é o enter do 'num pad', 10 é o código ascii da
	 * tecla 'Enter' do teclado. (VER: 'Source 6')
	 *
	 * Nota #4: Para ir buscar o caractere numa posição (y,x), uso a função mvinch(...)
	 * (VER: 'Source #7')
	 *
	 * Nota #5: No código desta função, não dou uso à função notimeout(...),
	 * pelo que o delay referido ocorre antes do ciclo terminar. No entanto,
	 * o delay não ocorre caso se insira uma Input Escape Sequence, que retorna como
	 * primeiro caractere, o caractere cujo código ASCII é 27 (correspondente também
	 * à tecla 'Escape' (VER: 'Nota #1')).
	 *
	 * Nota #6: Numa outra situação a tecla 'Delete', deveria apagar os caracters à direita
	 * do cursor, mas para estar em plena conformidade com o enunciado do trabalho prático,
	 * apaga os caracteresà esquerda.
	 *
	 * Nota #7: A leitura da tecla 'Escape' é desencorajada, mas utilizada nesta aplicação,
	 * para estar de novo em conformidade com o enunciado do trabalho prático.
	 * (VER: 'Source #6')
	 *
	 */

	chInfo.posX = MEDIT_OFFSET;
	chInfo.posY = posY;
	int lastLineChar, ch;
	/*cbreak();
	keypad(stdscr, TRUE);
	noecho();*/

	//Posiciona o cursor no inicio da linha
	UpdateCursorPosition(chInfo.posY, chInfo.posX);
	do{
		ch = getch();
		pthread_mutex_lock(&mutex_chInfo);
		//Critical section
			switch(ch){
				case KEY_LEFT:{
					if((chInfo.posX-1)>=MEDIT_OFFSET){
						chInfo.posX--;
						UpdateCursorPosition(chInfo.posY, chInfo.posX);
					}
				}break;
				case KEY_RIGHT:{
					if((chInfo.posX+1)<=maxColumns+MEDIT_OFFSET){
						chInfo.posX++;
						UpdateCursorPosition(chInfo.posY, chInfo.posX);
					}
				}break;
				default:{
					chInfo.ch = ch;
					InteractWithNamedPipe(O_WRONLY, interactionNamedPipe, &chInfo, sizeof(CharInfo));
				}break;
			}
		//End critical section
		pthread_mutex_unlock(&mutex_chInfo);
	}while(chInfo.ch != 27);
}

void WaitAndReadCharInfoAndLine(char* clientNamedPipe, int maxColumns, int* line){
	int fd;
	
	if((fd = open(clientNamedPipe, O_RDONLY)) >= 0){
		read(fd, &chInfoFIFO, sizeof(CharInfo));
		read(fd, line, (maxColumns+1)*sizeof(int));
		close(fd);
	}
}

void PrintLine(int posY, int* line, int maxColumns){
	for(int i = 0; i < maxColumns; i++)
		mvprintw(posY, i+MEDIT_OFFSET, "%c", line[i]);
}

void* ReadingThread(void* tArgs){
	ClientNamedPipeInfo tRArgs;	//tRArgs = thread "Reading" Arguments
	tRArgs = *(ClientNamedPipeInfo*) tArgs;
	int posY, posX;
	int* line = (int*) malloc((tRArgs.maxColumns+1)*sizeof(int));
	while(1){
		WaitAndReadCharInfoAndLine(tRArgs.clientNamedPipe, tRArgs.maxColumns, line);
		pthread_mutex_lock(&mutex_chInfo);
			PrintLine(chInfoFIFO.posY, line, tRArgs.maxColumns);
			if(chInfo.posY == chInfoFIFO.posY)
				chInfo.posX = chInfoFIFO.posX;
			UpdateCursorPosition(chInfo.posY, chInfo.posX);
		pthread_mutex_unlock(&mutex_chInfo);
	}
}

void StartReadingThread(int maxColumns){
	//rArgs -> global var (para não ser criada na thread "principal")
	rArgs.clientNamedPipe = clientNamedPipe;
	rArgs.maxColumns = maxColumns;
	pthread_t readingThread;

	int rc = pthread_create(&readingThread, NULL, ReadingThread, (void *)&rArgs);
  	if (rc){
  		//endwin();
    	mvprintw(5,5,"ERROR; return code from pthread_create(&interactionNamedPipeThread, [...]) is %d\n", rc);
    	refresh();
    	getch();
    	//exit(-1);
  	}
}

void InitTextEditor(char *username, CommonSettings commonSettings){
	//Quando chega aqui, o utilizador já fez login
	PrintLineNrs(commonSettings.maxLines);
	StartReadingThread(commonSettings.maxColumns);
	/* TODO LIST:
	 * (1) -> recebe estrutura de como estão as coisas neste momento e imprime estrutura (tudo tem que bloquear até
	 * ter carregado as coisas todas, para nao haver conflitos)
	 *
	 * (2) -> entra em modo de "navegação livre"
	 *******************************************/

	int ch, posYAux;

	cbreak();
	keypad(stdscr, TRUE);
	noecho();

	pthread_mutex_lock(&mutex_chInfo);
		chInfo.posX = MEDIT_OFFSET;
		chInfo.posY = 0;
		move(chInfo.posY, chInfo.posX);
		refresh();
	pthread_mutex_unlock(&mutex_chInfo);

	do{
		ch = getch();
		if(ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_DOWN || ch == KEY_UP){
			pthread_mutex_lock(&mutex_chInfo);
				switch(ch){
					case KEY_LEFT: {
						if((chInfo.posX-1) >= MEDIT_OFFSET)
							chInfo.posX--;
					} break;
					case KEY_RIGHT: {
						if((chInfo.posX+1) <= commonSettings.maxColumns+MEDIT_OFFSET)
							chInfo.posX++;
					} break;
					case KEY_DOWN: {
						if((chInfo.posY+1) <= commonSettings.maxLines-1)
							chInfo.posY++;
					} break;
					case KEY_UP: {
						if((chInfo.posY-1) >= 0)
							chInfo.posY--;
					} break;
				}
				UpdateCursorPosition(chInfo.posY, chInfo.posX);
			pthread_mutex_unlock(&mutex_chInfo);
		} else if(ch == 10) {
			//Source #1: https://stackoverflow.com/questions/11067800/ncurses-key-enter-is-fail
			pthread_mutex_lock(&mutex_chInfo);
				posYAux = chInfo.posY;
			pthread_mutex_unlock(&mutex_chInfo);
			EnterLineEditMode(posYAux, commonSettings.maxColumns, commonSettings.mainNamedPipeName);
		}
	}while(ch != 27);
	//quando o user clica "Enter", se a linha estiver livre, entra no modo de edição dessa linha
}

void InitCommonSettingsStruct(CommonSettings* commonSettings){
	//As vars maxLines e maxColumns serão preenchidas com a informação do servidor,
	//enviada por namepipe. (numa meta futura)
	(*commonSettings).maxLines = 15;
	(*commonSettings).maxColumns = 45;

	(*commonSettings).mainNamedPipeName = MEDIT_MAIN_NAMED_PIPE_NAME;
}

void SigpipeHanler(){
	printf("Caught SIGPIPE:\nError writting to pipe that's not open for reading.\n");
	exit(1);
}

void Sigusr1Handler(){
	mvprintw(5, 5, "Caught SIGUSR1: Exiting...");
	refresh();
	endwin();
	exit(1);
}

void InitSignalHandlers(){
	signal(SIGPIPE, SigpipeHanler);
	signal(SIGUSR1, Sigusr1Handler);
}

int main(int argc, char* const argv[]){
	CommonSettings commonSettings;
	ClientInfo clientInfo = {.username = {0}, .PID = getpid()};

	InitCommonSettingsStruct(&commonSettings);
	InitFromOpts(argc, argv, clientInfo.username, &commonSettings.mainNamedPipeName);

	if(ServerIsRunningOnNamedPipe(commonSettings.mainNamedPipeName) == 1){
		initscr();
		clear();

		InitSignalHandlers();

		AskUsernameWhileNotLoggedIn(commonSettings.mainNamedPipeName, &clientInfo);
		InitTextEditor(clientInfo.username, commonSettings);
		endwin();
	} else
		printf("No server is running on namedpipe: %s\nExiting...\n", commonSettings.mainNamedPipeName);

	return 0;
}