#!/bin/bash

exec=ldpc

help()
{
	echo "Usage: $0 <code_length>"
}

main()
{
	if [ -z $@ ]; then
		help
		exit 1
	fi

	dir=$@
	files=`ls ../test_input/$dir | grep txt`
#	files="1.txt 2.txt 3.txt"
	
	index=1
	for file in $files
		do 
			cp ../test_input/$dir/$file channel_output.txt
			echo "==== test $index ====" >> log.txt
			./$exec -l $dir -f channel_output.txt >> log.txt
			echo ' ' >> log.txt
			index=$((index+1))
		done
}

main $@
