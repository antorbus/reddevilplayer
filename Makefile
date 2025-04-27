SRCDIR        := src
METALDIR      := $(SRCDIR)/metal
EXTERNALDIR   := external
MINIAUDIODIR  := $(EXTERNALDIR)/miniaudio/src


CC      := clang
CFLAGS  := -Wall -g -I$(SRCDIR)

LDFLAGS := -lpthread \
           -framework CoreFoundation \
           -framework Carbon \
		   -framework Cocoa \

CURL          := curl -L

SRCS := $(MINIAUDIODIR)/miniaudio.c \
		$(wildcard $(SRCDIR)/*.c) \
		$(wildcard $(METALDIR)/*.c) 		

OBJS := $(SRCS:.c=.o)
CFLAGS +=  -I$(METALDIR) -I$(MINIAUDIODIR) -I/opt/homebrew/include 
LDFLAGS += -L/opt/homebrew/lib -lSDL2

TARGET := RedDevilPlayer

.PHONY: all clean run external

all: external $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

external: $(MINIAUDIODIR)/miniaudio.c \
          $(MINIAUDIODIR)/miniaudio.h 
	@echo "External libraries ready." 


$(EXTERNALDIR):
	@mkdir -p $@

$(MINIAUDIODIR):
	@mkdir -p $@

$(MINIAUDIODIR)/miniaudio.h: | $(MINIAUDIODIR)
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h -o $@

$(MINIAUDIODIR)/miniaudio.c: | $(MINIAUDIODIR)
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.c -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
