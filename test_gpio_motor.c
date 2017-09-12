#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(){
	int a[8] = {(1<<7),(1<<7) + (1<<6),(1<<6),(1<<6) + (1<<5),(1<<5),(1<<5) + (1<<4),(1<<4),(1<<4) + (1<<7)};
	

	
	int fd;
	int t=0;
	int i,j;
	fd = open("/dev/gpiomotor", O_RDWR);
	
	while(1)
	{
		if(t<=250)
		{
			for(i=0;i<8;i++)
			{
				write(fd, &a[i], sizeof(char), NULL);
				usleep(10000);
			}
			t++;
			printf("if t=%d\n",t);
		}
		else
		{
			for(j=7;j>=0;j--)
			{
				write(fd, &a[j], sizeof(char), NULL);
				usleep(10000);

			}
			printf("if t=%d\n",t);
			t++;
			if(t==500)
				t=0;

		}
	}
	

	return 0;
}