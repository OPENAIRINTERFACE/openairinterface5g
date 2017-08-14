
2012 07 10 - Working version
It is just generating the XML file and it is placing it in $(OPENAIR_TARGETS)/SIMU/EXAMPLE/OSD/WEBXML/ or http://localhost/xmlfile/ so that the oaisim could pars and run the simulation/emulation

Please follow the steps described below: 

- install XAMPP for Linux (http://www.apachefriends.org/en/xampp-linux.html) in /opt (highly recommanded)
- create a symbolic link to this OSD directory into lampp/htdocs (e.g. cd ln -s $OPENAIR2_DIR/UTIL/OSD /opt/lampp/htdocs/OSD)
- make sure that the path to $(OPENAIR_TARGETS)/SIMU/EXAMPLE/OSD/ and all the subdirectories are in 777 mode ("chmod -R 777 $(OPENAIR_TARGETS)/SIMU/EXAMPLE/OSD/). This is requires since apache client is nobody and needs to access to this directory for RW
- set the OAI env vars in $OPENAIR2_DIR/UTIL/OSD/OpenAirEmu.conf 
- copy the $OPENAIR2_DIR/UTIL/OSD/httpd-results.conf in /opt/lampp/etc/extras/
- add "Include etc/extra/httpd-result.conf" at the end of the /opt/lampp/etc/httpd.conf
- if does not exist, create a symbolic link pointing at $(OPENAIR_TARGETS)/SIMU/EXAMPLES/OSD/ in /opt/lampp/htdocs/OSD/ (e.g. ln -s $(OPENAIR_TARGETS)/SIMU/EXAMPLES/OSD OSD )
- restart the lampp (e.g. sudo /opt/lampp/lampp restart)
- open the browser, and goto http://localhost/OSD

For the license, see the file COPYING
