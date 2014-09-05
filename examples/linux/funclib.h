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

	void debugn(int n, void* rt)
	{
		printf("DEBUG\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	void errorn(int n, void* rt)
	{
		printf("ERROR\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	void myfunc(byte b[16], void* rt)
	{
		printf("Custom user function, parameters: %i, %i, %i, %i, %i, %i, %i, %i\n", _getval(b[0], b[1], rt), _getval(b[2], b[3], rt), _getval(b[4], b[5], rt), _getval(b[6], b[7], rt), _getval(b[8], b[9], rt), _getval(b[10], b[11], rt), _getval(b[12], b[13], rt), _getval(b[14], b[15], rt));
	}

    void msgbox(byte b[16], void* rt) {
        printf ( "msgbox..\n" );
    }
	
	void getnum(byte b[16], void* rt)
	{
		int n;
		printf("Enter a number: ");
		scanf("%i", &n);
		_setval(b[0], b[1], (byte)n, rt);
	}
	
#endif
