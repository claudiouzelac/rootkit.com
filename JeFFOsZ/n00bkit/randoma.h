/***************************** RANDOMA.H ********************** 2004-03-31 AF *
*
* This file contains function prototypes for several random number generators.
*
* These functions are coded in assembly language for improved speed and 
* contained in the libraries RANDOMAO.LIB, RANDOMAC.LIB, RANDOMAE.A.
*
* The functions are compatible with C++ and many other programming languages
* under Windows, Linux, BSD, UNIX and other 32-bit operating systems running
* on Intel 80x86 and compatible microprocessors, such as the Pentium family
* of microprocessors and corresponding microprocessors from AMD. 
* For computers that are not compatible with these, you have to use the
* C++ version of the functions.
*
* The theory of these random number generators is explained at
* www.agner.org/random
*
*
* Overview of random number generators in this library:
* =====================================================
*
* TRandom uses the Mersenne Twister type MT11213A.
* The resolution is 32 bits.
*
* MRandom uses the Mother-of-All algorithm.
* The resolution is 32 bits.
*
* WRandom uses the RANROT type W algorithm with self-test.
* The resolution is 63 bits for WRandom and 64 bits for WIRandom.
*
* XRandom combines the RANROT type W and the Mother-of-All generators.
* The resolution is 63 bits for WRandom and 64 bits for WIRandom.
*
*
* Function descriptions:
* ======================
*
* All these generators are used in the same way: They must be initialized
* with a random seed before the first call to any of the random functions.
* You may use the time in seconds or milliseconds as seed.
*
* Call the appropriate version of _RandomInit with your seed before gene-
* rating any random number, where _ is the corresponding prefix letter.
* The Mersenne Twister has an additional alternative initialization function
* TRandomInitByArray which you can use for longer seeds.
*
* All the generators can generate three different kinds of random numbers:
*
* _Random gives a uniformly distributed floating point number in the 
* interval 0 <= x < 1.
*
* _IRandom gives a uniformly distributed integer in the closed interval
* defined by min and max:  min <= x <= max.
*
* _BRandom gives 32 random bits.
*
*
* Further documentation:
* ======================
* See the file randoma.htm for further documentation.
*
* © 2001, 2004 Agner Fog. All code in this library is published under the
* GNU General Public License: www.gnu.org/copyleft/gpl.html
*******************************************************************************/

#ifndef RANDOMA_H
#define RANDOMA_H

#pragma comment(lib,"randomam.lib")

// Mersenne Twister
extern void     TRandomInit (int seed);
extern void     TRandomInitByArray (unsigned long int seeds[], int length);
extern double   TRandom (void);
extern double   TRandom2 (void);
extern long double TRandom3 (void);
extern int      TIRandom (int min, int max);
extern unsigned TBRandom ();

// Mother-of-all generator
extern void     MRandomInit (int seed);
extern double   MRandom (void);
extern int      MIRandom (int min, int max);
extern unsigned MBRandom (void);

// RANROT type W generator                                                                              
extern void     WRandomInit (int seed);
extern double   WRandom  (void);
extern int      WIRandom (int min, int max);
extern unsigned WBRandom (void);

// Combined generator
extern void     XRandomInit (int seed);
extern double   XRandom (void);
extern int      XIRandom (int min, int max);
extern unsigned XBRandom ();

// replacements for some C++ functions in wnchyppr.cpp:
extern double pow2_1(double q, double * y0); // calculate 2^q and (1-2^q) without loss of precision.
extern double log1mx(double x, double x1);   // calculate natural log of x1 = (1-x) without loss of precision
extern double log1pow(double q, double x);   // calculate log((1-e^q)^x)

// various other auxiliary functions
extern int DetectProcessor (void);           // detect which instruction set supported by microprocessor
extern int Round (double x);                 // round to nearest integer or even
extern int Truncate (double x);              // round towards zero
extern int ReadClock (void);                 // read microprocessor internal clock
extern int MinI (int a, int b);              // the smallest of two integers
extern int MaxI (int a, int b);              // the biggest  of two integers
extern double MinD (double a, double b);     // the smallest of two double precision numbers
extern double MaxD (double a, double b);     // the biggest  of two double precision numbers
  
#endif

