#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>		
#include <sys/types.h>	//Used for open()
#include <sys/stat.h>	//Used for open()
#include <fcntl.h>		//Used for open()
#include <semaphore.h>	//Used for sem_open(), sem_wait() and sem_post()
#include <signal.h>		//Used for signal
#include "client-defaults.h"

sem_t* interprocMutex;

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

void WriteReadNamedpipe(char* mainNamedPipeName, void* writeVal, int writeValSize, void* readVal, int readValSize){
	interprocMutex = sem_open(MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME, O_EXCL);
	sem_wait(interprocMutex);
		//Critical section
		InteractWithNamedPipe(O_WRONLY, mainNamedPipeName, writeVal, writeValSize);
		InteractWithNamedPipe(O_RDONLY, mainNamedPipeName, readVal, readValSize);
		//End critical section
	sem_post(interprocMutex);
}

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

int IsValidUsername(char* mainNamedPipeName, ClientInfo* clientInfo){
	char respServ = '0';
	WriteReadNamedpipe(mainNamedPipeName, clientInfo, sizeof((*clientInfo)), &respServ, 1);
	if(respServ == '1')
		return 1;
	return 0;
}

void AskUsernameWhileInvalid(char* mainNamedPipeName, ClientInfo* clientInfo){
	while(IsValidUsername(mainNamedPipeName, clientInfo) == 0){
		mvprintw(0,0,"Insert your username: ");
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
	      /*case 'u': UpdateUsername(username, optarg); break;
	      case 'p':	UpdateMainNamedPipeName(mainNamedPipeName, optarg); break;*/
	    	case 'u': UpdateUsername(username, optarg); break;
	      	case 'p': UpdateMainNamedPipeName(mainNamedPipeName, optarg); break;
	    }
	}
}

void PrintLineNrs(int maxLinesTMP){
	for(int i = 0; i < maxLinesTMP; i++)
		mvprintw(i, 0, "%2d:", i);
}

void EnterLineEditMode(int posY, int maxColumns, int offset){
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
						int auxCh, auxCh2;
						auxCh = mvinch(posY, posX);
						mvprintw(posY, posX, "%c", ch);
						for(int i = posX+1; i < maxColumns; i++){
							auxCh2 = mvinch(posY, i); 
							mvprintw(posY, i, "%c", auxCh);
							auxCh = auxCh2;
						}
						posX++;
					}
				}
			}break;
		}	
		move(posY, posX);
		refresh();
	}while(ch != 27);
}

void InitTextEditor(char *username, CommonSettings commonSettings){
	PrintLineNrs(commonSettings.maxLines);
	EnterLineEditMode(0, commonSettings.maxColumns, 3);
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

		AskUsernameWhileInvalid(commonSettings.mainNamedPipeName, &clientInfo);
		InitTextEditor(clientInfo.username, commonSettings);
		endwin();
	} else
		printf("No server is running on namedpipe: %s\nExiting...\n", commonSettings.mainNamedPipeName);

	return 0;
}