/* ========================================================================== */
/*                                                                            */
/*   funclib.h                                                                */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#include <stdio.h>
#include <string.h>

#include <sbi.h>

//#include <windows.h>

#ifndef _PCLIB
	#define _PCLIB

    FILE* f;

	void debugn(byte n, sbi_context_t* ctx)
	{
		printf("DEBUG\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	void errorn(byte n, sbi_context_t* ctx)
	{
		printf("ERROR\t\t0x%02X\t\t%i\n", n, n);
		return;
	}
	
	void myfunc(byte b[16], sbi_context_t* ctx)
	{
		printf("Custom user function, myfunc: %i, %i, %i, %i, %i, %i, %i, %i\n", _getval(b[0], b[1]), _getval(b[2], b[3]), _getval(b[4], b[5]), _getval(b[6], b[7]), _getval(b[8], b[9]), _getval(b[10], b[11]), _getval(b[12], b[13]), _getval(b[14], b[15]));
	}
	
	void msgbox(byte b[16], sbi_context_t* ctx)
	{
		printf("Custom user function, msgbox: %i, %i, %i, %i, %i, %i, %i, %i\n", _getval(b[0], b[1]), _getval(b[2], b[3]), _getval(b[4], b[5]), _getval(b[6], b[7]), _getval(b[8], b[9]), _getval(b[10], b[11]), _getval(b[12], b[13]), _getval(b[14], b[15]));
		//char message[512];
		//char buf[4];
		//strcpy(message, "First parameter: ");
		//itoa(_getval(b[0], b[1]), buf, 10);
		//strcat(message, (const char*)buf);
		//strcat(message, "\nSecond parameter: ");
		//itoa(_getval(b[2], b[3]), buf, 10);
		//strcat(message, (const char*)buf);
		//strcat(message, "\nThird parameter: ");
		//itoa(_getval(b[4], b[5]), buf, 10);
		//strcat(message, (const char*)buf);
		//MessageBox(0, (const char*)message, "SBI Runtime", 1);
	}
	
	void getnum(byte b[16], sbi_context_t* ctx)
	{
		int n;
		printf("Enter a number: ");
		scanf("%i", &n);
		_setval(b[0], b[1], (byte)n);
	}

    byte getfch(sbi_context_t* ctx)
    {
    	return fgetc(f);
    }
    
    void setfpos(const unsigned int p,sbi_context_t* ctx)
    {
    	fseek (f, p, SEEK_SET);
    	return;
    }
    
    unsigned int getfpos(sbi_context_t* ctx)
    {
    	return (int)ftell(f);
    }
	
#endif
