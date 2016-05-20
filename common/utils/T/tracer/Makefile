all: local remote

local:
	make -f Makefile.local

remote:
	make -f Makefile.remote tracer_remote textlog enb vcd

clean:
	make -f Makefile.local clean
	make -f Makefile.remote clean
