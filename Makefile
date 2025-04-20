CC      := clang
CFLAGS  := -Wall -g -I src -I src/metal

LDFLAGS := -lpthread \
           -framework CoreFoundation \
           -framework CoreGraphics \
           -framework AudioToolbox \
		   -framework CoreAudio \
           -framework Carbon \

SRCDIR   := src
METALDIR := $(SRCDIR)/metal

SRCS := $(wildcard $(SRCDIR)/*.c) \
        $(wildcard $(METALDIR)/*.c)

OBJS := $(SRCS:.c=.o)

TARGET  := RedDevilPlayer

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
