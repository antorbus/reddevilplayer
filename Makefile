# //make PLATFORM=PLATFORM_DESKTOP RAYLIB_MODULE_AUDIO=FALSE RAYLIB_MODULE_MODELS=FALSE RAYLIB_CONFIG_FLAGS='-DSUPPORT_MODULE_RAUDIO=0 -DSUPPORT_MODULE_RMODELS=0'
# //clang -c miniaudio.h -o miniaudio.o
CC      := clang
CFLAGS  := -Wall -g -I src -I src/metal 

LDFLAGS := -lpthread \
           -framework CoreFoundation \
           -framework Carbon \
		   -framework OpenGL \
		   -framework CoreVideo \
		   -framework Cocoa \
		   -framework IOKit \

SRCDIR        := src
METALDIR      := $(SRCDIR)/metal
EXTERNALDIR   := external
MINIAUDIODIR  := $(EXTERNALDIR)/miniaudio
RAYLIBDIR     := $(EXTERNALDIR)/raylib/src
RAYGUIDIR     := $(EXTERNALDIR)/raygui

RAYLIB_REPO   := https://github.com/raysan5/raylib.git
CURL          := curl -L

SRCS := $(wildcard $(SRCDIR)/*.c) \
		$(wildcard $(METALDIR)/*.c) \
		$(MINIAUDIODIR)/miniaudio.c 

OBJS := $(SRCS:.c=.o)

CFLAGS += -I$(MINIAUDIODIR) -I$(RAYLIBDIR) -I$(RAYGUIDIR)

LFLAGS := $(RAYLIBDIR)/libraylib.a

TARGET := RedDevilPlayer

.PHONY: all clean run external

all: external $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

external: $(RAYLIBDIR) \
          $(MINIAUDIODIR)/miniaudio.c \
          $(MINIAUDIODIR)/miniaudio.h \
          $(RAYGUIDIR)/raygui.h
	@echo "External libraries ready."

$(EXTERNALDIR):
	@mkdir -p $@

$(RAYLIBDIR): | $(EXTERNALDIR)
	@git clone --depth 1 $(RAYLIB_REPO) $@

$(MINIAUDIODIR):
	@mkdir -p $@

$(MINIAUDIODIR)/miniaudio.h: | $(MINIAUDIODIR)
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h -o $@

$(MINIAUDIODIR)/miniaudio.c: | $(MINIAUDIODIR)
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.c -o $@

$(RAYGUIDIR):
	@mkdir -p $@

$(RAYGUIDIR)/raygui.h: | $(RAYGUIDIR)
	@$(CURL) https://raw.githubusercontent.com/raysan5/raygui/master/src/raygui.h -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
