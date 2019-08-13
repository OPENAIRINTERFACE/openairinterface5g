# `loader_shlibfunc_t`structure
It is defined in include file [ common/util/load_module_shlib.h ](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/utils/load_module_shlib.h#L38). This structure is used  to list the symbols that should be searched by the loader when calling the `load_module_shlib` function.

| Fields     | Description                                                       | I/O |
|:-----------|:------------------------------------------------------------------|----:|
| `fname`    | symbol name, is passed to the [`dlsym`](http://man7.org/linux/man-pages/man3/dlsym.3.html) system call performed by the loader to get a pointer to the symbol | I |
| `fptr`     | pointer to the symbol name, set by the loader. `fptr` is defined as a `int (*fptr)(void)` function type | O |

[loader home page](../../loader.md)
[loader developer home page](../devusage.md)
