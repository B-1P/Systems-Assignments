PORT=57171
CFLAGS=-DPORT=\$(PORT) -g -Wall

all : xmodemserver


xmodemserver : xmodemserver.o crc16.o helper.o wrappers.o
	gcc $(CFLAGS) -o $@ $^

%.o : %.c
	gcc $(CFLAGS) -c $^

clean : 
	rm *.o xmodemserver
