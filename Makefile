.PHONY:all
CC=g++
CFLAGS=-Wall -pthread
srcs := src/
bins := bin/
inc  := include/
name1 := server
name2 := client

all: bin $(bins)$(name1) bin $(bins)$(name2) 

bin:
	mkdir bin

$(bins)%: $(srcs)%.cpp $(bins)
	$(CC) $< $(CFLAGS) -o $@
clean:
	-rm -f $(bins)$(name1) $(bins)$(name2)
	-rm -rf bin
