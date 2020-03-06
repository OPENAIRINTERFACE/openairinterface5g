set +x
index=1
for file in ../test_input/8448/*
do 
		cp $file channel_output.txt
		echo "===== test $index =====" >> log.txt
		./ldpc channel_output.txt >> log.txt
		echo ' ' >> log.txt
		index=$((index+1))
	done
