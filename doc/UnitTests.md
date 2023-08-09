# Unit Testing in OAI

OpenAirInterface uses
[ctest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) for unit
testing. The cmake documentation has a
[tutorial](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Testing%20With%20CMake%20and%20CTest.html)
explaining how to test with cmake and ctest; it is a suggested read, and the
following just lists the main points of how to compile the tests and how to add
new ones.

At the time of writing, only the NR RLC tests have been integrated. The author
hopes that more tests follow suit.

# How to compile tests

To compile only the tests, a special target `tests` is available. It has to be
enabled with the special cmake variable `ENABLE_TESTS`:

```bash
cd openairinterface5g
mkdir build && cd build # you can also do that in cmake_targets/ran_build/build
cmake .. -GNinja -DENABLE_TESTS=ON
ninja tests
```

Then, you can run `ctest` to run all tests:
```
$ ctest
Test project /home/richie/w/ctest/build
    Start 1: nr_rlc_tests
1/1 Test #1: nr_rlc_tests .....................   Passed    0.06 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.06 sec
```

A couple of interesting variables are `--verbose`, `--output-on-failure`.

# How to add a new test

As of now, there is no dedicated testing directory. Rather, tests are together
with the sources of the corresponding (sub)system. The generic four-step
process is

1. Guard all the following steps with `if(ENABLE_TESTS)`. In a world where OAI
   is tested completely, there would be many executables that would be of
   tangential interest to the average user only. A "normal" build without tests
   would result in less executables, due to this guard.
2. Add an executable that you want to execute. In a `CMakeLists.txt`, do for
   instance `add_executable(my_test mytest.c)` where `mytest.c` contains
   `main()`. You can then build this executable with `ninja/make my_test`,
   given you ran `cmake -DENABLE_TESTS=ON ...` before.
3. Create a dependency to `tests` so that triggering the `tests` (`ninja/make
   tests`) target will build your test: `add_dependencies(tests my_test)`.
4. Use `add_test(NAME my_new_test COMMAND my_test <options>)` to declare a new
   test that will be run by `ctest` under name `my_new_test`.

In the simplest case, in an existing `CMakeLists.txt`, you might add the
following:
```
if(ENABLE_TESTS)
  add_executable(my_test mytest.c)
  add_dependencies(tests my_test)
  add_test(NAME my_new_test COMMAND my_test) # no options required
endif()
```

Note that this might get more complicated, e.g., typically you will have to
link some library into the executable with `target_link_libraries()`, or pass
some option to the test program.

`ctest` decides if a test passed via the return code of the program. So a test
executable that always passes is `int main() { return 0; }` and one that always
fails `int main() { return 1; }`. It is left as an exercise to the reader to
include these examples into `ctest`. Other programming languages other than C
or shell scripts are possible but discouraged. Obviously, though, a test in
a mainstream non-C programming language/shell script (C++, Rust, Python, Perl)
is preferable over no test.

Let's look at a more concrete, elaborate example, the NR RLC tests.
They are located in `openair2/LAYER2/nr_rlc/tests/`. Note that due to
historical reasons, a test script `run_tests.sh` allows to run all tests from
that directory directly, which you might also use to compare to the
`cmake`/`ctest` implementation.

1. Since the tests are in a sub-directory `tests/`, the inclusion of the entire
   directory is guarded in `openair2/LAYER2/nr_rlc/CMakeLists.txt` (in fact, it
   might in general be a good idea to create a separate sub-directory
   `tests/`!).
2. The NR RLC tests in fact consist of one "test driver program" (`test.c`)
   which is compiled with different "test stimuli" into the program. In total,
   there are 17 stimuli (`test1.h` to `test17.h`) with corresponding known
   "good" outputs after running (in `test1.txt.gz` to `test17.txt.gz`). To
   implement this, the `tests/CMakeLists.txt` creates multiple executables
   `nr_rlc_test_X` via the loop over `TESTNUM`, links necessary libraries into
   the test driver and a compile definition for the test stimuli.
3. For each executable, create a dependency to `tests`.
4. Finally, there is a single(!) `ctest` test that runs all the 17 test
   executables at once(!). If you look at the shell script
   `exec_nr_rlc_test.sh`, you see that it runs the program, filters for `TEST`,
   and compares with a predefined output from each test in `testX.txt.gz`,
   which is `gunzip`ed on the fly... Anyway, the actual `add_test()` definition
   just tells `ctest` to run this script (in the source directory), and passes
   an option where to find the executables (in the build directory). This
   slight complication is due to using shell scripts. An easier way is to
   directly declare the executable in `add_test()`, and `ctest` will locate and
   run the executable properly.
