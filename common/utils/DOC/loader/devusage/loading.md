Implementing a shared library dynamic load using the oai loader  is a two steps task:
1.  define the `loader_shlibfunc_t` array, describing the list of externally available functions implemented in the library. This is the interface of the module.
1.  Call the `load_module_shlib` function, passing it the previously defined array and the number of items in this array. The first argument to `load_module_shlib` is the name identifying the module, which is also used to format the corresponding library name, as described [here](loader/rtusage#shared-library-names)

After a successful `load__module_shlib` call, the function pointer of each `loader_shlibfunc_t` array item has been set and can be used to call the corresponding function.

Typical loader usage looks like:

```c
/* shared library loader include file */
#include "common/utils/load_module_shlib.h"
.............
/*
  define and initialize the array, describing the list of functions
  implemented in "mymodule"
*/
  loader_shlibfunc_t mymodule_fdesc[2];
  mymodule_fdesc[0].fname="mymodule_f1";
  mymodule_fdesc[1].fname="mymodule_f2";

/*
 load the library, it's name must be libmymod.so. Configuration can be
 used to specify a specific path to look for libmymod.so. Configuration
 can also specify a version, for example "V1", in this case the loader
 will look for libmymodV1.so
*/
  ret=load_module_shlib("mymod",mymodule_fdesc, sizeof(mymodule_fdesc)/sizeof(loader_shlibfunc_t));
  if (ret < 0) {
       fprintf(stderr,"Library couldn't be loaded\n");
  } else {
/*
library has been loaded, we probably want to call some functions...
*/
  ret=((funcf1_t)mymodule_fdesc[0].fptr)();

..................
/*
later and/or somewhere else in the code you may want to call function "mymodule_f2"
You can use the loader get_shlibmodule_fptr(char *modname, char *fname) function
to retrieve the pointer to that function
*/
funcf2_t *f2;
int ret;
int intarg1;
char *strarg;
........................
f2 = (funcf2_t)get_shlibmodule_fptr("mymodule", "mymodule_f2")
if (f2 != NULL) {
  ret = f2(intarg1,strarg);
}
...............
```
When loading a shared library the loader looks for a symbol named `< module name > _autoinit` and, if it finds it, calls it. The `autoinit` function is called without any argument and the returned value, if any, is not tested.

[loader home page](../loader.md)
[loader developer home page](../../loader/devusage.md)
