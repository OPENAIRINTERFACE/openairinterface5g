# `paramdef_t`structure
It is defined in include file [ common/config/config_paramdesc.h ](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/config/config_paramdesc.h#L103). This structure is used by developers to describe parameters and by the configuration module to return parameters value. A pointer to a `paramdef_t` array is passed to `config_get` and `config_getlist` calls to instruct the configuration module what parameters it should read.

| Fields     | Description                                                       | I/O |
|:-----------|:------------------------------------------------------------------|----:|
| `optname`    | parameter name, as used when looking for it in the config source, 63 bytes max (64 with trailing \0) | I |
| `helstr`     | pointer to a C string printed when using --help on the command line | I |
| `strptr` `strlistptr` `u8ptr` `i8ptr` `u16ptr` `i16ptr` `uptr` `iptr` `u64ptr` `i64ptr` `dblptr` `voidptr` | a pointer to a variable where the parameter value(s) will be returned. This field is an anonymous union, the supported pointer types have been built to avoid type mismatch warnings at compile time. | O |
| `defstrval` `defstrlistval` `defuintval` `defintval` `defint64val` `defintarrayval` `defdblval` | this field is an anonymous union, it can be used to define the default value for the parameter. It is ignored if `PARAMFLAG_MANDATORY` is set in the `paramflags` field.| I |
| `type` | Supported parameter types are defined as integer macros. Supported simple types are `TYPE_STRING`, parameter value is returned in `strptr` field,  `TYPE_INT8` `TYPE_UINT8` `TYPE_INT16` `TYPE_UINT16` `TYPE_INT32` `TYPE_UINT32` `TYPE_INT64` `TYPE_UINT64`, parameter value is returned in the corresponding uXptr or iXptr, `TYPE_MASK`, value is returned in `u32ptr`, `TYPE_DOUBLE` value is returned in `dblptr`, `TYPE_IPV4ADDR` value is returned in binary, network bytes order in `u32ptr` field. `TYPE_STRINGLIST`, `TYPE_INTARRAY` and `TYPE_UINTARRAY` are multiple values types. Multiple values are returned in respectively, `strlistptr`, `iptr` and `uptr` fields which then point to arrays. The  `numelt` field gives the number of item in the array. | I |
| `numelt` | For `TYPE_STRING` where `strptr` points to a preallocated string, this field must contain the size in bytes of the available memory. For all multiple values types, this field contains the number of values in the value field.| I/O |
| `chkPptr` | possible pointer to the structure containing the info used to check parameter values | I |
| `processedvalue` | When `chkPptr` is not `ǸULL`, is used to return a value, computed from the original parameter, as read from the configuration source. | O |

# `paramlist_def_t`structure
It is defined in include file [ common/config/config_paramdesc.h ](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/config/config_paramdesc.h#L160).
It is used as an argument to `config_getlist` calls, to get values of multiple occurrences of group of parameters.

| Fields     | Description                                                       | I/O |
|:-----------|:------------------------------------------------------------------|----:|
| `listname`    | Name of the section containing the list,  63 bytes max (64 with trailing \0). It is used to prefix each paramater name when looking for its value. In the libconfig syntax, the parameter name full path build by the configuration module is: listname.[occurence index].optname. | I |
| `paramarray`    | Pointer to an array of `ǹumelt` `paramdef_t` pointers. It is allocated by the configuration module and is used to return the parameters values. All input fields of each `paramdef_t` occurence are a copy of the `paramdef_t` argument passed to the `config_getlist` call. | O |
| `numelt` | Number of items in the `paramarray` field | O |

# `checkedparam_t` union
It is defined in include file [ common/config/config_paramdesc.h ](https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/config/config_paramdesc.h#L62).
This union of structures is used to provide a parameter checking mechanism. Each `paramdef_t` instance may include a pointer to a `checkedparam_t`structure which is then used by the configuration module to check the value it got from the config source.
Each structure in the union provides one parameter verification method, which returns `-1` when the verification fails. Currently the following structures  are defined in the `checkedparam_t` union:

| structure name    | Description                                                     |
|:-----------|:------------------------------------------------------------------|----:|
| `s1` | check an integer against a list of authorized values |
| `s1a` | check an integer against a list of authorized values and set the parameter value to another integer depending on the read value|
| `s2` | check an integer against an authorized range, defined by its min and max value|
| `s3` | check a string against a list of authorized values |
| `s3a` | check a string against a list of authorized values and set the parameter value to an integer depending on the read value|
| `s4` | generic structure, to be used to provide a specific, not defined in the configuration module, verification algorithm |

each of these structures provide the required fields to perform the specified parameter check. The first field is a pointer to a function, taking one argument, the `paramdef_t` structure describing the parameter to be checked. This function is called by the configuration module, in the `config_get` call , after the parameter value has been set, it then uses the other fields of the structure to perform the check.
The configuration module provides an implementation of the functions to be used to check parameters, qs described below.

## `s1` structure
| field    | Description                                                     |
|:-----------|:------------------------------------------------------------------|
| `f1` | pointer to the checking function. Initialize to `config_check_intval` to use the config module implementation |
| `okintval` | array of `CONFIG_MAX_NUMCHECKVAL` integers containing the authorized values |
| `num_okintval` | number of used values in `okintval` |

## `s1a` structure
| field    | Description                                                     |
|:-----------|:------------------------------------------------------------------|
| `f1a` | pointer to the checking function. Initialize to `config_check_modify_integer` to use the config module implementation |
| `okintval` | array of `CONFIG_MAX_NUMCHECKVAL` integers containing the authorized values |
| `setintval` | array of `CONFIG_MAX_NUMCHECKVAL` integers containing the values to be used for the parameter. The configuration module implementation set the parameter value to `setintval[i]` when the configured value is `okintval[i]` |
| `num_okintval` | number of used values in `okintval` and `setintval` |

## `s2` structure
| field    | Description                                                     |
|:-----------|:------------------------------------------------------------------|
| `f2` | pointer to the checking function. Initialize to `config_check_intrange` to use the config module implementation |
| `okintrange` | array of 2 integers containing the min and max values for the parameter |

## `s3` structure
| field    | Description                                                     |
|:-----------|:------------------------------------------------------------------|
| `f3` | pointer to the checking function. Initialize to `config_check_strval` to use the config module implementation |
| `okstrval` | array of `CONFIG_MAX_NUMCHECKVAL` C string pointers containing the authorized values |
| `num_okstrval` | number of used values in `okstrtval` |


## `s3a` structure
| field    | Description                                                     |
|:-----------|:------------------------------------------------------------------|
| `f3a` | pointer to the checking function. Initialize to `config_checkstr_assign_integer` to use the config module implementation |
| `okstrval` | array of `CONFIG_MAX_NUMCHECKVAL` C string pointers containing the authorized values |
| `setintval` | array of `CONFIG_MAX_NUMCHECKVAL` integers containing the values to be used for the parameter. The configuration module implementation set the parameter value to `setintval[i]` when the configured value is `okstrval[i]` |
| `num_okstrval` | number of used values in `okintval` and `setintval` |

## `s4` and `s5` structures
| field    | Description                                                     |
|:-----------|:------------------------------------------------------------------|
| `f4`  or `f5` | pointer to the checking function. they are generic structures to be used  when no existing structure provides the required parameter verification. `f4` takes one argument, the `paramdef_t` structure corresponding to the parameter to be checked (`int  (*f4)(paramdef_t *param)`), `f5` taxes no argument (`void (*checkfunc)(void)`) |

[Configuration module developer main page](../../config/devusage.md)
[Configuration module home](../../config.md)
