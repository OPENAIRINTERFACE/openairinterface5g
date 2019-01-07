echo "Hint: this may not work if /usr/src/linux is not set to currently booted kernel."
echo "Try 'make' instead."

make -C /usr/src/linux V=1 M=`pwd`
