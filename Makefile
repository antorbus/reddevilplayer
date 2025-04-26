# //make PLATFORM=PLATFORM_DESKTOP RAYLIB_MODULE_AUDIO=FALSE RAYLIB_MODULE_MODELS=FALSE RAYLIB_CONFIG_FLAGS='-DSUPPORT_MODULE_RAUDIO=0 -DSUPPORT_MODULE_RMODELS=0'
# //clang -c miniaudio.h -o miniaudio.o
SRCDIR        := src
METALDIR      := $(SRCDIR)/metal
EXTERNALDIR   := external
MINIAUDIODIR  := $(EXTERNALDIR)/miniaudio/src
RAYLIBDIR     := $(EXTERNALDIR)/raylib
RAYGUIDIR     := $(EXTERNALDIR)/raygui/src

CC      := clang
CFLAGS  := -Wall -g -I $(SRCDIR)

LDFLAGS := -lpthread \
           -framework CoreFoundation \
           -framework Carbon \
		   -framework OpenGL \
		   -framework CoreVideo \
		   -framework Cocoa \
		   -framework IOKit \


RAYLIB_REPO   := https://github.com/raysan5/raylib.git
CURL          := curl -L

SRCS := $(MINIAUDIODIR)/miniaudio.c \
		$(wildcard $(SRCDIR)/*.c) \
		$(wildcard $(METALDIR)/*.c) 		

OBJS := $(SRCS:.c=.o)

CFLAGS +=  -I$(METALDIR) -I$(MINIAUDIODIR) -I$(RAYLIBDIR)/src -I$(RAYGUIDIR)

LFLAGS := $(RAYLIBDIR)/src/libraylib.a

TARGET := RedDevilPlayer

.PHONY: all clean run external

all: external raylib $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

external: $(RAYLIBDIR) \
          $(MINIAUDIODIR)/miniaudio.c \
          $(MINIAUDIODIR)/miniaudio.h \
          $(RAYGUIDIR)/raygui.h
	@echo "External libraries ready."

raylib:
	$(MAKE) -C $(RAYLIBDIR)/src \
	  PLATFORM=PLATFORM_DESKTOP \
	  RAYLIB_MODULE_AUDIO=FALSE \
	  RAYLIB_MODULE_MODELS=FALSE \
	  RAYLIB_MODULE_RAYGUI=TRUE\
	  RAYLIB_CONFIG_FLAGS="-DSUPPORT_MODULE_RAUDIO=0 -DSUPPORT_MODULE_RMODELS=0"

$(EXTERNALDIR):
	@mkdir -p $@

$(RAYLIBDIR): | $(EXTERNALDIR)
	@git clone --depth 1 $(RAYLIB_REPO) $@ \
	
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
