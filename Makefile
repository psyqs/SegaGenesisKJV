GDK = $(HOME)/SGDK
BIN = $(GDK)/bin
LIB = $(GDK)/lib
SRC = .
RES = res
INCLUDE = $(GDK)/inc
RESCOMP = $(BIN)/rescomp
RES_RES = $(RES)/resources.res
RES_S = $(RES)/resources.s
RES_H = $(RES)/resources.h
RES_OBJ = $(RES)/resources.o

SHELL = /bin/sh
RM = rm -f
CC = $(BIN)/m68k-elf-gcc
AS = $(BIN)/m68k-elf-as
AR = $(BIN)/m68k-elf-ar
LD = $(BIN)/m68k-elf-ld
OBJCPY = $(BIN)/m68k-elf-objcopy

OPTION = -m68000 -Wall -Wextra -std=c99 -ffreestanding
INCS = -I$(INCLUDE) -I$(GDK)/res -I$(RES)
CCFLAGS = $(OPTION) $(INCS) -O3 -fomit-frame-pointer
ASFLAGS = -m68000 --register-prefix-optional
LIBS = -L$(LIB) -lmd -lgcc
LINKFLAGS = -T $(GDK)/md.ld -nostdlib

OBJS = main.o bible_data.o $(RES_OBJ)

.PHONY: all clean

all: bible.bin

main.o: $(RES_H)

bible_data.o: bible_data.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(RES_S) $(RES_H): $(RES_RES) $(RES)/splash.png
	$(RESCOMP) $(RES_RES) $(RES_S)

$(RES_OBJ): $(RES_S)
	$(CC) $(CCFLAGS) -c $(RES_S) -o $(RES_OBJ)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

bible.elf: $(OBJS)
	$(LD) $(LINKFLAGS) $(LIB)/sega.o $(OBJS) $(LIBS) -o $@

bible.bin: bible.elf
	$(OBJCPY) -O binary $< $@

clean:
	$(RM) *.o *.elf *.bin $(RES)/*.o $(RES)/*.s $(RES)/*.h
