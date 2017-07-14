
LIBNUPHASE_DIR=..
PREFIX=/usr/local 
CFLAGS +=-g -Iinclude -Wall -I$(LIBNUPHASE_DIR)/libnuphase
LDFLAGS+=-L$(LIBNUPHASE_DIR)/libnuphase -lnuphase -lz 

CC=gcc 
BUILDDIR=build
INCLUDEDIR=include
BINDIR=bin

.PHONY: clean install all doc

OBJS:= $(addprefix $(BUILDDIR)/, nuphase-buf.o )
PROGRAMS := $(addprefix $(BINDIR)/, nuphase-acq )
INCLUDES := $(addprefix $(INCLUDEDIR)/, $(shell ls $(INCLUDEDIR)))

all: $(PROGRAMS) 

$(BUILDDIR): 
	mkdir -p $(BUILDDIR)

$(BINDIR): 
	mkdir -p $(BINDIR)

$(BUILDDIR)/%.o: src/%.c $(INCLUDES) Makefile | $(BUILDDIR)
	@echo Compiling  $< 
	@$(CC)  $(CFLAGS) -o $@ -c $< 

$(BINDIR)/%: src/%.c $(INCLUDES) $(OBJS) Makefile | $(BINDIR)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $< $(OBJS) -o $@ -L./$(LIBDIR) $(LDFLAGS) 


install: 


clean: 
	rm -rf build
	rm -rf bin






