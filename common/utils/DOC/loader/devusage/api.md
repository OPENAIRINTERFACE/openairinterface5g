 Loader API is defined in the [common/utils/load_module_shlib.h](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/utils/load_module_shlib.h) include file.
```c
int load_module_shlib(char *modname,loader_shlibfunc_t *farray, int numf)
```
* possibly initializes the loader, if it has not been already initialized
* Formats the full shared library path, using the `modname` argument and the loader `shlibpath` and `shlibversion`configuration parameters.
* loads the shared library, using the dlopen system call
* looks for `< modname >_autoinit` symbol, using the `dlsym` system call and possibly call the corresponding function.
* looks for `< modname >_checkbuildver` symbol, using the `dlsym` system call and possibly calls the corresponding function. If the return value of this call is `-1`, program execution is stopped. It is the responsibility of the shared library developer to implement or not a `< modname >_checkbuildver` function and to decide if a version mismatch is a fatal condition. The `< modname >_checkbuildver` function must match the `checkverfunc_t` function type. The first argument is the main executable version, as set  in the `PACKAGE_VERSION` macro defined in the [oai CMakeLists](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/cmake_targets/CMakeLists.txt#L218), around line 218. The second argument points to the shared library version which should be set  by the `< modname >_checkbuildver` function.
* If the farray pointer is null,  looks for `< modname >_getfarray` symbol, calls the corresponding function when the symbol is found. `< modname >_getfarray` takes one argument, a pointer to a  `loader_shlibfunc_t` array, and returns the number of items in this array, as defined by the `getfarrayfunc_t` type. The `loader_shlibfunc_t` array returned by the shared library must be fully filled (both `fname` and `fptr` fields).
* looks for the `numf` function symbols listed in the `farray[i].fname` arguments and set the corresponding `farray[i].fptr`function pointers


```c
void * get_shlibmodule_fptr(char *modname, char *fname)
```
Returns a pointer to the symbol `fname`, defined in module `modname`, or `NULL` if the symbol is not found.

[loader home page](../../loader.md)
[loader developer home page](../devusage.md)
