<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./../images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI Build Procedures</font></b>
    </td>
  </tr>
</table>

[[_TOC_]]

# Sanitize options in `build_oai`

The `build_oai` script provides various [Instrumentation Options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html) to add run-time instrumentation to the code and enable runtime error checkers, i.e. sanitizers, in order to help find various types of bugs in the codebase and eventually enhance the stability of the OAI softmodems.  The following sanitizers can be enabled using different build options:

## Address Sanitizer (ASAN)
[Address Sanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer) is enabled using the `--sanitize-address` option or its shorthand `-fsanitize=address`. It serves as a fast memory error detector and helps detect issues like out-of-bounds accesses, use-after-free bugs, and memory leaks.

### Run OAI Softmodem with ASAN on

It is necessary to add the envinronment variable `LD_LIBRARY_PATH` to the run command, e.g.:
```
cd cmake_targets/ran_build/build
sudo LD_LIBRARY_PATH=. ./nr-softmodem ...
```

## Undefined Behavior Sanitizer (UBSAN)
[Undefined Behavior Sanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) (UBSAN) is a runtime undefined behavior checker. It uses compile-time instrumentation to catch undefined behavior by inserting code that performs specific checks before operations that may cause it. UBSAN can be activated with the `--sanitize-undefined` option or `-fsanitize=undefined`.

UBSAN offers a range of suboptions that enable precise checks for various types of undefined behavior at runtime. These suboptions can be set by tweaking the [CMakeLists.txt](../../CMakeLists.txt) file. Here is an overview of some key suboptions:

* `-fsanitize=shift`: Enables checks for the result of shift operations, with suboptions for base and exponent.
* `-fsanitize=integer-divide-by-zero`: Detects integer division by zero.
* `-fsanitize=null`: Enables pointer checking, issuing an error message when dereferencing a NULL pointer or binding a reference to a NULL pointer.
* `-fsanitize=signed-integer-overflow`: Detects signed integer overflow.
* `-fsanitize=bounds`: Instruments array bounds, detecting various out-of-bounds accesses.
* `-fsanitize=alignment`: Checks the alignment of pointers when dereferenced or when a reference is bound to an insufficiently aligned target.
* `-fsanitize=object-size`: Checks out-of-bounds pointer accesses.
* `-fsanitize=float-divide-by-zero`: Detects floating-point division by zero.

## Memory Sanitizer
To enable [Memory Sanitizer](https://clang.llvm.org/docs/MemorySanitizer.html), use the `--sanitize-memory` option or `-fsanitize=memory`. It requires clang and is incompatible with ASAN and UBSAN. Building with this option helps catch issues related to uninitialized memory reads.

To build with Memory Sanitizer, use the following command:
```
CC=/usr/bin/clang CXX=/usr/bin/clang++ ./build_oai ... --sanitize-memory
```

## Thread Sanitizer
[Thread Sanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) can be activated using the `--sanitize-thread` option or `-fsanitize=thread`. This sanitizer helps identify data races and other threading-related issues in the code.

# Summary of Sanitizer Options

- `--sanitize`: Shortcut for using both ASAN and UBSAN.
- `--sanitize-address` or `-fsanitize=address`: Enable ASAN on all targets.
- `--sanitize-undefined` or `-fsanitize=undefined`: Enable UBSAN on all targets.
- `--sanitize-memory` or `-fsanitize=memory`: Enable Memory Sanitizer on all targets (requires clang).
- `--sanitize-thread` or `-fsanitize=thread`: Enable Thread Sanitizer on all targets.
