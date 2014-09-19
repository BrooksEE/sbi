#ifndef _SBI_FORMAT_H
#define _SBI_FORMAT_H
	// SBI format
	#define HEADER_0						0xAA
	#define HEADER_1						0x4B // 0x3B, 0x2B or 0x1B for older versions
	
	#define LABELSECTION						0xA3
	#define INTERRUPTSECTION					0xB3
	
	#define SEPARATOR						0xB7
	
	#define FOOTER_0						0x3A
	#define FOOTER_1						0xF0
	
	// Instructions
    // TODO when adding instruction
    // also add to instructions map in sasmc
    // perhaps add automated way to do that.

    // <var> = global variable _t[0-255]
    //         thread local _r[0-15]
    // <const> = 0-max int as defined by DATA_WIDTH
    // <any> = <var> or <const>
	#define _istr_assign					0x01 // assign <var> <const> ; var=const // TODO redundant w/ move deprecate 
	#define _istr_move						0x02 // move <var> <any> ; var=any
	#define _istr_add						0x10 // add <any> <any> <var> ; var=any+any
	#define _istr_sub						0x11 // sub <any> <any> <var> ; var=any-any
	#define _istr_mul						0x12 // mul <any> <any> <var> ; var=any*any
	#define _istr_div						0x13 // div <any> <any> <var> ; var=any/any
    #define _istr_push                      0x14 // push <any> ; push a value to the stack
    #define _istr_pop                       0x15 // pop |<var> ; pop value from stack, store value is optional
	#define _istr_incr						0x20 // incr <var> ; <var> += 1
	#define _istr_decr						0x21 // decr <var> ; <var> -= 1
	#define _istr_inv						0x22 // inv <var> ; <var> = !<var>
	#define _istr_tob						0x23 // tob <var> ; <var> = <var> ? 1 : 0 
	#define _istr_cmp						0x30 // cmp <any> <any> <var> ; <var> = <any> == <any>
	#define _istr_high						0x31 // high <any> <any> <var> ; <var> = <any> > <any>
	#define _istr_low						0x32 // low <any> <any> <var> ; <var> = <any> < <any>
    #define _istr_lte                       0x33 // lte <any> <any> <var> ; <var> = <any> <= <any>
    #define _istr_gte                       0x34 // gte <any> <any> <var> ; <var> = <any> >= <any>
	#define _istr_jump						0x41 // jump <any> 1|0 ; PC=<any>, i
                                                 // 2nd param if 1 causes ret to jump back to next PC
                                                 // use 0 for loops and 1 for function calls
	#define _istr_cmpjump                   0x42 // cmpjump <any> <any> <var> 1|0 ; if <any> == <any> then like jump 
	#define	_istr_ret						0x43 // ret ; cause PC to jump to previous stored PC.  Function return.
	#define _istr_debug						0x50 // debug <any> ;  cause programs debug function handler to be called
	#define _istr_error						0x52 // error <any> ;  cause programs error function to be called - aborts program
	#define _istr_sint						0x60 // sint <const> ;  set current thread user function to <const>
	#define _istr_int						0x61 // int |<any> ... ; call current thread user function any number of args as parameters. 
    #define _istr_intr                      0x62 // intr <var> |<any> ... ; same as int but store func result in <var>
    #define _istr_thread                    0x70 // thread <const> <var> ; start thread at const, store threadId in <var>
    #define _istr_wait                      0x71 // wait <var> ;  wait for thread to end that has threadId stored in <var>
	#define _istr_exit						0xFF // exit ; immediately exit program. 
	
    // varIds used in bytecode to differentiate variable types.
	#define _varid							0x04
    #define _regid                          0x05
	#define _value8							0xF1
    #define _value16                        0XF2
    #define _value32                        0XF4

#endif
