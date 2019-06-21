#include "../medit-defaults.h"
#define MEDIT_NR_OF_INTERACTION_NAMED_PIPES 5		//Valor aleatório (não encontrei um no enunciado)
#define MEDIT_TIMEOUT 10							//Valor em segundos. Possivel de substituir por uma envVar (with the same name)
#define MEDIT_MAXUSERS 3							//Usada caso a envVar nao esteja definida
#define MEDIT_DB_NAME "medit.db"					//Nome por omissao da base de dados
//Defines usados para inicalizar CommonSettingsStruct
#define MEDIT_MAXLINES 15							//Apenas usar se a envVar nao estiver definida (same name)
#define MEDIT_MAXCOLUMNS 45							//Apenas usar se a envVar nao estiver definida (same name)

typedef struct LineStruct{
	int lineNumber;		//Used to know current line (occupiedLine)
	int *column;
	char *username;
} Line;

typedef struct ScreenStruct{
	Line *line;
} Screen;

typedef struct ServerSettingsStruct{
	int nrOfInteractionNamedPipes,
		timeout,
		maxUsers;
	char *dbFilename;
} ServerSettings;

typedef struct MainNamedPipeThreadArgsStruct{
	char* mainNamedPipeName;
	char* dbFilename;
	int maxUsers,
		nrOfInteractionNamedPipes;
	WINDOW* threadEventsWindow;
} MainNamedPipeThreadArgs;

typedef struct InteractionNamedPipeThreadArgsStruct{
	int maxUsers, maxColumns;
	WINDOW* threadEventsWindow;
	char** interactionNamedPipeName;
} InteractionNamedPipeThreadArgs;