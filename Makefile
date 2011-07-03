CFLAGS=-Wall -Werror -g

all: state_machine

state_machine: state_machine.o
	$(CC) -o state_machine state_machine.o -lssl -lcrypto

test: state_machine
	./state_machine 10000 server.pem server.pem
