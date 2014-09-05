/* ========================================================================== */
/*                                                                            */
/*   sbi.h                                                                    */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#ifndef _SBI_H
	#define _SBI_H
    
    // typedef
	typedef unsigned char byte;

	// Configuration
	#define VARIABLESNUM				64
	#define USERFUNCTIONSN				16
	#define RETURNADDRESSESN			16
	#define THREADMAXNUM				10
	
	// Multithreading configuration
    // #define _SBI_MULTITHREADING_EQUALTIME=0 to disable
    #ifndef _SBI_MULTITHREADING_EQUALTIME
  		#define _SBI_MULTITHREADING_EQUALTIME 1
    #endif
	
	// Define 'byte' variable type
	typedef unsigned char byte;
	
	// Define 'program counter' variable type
	typedef unsigned int PCOUNT;
	
	// Define 'label address' variable type
	typedef unsigned int LABEL;
	
	// Define 'return address' variable type
	typedef unsigned int RETADDR;
	
	// Define 'interrupt address' variable type
	typedef unsigned int INTERRUPT;
	
	// Define 'user function id' variable type
	typedef unsigned int USERFUNCID;
	
	// Define 'sbi thread number' variable type
	typedef unsigned int SBITHREADNUM;
	
	// Define 'program variable' variable type
	typedef unsigned char VARIABLE;
	
	// SBI format
	#define HEADER_0						0xAA
	#define HEADER_1						0x4B // 0x3B, 0x2B or 0x1B for older versions
	
	#define LABELSECTION						0xA3
	#define INTERRUPTSECTION					0xB3
	
	#define SEPARATOR						0xB7
	
	#define FOOTER_0						0x3A
	#define FOOTER_1						0xF0
	
	// Instructions
	#define _istr_assign						0x01
	#define _istr_move						0x02
	#define _istr_add						0x10
	#define _istr_sub						0x11
	#define _istr_mul						0x12
	#define _istr_div						0x13
	#define _istr_incr						0x20
	#define _istr_decr						0x21
	#define _istr_inv						0x22
	#define _istr_tob						0x23
	#define _istr_cmp						0x30
	#define _istr_high						0x31
	#define _istr_low						0x32
	#define _istr_jump						0x41
	#define _istr_cmpjump						0x42
	#define	_istr_ret						0x43
	#define _istr_debug						0x50
	#define _istr_error						0x52
	#define _istr_sint						0x60
	#define _istr_int						0x61
	#define _istr_exit						0xFF
	
	#define _varid							0x04
	#define _value							0xF4

	// User functions
    // The user must pass the void* to _getval, _setval if used
	typedef void (*sbi_user_func)(byte[], void* );

	// Put here your debug code
	typedef void(*debugn_func)(int n, void*);

	// Put here your error printing code
	typedef void(*errorn_func)(int n, void* );
    
    // returns the next byte from the source sbi
	typedef byte (*getfch_func)(PCOUNT p, void*);
	
	typedef struct {
		debugn_func debugn;
		errorn_func errorn;
        getfch_func getfch;
	    sbi_user_func sbi_user_funcs[USERFUNCTIONSN];
        void* userdata;
	} sbi_context_t;
	
	
    // public api 
	void* sbi_init(sbi_context_t*);
    unsigned int sbi_begin();
	unsigned int sbi_running(void*);
	unsigned int sbi_step();
	void interrupt(const INTERRUPT id, void*);
    void sbi_cleanup(void*); // free resources after program run

    // accessor functions can be used
    // in user_function context to get/set parameters.
	byte getval(const byte type, const byte val, void*);
	unsigned int setval(const byte type, const byte num, const byte val, void*);
	
	
#endif
