
CC      = arm-linux-gnueabihf-gcc
CFLAGS  ?= \
	-O2 -g -Wall \
	--sysroot=$(MAKETOP)
LDFLAGS ?= \
	--sysroot=$(MAKETOP)

INSTALL ?= install
MKDIR   ?= mkdir
RM      ?= rm

TARGETS = dma_alloc_test dma_share_test
DMA_ALLOC_OBJS = dma_alloc_test.o send_fd.o
DMA_SHARE_OBJS = dma_share_test.o send_fd.o

all: $(TARGETS)

install: all
	$(MKDIR) -p $(MAKETOP)/usr/local/bin/
	$(INSTALL) $(TARGETS) $(MAKETOP)/usr/local/bin/

clean:
	$(RM) -f $(TARGETS)
	$(RM) -f $(DMA_ALLOC_OBJS)
	$(RM) -f $(DMA_SHARE_OBJS)

distclean: clean

dma_alloc_test: $(DMA_ALLOC_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(DMA_ALLOC_OBJS)

dma_share_test: $(DMA_SHARE_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(DMA_SHARE_OBJS)

.PHONY: all install clean distclean
