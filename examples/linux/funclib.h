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

	void debugn(DTYPE n, void* rt)
	{
		printf("DEBUG\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	void errorn(DTYPE n, void* rt)
	{
		printf("ERROR\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	DTYPE myfunc(uint8_t argc, DTYPE* args)
	{
        int i;
        printf ("Custom user function, parameters: ");
        for (i=0;i<argc;++i)
            printf ( "%d, ", args[i] );
        printf ( "\n" );
        return 0;
	}

    DTYPE msgbox(uint8_t argc, DTYPE* args) {
       printf ( "msgbox .. tmp ignored args.\n" );
       return 0;
    }
	
	DTYPE getnum(uint8_t argc, DTYPE* args)
	{
		int n;
		printf("Enter a number: ");
		scanf("%i", &n);
        return n;
	}
	
#endif
