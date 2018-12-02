//Constantes comuns ao servidor e ao cliente
#define MEDIT_USERNAME_MAXLENGHT 8
#define MEDIT_MAIN_NAMED_PIPE_NAME "../mainNamedPipe"	//Nome do named pipe pricipal
#define MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME "mainNamedPipeSemaphore"
typedef struct CommonSettingsStruct{
	int maxLines, maxColumns;
	char *mainNamedPipeName;
} CommonSettings;
typedef struct ClientInfoStruct{
	char username[MEDIT_USERNAME_MAXLENGHT+1];
	pid_t PID;
	int isUsed;
} ClientInfo;