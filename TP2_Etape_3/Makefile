CC = gcc
CFLAGS = -Wall -Werror -g -DTRACE
LDFLAGS = -lreadline

TARGET = biceps
OBJS = biceps.o gescom.o creme.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

biceps.o: biceps.c gescom.h
	$(CC) $(CFLAGS) -c biceps.c

gescom.o: gescom.c gescom.h creme.h
	$(CC) $(CFLAGS) -c gescom.c

creme.o: creme.c creme.h
	$(CC) $(CFLAGS) -c creme.c

clean:
	rm -f $(OBJS) $(TARGET)
