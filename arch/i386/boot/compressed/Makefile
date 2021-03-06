#
# linux/arch/i386/boot/compressed/Makefile
#
# create a compressed vmlinux image from the original vmlinux
#

HEAD = head.o
SYSTEM = $(TOPDIR)/vmlinux

OBJECTS = $(HEAD) inflate.o unzip.o misc.o

CFLAGS = -O2 -DSTDC_HEADERS

ifdef CONFIG_KERNEL_ELF
TARGET=--target elf32-i386
INPUT_DATA=input_data
INPUT_LEN=input_len
endif

all: vmlinux

vmlinux: piggy.o $(OBJECTS)
	$(LD) $(ZLINKFLAGS) -o vmlinux $(OBJECTS) piggy.o

head.o: head.S $(TOPDIR)/include/linux/tasks.h
	$(CC) -traditional -c head.S

ifdef CONFIG_KERNEL_ELF

# You cannot compress a file and have the kernel uncompress it, it must
# be stdin
piggy.o:	$(SYSTEM)
	tmppiggy=/tmp/$$.piggy; \
	rm -f $$tmppiggy $$tmppiggy.gz; \
	$(OBJDUMP) $(OBJDUMP_FLAGS) -o $(IMAGE_OFFSET) $(SYSTEM) > $$tmppiggy; \
	gzip -f -9 < $$tmppiggy > $$tmppiggy.gz; \
	encaps $(TARGET) piggy.o $$tmppiggy.gz $(INPUT_DATA) $(INPUT_LEN); \
	rm -f $$tmppiggy $$tmppiggy.gz
else

piggy.o:	$(SYSTEM) xtract piggyback
		./xtract $(SYSTEM) | gzip -9 | ./piggyback > piggy.o

xtract: xtract.c
	$(HOSTCC) $(CFLAGS) -o xtract xtract.c

piggyback: piggyback.c
	$(HOSTCC) $(CFLAGS) -o piggyback piggyback.c

endif

clean:
	rm -f xtract piggyback vmlinux
