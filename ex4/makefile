testlist : testlist.o list.o test1.o
	gcc -Wall -g -o $@ $^

%.o : %.c 
	gcc -Wall -g -c $^

list.o : list.c list.h
	gcc -Wall -g -c $<

test1.o : test1.c list.h
	gcc -Wall -g -c $<

clean :
	rm -f *.o testlist


