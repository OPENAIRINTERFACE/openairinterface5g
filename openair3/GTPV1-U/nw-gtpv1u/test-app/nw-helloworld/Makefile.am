bin_PROGRAMS = helloworld

AM_CFLAGS = -D__WITH_LIBEVENT__ -I../../shared -I../../include -I../../test-app/nw-udp -I../../test-app/nw-log @AM_CFLAGS@

helloworld_SOURCES =    \
			NwEvt.h\
			NwMiniLogMgrEntity.h\
			NwMiniTmrMgrEntity.h\
			NwMiniUlpEntity.h\
			NwMiniUdpEntity.h\
			NwMiniLogMgrEntity.c\
			NwMiniTmrMgrEntity.c\
			NwMiniUlpEntity.c\
			NwMiniUdpEntity.c\
                        helloworld.c

helloworld_LDADD =  ../../src/libNwGtpv1u.a -levent $(AM_LDFLAGS)
