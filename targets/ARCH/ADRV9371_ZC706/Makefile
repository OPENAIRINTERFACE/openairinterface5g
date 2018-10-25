#			Creates shared library

CC=			gcc
CFLAGS+=	-DADRV9371_ZC706 -DHWLAT -O0 -g3 -Wall -fmessage-length=0 -fPIC

DROOT=		./USERSPACE
DIR=		$(DROOT)/LIB

IFLAGS+=	-I$(DIR)/								\
			-I$(DROOT)/libini/						\
			-I../COMMON/							\
			-I/usr/include/

LDFLAGS+=	-L/usr/lib/x86/64-linux-gnu/			\
			-lm -liio -ldl -lriffa

OUTDIR=		./slib
TARGET=		libadrv9371_zc706.so

SRC=		$(DIR)/adrv9371_dump.c					\
			$(DIR)/adrv9371_helper.c				\
			$(DIR)/adrv9371_self_test.c				\
			$(DIR)/adrv9371_zc706_lib.c				\
			$(DIR)/adrv9371_zc706_riffa_lib.c		\
			$(DIR)/adrv9371_zc706_stats.c			\
			$(DIR)/hw_init.c			\
			$(DROOT)/libini/libini.c

OBJ=		$(OUTDIR)/adrv9371_dump.o				\
			$(OUTDIR)/adrv9371_helper.o				\
			$(OUTDIR)/adrv9371_self_test.o			\
			$(OUTDIR)/adrv9371_zc706_lib.o			\
			$(OUTDIR)/adrv9371_zc706_riffa_lib.o	\
			$(OUTDIR)/adrv9371_zc706_stats.o		\
			$(OUTDIR)/hw_init.o		\
			$(OUTDIR)/libini.o

all:	lib

dir:
	mkdir -p $(OUTDIR)

env:
	set PATH=/usr/lib/:${PATH}

obj:	dir env
	$(CC) -c $(CFLAGS) $(IFLAGS) $(SRC) 
	mv *.o $(OUTDIR)

lib:	dir obj
	$(CC) -shared $(OBJ) -o $(OUTDIR)/$(TARGET) $(LDFLAGS)

# exec: 	env
#	$(CC) $(CFLAGS) $(IFLAGS) main.c $(SRC) -o lib.exe $(LDFLAGS) -lpthread

.PHONY:	clean

clean:	
	rm -f $(OUTDIR)/*.o $(OUTDIR)/*~

rm:		clean
	rm -f $(OUTDIR)/$(TARGET)
	rmdir $(OUTDIR)