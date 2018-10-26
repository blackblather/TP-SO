//Constantes comuns ao servidor e ao cliente
#define MEDIT_USERNAME_MAXLENGHT 8
#define MEDIT_MAXLINES 15							//Apenas usar se a envVar nao estiver definida (same name)
#define MEDIT_MAXCOLUMNS 45							//Apenas usar se a envVar nao estiver definida (same name)
#define MEDIT_MAIN_NAMED_PIPE_NAME "mainNamedPipe"	//Nome do named pipe pricipal
typedef struct CommonSettingsStruct{
	int maxLines, maxColumns;
	char *mainNamedPipeName;
} CommonSettings;