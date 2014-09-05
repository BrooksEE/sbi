/* ========================================================================== */
/*                                                                            */
/*   sbi.h                                                                    */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#ifndef _SBI_H
	#define _SBI_H
    
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
