#!/bin/sh
valgrind --tool=memcheck --leak-check=full --show-reachable=yes --num-callers=20 --track-origins=yes --track-fds=yes ./objs/OAISIM_MME/oaisim_mme 2>leakage