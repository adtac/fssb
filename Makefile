components = fssb.o \
			 arguments.o \
			 utils.o \
			 proxyfile.o

all: $(components)
	cc -o fssb $(components) -lcrypto

fssb.o: fssb.c
arguments.o: arguments.c
utils.o: utils.c
proxyfile.o: proxyfile.c

clean:
	rm -rf *.o
	rm -rf fssb
