CC = gcc

# compiler flags:
#  -g    debug info
#  -Wall most warnings
CFLAGS  = -lreadline -lsqlite3


TARGET = ctodo

all: $(TARGET)

$(TARGET): main.o todo.o
	$(CC) -o $(TARGET) main.c todo.o $(CFLAGS)

clean:
	$(RM) $(TARGET)
