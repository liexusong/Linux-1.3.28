#
# Makefile for the Linux TCP/IP (INET) layer.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

O_TARGET := core.o

O_OBJS	:= sock.o skbuff.o iovec.o datagram.o

ifdef CONFIG_NET
O_OBJS	+= dev.o dev_mcast.o
endif

include $(TOPDIR)/Rules.make

tar:
		tar -cvf /dev/f1 .
