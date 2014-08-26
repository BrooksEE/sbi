/* ========================================================================== */
/*                                                                            */
/*   funclib.h                                                                */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#ifndef _FUNCLIB
	#define _FUNCLIB

	void debugn(int n, struct sbi_context_t*)
	{
		printf("DEBUG\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	void errorn(int n, struct sbi_context_t*)
	{
		printf("ERROR\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	void myfunc(byte b[16], struct sbi_context_t*)
	{
		printf("Custom user function, parameters: %i, %i, %i, %i, %i, %i, %i, %i\n", _getval(b[0], b[1]), _getval(b[2], b[3]), _getval(b[4], b[5]), _getval(b[6], b[7]), _getval(b[8], b[9]), _getval(b[10], b[11]), _getval(b[12], b[13]), _getval(b[14], b[15]));
	}
	
	void getnum(byte b[16], struct sbi_context_t*)
	{
		int n;
		printf("Enter a number: ");
		scanf("%i", &n);
		_setval(b[0], b[1], (byte)n);
	}
	
#endif
