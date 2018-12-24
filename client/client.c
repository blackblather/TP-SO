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

sem_t* interprocMutex;
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

int IsValidChar(int ch){
	//Source: https://theasciicode.com.ar/ascii-control-characters/null-character-ascii-code-0.html
	//Nota: Apenas são considerados válidos, os caracteres pertencentes à tabela 'ASCII printable characters'
	if(ch >= 32 && ch <= 126)
		return 1;
	return 0;
}

int IsEmptyOrSpaceChar(int ch){
	if(ch == 32 || ch == 0)
		return 1;
	return 0;
}

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
	interprocMutex = sem_open(MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME, O_EXCL);
	sem_wait(interprocMutex);
		//Critical section
		InteractWithNamedPipe(O_WRONLY, mainNamedPipeName, clientInfo, sizeof((*clientInfo)));
		InteractWithNamedPipe(O_RDONLY, mainNamedPipeName, &respServ, sizeof(interactionNamedPipeInfo));
		SetInteractionNamedPipePath();

	if(respServ.INPIndex == -1){
		//End critical section (fail login case)
		sem_post(interprocMutex);
		return 1;
	} else {
		InitClientNamedPipe();
		InteractWithNamedPipe(O_WRONLY, mainNamedPipeName, &clientNamedPipeIndex, sizeof(int));
		//End critical section (successful login case)
		sem_post(interprocMutex);
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

void EnterLineEditMode(int posY, int maxColumns, int offset, char* mainNamedPipeName){
	//Source #1: Exemplo NCURSES 1 -> Moodle
	//Source #2: Exemplo NCURSES 2 -> Moodle
	//Source #3: https://www.gnu.org/software/guile-ncurses/manual/html_node/Getting-characters-from-the-keyboard.html
	//Source #4: https://invisible-island.net/ncurses/man/curs_getch.3x.html#h3-Keypad-mode
	//Source #5: https://stackoverflow.com/questions/48274895/how-can-i-know-that-the-user-hit-the-esc-key-in-a-console-with-ncurses-linux?rq=1
	//Source #6: http://invisible-island.net/ncurses/man/curs_getch.3x.html#h2-NOTES
	//Source #7: http://pubs.opengroup.org/onlinepubs/7990989775/xcurses/inch.html
	/* 
	 * NOTAS:
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

	int posX = offset;
	int ch, lastLineChar;
	maxColumns = maxColumns+offset;

	cbreak();
	keypad(stdscr, TRUE);
	noecho();

	//Posiciona o cursor no inicio da linha
	move(posY, posX);
	refresh();
	int i = 0;
	do{
		ch = getch();
		switch(ch){
			case KEY_LEFT:{
				if((posX-1)>=offset)
					posX--;
			}break;
			case KEY_RIGHT:{
				if((posX+1)<=maxColumns)
					posX++;
			}break;
			default:{
				if(ch == KEY_BACKSPACE || ch == KEY_DC){
					if((posX-1)>=offset){
						posX--;
						mvprintw(posY, posX, "%c", ' ');
						for(int i = posX; i < maxColumns-1; i++)
							mvprintw(posY, i, "%c", mvinch(posY, i+1));
						mvprintw(posY, maxColumns-1, "%c", ' ');
					}
				}
				else{
					lastLineChar = (int) mvinch(posY, maxColumns-1);
					if(IsValidChar(ch) && ((posX+1) <= maxColumns) &&
				       IsValidChar(lastLineChar) && IsEmptyOrSpaceChar(lastLineChar)){
				       	//IS VALID CHAR
						int auxCh, auxCh2;
						auxCh = mvinch(posY, posX);
						mvprintw(posY, posX, "%c", ch);
						for(int i = posX+1; i < maxColumns; i++){
							auxCh2 = mvinch(posY, i); 
							mvprintw(posY, i, "%c", auxCh);
							auxCh = auxCh2;
						}
						posX++;
						//TESTING WRITING CHARS TO ALL CLIENTS
							sem_wait(interprocMutex);
							InteractWithNamedPipe(O_WRONLY, interactionNamedPipe, &ch, sizeof(int));
							sem_post(interprocMutex);
						//END TEST
					}
				}
			}break;
		}	
		move(posY, posX);
		refresh();
		i++;
	}while(ch != 27);
}

void* ReadingThread(void* tArgs){
	ClientNamedPipeInfo tRArgs;	//tRArgs = thread "Reading" Arguments
	tRArgs = *(ClientNamedPipeInfo*) tArgs;
	char inputChar;
	while(1){
		InteractWithNamedPipe(O_RDONLY, tRArgs.clientNamedPipe, &inputChar, sizeof(char));
		mvprintw(14,13, "%c", inputChar);
		refresh();
	}
}

void StartReadingThread(){
	//rArgs -> global var (para não ser criada na thread "principal")
	rArgs.clientNamedPipe = clientNamedPipe;

	pthread_t readingThread;

	int rc = pthread_create(&readingThread, NULL, ReadingThread, (void *)&rArgs);
  	if (rc){
  		endwin();
    	printf("ERROR; return code from pthread_create(&interactionNamedPipeThread, [...]) is %d\n", rc);
    	exit(-1);
  	}
}

void InitTextEditor(char *username, CommonSettings commonSettings){
	//Quando chega aqui, o utilizador já fez login
	PrintLineNrs(commonSettings.maxLines);
	StartReadingThread();
	/* TODO LIST:
	 * (1) -> recebe estrutura de como estão as coisas neste momento e imprime estrutura (tudo tem que bloquear até
	 * ter carregado as coisas todas, para nao haver conflitos)
	 *
	 * (2) -> inicia thread de receber chars do servidor (podia implementar uma espécie de pilha FILO (lista ligada com
	 * ponteiro para a ultima posição, e cada item aponta para a anterior) que armazena tudo o que é Chars a imprimir)1
	 *
	 * (3) -> entra em modo de "navegação livre"
	 *******************************************/
	//quando o user clica "Enter", se a linha estiver livre, entra no modo de edição dessa linha
	EnterLineEditMode(0, commonSettings.maxColumns, 3, commonSettings.mainNamedPipeName);
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