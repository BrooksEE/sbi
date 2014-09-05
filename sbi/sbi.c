/* ========================================================================== */
/*                                                                            */
/*   sbi.c                                                                    */
/*   (c) 2012 Gi@cky98                                                        */
/*                                                                            */
/* ========================================================================== */

#include "sbi.h"
#include "sbi_inst.h"

#include <stdlib.h>
#include <string.h> // memset
#include <stdbool.h>
    
// SBI  stuctures
typedef enum
{
	ERROR = 0,
	STOPPED,
	RUNNING
} SBITHREADSTATUS;

typedef enum
{
	NOERROR = 0,
	REACHEDEND,
	EXITED,
	WRONGINSTR,
	WRONGBYTE,
	USERERROR
} SBITHREADERROR;

typedef struct
{
    byte threadid;
    PCOUNT p;
	SBITHREADSTATUS status;
	SBITHREADERROR _lasterror;
	INTERRUPT* _interrupts;
	RETADDR _returnaddresses[RETURNADDRESSESN];
    unsigned int raddr_cnt;
	USERFUNCID _userfid;
} SBITHREAD;
    
// internal runtime structure for global api
typedef struct
{
    // Multithreading variables
    sbi_context_t * ctx;
    SBITHREAD* _sbi_threads[THREADMAXNUM];
    SBITHREAD* _sbi_currentthread;
	VARIABLE _t[VARIABLESNUM];
	LABEL* _labels;
    #if _SBI_MULTITHREADING_EQUALTIME
    	SBITHREADNUM _sbi_currentthreadn;
    #endif
    unsigned int thread_cnt;
    byte new_threadid;
    unsigned int _exec;
} sbi_runtime_t;


// multi threading internal functions
unsigned int _sbi_new_thread_at(PCOUNT, sbi_runtime_t*);
SBITHREAD* _sbi_createthread(PCOUNT);
unsigned int _sbi_loadthread(SBITHREAD* thread, sbi_runtime_t*);
void _sbi_removethread(SBITHREAD* thread, sbi_runtime_t*);
SBITHREAD* _sbi_getthread(SBITHREADNUM n);
void _sbi_startthread(SBITHREAD* thread);
void _sbi_stopthread(SBITHREAD* thread);
SBITHREADSTATUS _sbi_getthreadstatus(SBITHREAD* thread);
SBITHREADERROR _sbi_getthreaderror(SBITHREAD* thread);


// Variables, labels and subroutines
// easy access
#define _debug(d)				RT->ctx->debugn(d,rt)
#define _error(d)				RT->ctx->errorn(d,rt)
#define _getfch()               RT->ctx->getfch(thread->p++, rt)
#define _T                      RT->_t
// for casting void* to runtime
#define RT ((sbi_runtime_t*)rt)
#define _RETADDRS               thread->_returnaddresses
#define CUR_PCOUNT              thread->p
#define _LABELS                 RT->_labels

// Interrupts
// TODO how do interrupts work with just one program?
// they happen independent of any thread?
// or perhaps they happen on the main thread.
#if 0 //def _SBI_MULTITHREADING_ENABLE
	SBITHREAD** _interrupts_threads;
#else
	INTERRUPT* _interrupts;
#endif

INTERRUPT _intinqueue = 0;

/*
	Gets the value of a parameter
 */
byte _getval(const byte type, const byte val, void* rt )
{
	if (type==_varid) return RT->_t[val]; else return val;
}

/*
	Sets the value of a variable
	Useful for user functions
		Return:
			0: 	All ok
			1: 	The specified parameter
			is not a variable
 */
unsigned int _setval(const byte type, const byte num, const byte val, void* rt) 
{
    if (type==_varid)
    {
    	RT->_t[num]=val;
    	return 0;
    } else {
    	return 1;
    }
}


/*
	Initializes the interpreter

    Returns a runtime for other functions
    or NULL if memory broken.
 */
void* sbi_init(sbi_context_t * ctx)
{
    sbi_runtime_t *rt = (sbi_runtime_t*)malloc(sizeof(sbi_runtime_t));
    if (!rt) return NULL;
    memset ( rt, 0, sizeof(sbi_runtime_t) );

    rt->ctx = ctx;

    return rt;
}

/*
 * Starts the main thread
 * TODO update to return error codes from global struct
 */
unsigned int sbi_begin(void *rt) {

    if (!rt) return 1;

    // create the main thread
	
	SBITHREAD* thread = _sbi_createthread(0);
    if (!thread) return 1;
	
    if (!RT->ctx->getfch) return 1;
 
	// Read head
	if (_getfch()!=HEADER_0) return 3;
	byte rd = _getfch();
	if (rd!=HEADER_1) {
		if ((rd==0x1B)||(rd==0x2B)||(rd==0x3B))
			return 2;
		else
			return 3;
	}
	
	// Getting labels
	if (_getfch()!=LABELSECTION) return 3;
	unsigned int ln = _getfch();
	RT->_labels = malloc(ln * sizeof(LABEL));
	unsigned int c = 0;
	while (ln--)
	{
			RT->_labels[c] = _getfch() | (_getfch() << 8);
			c++;
	}
	if (_getfch()!=SEPARATOR) return 3;
	
	// Getting interrupts addresses
	if (_getfch()!=INTERRUPTSECTION) return 3;
	ln = _getfch();
	thread->_interrupts = malloc(ln * sizeof(INTERRUPT));
	c = 0;
	while (ln--)
	{
			thread->_interrupts[c] = _getfch() | (_getfch() << 8);
			c++;
	}
	if (_getfch()!=SEPARATOR) return 3;

	int ret = _sbi_loadthread(thread, RT);
    if (!ret) _sbi_startthread(thread);
    return ret;

}

unsigned int _sbi_new_thread_at(PCOUNT p, sbi_runtime_t* rt) {
    SBITHREAD *t = _sbi_createthread(p);
    int ret = _sbi_loadthread(t, rt);
    if (!ret) _sbi_startthread(t);
    return ret;
}

/*
	Steps the program of one instruction

	Returns:
		0: 	No errors
		1: 	Reached end (no exit found)
		2: 	Program exited
		3: 	Wrong instruction code
		4: 	Can't understand byte
		5: 	User error
*/
unsigned int _sbi_step_internal(SBITHREAD* thread, sbi_runtime_t* rt)
{

    byte rd, var1, var2, var1t, var2t, var3, var3t;
    int i;

	RT->_exec = 1;
	
	rd = _getfch();
	switch (rd)
	{
		case _istr_assign:
			var1 = _getfch();
			_T[var1] = _getfch();
			break;
		case _istr_move:
			var1 = _getfch();
			_T[var1] = _T[_getfch()];
			break;
		case _istr_add:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			_T[_getfch()] = _getval(var1t, var1, rt) + _getval(var2t, var2, rt);
			break;
		case _istr_sub:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			_T[_getfch()] = _getval(var1t, var1, rt) - _getval(var2t, var2, rt);
			break;
		case _istr_mul:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			_T[_getfch()] = _getval(var1t, var1, rt) * _getval(var2t, var2, rt);
			break;
		case _istr_div:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			_T[_getfch()] = _getval(var1t, var1, rt) / _getval(var2t, var2, rt);
			break;
		case _istr_incr:
			_T[_getfch()]++;
			break;
		case _istr_decr:
			_T[_getfch()]--;
			break;
		case _istr_inv:
			var1 = _getfch();
			if (_T[var1]==0) _T[var1]=1; else _T[var1]=0;
			break;
		case _istr_tob:
			var1 = _getfch();
			if (_T[var1]>0) _T[var1]=1; else _T[var1]=0;
			break;
		case _istr_cmp:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			if (_getval(var1t, var1, rt)==_getval(var2t, var2, rt)) _T[_getfch()]=1; else _T[_getfch()]=0;
			break;
		case _istr_high:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			if (_getval(var1t, var1, rt)>_getval(var2t, var2, rt)) _T[_getfch()]=1; else _T[_getfch()]=0;
			break;
		case _istr_low:
			var1t = _getfch();
			var1 = _getfch();
			var2t = _getfch();
			var2 = _getfch();
			if (_getval(var1t, var1, rt)<_getval(var2t, var2, rt)) _T[_getfch()]=1; else _T[_getfch()]=0;
			break;
		case _istr_jump:
			var1t = _getfch();
			var1 = _getfch();
			if (_getfch() > 0)
			{
				for (i=RETURNADDRESSESN-2; i>0; i--) _RETADDRS[i+1] = _RETADDRS[i];
				_RETADDRS[1] = _RETADDRS[0];
				_RETADDRS[0] = CUR_PCOUNT;
                ++thread->raddr_cnt;
			}
			CUR_PCOUNT = _LABELS[_getval(var1t, var1, rt)];
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
				for (i=RETURNADDRESSESN-2; i>0; i--) _RETADDRS[i+1] = _RETADDRS[i];
				_RETADDRS[1] = _RETADDRS[0];
				_RETADDRS[0] = CUR_PCOUNT;
                ++thread->raddr_cnt;
			}
			if (_getval(var1t, var1, rt)==_getval(var2t, var2, rt))
			{
				CUR_PCOUNT = _LABELS[_getval(var3t, var3, rt)];
			}
			break;
		case _istr_ret:
            if (thread->raddr_cnt>0) {
			    CUR_PCOUNT = _RETADDRS[0];
    			for (i=1; i<RETURNADDRESSESN; i++) _RETADDRS[i-1] = _RETADDRS[i];
                --thread->raddr_cnt;
            } else {
                // thread exit
                return 1;
            }
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
			thread->_userfid=_getval(var1t, _getfch(), rt);
			break;
		case _istr_int:
            {
            static byte b[16]; // TODO 
			for (i=0; i<16; i++) b[i] = _getfch();
			RT->ctx->sbi_user_funcs[thread->_userfid](b,thread);
            }
			break;
        case _istr_thread:
            {
                var1t = _getfch();
                var1 = _getfch();
                var2 = _getfch();
                int ret = _sbi_new_thread_at(_LABELS[_getval(var1t,var1,rt)], rt);
                byte tId = !ret ? rt->new_threadid : 0;
                _setval( _varid, var2, tId, rt );
                if (ret) _error(ret);
            }
            break;
        case _istr_wait:
            {
                var1 = _getfch();
                byte tId = _getval(_varid,var1,rt);
                for (i=0;i<rt->thread_cnt;++i) {
                    if ( rt->_sbi_threads[i]->threadid == tId &&
                         rt->_sbi_threads[i]->status == RUNNING ) {
                         CUR_PCOUNT-=2; // rerun wait
                         break;
                    }
                }
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
	
	RT->_exec = 0;
	
    // TODO eval interrupts
	if (_intinqueue) interrupt(_intinqueue - 1, rt); // If there are interrupts in the queue, do it
	
	return 0;
}


/*
	Creates a new thread from an SBI stream structure
	Remember to initialize the thread before starting executing it (_sbi_loadthread)
*/	
SBITHREAD* _sbi_createthread(PCOUNT p)
{
	// Allocate memory for a new structure
	SBITHREAD* t = (SBITHREAD*) malloc(sizeof(SBITHREAD));
    if (!t) return NULL;
	
	// Initialize thread
	t->p = p;
	t->status = STOPPED;
	t->_userfid = 0;
    t->raddr_cnt=0;
	
	// Return created structure
	return t;
}

/*
	Initializes the thread by reading the SBI stream header
	(load labels, interrupt addresses, etc...) and puts it into the threads list
	
	Returns:
        TODO update errors
		0: 	No errors
		1: 	No function pointer for _getfch
		2:	Old version of executable format
		3:	Invalid program file
		4:	Can't load thread (threads number overflow)
*/
unsigned int _sbi_loadthread(SBITHREAD* thread, sbi_runtime_t* rt)
{
	
	// Load thread into threads list
    if (rt->thread_cnt > THREADMAXNUM-1) {
       return 4;
    }
    rt->_sbi_threads[rt->thread_cnt++] = thread;
    do {
      thread->threadid = ++rt->new_threadid;
    } while (!thread->threadid); // ids wrap but skip 0
	
	// Done
	return 0;
}

void _sbi_removethread(SBITHREAD* thread, sbi_runtime_t* rt)
{
    int i;
    bool remove=false;
    for ( i=0; i< rt->thread_cnt; ++i ) {
        if (rt->_sbi_threads[i] == thread) {
            free(thread->_interrupts);
            free(thread);
            remove=true;
            break;
        }
    }
    if (remove) {
       for ( ; i< rt->thread_cnt-1; ++i) {
         rt->_sbi_threads[i] = rt->_sbi_threads[i+1];
       }
       --rt->thread_cnt;
    }

}

/*
	Step all threads of one instruction
	Returns the error code for current thread that was stepped 

*/
unsigned int sbi_step(void *rt)
{
    unsigned int ret=0;
	#if !_SBI_MULTITHREADING_EQUALTIME
        // TODO fix
		//int c = 0;
		//int i;
		//for (i=0; i<THREADMAXNUM; i++)
		//	if (RT->_sbi_threads[i])
		//	{
		//		if (RT->_sbi_threads[i]->status == RUNNING)
		//		{
		//			unsigned int ret = _sbi_step_internal(_sbi_threads[i], RT);
		//			c++;
		//			if (ret)
		//			{
		//				RT->_sbi_threads[i]->_lasterror = ret;
		//				if (ret > 2)
		//					RT->_sbi_threads[i]->status = ERROR;
		//				else
		//					RT->_sbi_threads[i]->status = STOPPED;
		//			}
		//		}
		//	}
		//return c;
	#else
        SBITHREAD* thread = RT->_sbi_threads[RT->_sbi_currentthreadn];
		if (thread)
		{
			if (thread->status == RUNNING)
			{
				ret = _sbi_step_internal(thread,rt);
				if (ret)
				{
                    _sbi_removethread(thread,RT);
                    thread=NULL;
                    if (ret == 2) {
                       // program exit
                       int i;
                       for ( i=0;i<RT->thread_cnt;++i ) _sbi_stopthread(RT->_sbi_threads[i]); 
                    } else {
                       // don't skip the next thread 
                       // set it to the previous threadn so the next thread
                       // is not starved.
                       RT->_sbi_currentthreadn ==
                        RT->_sbi_currentthreadn == 0 ? 
                            RT->thread_cnt-1 :
                            RT->_sbi_currentthreadn-1;
                    }
				}
			}
		}
		RT->_sbi_currentthreadn++;
		if (RT->_sbi_currentthreadn > (RT->thread_cnt - 1)) RT->_sbi_currentthreadn = 0;
		return ret; 
	#endif
}

unsigned int sbi_running(void* rt)
{
	int c = 0;
	int i;
	for (i=0; i<RT->thread_cnt; i++)
		if (RT->_sbi_threads[i])
			if (RT->_sbi_threads[i]->status == RUNNING)
				c++;
	return c;
}

//SBITHREAD* _sbi_getthread(SBITHREADNUM n)
//{
//	return _sbi_threads[n];
//}

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


void interrupt(const unsigned int id, void* rt)
{
	if (RT->_exec==1) // Some code in execution, queue interrupt
	{
		_intinqueue = id + 1;
		return;
	}
    int i;	
	for (i=RETURNADDRESSESN-2; i>0; i--) RT->_sbi_currentthread->_returnaddresses[i+1] = RT->_sbi_currentthread->_returnaddresses[i];
	RT->_sbi_currentthread->_returnaddresses[1] = RT->_sbi_currentthread->_returnaddresses[0];
	RT->_sbi_currentthread->_returnaddresses[0] = RT->_sbi_currentthread->p;
	
	RT->_sbi_currentthread->p = RT->_sbi_currentthread->_interrupts[id]; // Set the program counter to interrupt's address
	
	_intinqueue = 0; // Be sure to clean the queue
	
	return;
}

void sbi_cleanup(void *rt) {
    int i;
    for (i=0;i<RT->thread_cnt;++i) {
      if (RT->_sbi_threads[i]) {
        free( RT->_sbi_threads[i]->_interrupts);
        free ( RT->_sbi_threads[i] );
      } else break;
    }
    free(RT->_labels);
    free(rt);
}

