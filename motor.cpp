#include<stdio.h>
#include<wiringPi.h>
#include<stdlib.h>
#include<unistd.h>
#define  LED1 0 //BCM_GPIO 17
#define  LED2 3 //BCM_GPIO 22
#define  LED3 4 //BCM_GPIO 23
#define  LED4 5 //BCM_GPIO 24

int StepPins[4]={LED1,LED2,LED3,LED4};
int seq[8][4] ={{1,0,0,1},
       		{1,0,0,0},
      		{1,1,0,0},
     		{0,1,0,0},
    		{0,1,1,0},
     		{0,0,1,0},
    		{0,0,1,1},
      		{0,0,0,1}};

int main(void)
{
	if(wiringPiSetup() ==-1)
		return 1;

	pinMode(LED1,OUTPUT);
	pinMode(LED2,OUTPUT);
	pinMode(LED3,OUTPUT);
	pinMode(LED4,OUTPUT);

	int StepCount = (sizeof(seq)/sizeof(int))/(sizeof(seq[0])/sizeof(int));
	printf("StepCount = %d\n",StepCount);
	int StepDir =1;

	int waitTime =10/(float)1000;

	int StepCounter = 0;
	int t=0;


		
	while(1)
	{
		if(t<=2000)	
		{
			printf("StepCounter = %d\n",StepCounter);
	
			
			int pin,input;
			for (pin=0;pin<4;pin++)
			{
				digitalWrite(StepPins[pin],seq[StepCounter][pin]);
				printf("StepPin[]= %d\n",StepPins[pin]);
				printf("seq[] = %d\n",seq[StepCounter][pin]);
			}
			StepCounter += StepDir;
		
			if (StepCounter>=StepCount)
		      		StepCounter = 0;
		    	if (StepCounter<0)
	      			StepCounter = StepCount+StepDir;
			usleep(10000);
			t++;
		}
		else
		{
			printf("StepCounter = %d\n",StepCounter);
	
			
			int pin,input;
			for (pin=0;pin<4;pin++)
			{
				digitalWrite(StepPins[pin],seq[StepCounter][pin]);
				printf("StepPin[]= %d\n",StepPins[pin]);
				printf("seq[] = %d\n",seq[StepCounter][pin]);
			}
			StepCounter -= StepDir;
		
			if (StepCounter>=StepCount)
		      		StepCounter = 0;
		    	if (StepCounter<0)
	      			StepCounter = StepCount-StepDir;
			usleep(10000);
			t++;
				if(t==4000)
					t=0;
		}
		
		
	}
	return 0;
}


