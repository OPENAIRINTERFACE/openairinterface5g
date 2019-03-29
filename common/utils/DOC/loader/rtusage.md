## shared library names
Shared library full names are built by the loader using the format:
>  < *path* >/lib< *module name* >< *module version* >.so

1.  the < *module name* > is defined at development time, it comes from the `modname`  argument of the `load_module_shlib` call.
1.  the < *module version* > and < *path* > optional  parameters, are defined at run-time, depending on the configuration.

## loader parameters
The loader is using the [configuration module](../../../config/DOC/config.md), and defines global and per library parameters. Global parameters must be specified under the **loader** section and library specific parameters under a **loader.<*module name*>** section. Module specific parameters override the global parameters.
### Global loader parameters
| name | type | default | description |
|:---:|:---:|:---:|:----|
| `shlibpath` | `string of char` | `""` | directory path used to look for shared libraries, may be superseded by the library specific `shlibpath`.|
| `maxshlibs` | `integer` | 10 | Maximum number of shared libraries the loader can  manage. |

### library specific loader parameters
| name | type | default | description |
|:---:|:---:|:---:|:----|
| `shlibpath` | `string of char` | `""` | directory path used to look for this shared library.|
| `shlibversion` | `string of char` | `""` | version to be used to load this shared library.|

### loader configuration examples

The following configuration file example just reproduce the default loader parameters:
```c
loader :
{
   shlibpath = "./";
   maxshlibs = 10;
   liboai_device :
      {
      shlibpath = "./";
      shlibversion = "";
      }
};
```
If you want to load a device called *liboai_device_USRP.so* without writting a specific configuration, you can start the softmodem using the following command:
> ./lte-softmodem -O libconfig:/usr/local/oai/openairinterface5g/targets/PROJECTS/GENERIC-LTE-EPC/CONF/enb.nbiot.band7.tm1.50PRB.usrpb210.conf:dbgl5  --loader.oai_device.shlibversion _USRP

With this latest example, nn the softmodem logs, you can check that the right device library has been loaded:
```bash
[LIBCONFIG] loader.oai_device.shlibpath not found in /usr/local/oai/develop-nb-iot-merge/openairinterface5g/targets/PROJECTS/GENERIC-LTE-EPC/CONF/enb.nbiot.band7.tm1.50PRB.usrpb210.conf
[LIBCONFIG] loader.oai_device.shlibversion set to default value ""
[LIBCONFIG] loader.oai_device: 1/2 parameters successfully set, (1 to default value)
[CONFIG] shlibversion set to  _USRP from command line
[CONFIG] loader.oai_device 1 options set from command line

```

[loader home page](../loader.md)
