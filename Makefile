##################################################
## Welcome to the nuphase-ice-software Makefile ## 
##################################################

###################### Things you may need to change#################
# Location of libnuphase
LIBNUPHASE_DIR=../libnuphase

#installation prefix for programs 
PREFIX=/nuphase

# location for config files 
NUPHASE_CONFIG_DIR=${PREFIX}/cfg 

####################################################################
# Things not meant to be changed 
####################################################################


CFLAGS +=-g -O2 -Iinclude -Wall -I$(LIBNUPHASE_DIR) -D_GNU_SOURCE
LDFLAGS+=-L$(LIBNUPHASE_DIR) -lnuphase -lnuphasedaq  -lz -lpthread -lconfig -lrt

CC=gcc 
BUILDDIR=build
INCLUDEDIR=include
BINDIR=bin

.PHONY: clean install all doc default-configs

OBJS:= $(addprefix $(BUILDDIR)/, nuphase-buf.o nuphase-common.o nuphase-cfg.o )
PROGRAMS := $(addprefix $(BINDIR)/, nuphase-acq nuphase-startup nuphase-hk nuphase-copy \
																		nuphase-make-default-config nuphase-check-config  nuphase-current-hk\
																		nuphase-set-saved-thresholds)
INCLUDES := $(addprefix $(INCLUDEDIR)/, $(shell ls $(INCLUDEDIR)))

all: $(PROGRAMS) 

etc/nuphase.cfg: 
	mkdir -p etc 
	echo NUPHASE_PATH=${PREFIX}/bin > etc/nuphase.cfg 
	echo NUPHASE_CONFIG_DIR=${NUPHASE_CONFIG_DIR}  >> etc/nuphase.cfg 


default-configs: $(BINDIR)/nuphase-make-default-config 
	$(BINDIR)/nuphase-make-default-config acq cfg/acq.cfg
	$(BINDIR)/nuphase-make-default-config startup cfg/startup.cfg
	$(BINDIR)/nuphase-make-default-config hk cfg/hk.cfg
	$(BINDIR)/nuphase-make-default-config copy cfg/copy.cfg



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


install: $(PROGRAMS) $(INCLUDES) etc/nuphase.cfg 
	install -d $(PREFIX)
	install -d $(PREFIX)/bin
	install $(PROGRAMS) $(PREFIX)/bin
	install -d $(PREFIX)/cfg
	cp cfg/* $(PREFIX)/cfg 
	install etc/nuphase.cfg /etc
	install -d $(PREFIX)/include
	install $(INCLUDES) $(PREFIX)/include 
	cp systemd/* /etc/systemd/system/
	cp scripts/* $(PREFIX)/bin


clean: 
	rm -rf build
	rm -rf bin
	rm -rf etc 






