# Variables
CC = gcc
CFLAGS = -Wall -Werror -std=gnu99
OBJ = threads.o
TARGET = program

# Default rule to build the program
all: $(TARGET)

# Rule to build threads.o
threads.o: threads.c threads.h
	$(CC) $(CFLAGS) -c -o $(OBJ) threads.c

# Rule to build the program
$(TARGET): main.c $(OBJ)
	$(CC) $(CFLAGS) main.c $(OBJ) -o $(TARGET) -lpthread

# Rule for cleaning up the directory
clean:
	rm -f $(OBJ) $(TARGET)
