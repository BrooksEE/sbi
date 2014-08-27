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
	#define VARIABLESNUM	     	64
	#define USERFUNCTIONSN			16
	#define RETURNADDRESSESN		16
	
	
	// SBI format
	#define HEADER_0						0xAA
	#define HEADER_1						0x4B // 0x3B, 0x2B or 0x1B for older versions
	
	#define LABELSECTION				0xA3
	#define INTERRUPTSECTION		0xB3
	
	#define SEPARATOR						0xB7
	
	#define FOOTER_0						0x3A
	#define FOOTER_1						0xF0
	
	// Instructions
	#define _istr_assign				0x01
	#define _istr_move					0x02
	#define _istr_add						0x10
	#define _istr_sub						0x11
	#define _istr_mul						0x12
	#define _istr_div						0x13
	#define _istr_incr					0x20
	#define _istr_decr					0x21
	#define _istr_inv						0x22
	#define _istr_tob						0x23
	#define _istr_cmp						0x30
	#define _istr_high					0x31
	#define _istr_low						0x32
	#define _istr_jump					0x41
	#define _istr_cmpjump				0x42
	#define	_istr_ret						0x43
	#define _istr_debug					0x50
	#define _istr_error					0x52
	#define _istr_sint					0x60
	#define _istr_int						0x61
	#define _istr_exit					0xFF
	
	#define _varid							0x04
	#define _value							0xF4
	
    // all context functions take 2nd argument
    // of sbi_context_t* taken from passid in
    // context.

	// User functions
	typedef void (*sbi_user_func)(byte[], void*);

	// Put here your debug code
	typedef void(*debugn_func)(byte n, void*);

	// Put here your error printing code
	typedef void(*errorn_func)(byte n, void*);

    // returns the next byte from the source sbi
	typedef byte (*getfch_func)(void*);
    // set the source sbi pos
	typedef void (*setfpos_func)(const unsigned int, void*);
    // get the source sbi pos
	typedef unsigned int (*getfpos_func)(void*);
	
	typedef struct {
		debugn_func debugn;
		errorn_func errorn;
        getfch_func getfch;
        setfpos_func setfpos;
        getfpos_func getfpos;
	    sbi_user_func sbi_user_funcs[USERFUNCTIONSN];
	} sbi_context_t;
	
	byte _getval(const byte type, const byte val, void*);
	unsigned int _setval(const byte type, const byte num, const byte val, void*);
	
    /**
     *   Function: _sbi_init
     *   Param: ctx initialization values for current run context
     *   Param: userdata any type of additional userdata you may wish to use.  
     *                   can be NULL
     *   Return: void* for library state.  You must save this and pass it in
     *                 for each unique run context.
     **/
	void* _sbi_init(sbi_context_t* ctx, void* userdata);
	unsigned int _sbi_begin(void*);
	unsigned int _sbi_run(void*);

    // must be called to free resources
    // unless _sbi_init returned 0
    void _sbi_cleanup(void*);

    /**
     * returns your userdata from the runtime context
     **/
    void* _userdata(void*);
	
	int _interrupt(const unsigned int id, void*);
	
#endif
