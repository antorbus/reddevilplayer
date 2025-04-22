# //make PLATFORM=PLATFORM_DESKTOP RAYLIB_MODULE_AUDIO=FALSE RAYLIB_MODULE_MODELS=FALSE RAYLIB_CONFIG_FLAGS='-DSUPPORT_MODULE_RAUDIO=0 -DSUPPORT_MODULE_RMODELS=0'
# //clang -c miniaudio.h -o miniaudio.o
CC      := clang
CFLAGS  := -Wall -g -I src -I src/metal

LDFLAGS := -lpthread \
           -framework CoreFoundation \
           -framework CoreGraphics \
           -framework Carbon \

SRCDIR   		:= src
METALDIR 		:= $(SRCDIR)/metal
EXTERNALDIR 	:= external
MINIAUDIODIR 	:= $(EXTERNALDIR)/miniaudio

SRCS := $(wildcard $(SRCDIR)/*.c) \
        $(wildcard $(METALDIR)/*.c) \
		$(wildcard $(MINIAUDIODIR)/*.c) 

OBJS := $(SRCS:.c=.o)

CFLAGS += -I$(MINIAUDIODIR)

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
