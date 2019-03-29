To add a new parameter in an existing section you  insert an item in a `paramdef_t` array, which describes the parameters to be read in the existing `config_get` call. You also need to increment the numparams argument.

existing code:
```c
unsigned int varopt1;
paramdef_t someoptions[] = {
/*---------------------------------------------------------------------------------*/
/*                  configuration parameters for some module                       */
/* optname     helpstr     paramflags    XXXptr      defXXXval      type    numelt */
/*---------------------------------------------------------------------------------*/
   {"opt1",  "<help opt1>",   0,      uptr:&varopt1, defuintval:0, TYPE_UINT,  0 },
};

config_get( someoptions,sizeof(someoptions)/sizeof(paramdef_t),"somesection");

```
new code:
```c
unsigned int varopt1;


/* varopt2 will be allocated by the config module and free at end_configmodule call
   except if PARAMFLAG_NOFREE bit is set in paramflags field*/
char *varopt2;
paramdef_t someoptions[] = {
/*---------------------------------------------------------------------------------*/
/*                  configuration parameters for some module                       */
/* optname     helpstr     paramflags    XXXptr      defXXXval      type    numelt */
/*---------------------------------------------------------------------------------*/
   {"opt1",  "<help opt1>",   0,      uptr:&varopt1,  defuintval:0,TYPE_UINT,  0   },
   {"opt2",  "<help opt2>",   0,      strptr:&varopt2,defstrval:"",TYPE_STRING,0   },
};

config_get( someoptions,sizeof(someoptions)/sizeof(paramdef_t),"somesection");

```

The corresponding configuration file may now include opt2 parameter in the somesection section:

```c
somesection =
{
opt1 = 3;
opt2 = "abcd";
};
```

In these examples the variables used to retrieve the parameters values are pointed by the `uptr` or `strptr` fields of the `paramdef_t` structure. After the `config_get` call `varopt1` and `varopt2` have been set to the value specified in the config file. It is also possible to specify a `NULL` value to the `< XXX >ptr` fields, which are then allocated by the config module and possibly free when calling `config_end()`, if the `PARAMFLAG_NOFREE` bit has not been set in the `paramflags` field.

The configuration module provides a mechanism to check the parameter value read from the configuration source. The following functions are implemented:
1.  Check an integer parameter against a list of authorized values
1.  Check an integer parameter against a list of authorized values and set the parameter to a new value
1.  Check an integer parameter against a range
1.  Check a C string parameter against a list of authorized values
1. Check a C string parameter against a list of authorized values and set the parameter to a new integer value

 A `checkedparam_t` structure array provides the parameter verification procedures:

```c
/*
   definition of the verification to be done on param opt1 and opt2.
   opt1 is an integer option we must be set to 0,2,3,4 or 7 in the
   config source.
   if opt1 is set to 0 in the config file, it will be set to 1, etc
   opt2 is C string option with the authorize values "zero","oneThird","twoThird","one"
 */
#define OPT1_OKVALUES {0,2,3,4,7}
#define OPT1_NEWVALUES {1,1,0,6,9}
#define OPT2_OKVALUES {"zero","oneThird","twoThird","one"}

#define OPT_CHECK_DESC { \
             { .s1a= { config_check_modify_integer, OPT1_OKVALUE, OPT1_NEWVALUES ,5 }}, \
             { .s3=  { config_check_strval, OPT2_OKVALUES,4 }}, \
}
checkedparam_t checkopt[] = OPT_CHECK_DESC;
.....
/* assign the verification procedures to the parameters definitions */
for(int i=0 ; i < sizeof(someoptions)/sizeof(paramdesc_t) ; i ++) {
    someoptions[i].chkPptr = &(checkopt[i]);
}
....
```
When you need a specific verification algorithm, you can provide your own verification function and use it in place of the available ones, in the `checkedparam_t` union. If no existing structure definition match your need, you can enhance the configuration module. You then have to add a new verification function in https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/config/config_userapi.c and add a new structure definition in the `checkedparam_t` type defined in https://gitlab.eurecom.fr/oai/openairinterface5g/blob/develop/common/config/config_paramdesc.h

[Configuration module developer main page](../../config/devusage.md)
[Configuration module home](../../config.md)
