USRP device documentation

OAI works with the most common USRP models like, B200, B200mini, B210, X310, N300, N310, N320, X410. This is achieved by using the Ettus Universal Hardware Driver (UHD) (https://github.com/EttusResearch/uhd). The file usrp_lib.cpp provides an abstraction layer of UHD to OAI.

The USRP can be configured in the RU section of the config file. The field "sdr_addrs" uses the same syntax as the USRP device identification string (https://files.ettus.com/manual/page_identification.html). Here are a few examples

```bash
sdr_addrs = "addr=192.168.10.2" # uses a single 10Gb Ethernet interface on an N3x0 or X3x0 or X4x0


sdr_addrs = "addr=192.168.10.2,second_addr=192.168.20.2" # uses 2 10Gb Ethernet interfaces on a N3x0 or X3x0 or X4x0 (requires that you flashed the FPGA wth the XG image)
```

you can also use the multi USRP feature and specify multiple USRPs, in which case you will get the aggregated number of channels on all the devices

```bash
sdr_addrs = "addr0=192.168.10.2,addr1=192.168.30.2"
```

You can specify if you want to use external or interal clock or time source either by adding the parameters in the sdr_addrs field or by using the fields clock_src or time_src

```bash
sdr_addrs = "addr=192.168.10.2,clock_source=external,time_source=external"
```

is equivalent to

```bash
sdr_addrs = "addr=192.168.10.2"
clock_src = "external"
time_src  = "external"
```

Valid choices for clock and time source are "internal", "external", and "gpsdo".

Note 1: the USRP remembers the choice of the clock source. If you want to make sure it uses always the same, always specify the clock_source and time_source.

Note 2: when using multiple USRPs they always have to be synchronized using "external" or "gpsdo"

Last but not least you may specify that only a specfic subdevice of the USRP is used. See also https://files.ettus.com/manual/page_configuration.html#config_subdev

For example on a USRP N310 the following fields will specify that you use channel 0 of subdevice A.

```bash
tx_subdev = "A:0"
rx_subdev = "A:0"
```

When combining this with the multi USRP feature you can easily create a distributed antenna array with only 1 channel used at each USRP.
