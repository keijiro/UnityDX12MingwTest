#
# File listings
#

TARGET = Plugin.dll

DEST = ../Assets/

SRCS = Plugin.c

OBJS = $(SRCS:.c=.o)

LIBS = -Wl,--subsystem,windows -static -ld3d11

#
# Compiler/linker options
#

TOOLCHAIN = x86_64-w64-mingw32
CC = $(TOOLCHAIN)-gcc-posix
STRIP = $(TOOLCHAIN)-strip

CC_FLAGS = -O2 -I. -std=c11 -Wall

LD_FLAGS = -shared

#
# Building rules
#

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

copy: all
	cp $(TARGET) $(DEST)

$(TARGET): $(OBJS)
	$(CC) $(LD_FLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $@

%.o: %.c
	$(CC) $(CC_FLAGS) -c -o $@ $<
