SRCDIR   = src
BINDIR   = bin
INCLUDES = include

CC=g++
CFLAGS= -Wall -Wextra -g -fno-stack-protector -z execstack -pthread  -std=c++11 -I $(INCLUDES)/   # -lssl  -lcrypto
DEPS = $(wildcard $(INCLUDES)/%.hpp)

all: $(BINDIR)/client $(BINDIR)/server $(DEPS)

$(BINDIR)/client: $(SRCDIR)/client.cpp networking.o
	$(CC) $(CFLAGS) $< networking.o -o $@

$(BINDIR)/server: $(SRCDIR)/server.cpp Parser.o User.o networking.o grass.o
	$(CC) $< Parser.o commands.o User.o networking.o grass.o -o $@ $(CFLAGS)
	rm *.o
	
User.o: $(SRCDIR)/User.cpp $(SRCDIR)/User.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)/User.cpp

Parser.o: $(SRCDIR)/Parser.cpp $(SRCDIR)/Parser.hpp commands.o
	$(CC) $(CFLAGS) -c $(SRCDIR)/Parser.cpp
	
commands.o: $(SRCDIR)/commands.cpp $(SRCDIR)/commands.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)/commands.cpp

networking.o: $(SRCDIR)/networking.cpp $(SRCDIR)/networking.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)/networking.cpp
	
grass.o: $(SRCDIR)/grass.cpp $(SRCDIR)/grass.hpp
	$(CC) $(CFLAGS) -c $(SRCDIR)/grass.cpp
	
.PHONY: clean
clean:
	rm -f $(BINDIR)/client $(BINDIR)/server *.o
