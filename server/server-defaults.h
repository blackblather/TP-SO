#define MEDIT_NR_OF_INTERACTION_NAMED_PIPES 5		//Valor aleatório (não encontrei um no enunciado)
#define MEDIT_TIMEOUT 10							//Valor em segundos. Possivel de substituir por uma envVar (with the same name)
#define MEDIT_MAXUSERS 3							//Usada caso a envVar nao esteja definida
#define MEDIT_DB_NAME "medit.db"					//Nome por omissao da base de dados

typedef struct LineStruct{
	char *column;
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