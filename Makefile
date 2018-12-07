CC = gcc
SRCDIRECTORY = src
TARGET = oss user
LIBS = -pthread
all: $(TARGET)

oss:$(SRCDIRECTORY)/oss.c
	$(CC) -o oss $< $(LIBS)

user:$(SRCDIRECTORY)/user.c
	$(CC) -o user $< $(LIBS)

clean:
	/bin/rm -f *.o $(TARGET)
