/* ========================================================================== */
/*                                                                            */
/*   sbi.h                                                                    */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#ifdef _WIN32
#include <windows.h> // windows dtypes
#define uint8_t unsigned __int8 
#define uint16_t unsigned __int16 
#define uint32_t unsigned __int32 
#else
#include <stdint.h>
#endif

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

    // data width in bytes for variables
    // registers, stack etc.  
    #ifndef DATA_WIDTH
        #define DATA_WIDTH 1
    #endif

    #if DATA_WIDTH == 4
    typedef uint32_t DTYPE;
    #elif DATA_WIDTH == 2
    typedef uint16_t DTYPE; 
    #elif DATA_WIDTH == 1
    typedef uint8_t DTYPE;
    #else
    #error "DATA_WIDTH must be 1, 2 or 4"
    #endif
	
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
	
    typedef enum {
      SBI_NOERROR,
      SBI_THREAD_EXIT, // normal thread exit
      SBI_PROG_EXIT,  // normal exit via exit command
      SBI_PROG_EOF, // program exited no exit command
      SBI_INVALID_RT, // void* for context is null 
      SBI_ALLOC_ERROR, // unable to allocate memory
      SBI_CTX_ERROR,   // uninitialized function in context
      SBI_HEADER_ERROR, // invalid program header
      SBI_HEADER_OLD,  // old program format
      SBI_PROG_ERROR,  // program runtime error
      SBI_STACK_OVERFLOW, // push called when stack is full
      SBI_STACK_UNDERFLOW, // pop called when stack is empty
      SBI_DIV_BY_0,    // divice by 0
      SBI_INSTR_ERROR, // unrecognized instruction
      SBI_THREAD_MAX, // attempt to allocate more than max threads
      SBI_NOT_RUNNING, // returned by sbi step if all threads have exited.
      SBI_INT_LOST, // more interrupts queued than INTQUEUE_SIZE
      SBI_INTERNAL_ERROR // byte interpreter interpreter error.
    } sbi_error_t;
	

	// User functions
    // params = argc, argv
	typedef DTYPE (*sbi_user_func)(uint8_t, DTYPE*);

	// Put here your debug code
	typedef void(*debugn_func)(DTYPE n, void*);

	// Put here your error printing code
	typedef void(*errorn_func)(DTYPE n, void* );

    // Print support
    // this is optional, if the platform
    // can't support printing you can
    // leave it NULL
    typedef void(*print_func)(const char*);
    typedef void(*printd_func)(DTYPE d);
    
    // returns the next byte from the source sbi
	typedef uint8_t (*getfch_func)(PCOUNT p, void*);
	
	typedef struct {
		debugn_func debugn;
		errorn_func errorn;
        getfch_func getfch;
        print_func print;
        printd_func printd;
	    sbi_user_func sbi_user_funcs[USERFUNCTIONSN];
        void* userdata;
	} sbi_context_t;
	
	
    // public api 
    #ifdef __cplusplus
    extern "C" {
    #endif
	void* sbi_init(sbi_context_t*);
    sbi_error_t sbi_begin(void*);
	sbi_error_t sbi_running(void*);
	sbi_error_t sbi_step(void*);
	sbi_error_t sbi_interrupt(const INTERRUPT id, void*);
    void sbi_cleanup(void*); // free resources after program run
    #ifdef __cplusplus
    } // end extern c
    #endif

	
#endif
