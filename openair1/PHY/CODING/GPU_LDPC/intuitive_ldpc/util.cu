#include <stdio.h>
#include <cuda_runtime.h>
#include "util.h"


void ReadDataFromFile(const char *file, unsigned int *input_data_arr, int *channel_data_arr, int block_length, int BG_col, int Zc)
{
//	static const char testin[] = "../test_input/test_case_1.txt";
	file_t inputfile;
	strcpy(inputfile.filename, file);
	inputfile.fptr = fopen(inputfile.filename, "r");
	if(inputfile.fptr == NULL)
	{	
		puts("cannot open file");
	}
	
	// data processing
	fgets(inputfile.tmp, 100, inputfile.fptr);	// get rid of gen test
	for(int i = 0; i < block_length/8; i++)
	{
		fscanf(inputfile.fptr, "%d", &input_data_arr[i]);
	}
	fgets(inputfile.tmp, 100, inputfile.fptr);	// get rid of '\n'
	fgets(inputfile.tmp, 100, inputfile.fptr);	// get rid of test end
	fgets(inputfile.tmp, 100, inputfile.fptr);	// get rid of channel
	/*
	for(int i = 0; i < 2*384; i++)
	{
		channel_data_arr[i] = 0;
	}
	*/
	for(int i = 0; i < BG_col*Zc; i++)
	{
		fscanf(inputfile.fptr, "%d", &channel_data_arr[i]);
	}
	fclose(inputfile.fptr);
}
