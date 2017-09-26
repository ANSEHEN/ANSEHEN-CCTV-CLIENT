#KDIR = /lib/modules/3.18.7-v7/build
KDIR = /lib/modules/4.9.35-v7+/build
MYSQL_LIBS = -L/usr/lib/arm-linux-gnueabihf -lmysqlclient -lpthread -lz -lm -ldl
MYSQL_CFLAGS = -I/usr/include/mysql -DBIG_JOINS=1  -fno-strict-aliasing    -g -DNDEBUG

obj-m := gpio_module.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

all :
	g++ file_client_cctv.cpp -o file_client_cctv -lzmq -pthread $(MYSQL_LIBS) $(MYSQL_CFLAGS) -std=c++11 -lwiringPi
	g++ -std=c++0x detect_cctv.cpp -o detect -pthread `pkg-config opencv --libs` $(MYSQL_LIBS) $(MYSQL_CFLAGS) -lzmq

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean


