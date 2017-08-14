For reliable UDP multicast, the libpgm-dev should be installed on the machines,
otherwise the old multicast method using simply UDP will be used, resulting in
packet loss and unexpected behaviour of oaisim.

To compile OAI with PGM:

For Ubuntu 12.04 and earliers:
    sudo apt-get install libpgm-dev
    is sufficient

For Ubuntu older than 12.04:

wget http://openpgm.googlecode.com/files/libpgm-5.1.118.tar.gz
tar xvf libpgm-5.1.118.tar.gz
cd libpgm-5.1.118/openpgm/pgm
./configure --prefix=/usr
make
sudo make install

We have to clean oaisim compilation and re-compile


When running OAI, the -D should be used to provide the NIC to bind.
Parameter can be either an interface name or an ip address (refer to this link
for more informations: http://code.google.com/p/openpgm/wiki/OpenPgmConceptsTransport)
