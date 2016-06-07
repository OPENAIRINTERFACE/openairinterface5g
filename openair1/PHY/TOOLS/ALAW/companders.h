/**
 *	@companders.h - header definition file for embedded companding routines
 *		
 *	@copy Copyright (C) <2001-2012>  <M. A. Chatterjee>
 *  @author M A Chatterjee <deftio [at] deftio [dot] com>
 *	@version 1.01 M. A. Chatterjee, cleaned up naming
 *
 *  This file contains integer math settable fixed point radix math routines for
 *  use on systems in which floating point is not desired or unavailable.
 *
 *  @license: 
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

#ifndef __DIO_Compand_h__
#define __DIO_Compand_h__

#ifdef __cplusplus
extern "C"
{
#endif

//"DIO" prefixes are to assist in prevent name collisions if global namespace is used.

//typedefs ... change for platform dependant register size
// u/s = unsigned/signed 8/16/32 = num of bits
// keywords "int" and "long" in C/C++ are platform dependant
typedef unsigned char 	DIO_u8  ;  
typedef signed char 	DIO_s8  ;
typedef unsigned short 	DIO_u16 ;
typedef signed short 	DIO_s16 ;
typedef unsigned long 	DIO_u32 ;
typedef signed long 	DIO_s32 ;

// macros for converting from Fixed-Radix to integer, vice-versa
// r represents radix precision in bits, converts to/from integer w truncation
#define DIO_I2FR(x,r)		((x)<<(r))
#define DIO_FR2I(x,r)		((x)>>(r))


// convert FR to double, this is for debug only and WILL NOT compile under many embedded systems.
// use this in test harnesses. Since this is a macro if its not used it won't expand / link
#define DIO_FR2D(x,r) ((double)(((double)(x))/((double)(1<<(r)))))

//convert signed linear 16 bit sample to an 8 bit A-Law companded sample
DIO_s8  DIO_LinearToALaw(DIO_s16 sample);

//convert 8bit Alaw companded representation back to linear 16 bit
DIO_s16 DIO_ALawToLinear(DIO_s8 aLawByte);
			
			
//DC Offset correction for integer companders
//IIR: y_0=(y_1*(w-1)+x_0)/(w)
//where w is the window length
//below are fixed radix precision IIR averagers which allow runtime tradeoffs for windowLen & precision
//Note that (windowLen)*(1<<radix) must < 32767 


//DIO_IIRavgPower2FR() allows any window length but uses a divide instruction.
//output is in radix number of bits 
DIO_s32 DIO_IIRavgFR	   (DIO_s32 prevAvg, DIO_u16 windowLen, DIO_s16 newSample, DIO_u8 radix);

//DIO_IIRavgPower2FR() similar to above, but window length is specified as a number of bits
//which removes the need for a divide in the implementation
//outpit is in radix number of bits 
DIO_s32 DIO_IIRavgPower2FR (DIO_s32 prevAvg, DIO_u8 windowLenInBits, DIO_s16 newSample, DIO_u8 radix);

#ifdef __cplusplus
}
#endif

#endif /* __DIO_Compand_h__ */
