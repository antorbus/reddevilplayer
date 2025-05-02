SRCDIR        := src
METALDIR      := $(SRCDIR)/metal
PLUGINSDIR	  := $(SRCDIR)/plugins
EXTERNALDIR   := external
MINIAUDIODIR  := $(EXTERNALDIR)/miniaudio


CC      := clang
CFLAGS  := -Wall -g -I$(SRCDIR)

LDFLAGS := -lpthread \
           -framework CoreFoundation \
           -framework Carbon \
		   -framework Cocoa \
		   -lvorbisfile -lvorbis -logg \

CURL          := curl -L

SRCS := $(MINIAUDIODIR)/miniaudio.c \
		$(MINIAUDIODIR)/extras/decoders/libvorbis/miniaudio_libvorbis.c \
		$(wildcard $(SRCDIR)/*.c) \
		$(wildcard $(METALDIR)/*.c) \
		$(wildcard $(PLUGINSDIR)/*.c)

OBJS := $(SRCS:.c=.o)
CFLAGS +=  -I$(METALDIR) -I$(MINIAUDIODIR) -I$(PLUGINSDIR) -I/opt/homebrew/include 
LDFLAGS += -L/opt/homebrew/lib -lSDL2

TARGET := RedDevilPlayer

.PHONY: all clean run external

all: external $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

external: $(MINIAUDIODIR)/miniaudio.c \
          $(MINIAUDIODIR)/miniaudio.h \
		  $(MINIAUDIODIR)/extras/decoders/libvorbis/miniaudio_libvorbis.h \
		  $(MINIAUDIODIR)/extras/decoders/libvorbis/miniaudio_libvorbis.c \
		#   $(MINIAUDIODIR)/extras/decoders/libopus/miniaudio_libopus.h \
		#   $(MINIAUDIODIR)/extras/decoders/libopus/miniaudio_libopus.c
	@echo "External libraries ready." 


$(EXTERNALDIR):
	@mkdir -p $@

$(MINIAUDIODIR):
	@mkdir -p $@

$(MINIAUDIODIR)/extras/decoders/libvorbis:
	@mkdir -p $@

# $(MINIAUDIODIR)/extras/decoders/libopus:
# 	@mkdir -p $@

$(MINIAUDIODIR)/miniaudio.h: | $(MINIAUDIODIR)
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h -o $@

$(MINIAUDIODIR)/miniaudio.c: | $(MINIAUDIODIR)
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.c -o $@

$(MINIAUDIODIR)/extras/decoders/libvorbis/miniaudio_libvorbis.c: | $(MINIAUDIODIR)/extras/decoders/libvorbis
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/extras/decoders/libvorbis/miniaudio_libvorbis.c -o $@

$(MINIAUDIODIR)/extras/decoders/libvorbis/miniaudio_libvorbis.h: | $(MINIAUDIODIR)/extras/decoders/libvorbis
	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/extras/decoders/libvorbis/miniaudio_libvorbis.h -o $@

# $(MINIAUDIODIR)/extras/decoders/libopus/miniaudio_libopus.c: | $(MINIAUDIODIR)/extras/decoders/libopus
# 	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/extras/decoders/libopus/miniaudio_libopus.c -o $@

# $(MINIAUDIODIR)/extras/decoders/libopus/miniaudio_libopus.h: | $(MINIAUDIODIR)/extras/decoders/libopus
# 	@$(CURL) https://raw.githubusercontent.com/mackron/miniaudio/master/extras/decoders/libopus/miniaudio_libopus.h -o $@



run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
