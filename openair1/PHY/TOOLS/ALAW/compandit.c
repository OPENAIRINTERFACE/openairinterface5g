/**
 *	@file compandit.c - implementation test file for integer companding with
 *  compand/uncompand support & IIR correction
 *
 *	@copy Copyright (C) <2012>  <M. A. Chatterjee>
 *  @author M A Chatterjee <deftio [at] deftio [dot] com>
 *
 *	This software is provided 'as-is', without any express or implied
 *	warranty. In no event will the authors be held liable for any damages
 *	arising from the use of this software.
 *
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions:
 *
 *	1. The origin of this software must not be misrepresented; you must not
 *	claim that you wrote the original software. If you use this software
 *	in a product, an acknowledgment in the product documentation would be
 *	appreciated but is not required.
 *
 *	2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 *
 *	3. This notice may not be removed or altered from any source
 *	distribution.
 *
 */

#include <stdio.h>
#include <string.h>


#include "companders.h"


//===============================================
//main program for testing the functions
int main (int argc, char *argv[])
{
	int ret_val = 0;
	int i=0,j=0;
	printf("\n============================================================\n");
	printf("compandit library quikie program\n");
	printf("M. A. Chatterjee (c) 2012\n\n");
	printf("These routines were developed for use on embedded projects\n");
	printf("for more info see the accompanying compand.txt\n");
	printf("\n");
	
	//uncomment to 
	//show how linear-->alaw-->linear-->alaw progression / quantization error works
	/*
	
	{
		char alaw=0,alaw2;
		short rev=0;

		for (i= -32768; i <= 32767; i++)
		{
			alaw = DIO_LinearToALaw((short)i);
			rev  = DIO_ALawToLinear(alaw);
			alaw2 =DIO_LinearToALaw(rev);
			if (alaw != alaw2)
				printf("***********");
			printf("%7d  %7d  %7d %7d\n",i,(int)alaw,(int)rev,(int)alaw2);
		}
	}


    // IIR averager examples
    //IIR window length of 8 samples, using fractional precision of 4 bits
    {    
		int a3=0, a4=0;
		unsigned char rad=4; //4 bits fixed-radix fractional precision
		printf(" index   wave  IIRav(i)  IIRav(f)  IIRavP2(i) IIRavP2(f)\n");
		for (i=0; i < 300; i++)
		{
			j=(i&0x3f)-20; // triangle wave with range -20 to + 43
			a3= DIO_IIRavgFR(a3,8,j,rad);		  
			a4= DIO_IIRavgPower2FR(a4,3,j,rad);  
			printf("%6d  %6d  %6d  %9.4f     %6d  %9.4f \n",i,j, 
				DIO_FR2I(a3,rad),DIO_FR2D(a3,rad) ,DIO_FR2I(a4,rad),DIO_FR2D(a4,rad));
		}
	}
    //IIR window length of 64 samples
    {    
		int a3=0, a4=0;
		unsigned char rad=6; //rad is the number of bits of precision 
		printf(" index   wave  IIRav(i)  IIRav(f)  IIRavP2(i) IIRavP2(f)\n");
		for (i=0; i < 300; i++)
		{
			j=(i&0x3f)-20; // triangle wave with range -20 to + 43
			a3= DIO_IIRavgFR(a3,64,j,rad);	   
			a4= DIO_IIRavgPower2FR(a4,6,j,rad); 
			printf("%6d  %6d  %6d  %9.4f     %6d  %9.4f \n",i,j, 
				DIO_FR2I(a3,rad),DIO_FR2D(a3,rad) ,DIO_FR2I(a4,rad),DIO_FR2D(a4,rad));
		}
	}
	// */
	//Typical microcontroller application.  See readme-companders.txt
	// the input here simulates an A/D which has  a range 0..3.3V mapped as 12 bits (0..4095)
	// with a DC bias of 1.55V  ==> (1.55/3.3)*4095 counts = 1923 counts
	
	//now window length of 256 is used for rejecting the resistor bias.  at 8KHz this window
	// would be approx 8000/256 ~= 31 Hz (not quite but explaining Z xforms is beyond what
	// can be explained in this small space.)
	//we seed the DC average at 3.3/2 = 1.65V (we guess its in the middle) and let the long window
	//length hone in on the correct value.  (1.65V/3.3V) *4095 = 2048 counts
    {    
		int actualDCbias     =1923;
		int calculatedDCbias =2048;	//2048 is our initial estimate as outlined above
		unsigned char windowLenPow2InBits = 8; // 8 bit long window = 256 sample long window
		unsigned char rad=6; //rad is the number of bits of precision 

		calculatedDCbias = DIO_I2FR(calculatedDCbias,rad);
		printf(" index   wave  actDCbias  calcDCbias  calcDCbias(f) alaw\n");
		for (i=0; i < 1000; i++)  // if 8000 hz sample rate this represents the number of samples captured
		{
			j=(((i&0x3f)<<1)-63)+1923; // triangle wave w range 0..127  with a bias set at actualDCbias
			calculatedDCbias = DIO_IIRavgPower2FR(calculatedDCbias,windowLenPow2InBits,j,rad); 
			printf("%6d %6d   %6d     %6d      %9.4f     %3d\n",i,j,actualDCbias, 
				DIO_FR2I(calculatedDCbias,rad),DIO_FR2D(calculatedDCbias,rad),
				(int)(DIO_LinearToALaw(j-DIO_FR2I(calculatedDCbias,rad)) ));
		}
	}
	printf("\n");
	return ret_val;
}

