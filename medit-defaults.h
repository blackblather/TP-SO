//Constantes comuns ao servidor e ao cliente
#define MEDIT_USERNAME_MAXLENGHT 8
#define MEDIT_MAIN_NAMED_PIPE_NAME "../mainNamedPipe"	//Nome do named pipe pricipal
#define MEDIT_MAIN_NAMED_PIPE_SEMAPHORE_NAME "mainNamedPipeSemaphore"
#define MEDIT_MAIN_INTERACTION_NAMED_PIPE_PATH "../inp/"				//"inp" stands for "interaction namedpipe"
#define MEDIT_SERVER_SPECIFIC_INTERACTION_NAMED_PIPE_PATH "server_"
#define MEDIT_OFFSET 3

typedef struct CommonSettingsStruct{
	int maxLines, maxColumns;
	char *mainNamedPipeName;
} CommonSettings;

typedef struct ClientInfoStruct{
	char username[MEDIT_USERNAME_MAXLENGHT+1];
	pid_t PID;
	int isUsed, interactionNamedPipeIndex, clientNamedPipeIndex;
	Line* oldLine;
} ClientInfo;

typedef struct interactionNamedPipeInfoStruct{
	int INPIndex;
	int INPServerSpecificFolderIndex;
} interactionNamedPipeInfo;

typedef struct CharInfoStruct{
	int ch;
	int posX, posY;
} CharInfo;