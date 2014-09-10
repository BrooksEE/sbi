/* ========================================================================== */
/*                                                                            */
/*   sbi.h                                                                    */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#ifndef _SBI_H
	#define _SBI_H
    
	// Configuration
    // customize number of global variables
    // these variables are available to every thread.
    #ifndef VARIABLESNUM
	    #define VARIABLESNUM				64
    #endif

    // number of user functions.
    #ifndef USERFUNCTIONSN
	    #define USERFUNCTIONSN				16
    #endif

    // number of return addresses per thread
    // This directly effects the number of 
    // nested functions you can call.
    #ifndef RETURNADDRESSESN
	    #define RETURNADDRESSESN			16
    #endif

    // max number of threads your programs
    // can create.
    #ifndef THREADMAXNUM
	    #define THREADMAXNUM				10
    #endif

    // per thread stack size usable by push/pop
    #ifndef STACK_SIZE
        #define STACK_SIZE                  64
    #endif

    // per thread register space for thread
    // local storage.  Can be used for function
    // parameters, return values or whatever
    // not to be modified by another thread.
    #ifndef REG_SIZE 
        #define REG_SIZE                    16
    #endif
	
	// Multithreading configuration
    // #define _SBI_MULTITHREADING_EQUALTIME=0 to disable
    #ifndef _SBI_MULTITHREADING_EQUALTIME
  		#define _SBI_MULTITHREADING_EQUALTIME 1
    #endif

    // number of interrupts to queue
    // before they get lost if not handled.
    #ifndef INTQUEUE_SIZE
        #define INTQUEUE_SIZE               6
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

    typedef enum {
      SBI_NOERROR,
      SBI_THREAD_EXIT, // normal thread exit
      SBI_PROG_EXIT,  // normal exit via exit command
      SBI_INVALID_RT, // void* for context is null 
      SBI_ALLOC_ERROR, // unable to allocate memory
      SBI_CTX_ERROR,   // uninitialized function in context
      SBI_HEADER_ERROR, // invalid program header
      SBI_HEADER_OLD,  // old program format
      SBI_PROG_ERROR,  // program runtime error
      SBI_STACK_OVERFLOW, // push called when stack is full
      SBI_STACK_UNDERFLOW, // pop called when stack is empty
      SBI_INSTR_ERROR, // unrecognized instruction
      SBI_THREAD_MAX, // attempt to allocate more than max threads
      SBI_INT_LOST // more interrupts queued than INTQUEUE_SIZE
    } sbi_error_t;
	

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
    sbi_error_t sbi_begin(void*);
	sbi_error_t sbi_running(void*);
	sbi_error_t sbi_step(void*);
	sbi_error_t sbi_interrupt(const INTERRUPT id, void*);
    void sbi_cleanup(void*); // free resources after program run

    // accessor functions can be used
    // in user_function context to get/set parameters.
	byte getval(const byte type, const byte val, void*);
	unsigned int setval(const byte type, const byte num, const byte val, void*);
	
	
#endif
