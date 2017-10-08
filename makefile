CC = c99
CFLAGS = -g -Wall -Wextra -pedantic
OBJS = k.o

kfreestyle2d: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o kfreestyle2d
