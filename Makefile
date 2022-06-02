PROGRAM_NAME = lets-talk

all: lets-talk.c list.c list.h
	gcc -g -Wall -o $(PROGRAM_NAME) lets-talk.c list.c -lpthread -lm
clean: 
	rm $(PROGRAM_NAME)