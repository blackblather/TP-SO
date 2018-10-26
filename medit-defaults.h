//Constantes comuns ao servidor e ao cliente
#define MEDIT_USERNAME_MAXLENGHT 8
typedef struct CommonSettingsStruct{
	int maxLines, maxColumns;
	char *mainNamedPipeName;
} CommonSettings;