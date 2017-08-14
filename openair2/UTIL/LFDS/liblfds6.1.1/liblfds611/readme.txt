introduction
============
Welcome to liblfds, a portable, license-free, lock-free data structure library
written in C.

supported platforms
===================
Out-of-the-box ports are provided for;

Operating System  CPU            Toolchain Choices
================  =============  =================
Windows 64-bit    x64            1. Microsoft Visual Studio
                                 2. Microsoft Windows SDK and GNUmake

Windows 32-bit 	  x64, x86       1. Microsoft Visual Studio
                                 2. Visual C++ Express Edition
                                 3. Microsoft Windows SDK and GNUmake

Windows Kernel    x64, x86       1. Windows Driver Kit

Linux 64-bit      x64            1. GCC and GNUmake

Linux 32-bit      ARM, x64, x86  1. GCC and GNUmake

For more information including version requirements, see the building guide (lfds).

data structures
===============
This release of liblfds provides the following;

    * Freelist
    * Queue
    * Ringbuffer (each element read by a single reader)
    * Singly-linked list (logical delete only)
    * Stack

These are all many-readers, many-writers.

liblfds on-line
===============
On the liblfds home page, you will find the blog, a bugzilla, a forum, a
mediawiki and the current and all historical releases.

The mediawiki contains comprehensive documentation for development, building,
testing and porting.

http://www.liblfds.org

license
=======
There is no license.  You are free to use this code in any way.

using
=====
Once built, there is a single header file, /inc/liblfds.h, which you must include
in your source code, and a single library file /bin/liblfds.*, where the suffix
depends on your platform and your build choice (static or dynamic), to which,
if statically built, you must link directly or, if dynamically built, you must
arrange your system such that the library can be found by the loader at run-time. 

testing
=======
The library comes with a command line test and benchmark program.  This program
requires threads.  As such, it is only suitable for platforms  which can execute
a command line binary and provide thread support.  Currently this means the test
and benchmark program works for all platforms except the Windows Kernel.

For documentation, see the testing and benchmarking guide in the mediawiki.

porting
=======
Both the test program and liblfds provide an abstraction layer which acts to
mask platform differences. Porting is the act of implementing on your platform
the functions which make up the abstraction layers.  You do not need to port
the test program to port liblfds, but obviously it is recommended, so you can
test your port.

To support liblfds, your platform MUST support;

    * atomic single-word* increment
    * atomic single-word compare-and-swap
    * atomic contiguous double-word compare-and-swap*
    * malloc and free
    * compiler directive for alignment of variables declared on the stack
    * compiler directives for compiler barriers and processor barriers

* A ''word'' here means a type equal in length to the platform pointer size.
* This requirement excludes the Alpha, IA64, MIPS, PowerPC and SPARC platforms.

Also, your platform MAY support;

    * compiler keyword for function inlining 

To support the test programme, your platform MUST support;

    * determining the number of logical cores
    * threads (starting and waiting on for completion)

For documentation, see the porting guide (lfds) in the mediawiki.

release history
===============
release 1, 25th September 2009, svn revision 1574.
  - initial release

release 2, 5th October 2009, svn revision 1599.
  - added abstraction layer for Windows kernel
  - minor code tidyups/fixes

release 3, 25th October 2009, svn revision 1652.
  - added singly linked list (logical delete only)
  - minor code tidyups/fixes

release 4, 7th December 2009, svn revision 1716.
  - added ARM support
  - added benchmarking functionality to the test program
  - fixed a profound and pervasive pointer
    declaration bug; earlier releases of liblfds
    *should not be used*

release 5, 19th December 2009, svn revision 1738.
  - fixed subtle queue bug, which also affected ringbuffer
    and caused data re-ordering under high load
  - added benchmarks for freelist, ringbuffer and stack

release 6, 29th December 2009, svn revision 1746.
  - fixed two implementation errors, which reduced performance,
    spotted by Codeplug from "http://cboard.cprogramming.com".

release 6.0.0, 18th December 2012, svn revision 2537
  - introduction of namespaces, e.g. the "lfds600_" prefix
    code otherwise COMPLETELY AND WHOLLY UNCHANGED
    this release is a stepping-stone to 6.1.0

release 6.0.1, 2nd January 2013, svn revision 3296
  - bug fix where an enum wasn't moved into the new namespacing policy

release 6.1.0, 31th December 2012, svn revision 2600
  - fixed all existing non-enhancement bugs
  - discovered some new bugs and fixed them too
  - a very few minor changes/enhancements

release 6.1.1, 2nd January 2013, svn revision 3297
  - crucial bug fix where compiler barriers for atomic operations
    were not brought over from 7.0.0 during backporting
  - minor fix for abstraction tests, two missing store barriers

