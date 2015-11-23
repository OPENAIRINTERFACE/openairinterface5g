#!/bin/bash

#The following script checks if the author list changed since the last commits in svn. If it finds difference in author names, then we need to add the author names manually in authors.txt

wget https://bitbucket.org/atlassian/svn-migration-scripts/downloads/svn-migration-scripts.jar
java -jar ./svn-migration-scripts.jar verify
java -jar ./svn-migration-scripts.jar authors http://svn.eurecom.fr/openair4G > /tmp/authors.txt
diff /tmp/authors.txt ./authors_original_java_output.txt
