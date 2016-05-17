all: local remote

local:
	make -f Makefile.local

remote:
	make -f Makefile.remote
	make -f Makefile.remote textlog
	make -f Makefile.remote enb

clean:
	make -f Makefile.local clean
	make -f Makefile.remote clean
