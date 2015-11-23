VCD Plugin v0.1

The VCD plugin outputs signals to a .vcd file which is readable by gtkwave (http://en.wikipedia.org/wiki/GTKWave).
Signals may have two types:
- Variables to dump the state of a variable (range 0..2^64 - 1).
- Functions to log function usage (implies two call to VCD API: IN and OUT)

This module is always built in, but is only used when -V oaisim option is added.
The VCD file generated is called openair_vcd_dump.vcd and is located in the execution folder.

Usage:
1) To dump function usage:
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(FUNCTION_NAME, VCD_FUNCTION_IN); at start of function
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(FUNCTION_NAME, VCD_FUNCTION_OUT); at end of function
    FUNCTION_NAME should be defined in the enum in vcd_signal_dumper.h and the function name should be addded at the right place in the vcd_signal_dumper.c file.

2) To dump a variable:
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VARIABLE_NAME, value); where VARIABLE_NAME is defined as described above for functions and value is the state of the variable at log time.

TODO list:
- Add support for signed variables
- Create more than two module (for now: one for variable, one for functions)
- Integrate date in filename
- Use a more accurate clock (if necessary) or use a host specific clock (for other systems than Linux)

Useful link:
http://www.beyondttl.com/vcd.php
