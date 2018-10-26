#include <stdio.h>	//Used for scanf() and stuff
#include <string.h>	//Used for strlen()
#include <stdlib.h>
#include <unistd.h>	//Used for access() and getopt()
#include "client-defaults.h"

void FlushStdin(){
	//Source: https://stackoverflow.com/questions/2187474/i-am-not-able-to-flush-stdin
	//"Remove" os chars do buffer até chegar ao '\n' ou End Of File (EOF)
	int f;
	while((f = fgetc(stdin)) != '\n' && f != EOF);
}

int IsValidUsername(char *username){
	int usernameLenght = strlen(username);
	if(usernameLenght > 0 && usernameLenght <= 8)
		return 1;
	return 0;
}

void UpdateUsername(char *username, char* val) {
	if(IsValidUsername(val))
		strcpy(username, val);
}

void AskUsernameIfNull(char *username){
	if(strcmp(username, "") == 0){
		char usernameTmp[9];
		do{
			printf("Insira o seu username: ");
			scanf("%8[^ \n]", usernameTmp);
			FlushStdin();
		}while(IsValidUsername(usernameTmp) == 0);
		strcpy(username, usernameTmp);
	}
}

void InitFromOpts(int argc, char* const argv[], char *username){
	//Source: https://www.gnu.org/software/libc/manual/html_node/Getopt.html
	int c;
	while ((c = getopt(argc, argv, "n:")) != -1){
	    switch (c){
	      case 'n': UpdateUsername(username, optarg); break;
	    }
	}
}

int main(int argc, char* const argv[]){
	char username[9];
	CommonSettings settings;

	InitFromOpts(argc, argv, username);
	AskUsernameIfNull(username);

	printf("USERNAME: %s\n", username);
	return 0;
}