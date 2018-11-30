#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

void YOLO(int SWAG){
	printf("SIGINT RECIEVE BLYAT! >:(\n");
}

int main(int argc, char const *argv[])
{
	signal(SIGPIPE, YOLO);
	int fd;
	char recieved;
	mkfifo("conaDaPrima", 0600);
	if((fd = open("conaDaPrima", O_RDONLY)) >= 0){
		printf("Antes do getchar()\n");
		getchar();
		while(read(fd, &recieved, 1))
			printf("-> %c\n", recieved);
	}
	return 0;
}