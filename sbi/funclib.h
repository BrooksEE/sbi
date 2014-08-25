/* ========================================================================== */
/*                                                                            */
/*   funclib.h                                                                */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#include <stdio.h>

#ifndef _FUNCLIB
	#define _FUNCLIB

	// Define 'byte' variable type
	typedef unsigned char byte;


	// Put here your debug code
		
	// For windows, for example, use:
	//printf("DEBUG\t\t0x%02X\t\t%i\n", n, n);

	// For AVR, for example, use:
	//uart_sendstring("DEBUG ");
	//uart_sendnumber(n);
	//uart_sendstring("\n");
	typedef void(*debugn_func)(byte n);

	// Put here your error printing code

	// For windows, for example, use:
	//printf("ERROR\t\t0x%02X\t\t%i\n", n, n);
	
	// For AVR, for example, use:
	//uart_sendstring("ERROR ");
	//uart_sendnumber(n);
	//uart_sendstring("\n");
	typedef void(*errorn_func)(byte n);
	
	// Put here your function library initialization code
	// es: _sbifuncs[0] = &myfunc;
	typedef void(*initui_func)(void);

	typedef struct {
		debugn_func debugn;
		errorn_func errorn;
		initui_func initui;
	} sbi_config_t;
	
	
#endif
