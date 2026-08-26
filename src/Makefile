CC=gcc
CFLAGS=-Wall -O3 -std=gnu99 -I.
LDFLAGS=-lpthread -lcurl -lm

TARGET=deskclock
SRCS=main.c tft.c max7219.c weather.c anim.c camera.c video.c config.c data.c
OBJS=$(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
