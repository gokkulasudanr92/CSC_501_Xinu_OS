###############################################
#Macros

TARGET = string_search


##############################################
all: string_search.o
	gcc string_search.o -o ${TARGET} -pthread
    
string_search.o: string_search.c
	gcc -Wall -c string_search.c -pthread

clean:
	rm -rf *.o
	rm ${TARGET}
