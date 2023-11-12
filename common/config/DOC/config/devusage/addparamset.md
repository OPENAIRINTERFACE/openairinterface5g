The configuration module maps a configuration file section to a `paramdef_t` structure array. One `config_get` call can be used to return the values of all the parameters described in the `paramdef_t` array.
Retrieving a single occurence of a parameter set ( a group in the libconfig terminology) is just a two steps task:
1.  describe the parameters in a `paramdef_t` array
2.  call the `config_get` function


[config_get example](../../config/devusage/addaparam.md)


In oai some parameters set may be instantiated a variable number of occurrences. In a configuration file this is mapped to lists of group of parameters, with this syntax:
```c
NB-IoT_MACRLCs =
/* start list element 1 */
(
  /* start a group of parameters */
  {
  num_cc = 2;
  .....
  remote_s_portd = 55001;
  }
),
/* start list element 2 */
(
  /* start a group of parameters */
  {
  num_cc = 1;
  ......
  remote_s_portd = 65001;
  }
);

```
The configuration module provides the `config_getlist` call to support lists of group of parameters. Below is a commented code example, using the config_getlist call.

```c
/* name of section containing the list */
#define NBIOT_MACRLCLIST_CONFIG_STRING      "NB-IoT_MACRLCs"

/*
The following macro define the parameters names, as used in the
configuration file
*/
#define CONFIG_STRING_MACRLC_CC             "num_cc"
.....
#define CONFIG_MACRLC_S_PORTD               "remote_s_portd"

/*
   now define a macro which will be used to initialize the NbIoT_MacRLC_Params
   variable. NbIoT_MacRLC_Params is an array of paramdef_t structure, each item
   describing a parameter. When using the config_getlist call you must let the config
   module allocate the memory for the parameters values.
*/
/*------------------------------------------------------------------------------------------------------------*/
/*   optname               helpstr   paramflags    XXXptr              defXXXval                  type           numelt     */
/*------------------------------------------------------------------------------------------------------------*/

#define MACRLCPARAMS_DESC { \
{CONFIG_STRING_MACRLC_CC, NULL,     0,          uptr:NULL,           defintval:1,           TYPE_UINT,     0}, \
.............  \
{CONFIG_MACRLC_S_PORTD,   NULL,     0,          uptr:NULL,           defintval:50021,           TYPE_UINT,     0}, \
}

/*
the following macros define the indexes used to access the NbIoT_MacRLC_Params array
items. They must be maintained consistent with the previous  MACRLCPARAMS_DESC macro
which is used to initialize the NbIoT_MacRLC_Params variable
*/
#define MACRLC_CC_IDX                                          0
.........
#define MACRLC_REMOTE_S_PORTD_IDX                              16



void RCconfig_NbIoTmacrlc(void) {


/*
   define and initialize the array of paramdef_t structures describing the groups of
   parameters we want to read. It will be passed as the second argument of the
   config_getlist call, which will use it as an input only argument.
*/
  paramdef_t NbIoT_MacRLC_Params[] = MACRLCPARAMS_DESC;

/*
      now define and initialize a paramlist_def_t structure which will be passed
   to the config_getlist call. The first field is the only one to be initialized
   it contains the name of the section to be read.
   that section contains the list of group of parameters.
      The two other fields are output parameters used to return respectively
   a pointer to a two dimensional array of paramdef_t structures pointers, and the
   number  of items in the list of groups (size of first dimension, the second one
   being the number of parameters in each group.
*/
  paramlist_def_t NbIoT_MacRLC_ParamList = {NBIOT_MACRLCLIST_CONFIG_STRING,NULL,0};


 /*
   the config_getlist will allocate the second field of the paramlist_def_t structure, a
two dimensional array of paramdef_t pointers. In each param_def item it will allocate
the value pointer and set the value to what it will get from the config source. The
numelt field of the paramlist_def_t structure will be set to the number of groups of
parameters in the list.
    in this example the last argument of config_getlist is unused, it may contain a
character string, used as a prefix for the section name. It has to be specified when the
list to be read is under another section.
*/
  config_getlist( &NbIoT_MacRLC_ParamList,NbIoT_MacRLC_Params,
                  sizeofArray(NbIoT_MacRLC_Params),
                  NULL);

/*
  start a loop in the nuber of groups in the list, the numelt field of the
  paramlist_def_t structure has been set in the config_getlist call
*/
  for (j=0 ; j<NbIoT_MacRLC_ParamList.numelt ; j++) {

..........

/* access the MACRLC_REMOTE_S_PORTD parameter in the j ieme group of the list */
	RC.nb_iot_mac[j]->eth_params_s.remote_portd =
               *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTD_IDX].iptr);
............

  } // MacRLC_ParamList.numelt > 0
}


```

[Configuration module developer main page](../../config/devusage.md)
[Configuration module home](../../config.md)
