CC      = gcc
CFLAGS  = -Wall -pthread
LDLIBS  = -pthread -lrt
TARGET  = aeropuerto
SRC     = aeropuerto.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET) *.o
