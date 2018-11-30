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
	char sent[12] = "xsdijnmklcx";
	mkfifo("conaDaPrima", 0600);
	if((fd = open("conaDaPrima", O_WRONLY)) >= 0){
		printf("Antes do getchar()\n");
		getchar();
		write(fd, &sent, 12);
	}
	return 0;
}