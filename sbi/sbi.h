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

    struct sbi_context_t;

	// User functions
	typedef void (*sbi_user_func)(byte[], sbi_context_t*);

	// Put here your debug code
	typedef void(*debugn_func)(byte n, sbi_context_t*);

	// Put here your error printing code
	typedef void(*errorn_func)(byte n, sbi_context_t*);

    // returns the next byte from the source sbi
	typedef byte (*getfch_func)(sbi_context_t*);
    // set the source sbi pos
	typedef void (*setfpos_func)(const unsigned int, sbi_context_t*);
    // get the source sbi pos
	typedef unsigned int (*getfpos_func)(sbi_context_t*);
	
	struct sbi_context_t {
		debugn_func debugn;
		errorn_func errorn;
        getfch_func getfch;
        setfpos_func setfpos;
        getfpos_func getfpos;
	    sbi_user_func sbi_user_funcs[USERFUNCTIONSN];
	};
	
	byte _getval(const byte type, const byte val);
	unsigned int _setval(const byte type, const byte num, const byte val);
	
	void _sbi_init(sbi_context_t*);
	unsigned int _sbi_begin(sbi_context_t*);
	unsigned int _sbi_run(sbi_context_t*);
	
	void _interrupt(const unsigned int id, sbi_context_t*);
	
#endif
