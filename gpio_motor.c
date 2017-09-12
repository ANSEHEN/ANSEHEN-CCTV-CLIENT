#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(){
	char a = (1<<7);
	char b = (1<<7) + (1<<6);
	char c = (1<<6);
	char d = (1<<6) + (1<<5);
	char e = (1<<5);
	char f = (1<<5) + (1<<4);
	char g = (1<<4);
	char h = (1<<4) + (1<<7);
	
	int fd;
	int t=0;
	fd = open("/dev/gpiomotor", O_RDWR);
	
	while(1){
		if(t<=250)
		{
			write(fd, &a, sizeof(char), NULL);
			usleep(10000);
			write(fd, &b, sizeof(char), NULL);
			usleep(10000);
			write(fd, &c, sizeof(char), NULL);
			usleep(10000);
			write(fd, &d, sizeof(char), NULL);
			usleep(10000);
			write(fd, &e, sizeof(char), NULL);
			usleep(10000);
			write(fd, &f, sizeof(char), NULL);
			usleep(10000);
			write(fd, &g, sizeof(char), NULL);
			usleep(10000);
			write(fd, &h, sizeof(char), NULL);
			usleep(10000);
			t++;
		}
		else
		{
			write(fd, &h, sizeof(char), NULL);
			usleep(10000);
			write(fd, &g, sizeof(char), NULL);
			usleep(10000);
			write(fd, &f, sizeof(char), NULL);
			usleep(10000);
			write(fd, &e, sizeof(char), NULL);
			usleep(10000);
			write(fd, &d, sizeof(char), NULL);
			usleep(10000);
			write(fd, &c, sizeof(char), NULL);
			usleep(10000);
			write(fd, &b, sizeof(char), NULL);
			usleep(10000);
			write(fd, &a, sizeof(char), NULL);
			usleep(10000);
			t++;
				if(t==500)
					t=0;
		}
	}
	

	return 0;
}