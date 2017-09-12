#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/timer.h>



//#define  LED1 0 //BCM_GPIO 17
//#define  LED2 3 //BCM_GPIO 22
//#define  LED3 4 //BCM_GPIO 23
//#define  LED4 5 //BCM_GPIO 24


#define BCM_IO_BASE		0x3F000000                   // RaspberryPi 2,3 I/O Peripherals Base 
#define GPIO_BASE		(BCM_IO_BASE + 0x200000)     // GPIO Register Base 
#define GPIO_SIZE       0xB4                         // 0x7E200000 – 0x7E20000B3  

// GPIO Function Set Register
#define GPIO_IN(g)     (*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))) 
#define GPIO_OUT(g)    (*(gpio+((g)/10)) |= (1<<(((g)%10)*3)))

#define GPIO_SET(g)    (*(gpio+7) = 1<<g)            // GPIO Pin Output Set 0 Register
#define GPIO_CLR(g)    (*(gpio+10) = 1<<g)           // GPIO Pin Output Clear 0 Register
#define GPIO_GET(g)    (*(gpio+13)&(1<<g))           // GPIO Pin Level 0 Register
 
#define GPIO_MAJOR 		201
#define GPIO_MINOR 		198
#define GPIO_DEVICE     "gpiomotor"                    // Device Name 
                          
#define GPIO_MOTOR1	17			       // GPIO Pin Number
#define GPIO_MOTOR2	22			       // GPIO Pin Number
#define GPIO_MOTOR3	23			       // GPIO Pin Number
#define GPIO_MOTOR4	24			       // GPIO Pin Number

int GPIO_MOTOR[4]={GPIO_MOTOR1,GPIO_MOTOR2,GPIO_MOTOR3,GPIO_MOTOR4};

volatile unsigned *gpio;                             // I/O 접근을 위한 가상 어드레스 
static char msg[BLOCK_SIZE] = {0};                   // write( ) 함수에서 읽은 데이터 보관 
 
static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);


int StepPins[4]={GPIO_MOTOR1,GPIO_MOTOR2,GPIO_MOTOR3,GPIO_MOTOR4};
int seq[8][4] ={{1,0,0,1},
       		{1,0,0,0},
      		{1,1,0,0},
     		{0,1,0,0},
    		{0,1,1,0},
     		{0,0,1,0},
    		{0,0,1,1},
      		{0,0,0,1}};
      		
      		
//  입출력 함수 처리를 위한 구조체  
static struct file_operations gpio_fops = {
   .owner   = THIS_MODULE,
   .read    = gpio_read,
   .write   = gpio_write,
   .open    = gpio_open,
   .release = gpio_close,
};

struct cdev gpio_cdev;   

static int gpio_open(struct inode *inod, struct file *fil)
{
	//===========================================================
	// 모듈 사용 횟수 카운트 
	// 모듈을 여러곳에서 동시에 사용하고 있는 경우 사용 횟수 카운트를 증가 시킨다.
	//===========================================================
    try_module_get(THIS_MODULE);
    printk("GPIO Device opened(%d:%d)\n", imajor(inod), iminor(inod));
    return 0;
}

static int gpio_close(struct inode *inod, struct file *fil)
{
    printk("GPIO Device closed(%d:%d)\n", imajor(inod), iminor(inod));
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t gpio_read(struct file *inode, char *buff, size_t len, loff_t *off)
{
    int count;
    strcat(msg, " from Kernel");

    //===========================================================
    // 유저 영역으로 데이터를 보낸다 
    //===========================================================
    count = copy_to_user(buff, msg, strlen(msg)+1);     
 
    printk("GPIO Device read : %s(%d)\n", msg, count);
    return count;
}

static ssize_t gpio_write(struct file *inode, const char *buff, size_t len, loff_t *off)
{
    short count;
    memset(msg, 0, BLOCK_SIZE);

    //===========================================================
    // 유저 영역으로 부터 데이터를 가져온다.
     //===========================================================
    count = copy_from_user(msg, buff, len);             

    // 사용자가 보낸 데이터가 0인 경우 LED를 끄고, 0이 아닌 경우 LED를 켠다.
    //(!strcmp(msg, "0"))?GPIO_CLR(GPIO_LED):GPIO_SET(GPIO_LED);
    


	int StepCount = (sizeof(seq)/sizeof(int))/(sizeof(seq[0])/sizeof(int));
	int StepDir =1;

	//int waitTime =10/(float)1000;
	int temp;
	int StepCounter = 0;
	int t=0;


		
/*	while(1)
	{
		if(t<=1980)	
		{*/

			int pin;
			for (pin=7;pin>3;pin--)
			{	
				temp = (1<<pin)&(*msg);
				printk("msg = %d\n1<<pin = %d\ntemp = %d", *msg, 1<<pin, temp);
				if(temp){
					printk("%d = if", pin);
					GPIO_SET(GPIO_MOTOR[7-pin]);
				}
				else{
					printk("%d = else", pin);
					GPIO_CLR(GPIO_MOTOR[7-pin]);
				}

			}
			/*
			StepCounter += StepDir;
		
			if (StepCounter>=StepCount)
		      		StepCounter = 0;
		    	if (StepCounter<0)
	      			StepCounter = StepCount+StepDir;
			
			t++;
		}
		else
		{
			
			int pin;
			for (pin=0;pin<4;pin++)
			{
				if(((1<<pin) && msg) == 1)
					GPIO_SET(GPIO_MOTOR[7-pin]);
				else
					GPIO_CLR(GPIO_MOTOR[7-pin]);
			}
			StepCounter -= StepDir;
		
			if (StepCounter>=StepCount)
		      		StepCounter = 0;
		    	if (StepCounter<0)
	      			StepCounter = StepCount-StepDir;
			
			t++;
				if(t==3960)
					t=0;
		}
		
		
	}
	*/

    printk("GPIO Device write : %s(%d)\n", msg, len);
    return count;
}

int GPIO_init(void)
{
    //===========================================================
    // dev_t 
    // 디바이스 파일의 major와 minor를 지정하기 위한 변수 타입으로 
    // 32 비트의 크기를 갖는다. (major 12bit, minor 20bit)
    //===========================================================
    dev_t devNo;
    unsigned int count;
    static void *map;
    int err;

    //===========================================================
    // insmod를 통해 initModule이 호출되었음을 확인 
    //===========================================================
    printk(KERN_INFO "GPIO_init\n");
	
    

    //===========================================================
    // major와 minor를 전달하고 디바이스 파일의 번호를 받는다.
    // Major 200, Minor 199의 경우 devNo=0x0c8000c7이다. 
    //===========================================================
    devNo = MKDEV(GPIO_MAJOR, GPIO_MINOR);
    printk(KERN_INFO"devNo=0x%x\n", devNo);

    //===========================================================
    // char device 등록
    //===========================================================
    register_chrdev_region(devNo, 1, GPIO_DEVICE);

    //===========================================================
    // char device 구조체 초기화 
    // register_chrdev_region 와 alloc_chrdev_region 를 이용하면
    // major 와 minor 예약은 할 수 있지만 cdev(character device)를 
    // 생성 하지는 않는다. 별도로 cdev_init, cdev_add 함수를 호출하여야 한다.
    //===========================================================
    cdev_init(&gpio_cdev, &gpio_fops);

    gpio_cdev.owner = THIS_MODULE;
    count = 1;
    err = cdev_add(&gpio_cdev, devNo, count);
    if (err < 0) {
        printk("Error : Device Add\n");
        return -1;
    }

    //===========================================================
    // 물리 주소 번지를 주면 가상 주소 번지를 알려준다. 
    //===========================================================
    map = ioremap(GPIO_BASE, GPIO_SIZE);              
    if(!map) {
        printk("Error : mapping GPIO memory\n");
        iounmap(map);
        return -EBUSY;
    }

    //===========================================================
    // gpio는 GPIO_BASE의 가상 주소 번지 
    //===========================================================
    gpio = (volatile unsigned int *)map; //volatile : 캐시에 한번들어가는 것은 다시 들어가지 않음
    					//레지스터에 있는 실질적인 값이 아닌 캐시에 있는 것을 읽어오는 것을
    					//방지하기위해서.
    					// cpu는 자신이 바꾸지 않았다면 레지스터의 값이
    					//변경되었을 때 인지하지 못함. 캐시에 있는 것을 참조함.
    					//이때 캐시가 아닌 레지스터의 값을 항상 참조하라는 의미에서
    					//volatile을 붙임. 

    GPIO_IN(GPIO_MOTOR1);
    GPIO_IN(GPIO_MOTOR2);
    GPIO_IN(GPIO_MOTOR3);
    GPIO_IN(GPIO_MOTOR4);
    GPIO_OUT(GPIO_MOTOR1);
    GPIO_OUT(GPIO_MOTOR2);
    GPIO_OUT(GPIO_MOTOR3);
    GPIO_OUT(GPIO_MOTOR4);

    return 0;
}

void GPIO_exit(void)
{
    dev_t devNo = MKDEV(GPIO_MAJOR, GPIO_MINOR);
    //===========================================================
    // 문자 디바이스의 등록을 해제한다.
    //===========================================================   
    unregister_chrdev_region(devNo, 1);

    //===========================================================
    // 문자 디바이스의 구조체를 해제한다.
    //===========================================================
    cdev_del(&gpio_cdev);                                

    if (gpio) {
        //=======================================================
        // 매핑된 메모리를 삭제한다.
        //=======================================================
        iounmap(gpio);                                          
    }

    printk(KERN_INFO "GPIO_exit\n");
}

module_init(GPIO_init);
module_exit(GPIO_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heejin Park");
MODULE_DESCRIPTION("Raspberry Pi 3 GPIO Device Module");
