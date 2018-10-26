#include <stdio.h>
#include "../medit-defaults.h"

void InitSettingsStruct(CommonSettings* settings){
	(*settings).maxLines = MEDIT_MAXLINES;
	(*settings).maxColumns = MEDIT_MAXCOLUMNS;
	(*settings).mainNamedPipeName = MEDIT_MAIN_NAMED_PIPE_NAME;
}

int main(int argc, char const *argv[])
{
	CommonSettings settings;
	InitSettingsStruct(&settings);
	return 0;
}