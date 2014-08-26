/* ========================================================================== */
/*                                                                            */
/*   sbi.c                                                                    */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#include "sbi.h"

#include <stdlib.h>

// function macros for easy access 
#define _debug(d)				ctx->debugn(d,ctx)
#define _error(d)				ctx->errorn(d,ctx)
#define _getfch()               ctx->getfch(ctx)
//#define _setfpos(p)             ctx->setfpos(p,ctx)
//#define _getfpos(p)             ctx->getfpos(ctx)

// Variables, labels and subroutines
#ifndef _SBI_MULTITHREADING_ENABLE
	VARIABLE _t[VARIABLESNUM];
	LABEL* _labels;
	RETADDR _returnaddresses[RETURNADDRESSESN];
	PCOUNT p;
	USERFUNCID _userfid;
    getfch_func _getfch;
#else
	SBITHREAD* _sbi_threads[THREADMAXNUM];
	SBITHREAD* _sbi_currentthread;
	#if _SBI_MULTITHREADING_EQUALTIME
		SBITHREADNUM _sbi_currentthreadn;
	#endif
#endif

// Interrupts
#ifdef _SBI_MULTITHREADING_ENABLE
	SBITHREAD** _interrupts_threads;
#else
	INTERRUPT* _interrupts;
#endif

unsigned int _exec = 0;
INTERRUPT _intinqueue = 0;

// User functions
#ifndef _SBI_MULTITHREADING_ENABLE
	USERFUNCID _userfid = 0;
#endif

/*
	Gets the value of a parameter
 */
byte _getval(const byte type, const byte val)
{
	#ifndef _SBI_MULTITHREADING_ENABLE
		if (type==_varid) return _t[val]; else return val;
	#else
		if (type==_varid) return _sbi_currentthread->_t[val]; else return val;
	#endif
}

/*
	Sets the value of a variable
	Useful for user functions
		Return:
			0: 	All ok
			1: 	The specified parameter
			is not a variable
 */
unsigned int _setval(const byte type, const byte num, const byte val) 
{
	#ifndef _SBI_MULTITHREADING_ENABLE
		if (type==_varid)
		{
			_t[num]=val;
			return 0;
		} else {
			return 1;
		}
	#else
		if (type==_varid)
		{
			_sbi_currentthread->_t[num]=val;
			return 0;
		} else {
			return 1;
		}
	#endif
}

/*
	Common variables
 */
byte rd;
unsigned int var1t;
unsigned int var1;
unsigned int var2t;
unsigned int var2;
unsigned int var3t;
unsigned int var3;
byte b[16];
unsigned int i;

/*
	Initializes the interpreter
 */
void _sbi_init(struct sbi_context_t * ctx)
{
	
	#ifdef _SBI_MULTITHREADING_ENABLE
		int i;
		for (i=0; i<THREADMAXNUM; i++) _sbi_threads[i] = NULL;
		_sbi_currentthread = NULL;
		#if _SBI_MULTITHREADING_EQUALTIME
			_sbi_currentthreadn = 0;
		#endif
	#else
		_userfid = 0;
		p = 0;
	#endif
}

/*
	Following ONLY in NON multithreading mode
*/

#ifndef _SBI_MULTITHREADING_ENABLE
	/*
		Begins program execution (no multithreading)
		
		Returns:
			0: 	No errors
			1: 	No function pointer for _getfch
			2:	Old version of executable format
			3:	Invalid program file
	*/

	unsigned int _sbi_begin(struct sbi_context_t *ctx)

    {
    	// Check function pointers
    	if ((ctx->getfch==0)) return 1;
        
   		// Initialize program counter
		p = 0;
 	
    	// Read head
    	rd = _getfch();
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
    	_labels = (unsigned int*)malloc(ln * sizeof(int));
    	unsigned int c = 0;
		while (ln--)
		{
    			_labels[c] = _getfch() | (_getfch() << 8);
    			c++;
		}
		if (_getfch()!=SEPARATOR) return 3;
		
		// Getting interrupts addresses
		if (_getfch()!=INTERRUPTSECTION) return 3;
		ln = _getfch();
		_interrupts = malloc(ln + ln); //ln * sizeof(unsigned int) -> ln * 2 -> ln+ln
		c = 0;
		while (ln--)
		{
    			_interrupts[c] = _getfch() | (_getfch() << 8);
    			c++;
		}
		if (_getfch()!=SEPARATOR) return 3;
		
		// Done
		return 0;
	}

	/*
		Steps the program of one instruction (no multithreading)
	
		Returns:
			0: 	No errors
			1: 	Reached end (no exit found)
			2: 	Program exited
			3: 	Wrong instruction code
			4: 	Can't understand byte
			5: 	User error
	*/
	unsigned int _sbi_step(struct sbi_context_t* ctx)
	{
		_exec = 1;
		
		rd = _getfch();
		switch (rd)
		{
			case _istr_assign:
				var1 = _getfch();
				_t[var1] = _getfch();
				break;
			case _istr_move:
				var1 = _getfch();
				_t[var1] = _t[_getfch()];
				break;
			case _istr_add:
				var1t = _getfch();
				var1 = _getfch();
				var2t = _getfch();
				var2 = _getfch();
				_t[_getfch()] = _getval(var1t, var1) + _getval(var2t, var2);
				break;
			case _istr_sub:
				var1t = _getfch();
				var1 = _getfch();
				var2t = _getfch();
				var2 = _getfch();
				_t[_getfch()] = _getval(var1t, var1) - _getval(var2t, var2);
				break;
			case _istr_mul:
				var1t = _getfch();
				var1 = _getfch();
				var2t = _getfch();
				var2 = _getfch();
				_t[_getfch()] = _getval(var1t, var1) * _getval(var2t, var2);
				break;
			case _istr_div:
				var1t = _getfch();
				var1 = _getfch();
				var2t = _getfch();
				var2 = _getfch();
				_t[_getfch()] = _getval(var1t, var1) / _getval(var2t, var2);
				break;
			case _istr_incr:
				_t[_getfch()]++;
				break;
			case _istr_decr:
				_t[_getfch()]--;
				break;
			case _istr_inv:
				var1 = _getfch();
				if (_t[var1]==0) _t[var1]=1; else _t[var1]=0;
				break;
			case _istr_tob:
				var1 = _getfch();
				if (_t[var1]>0) _t[var1]=1; else _t[var1]=0;
				break;
			case _istr_cmp:
				var1t = _getfch();
				var1 = _getfch();
				var2t = _getfch();
				var2 = _getfch();
				if (_getval(var1t, var1)==_getval(var2t, var2)) _t[_getfch()]=1; else _t[_getfch()]=0;
				break;
			case _istr_high:
				var1t = _getfch();
				var1 = _getfch();
				var2t = _getfch();
				var2 = _getfch();
				if (_getval(var1t, var1)>_getval(var2t, var2)) _t[_getfch()]=1; else _t[_getfch()]=0;
				break;
			case _istr_low:
				var1t = _getfch();
				var1 = _getfch();
				var2t = _getfch();
				var2 = _getfch();
				if (_getval(var1t, var1)<_getval(var2t, var2)) _t[_getfch()]=1; else _t[_getfch()]=0;
				break;
			case _istr_jump:
				var1t = _getfch();
				var1 = _getfch();
				if (_getfch() > 0)
				{
					for (i=RETURNADDRESSESN-2; i>0; i--) _returnaddresses[i+1] = _returnaddresses[i];
					_returnaddresses[1] = _returnaddresses[0];
					_returnaddresses[0] = p;
				}
				p = _labels[_getval(var1t, var1)];
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
					for (i=RETURNADDRESSESN-2; i>0; i--) _returnaddresses[i+1] = _returnaddresses[i];
					_returnaddresses[1] = _returnaddresses[0];
					_returnaddresses[0] = p;
				}
				if (_getval(var1t, var1)==_getval(var2t, var2))
				{
					p = _labels[_getval(var3t, var3)];
				}
				break;
			case _istr_ret:
				p = _returnaddresses[0];
				for (i=1; i<RETURNADDRESSESN; i++) _returnaddresses[i-1] = _returnaddresses[i];
				break;
			case _istr_debug:
				var1t = _getfch();
				_debug(_getval(var1t, _getfch()));
				break;
			case _istr_error:
				var1t = _getfch();
				_error(_getval(var1t, _getfch()));
				return 5;
				break;
			case _istr_sint:
				var1t = _getfch();
				_userfid=_getval(var1t, _getfch());
				break;
			case _istr_int:
				for (i=0; i<16; i++) b[i] = _getfch();
				ctx->sbi_user_funcs[_userfid](b,ctx);
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
		
		_exec = 0;
		
		if (_intinqueue) _interrupt(_intinqueue - 1, ctx); // If there are interrupts in the queue, do it
		
		return 0;
	}

#else

/*
	Following ONLY IN multithreading mode
*/
	
	SBISTREAM* _sbi_createstream(byte (*_getfch)(PCOUNT*))
	{
		// Allocate memory for a new structure
		SBISTREAM* s = (SBISTREAM*) malloc(sizeof(SBISTREAM));
		
		// Initialize stream
		s->_getfch = _getfch;
		s->p = 0;
		
		// Return created structure
		return s;
	}
	/*
		Creates a new thread from an SBI stream structure
		Remember to initialize the thread before starting executing it (_sbi_loadthread)
	*/	
	SBITHREAD* _sbi_createthread(SBISTREAM* stream)
	{
		// Allocate memory for a new structure
		SBITHREAD* t = (SBITHREAD*) malloc(sizeof(SBITHREAD));
		
		// Initialize thread
		t->stream = stream;
		t->status = STOPPED;
		t->_userfid = 0;
		t->_labels = NULL; // Used as thread-loaded indicator
		
		// Return created structure
		return t;
	}
	
	/*
		Initializes the thread by reading the SBI stream header
		(load labels, interrupt addresses, etc...) and puts it into the threads list
		
		Returns:
			0: 	No errors
			1: 	No function pointer for _getfch
			2:	Old version of executable format
			3:	Invalid program file
			4:	Can't load thread (threads number overflow)
	*/
	unsigned int _sbi_loadthread(SBITHREAD* thread)
	{
		// Check function pointers
		if (!(thread->stream->_getfch)) return 1;
		
		// Initialize program counter
		thread->stream->p = 0;
		
		// Read head
		if ((*thread->stream->_getfch)(&thread->stream->p)!=HEADER_0) return 3;
		rd = (*thread->stream->_getfch)(&thread->stream->p);
		if (rd!=HEADER_1) {
			if ((rd==0x1B)||(rd==0x2B)||(rd==0x3B))
				return 2;
			else
				return 3;
		}
		
		// Getting labels
		if ((*thread->stream->_getfch)(&thread->stream->p)!=LABELSECTION) return 3;
		unsigned int ln = (*thread->stream->_getfch)(&thread->stream->p);
		thread->_labels = malloc(ln * sizeof(LABEL));
		unsigned int c = 0;
		while (ln--)
		{
    			thread->_labels[c] = (*thread->stream->_getfch)(&thread->stream->p) | ((*thread->stream->_getfch)(&thread->stream->p) << 8);
    			c++;
		}
		if ((*thread->stream->_getfch)(&thread->stream->p)!=SEPARATOR) return 3;
		
		// Getting interrupts addresses
		if ((*thread->stream->_getfch)(&thread->stream->p)!=INTERRUPTSECTION) return 3;
		ln = (*thread->stream->_getfch)(&thread->stream->p);
		thread->_interrupts = malloc(ln * sizeof(INTERRUPT));
		c = 0;
		while (ln--)
		{
    			thread->_interrupts[c] = (*thread->stream->_getfch)(&thread->stream->p) | ((*thread->stream->_getfch)(&thread->stream->p) << 8);
    			c++;
		}
		if ((*thread->stream->_getfch)(&thread->stream->p)!=SEPARATOR) return 3;
		
		// Load thread into threads list
		unsigned int i;
		byte ok = 0;
		for (i=0; i<THREADMAXNUM; i++)
			if (!_sbi_threads[i])
			{
				_sbi_threads[i] = thread;
				ok = 1;
				break;
			}
		
		// Check if the thread was loaded
		if (!ok) return 4;
		
		// Done
		return 0;
	}
	
	void _sbi_removethread(SBITHREAD* thread)
	{
		// TODO
	}
	
	/*
		Steps the specified thread of one instruction (only in multithreading mode)
	
		Returns:
			0: 	No errors
			1: 	Reached end (no exit found)
			2: 	Program exited
			3: 	Wrong instruction code
			4: 	Can't understand byte
			5: 	User error
	*/
	unsigned int _sbi_step(SBITHREAD* thread)
	{
		_sbi_currentthread = thread;
		
		_exec = 1;
		
		rd = (*thread->stream->_getfch)(&thread->stream->p);
		switch (rd)
		{
			case _istr_assign:
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				thread->_t[var1] = (*thread->stream->_getfch)(&thread->stream->p);
				break;
			case _istr_move:
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				thread->_t[var1] = thread->_t[(*thread->stream->_getfch)(&thread->stream->p)];
				break;
			case _istr_add:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				thread->_t[(*thread->stream->_getfch)(&thread->stream->p)] = _getval(var1t, var1) + _getval(var2t, var2);
				break;
			case _istr_sub:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				thread->_t[(*thread->stream->_getfch)(&thread->stream->p)] = _getval(var1t, var1) - _getval(var2t, var2);
				break;
			case _istr_mul:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				thread->_t[(*thread->stream->_getfch)(&thread->stream->p)] = _getval(var1t, var1) * _getval(var2t, var2);
				break;
			case _istr_div:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				thread->_t[(*thread->stream->_getfch)(&thread->stream->p)] = _getval(var1t, var1) / _getval(var2t, var2);
				break;
			case _istr_incr:
				thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]++;
				break;
			case _istr_decr:
				thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]--;
				break;
			case _istr_inv:
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				if (thread->_t[var1]==0) thread->_t[var1]=1; else thread->_t[var1]=0;
				break;
			case _istr_tob:
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				if (thread->_t[var1]>0) thread->_t[var1]=1; else thread->_t[var1]=0;
				break;
			case _istr_cmp:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				if (_getval(var1t, var1)==_getval(var2t, var2)) thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]=1; else thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]=0;
				break;
			case _istr_high:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				if (_getval(var1t, var1)>_getval(var2t, var2)) thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]=1; else thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]=0;
				break;
			case _istr_low:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				if (_getval(var1t, var1)<_getval(var2t, var2)) thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]=1; else thread->_t[(*thread->stream->_getfch)(&thread->stream->p)]=0;
				break;
			case _istr_jump:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				if ((*thread->stream->_getfch)(&thread->stream->p) > 0)
				{
					for (i=RETURNADDRESSESN-2; i>0; i--) thread->_returnaddresses[i+1] = thread->_returnaddresses[i];
					thread->_returnaddresses[1] = thread->_returnaddresses[0];
					thread->_returnaddresses[0] = thread->stream->p;
				}
				thread->stream->p = thread->_labels[_getval(var1t, var1)];
				break;
			case _istr_cmpjump:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				var1 = (*thread->stream->_getfch)(&thread->stream->p);
				var2t = (*thread->stream->_getfch)(&thread->stream->p);
				var2 = (*thread->stream->_getfch)(&thread->stream->p);
				var3t = (*thread->stream->_getfch)(&thread->stream->p);
				var3 = (*thread->stream->_getfch)(&thread->stream->p);
				if ((*thread->stream->_getfch)(&thread->stream->p) > 0)
				{
					for (i=RETURNADDRESSESN-2; i>0; i--) thread->_returnaddresses[i+1] = thread->_returnaddresses[i];
					thread->_returnaddresses[1] = thread->_returnaddresses[0];
					thread->_returnaddresses[0] = thread->stream->p;
				}
				if (_getval(var1t, var1)==_getval(var2t, var2))
				{
					thread->stream->p = thread->_labels[_getval(var3t, var3)];
				}
				break;
			case _istr_ret:
				thread->stream->p = thread->_returnaddresses[0];
				for (i=1; i<RETURNADDRESSESN; i++) thread->_returnaddresses[i-1] = thread->_returnaddresses[i];
				break;
			case _istr_debug:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				_debug(_getval(var1t, (*thread->stream->_getfch)(&thread->stream->p)));
				break;
			case _istr_error:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				_error(_getval(var1t, (*thread->stream->_getfch)(&thread->stream->p)));
				return 5;
				break;
			case _istr_sint:
				var1t = (*thread->stream->_getfch)(&thread->stream->p);
				thread->_userfid=_getval(var1t, (*thread->stream->_getfch)(&thread->stream->p));
				break;
			case _istr_int:
				for (i=0; i<16; i++) b[i] = (*thread->stream->_getfch)(&thread->stream->p);
				ctx->sbi_user_funcs[thread->_userfid](b, ctx);
				break;
			case _istr_exit:
				return 2;
				break;
			case FOOTER_0:
				if ((*thread->stream->_getfch)(&thread->stream->p)==FOOTER_1) return 1; else return 4;
			default:
				_error(0xB1);
				return 3;
				break;
		}
		
		_exec = 0;
		
		if (_intinqueue) _interrupt(_intinqueue - 1); // If there are interrupts in the queue, do it
		
		return 0;
	}
	
	/*
		Step all threads of one instruction
		Returns the number of threads "stepped"
	*/
	unsigned int _sbi_stepall(void)
	{
		#if !_SBI_MULTITHREADING_EQUALTIME
			int c = 0;
			int i;
			for (i=0; i<THREADMAXNUM; i++)
				if (_sbi_threads[i])
				{
					if (_sbi_threads[i]->status == RUNNING)
					{
						unsigned int ret = _sbi_step(_sbi_threads[i]);
						c++;
						if (ret)
						{
							_sbi_threads[i]->_lasterror = ret;
							if (ret > 2)
								_sbi_threads[i]->status = ERROR;
							else
								_sbi_threads[i]->status = STOPPED;
						}
					}
				}
			return c;
		#else
			byte done = 0;
			if (_sbi_threads[_sbi_currentthreadn])
			{
				if (_sbi_threads[_sbi_currentthreadn]->status == RUNNING)
				{
					unsigned int ret = _sbi_step(_sbi_threads[_sbi_currentthreadn]);
					done = 1;
					if (ret)
					{
						_sbi_threads[_sbi_currentthreadn]->_lasterror = ret;
						if (ret > 2)
							_sbi_threads[_sbi_currentthreadn]->status = ERROR;
						else
							_sbi_threads[_sbi_currentthreadn]->status = STOPPED;
					}
				}
			}
			_sbi_currentthreadn++;
			if (_sbi_currentthreadn > (THREADMAXNUM - 1)) _sbi_currentthreadn = 0;
			return (unsigned int)done;
		#endif
	}
	
	unsigned int _sbi_running(void)
	{
		int c = 0;
		int i;
		for (i=0; i<THREADMAXNUM; i++)
			if (_sbi_threads[i])
				if (_sbi_threads[i]->status == RUNNING)
					c++;
		return c;
	}
	
	SBITHREAD* _sbi_getthread(SBITHREADNUM n)
	{
		return _sbi_threads[n];
	}
	
	void _sbi_startthread(SBITHREAD* thread)
	{
		thread->status = RUNNING;
	}
	
	void _sbi_stopthread(SBITHREAD* thread)
	{
		thread->status = STOPPED;
	}
	
	SBITHREADSTATUS _sbi_getthreadstatus(SBITHREAD* thread)
	{
		return thread->status;
	}
	
	SBITHREADERROR _sbi_getthreaderror(SBITHREAD* thread)
	{
		return thread->_lasterror;
	}

#endif

void _interrupt(const unsigned int id, struct sbi_context_t* ctx)
{
	if (_exec==1) // Some code in execution, queue interrupt
	{
		_intinqueue = id + 1;
		return;
	}
	
	#ifndef _SBI_MULTITHREADING_ENABLE
		for (i=RETURNADDRESSESN-2; i>0; i--) _returnaddresses[i+1] = _returnaddresses[i];
		_returnaddresses[1] = _returnaddresses[0];
		_returnaddresses[0] = p;
		
		p = _interrupts[id]; // Set the program counter to interrupt's address
		
		_intinqueue = 0; // Be sure to clean the queue
	#else
		for (i=RETURNADDRESSESN-2; i>0; i--) _sbi_currentthread->_returnaddresses[i+1] = _sbi_currentthread->_returnaddresses[i];
		_sbi_currentthread->_returnaddresses[1] = _sbi_currentthread->_returnaddresses[0];
		_sbi_currentthread->_returnaddresses[0] = _sbi_currentthread->stream->p;
		
		_sbi_currentthread->stream->p = _sbi_currentthread->_interrupts[id]; // Set the program counter to interrupt's address
		
		_intinqueue = 0; // Be sure to clean the queue
	#endif
	
	return;
}
