CC65_TARGET = none

ifndef CC65_HOME
$(warning CC65_HOME not set. Defaulting to /usr/share/cc65)
export CC65_HOME = /usr/share/cc65
endif

SOURCES = main.c tui.c persist.c filemanager.c
ASOURCES = display.s keyboard.s irq.s timings.s vectors.s versions.s tui_asm.s snapshot.s

LSOURCES = $(wildcard libsrc/*.c)
LASOURCES = $(wildcard libsrc/*.s)

PROGRAM = locirom
LIBRARY = loci.lib

word-dot = $(word $2,$(subst ., ,$1))

ifdef VERSION 
export VERSION
CFLAGS = -D VERSION
VERDEFS = --asm-define VERSION --asm-define VERMAJOR=$(call word-dot,$(VERSION),1) --asm-define VERMINOR=$(call word-dot,$(VERSION),2) --asm-define VERPATCH=$(call word-dot,$(VERSION),3)
else
VERDEFS = --asm-define VERMAJOR=0 --asm-define VERMINOR=0 --asm-define VERPATCH=0
endif


ifneq ($(wildcard $(CC65_HOME)/bin/.*),)
CC      = $(CC65_HOME)/bin/cl65
AR      = $(CC65_HOME)/bin/ar65
else
CC      = cl65
AR      = ar65
endif

CFLAGS  +=  -t $(CC65_TARGET) -O --debug-info -I ./include --asm-include-dir ./asminc -I ./asminc
LDFLAGS =  -C cfg/loci.cfg --debug-info -m $(PROGRAM).map
CP      = cp -f

#CC65 has changed internal library prefix for errno related symbols. Check which one we have.
CHECK_CC65 != CC65_HOME=$(CC65_HOME) $(CC) -c $(CFLAGS) -o .asm_check libsrc/mia.s 2>&1

.SUFFIXES:
.PHONY: all clean
all: $(PROGRAM).rp6502

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.s
ifneq ($(strip $(CHECK_CC65)),)
	$(CC) -c $(CFLAGS) $(VERDEFS) --asm-define OLD_CC65 -o $@ $<
else
	$(CC) -c $(CFLAGS) $(VERDEFS) -o $@ $<
endif

$(LIBRARY): $(LSOURCES:.c=.o) $(LASOURCES:.s=.o)
	$(CP) $(CC65_HOME)/lib/$(CC65_TARGET).lib $(LIBRARY)
	$(AR) a $(LIBRARY) $(LSOURCES:.c=.o) $(LASOURCES:.s=.o)

$(PROGRAM): $(ASOURCES:.s=.o) $(SOURCES:.c=.o) $(LIBRARY)
	$(CC) $(LDFLAGS) -o $@ $^

$(PROGRAM).rp6502: $(PROGRAM)
	../tools/rp6502.py -o $(PROGRAM).rp6502 -a 0xC000 create $(PROGRAM)

clean:
	$(RM) $(SOURCES:.c=.o) $(ASOURCES:.s=.o) $(LSOURCES:.c=.o) $(LASOURCES:.s=.o) $(SOURCES:.c=.d) $(PROGRAM) $(PROGRAM).map $(PROGRAM).rp6502 $(LIBRARY) .asm_check
