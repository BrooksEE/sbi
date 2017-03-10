#ifndef _SBI_FORMAT_H
#define _SBI_FORMAT_H
	// SBI format
	#define HEADER_0						0xAA
	#define HEADER_1						0x5B // 0x4B 0x3B, 0x2B or 0x1B for older versions
	
	#define LABELSECTION						0xA3
	#define INTERRUPTSECTION					0xB3
	
	#define SEPARATOR						0xB7
	
	#define FOOTER_0						0x3A
	#define FOOTER_1						0xF0
	
	// Instructions
    // TODO when adding instruction
    // also add to instructions map in sasmc
    // perhaps add automated way to do that.

    // <var> = global variable _t[0-VARIABLESNUM]
    //         thread local _s[0-STACK_SIZE]
    //         _s+N special syntax to get to a current place
    //              in the stack. (Useful for retrieving local variables.)
    //         _sp current location of stack (read only)
    // <const> = 0-max int as defined by DATA_WIDTH
    // <any> = <var> or <const>
    // "string" used only with print. 
    
    // variables
	#define _istr_assign					0x01 // assign <var> <const> ; var=const // TODO redundant w/ move deprecate 
	#define _istr_move						0x02 // move <var> <any> ; var=any
	#define _istr_add						0x03 // add <any1> <any2> <var> ; var=any1+any2
	#define _istr_sub						0x04 // sub <any1> <any2> <var> ; var=any1-any2
	#define _istr_mul						0x05 // mul <any1> <any2> <var> ; var=any1*any2
	#define _istr_div						0x06 // div <any1> <any2> <var> ; var=any1/any2
    #define _istr_mod                       0x07 // mod <any1> <any2> <var> ; var=any1 % any2
    #define _istr_incr						0x08 // incr <var> ; <var> += 1
	#define _istr_decr						0x09 // decr <var> ; <var> -= 1
	#define _istr_inv						0x0A // inv <var> ; <var> = !<var>
	#define _istr_tob						0x0B // tob <var> ; <var> > 0  ? 1 : 0 
	#define _istr_cmp						0x0C // cmp <any1> <any2> <var> ; <var> = <any1> == <any2>
	#define _istr_high						0x0D // high <any1> <any2> <var> ; <var> = <any1> > <any2>
	#define _istr_low						0x0E // low <any1> <any2> <var> ; <var> = <any1> < <any2>
    #define _istr_lte                       0x0F // lte <any1> <any2> <var> ; <var> = <any1> <= <any2>
    #define _istr_gte                       0x10 // gte <any1> <any2> <var> ; <var> = <any1> >= <any2>

    // prg flow
	#define _istr_jump						0x20 // jump <any> 1|0 ; PC=<any>, i
	#define _istr_cmpjump                   0x21 // cmpjump <any1> <any2> <any3> 1|0 ; if <any1> == <any2> then like jump <any3> 
	#define	_istr_ret						0x22 // ret ; cause PC to jump to previous stored PC.  Function return.
	#define _istr_exit						0x23 // exit ; immediately exit program. 

    // stack
    #define _istr_push                      0x30 // push <any> ; push a value to the stack
    #define _istr_pop                       0x31 // pop |<var> ; pop value from stack, store value is optional
	                                                 // 2nd param if 1 causes ret to jump back to next PC
                                                 // use 0 for loops and 1 for function calls
    // functions
	#define _istr_debug						0x40 // debug <any> ;  cause programs debug function handler to be called
	#define _istr_error						0x41 // error <any> ;  cause programs error function to be called - aborts program
    #define _istr_print                     0x42 // print "string" ; print a string constant
    #define _istr_printd                    0x43 // printd <any>   ; print an integer value
	#define _istr_sint						0x44 // sint <const> ;  set current thread user function to <const>
	#define _istr_int						0x45 // int |<any> ... ; call current thread user function any number of args as parameters. 
    #define _istr_intr                      0x46 // intr <var> |<any> ... ; same as int but store func result in <var>

    // threads
    #define _istr_thread                    0x50 // thread <const> <var> ; start thread at const, store threadId in <var>
    #define _istr_wait                      0x51 // wait <var> ;  wait for thread to end that has threadId stored in <var>
    #define _istr_alive                     0x52 // alive <var1> <var2> ; store 1 in <var2> if <var1> is a valid running thread id.
    #define _istr_stop                      0x53 // stop <var> ; causes thread with id in <var> to stop running.
	
    // varIds used in bytecode to differentiate variable types.
	#define _varid							0x84 // a global variable
	#define _regid                          0x85 // thread local variables
    #define _stackid                        0x86 // a global stack location
    #define _stackvid                       0x87 // a global stack location specified by + N
    #define _stackpid                       0x88 // the current stack offset
	#define _value8							0xc1
    #define _value16                        0Xc2
    #define _value32                        0Xc4

#endif
