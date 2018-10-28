#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "client-defaults.h"

int IsValidChar(int ch){
	//Source: https://theasciicode.com.ar/ascii-control-characters/null-character-ascii-code-0.html
	//Nota: Apenas são considerados válidos, os caracteres pertencentes à tabela 'ASCII printable characters'
	if(ch >= 32 && ch <= 126)
		return 1;
	return 0;
}

int IsValidUsername(char *username){
	int usernameLenght = strlen(username);
	if(usernameLenght > 0 && usernameLenght <= 8)
		return 1;
	return 0;
}

int UpdateUsername(char *username, char* val) {
	if(IsValidUsername(val)){
		strcpy(username, val);
		return 1;
	}
	return 0;
}

void AskUsernameIfEmpty(char *username){
	if(strlen(username) == 0){
		char usernameTmp[9] = {0};
		do{
			printw("Insert your username: ");
			scanw(" %8[^ \n]", usernameTmp);
			clear();
		}while(!UpdateUsername(username, usernameTmp));
	}
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
	      case 'p':	UpdateMainNamedPipeName(mainNamedPipeName, optarg); break;
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
	 * Nota #4: No código desta função, não dou uso à função notimeout(...),
	 * pelo que o delay referido ocorre antes do ciclo terminar. No entanto,
	 * o delay não ocorre caso se insira uma Input Escape Sequence, que retorna como
	 * primeiro carácter, o carácter cujo código ASCII é 27 (correspondente também
	 * à tecla 'Escape' (VER: 'Nota #1')).
	 *
	 * Nota #5: A leitura da tecla 'Escape' é desencorajada, mas utilizada nesta aplicação,
	 * para estar em conformidade com o enunciado do trabalho prático. (VER: 'Source #6')
	 */

	int posX = offset;
	int ch;
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
			case KEY_BACKSPACE:{
				if((posX-1)>=offset){
					posX--;
					mvprintw(posY, posX, "%c", ' ');
				}
			}break;
			default:{
				if(IsValidChar(ch) && (posX+1)<=maxColumns){
					mvprintw(posY, posX, "%c", ch);
					posX++;
				}
			}break;
		}	
		move(posY, posX);
		refresh();
	}while(ch != 27);
}

void InitTextEditor(char *username, CommonSettings commonSettings){
	AskUsernameIfEmpty(username);
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

int main(int argc, char* const argv[]){
	CommonSettings commonSettings;
	InitCommonSettingsStruct(&commonSettings);
	

	char username[9] = {0};
	//---------------------------------------
	initscr();
	clear();
	InitFromOpts(argc, argv, username, &commonSettings.mainNamedPipeName);
	InitTextEditor(username, commonSettings);
	endwin();
	return 0;
}