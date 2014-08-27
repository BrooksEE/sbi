/* ========================================================================== */
/*                                                                            */
/*   sbi.c                                                                    */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#include "sbi.h"

#include <stdlib.h>




// function macros for easy access 
#define RT(v)                   ((sbi_runtime_t*)v)

#define _debug(d)				RT(rt)->_userCtx->debugn(d,rt)
#define _error(d)				RT(rt)->_userCtx->errorn(d,rt)
#define _getfch()               RT(rt)->_userCtx->getfch(rt)
#define _setfpos(p)             RT(rt)->_userCtx->setfpos(p,rt)
#define _getfpos(p)             RT(rt)->_userCtx->getfpos(rt)


// internal structure for carrying program state

typedef struct {
    // Variables, labels and subroutines
    byte _t[VARIABLESNUM];
    unsigned int* _labels;
    unsigned int _returnaddresses[RETURNADDRESSESN];

    // Interrupts
    unsigned int* _interrupts;
    unsigned int _exec;
    unsigned int _intinqueue;
    unsigned int _queuedint;

   // User functions
   unsigned int _userfid;

   sbi_context_t * _userCtx; 
   void * _userdata;
 
} sbi_runtime_t;

void* _userdata(void* rt) {
  return RT(rt)->_userdata; 
}


/*
	Gets the value of a parameter
 */
byte _getval(const byte type, const byte val, void *rt)
{
	if (type==_varid) return RT(rt)->_t[val]; else return val;
}

/*
	Sets the value of a variable
	Useful for user functions
		Return:
			0: 	All ok
			1: 	The specified parameter
			is not a variable
 */
unsigned int _setval(const byte type, const byte num, const byte val, void *rt) 
{
	if (type==_varid)
	{
		RT(rt)->_t[num]=val;
		return 0;
	} else {
		return 1;
	}
}


/*
	Initializes the interpreter
 */
void* _sbi_init(sbi_context_t* c, void* d)
{
    sbi_runtime_t* rt = (sbi_runtime_t*)calloc(sizeof(sbi_runtime_t), 1);
    if (!rt) return 0;

    rt->_userCtx=c;
    rt->_userdata=d;
    return rt;
}

void _sbi_cleanup( void* rt){
    if (!rt) return;
    if (RT(rt)->_labels)
        free ( RT(rt)->_labels );
    if (RT(rt)->_interrupts)
        free ( RT(rt)->_interrupts);
    free(rt);
}

/*
	Begins program execution
 */
unsigned int _sbi_begin(void* rt)       		// Returns:
												// 					0: 	No errors
												//					1: 	No function pointers for _getfch,
												//							_setfpos and _getfpos
												//					2:	Old version of executable format
												//					3:	Invalid program file
{
	// Check function pointers
    if (RT(rt)->_userCtx==0) return 1;
	if ((RT(rt)->_userCtx->getfch==0)||(RT(rt)->_userCtx->setfpos==0)||(RT(rt)->_userCtx->getfpos==0)) return 1;
	
	// Read head
	byte rd = _getfch();
	if (rd!=HEADER_0) return 3;
	rd = _getfch();
	if (rd!=HEADER_1) {
		if ((rd==0x1B)||(rd==0x2B)||(rd==0x3B))
			return 2;
		else
			return 3;
	}
	
	// Getting labels
	if (_getfch()!=LABELSECTION) return 3;
	unsigned int ln = _getfch();
	RT(rt)->_labels = (unsigned int*)malloc(ln * sizeof(int));
	unsigned int c = 0;
	while (ln--)
	{
    RT(rt)->_labels[c] = _getfch() | (_getfch() << 8);
    c++;
	}
	if (_getfch()!=SEPARATOR) return 3;
	
	// Getting interrupts addresses
	if (_getfch()!=INTERRUPTSECTION) return 3;
	ln = _getfch();
	RT(rt)->_interrupts = (unsigned int*)malloc(ln + ln); //ln * sizeof(unsigned int) -> ln * 2 -> ln+ln
	c = 0;
	while (ln--)
	{
    RT(rt)->_interrupts[c] = _getfch() | (_getfch() << 8);
    c++;
	}
	if (_getfch()!=SEPARATOR) return 3;
	
	// Done
	return 0;
}

/*
	Executes the program
 */
unsigned int _sbi_run(void *rt)       // Runs a SBI program
												// Returns:
												// 					0: 	No errors
												// 					1: 	Reached end (no exit found)
												//					2: 	Program exited
												//					3: 	Wrong instruction code
												//					4: 	Can't understand byte
												//					5: 	User error
{
	RT(rt)->_exec = 1;
	
	byte rd = _getfch();
    byte var1, var1t, var2, var2t, var3, var3t;
    unsigned int i;
    byte b[16];

	switch (rd)
	{
		case _istr_assign:
			var1 = _getfch();
			RT(rt)->_t[var1] = _getfch();
			break;
		case _istr_move:
			var1 = _getfch();
			RT(rt)->_t[var1] = RT(rt)->_t[_getfch()];
			break;
		case _istr_add:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			RT(rt)->_t[_getfch()] = _getval(var1t, var1, rt) + _getval(var2t, var2, rt);
			break;
		case _istr_sub:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			RT(rt)->_t[_getfch()] = _getval(var1t, var1, rt) - _getval(var2t, var2, rt);
			break;
		case _istr_mul:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			RT(rt)->_t[_getfch()] = _getval(var1t, var1, rt) * _getval(var2t, var2, rt);
			break;
		case _istr_div:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			RT(rt)->_t[_getfch()] = _getval(var1t, var1, rt) / _getval(var2t, var2, rt);
			break;
		case _istr_incr:
			RT(rt)->_t[_getfch()]++;
			break;
		case _istr_decr:
			RT(rt)->_t[_getfch()]--;
			break;
		case _istr_inv:
			var1 = _getfch();
			if (RT(rt)->_t[var1]==0) RT(rt)->_t[var1]=1; else RT(rt)->_t[var1]=0;
			break;
		case _istr_tob:
			var1 = _getfch();
			if (RT(rt)->_t[var1]>0) RT(rt)->_t[var1]=1; else RT(rt)->_t[var1]=0;
			break;
		case _istr_cmp:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			if (_getval(var1t, var1, rt)==_getval(var2t, var2, rt)) RT(rt)->_t[_getfch()]=1; else RT(rt)->_t[_getfch()]=0;
			break;
		case _istr_high:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			if (_getval(var1t, var1, rt)>_getval(var2t, var2, rt)) RT(rt)->_t[_getfch()]=1; else RT(rt)->_t[_getfch()]=0;
			break;
		case _istr_low:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			if (_getval(var1t, var1, rt)<_getval(var2t, var2, rt)) RT(rt)->_t[_getfch()]=1; else RT(rt)->_t[_getfch()]=0;
			break;
		case _istr_jump:
			var1t = _getfch();
			var1 = _getfch();
			if (_getfch() > 0)
			{
				for (i=RETURNADDRESSESN-2; i>0; i--) RT(rt)->_returnaddresses[i+1] = RT(rt)->_returnaddresses[i];
				RT(rt)->_returnaddresses[1] = RT(rt)->_returnaddresses[0];
				RT(rt)->_returnaddresses[0] = _getfpos();
			}
			_setfpos(RT(rt)->_labels[_getval(var1t, var1, rt)]);
			break;
		case _istr_cmpjump:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			var3t = _getfch();
			var3 = _getfch();
			if (_getfch() > 0)
			{
				for (i=RETURNADDRESSESN-2; i>0; i--) RT(rt)->_returnaddresses[i+1] = RT(rt)->_returnaddresses[i];
				RT(rt)->_returnaddresses[1] = RT(rt)->_returnaddresses[0];
				RT(rt)->_returnaddresses[0] = _getfpos();
			}
			if (_getval(var1t, var1, rt)==_getval(var2t, var2, rt))
			{
				_setfpos(RT(rt)->_labels[_getval(var3t, var3, rt)]);
			}
			break;
		case _istr_ret:
			_setfpos(RT(rt)->_returnaddresses[0]);
			for (i=1; i<RETURNADDRESSESN; i++) RT(rt)->_returnaddresses[i-1] = RT(rt)->_returnaddresses[i];
			break;
		case _istr_debug:
			var1t = _getfch();
			_debug(_getval(var1t, _getfch(), rt));
			break;
		case _istr_error:
			var1t = _getfch();
			_error(_getval(var1t, _getfch(), rt));
			return 5;
			break;
		case _istr_sint:
			var1t = _getfch();
			RT(rt)->_userfid=_getval(var1t, _getfch(), rt);
			break;
		case _istr_int:
            {
			   for (i=0; i<16; i++) b[i] = _getfch();
			   RT(rt)->_userCtx->sbi_user_funcs[RT(rt)->_userfid](b,rt);
            }
			break;
		case _istr_exit:
			return 2;
			break;
		case FOOTER_0:
			if (_getfch()==FOOTER_1) return 1; else return 4;
		default:
			_error(0xB1);
			return 3;
			break;
	}
	
	RT(rt)->_exec = 0;
	
	if (RT(rt)->_intinqueue==1) _interrupt(RT(rt)->_queuedint, rt); // If there are interrupts in
																							// in the queue, do it
	
	return 0;
}

void _interrupt(const unsigned int id, void *rt)
{
    unsigned int i;
	if (RT(rt)->_exec==1) // Some code in execution, queue interrupt
	{
		RT(rt)->_intinqueue = 1;
		RT(rt)->_queuedint = id;
		return;
	}
	
	for (i=RETURNADDRESSESN-2; i>0; i--) RT(rt)->_returnaddresses[i+1] = RT(rt)->_returnaddresses[i];
	RT(rt)->_returnaddresses[1] = RT(rt)->_returnaddresses[0];
	RT(rt)->_returnaddresses[0] = _getfpos();
	
	_setfpos(RT(rt)->_interrupts[id]); // Set the program counter to interrupt's address
	
	RT(rt)->_intinqueue = 0; // Be sure to clean the queue
	
	return;
}
