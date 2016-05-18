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

#ifdef TRACE 
 #include <stdio.h>
 #define _ERR printf
 #define _TRACE printf
#else
 #define _ERR(...)
 #define _TRACE(...)
#endif


#ifdef ERROR
#undef ERROR // windows const
#endif
    
// SBI  stuctures
typedef enum
{
	ERROR,
	STOPPED,
	RUNNING
} SBITHREADSTATUS;

typedef struct
{
    DTYPE threadid;
    PCOUNT p;
	SBITHREADSTATUS status;
	RETADDR _returnaddresses[RETURNADDRESSESN];
    unsigned int raddr_cnt;
	USERFUNCID _userfid;
    DTYPE _r[REG_SIZE];
    DTYPE *_t; // ptr to global variables
    DTYPE stack[STACK_SIZE];
    unsigned int stackp;
} SBITHREAD;
    
// internal runtime structure for global api
typedef struct
{
    // Multithreading variables
    sbi_context_t * ctx;
    SBITHREAD* _sbi_threads[THREADMAXNUM];
    SBITHREAD* _sbi_currentthread;
	DTYPE _t[VARIABLESNUM];
	LABEL* _labels;
	INTERRUPT* _interrupts;
    INTERRUPT _intinqueue[INTQUEUE_SIZE];
    unsigned int int_cnt;
    #if _SBI_MULTITHREADING_EQUALTIME
    	SBITHREADNUM _sbi_currentthreadn;
    #endif
    unsigned int thread_cnt;
    DTYPE new_threadid;
} sbi_runtime_t;


// multi threading internal functions
sbi_error_t _sbi_new_thread_at(PCOUNT, sbi_runtime_t*);
SBITHREAD* _sbi_createthread(PCOUNT, sbi_runtime_t*);
sbi_error_t _sbi_loadthread(SBITHREAD* thread, sbi_runtime_t*);
void _sbi_removethread(SBITHREAD* thread, sbi_runtime_t*);

// for casting void* to runtime
#define RT ((sbi_runtime_t*)rt)
#define _RETADDRS               thread->_returnaddresses
#define CUR_PCOUNT              thread->p
#define _LABELS                 RT->_labels

// Variables, labels and subroutines
// easy access
#define _debug(d)				RT->ctx->debugn(d,rt)
#define _error(d)				RT->ctx->errorn(d,rt)
#define _print(d)               RT->ctx->print(d)
#define _printd(d)             RT->ctx->printd(d)
#define _getfch()               RT->ctx->getfch(thread->p++, rt)

/*
	Gets the value of a parameter
 */
DTYPE _getval(uint8_t type, DTYPE val, void* t)
{
    SBITHREAD *thread=(SBITHREAD*)t;
	if (type==_varid) return thread->_t[val];
    if (type==_regid) return thread->_r[val];
    return val;
}

/*
	Sets the value of a variable
	Useful for user functions
		Return:
			0: 	All ok
			1: 	The specified parameter
			is not a variable
 */
unsigned int _setval(uint8_t type, uint8_t num, DTYPE val, void* t) 
{
    SBITHREAD *thread=(SBITHREAD*)t;
    if (type==_varid)
    {
    	thread->_t[num]=val;
    	return 0;
    }
    if (type==_regid) {
        thread->_r[num]=val;
        return 0;
    }

    return 1;
}


/*
	Initializes the interpreter

    Returns a runtime for other functions
    or NULL if memory broken.
 */
void* sbi_init(sbi_context_t * ctx)
{
//	_TRACE ( "sbi_init require %d bytes\n", sizeof(sbi_runtime_t) );
    sbi_runtime_t *rt = (sbi_runtime_t*)malloc(sizeof(sbi_runtime_t));
    if (!rt) {
    	_ERR ( "sbi_init malloc error\n");
    	return NULL;
    }
    memset ( rt, 0, sizeof(sbi_runtime_t) );

    rt->ctx = ctx;

    return rt;
}

/*
 * Starts the main thread
 */
sbi_error_t sbi_begin(void *rt) {

    SBITHREAD *thread;
    uint8_t rd;
    unsigned int ln, c;
    int ret;

    if (!rt) return SBI_INVALID_RT;

    // create the main thread
	
	thread = _sbi_createthread(0, (sbi_runtime_t*)rt);
    if (!thread) return SBI_ALLOC_ERROR;
	
    if (!RT->ctx->getfch) return SBI_CTX_ERROR;
 
	// Read head
	if (_getfch()!=HEADER_0) return SBI_HEADER_ERROR;
	rd = _getfch();
	if (rd!=HEADER_1) {
		if ((rd==0x1B)||(rd==0x2B)||(rd==0x3B))
			return SBI_HEADER_OLD;
		else
			return SBI_HEADER_ERROR;
	}
	
	// Getting labels
	if (_getfch()!=LABELSECTION) return SBI_HEADER_ERROR;
	ln = _getfch();
	RT->_labels = malloc(ln * sizeof(LABEL));
	c = 0;
	while (ln--)
	{
			RT->_labels[c] = _getfch() | (_getfch() << 8);
			c++;
	}
	if (_getfch()!=SEPARATOR) return SBI_HEADER_ERROR;
	
	// Getting interrupts addresses
	if (_getfch()!=INTERRUPTSECTION) return SBI_HEADER_ERROR;
	ln = _getfch();
	RT->_interrupts = malloc(ln * sizeof(INTERRUPT));
	c = 0;
	while (ln--)
	{
			RT->_interrupts[c] = _getfch() | (_getfch() << 8);
			c++;
	}
	if (_getfch()!=SEPARATOR) return SBI_HEADER_ERROR;

	ret = _sbi_loadthread(thread, RT);
    return ret;

}

sbi_error_t _sbi_new_thread_at(PCOUNT p, sbi_runtime_t* rt) {
    SBITHREAD *t = _sbi_createthread(p, rt);
    int ret = _sbi_loadthread(t, rt);
    return ret;
}

DTYPE _sbi_getfval(uint8_t type, SBITHREAD* thread, sbi_runtime_t* rt) {
    DTYPE val = _getfch();
    switch (type) {
        case _regid:
        case _varid:
        case _value8:
            break;
        case _value16:
        case _value32:
            val |= (DTYPE)_getfch() << 8; 
            if (type==_value16) break; 
            val |= (DTYPE)_getfch() << 16;
            val |= (DTYPE)_getfch() << 24;
            break;
    }
    return val;
}
#define _getfval(type) _sbi_getfval(type,thread,rt)

/*
	Steps the program of one instruction

	Returns: sbi_error_t
*/
sbi_error_t _sbi_step_internal(SBITHREAD* thread, sbi_runtime_t* rt)
{

    uint8_t rd, var1t, var2t, var3t;
    DTYPE var1, var2, var3;
    int i;

	rd = _getfch();
    _TRACE("Instruction code 0x%02x at pcount: 0x%02x thread %d\n", rd, CUR_PCOUNT-1, thread->threadid );
	switch (rd)
	{
		case _istr_assign:
            var1t= _getfch();
			var1 = _getfch();
			_setval(var1t,var1,_getfval(_getfch()), thread);
			break;
		case _istr_move:
            var1t = _getfch();
			var1 = _getfch();
            var2t = _getfch();
            var2 = _getfval(var2t);
            _setval ( var1t, var1,
                      _getval(var2t,var2, thread),
                      thread );
			break;
		case _istr_add:
		case _istr_sub:
        case _istr_mul:
        case _istr_div:
        case _istr_cmp:
        case _istr_high:
        case _istr_low:
        case _istr_lte:
        case _istr_gte:
	    {
	    	DTYPE v1, v2, val;

			var1t = _getfch();
			var1 = _getfval(var1t);
			var2t = _getfch();
			var2 = _getfval(var2t);
            var3t = _getfch();
            var3 = _getfch();
            v1=_getval(var1t,var1, thread); 
            v2=_getval(var2t,var2, thread);
            val = rd == _istr_add ? v1+v2 :
                       rd == _istr_sub ? v1-v2 :
                       rd == _istr_mul ? v1*v2 :
                       rd == _istr_div ? v1/v2 :
                       rd == _istr_cmp ? (v1==v2?1:0) :
                       rd == _istr_high ? (v1>v2?1:0) :
                       rd == _istr_low ? (v1<v2?1:0) :
                       rd == _istr_lte ? (v1<=v2?1:0) :
                       rd == _istr_gte ? (v1>=v2?1:0) :
                       0;
            _setval( var3t, var3, 
                     val,
                     thread );
	    }
			break;
        case _istr_push:
            if (thread->stackp>=STACK_SIZE-1) {
                _error(SBI_STACK_OVERFLOW); // TODO error codes (overflow)
                return SBI_PROG_ERROR;
            }
            var1t=_getfch();
            var1 = _getfval(var1t);
            thread->stack[thread->stackp++] = _getval(var1t,var1,thread);
            break;
        case _istr_pop:
            {
	    	uint8_t n;
                if (thread->stackp==0) {
                    _error(SBI_STACK_UNDERFLOW); // underflow
                    return SBI_PROG_ERROR;
                }
                n = _getfch();
                --thread->stackp;
                if (n) {
                    DTYPE val = thread->stack[thread->stackp];
                    var1t=_getfch();
                    var1=_getfch();
                    _setval(var1t,var1,val,thread);
                }
            }
            break;
		case _istr_incr:
		case _istr_decr:
        case _istr_inv:
        case _istr_tob:
            {
	    	DTYPE val;
                var1t=_getfch();
                var1=_getfch();
                val = _getval( var1t, var1, thread);
                val = rd == _istr_incr ? val+1 :
                      rd == _istr_decr ? val-1 :
                      rd == _istr_inv ? !val :
                      rd == _istr_tob ? (val?1:0) :
                      0;
                _setval(var1t,var1,val,thread);
            }
            break;
		case _istr_jump:
			var1t = _getfch();
			var1 = _getfval(var1t);
			if (_getfch() > 0)
			{
                _RETADDRS[thread->raddr_cnt++] = CUR_PCOUNT;
			}
			CUR_PCOUNT = _LABELS[_getval(var1t, var1, thread)];
			break;
		case _istr_cmpjump:
			var1t = _getfch();
			var1 = _getfval(var1t);
			var2t = _getfch();
			var2 = _getfval(var2t);
			var3t = _getfch();
			var3 = _getfch();
            i=_getfch(); // push ret
			if (_getval(var1t, var1, thread)==_getval(var2t, var2, thread))
			{
			    if (i > 0)
			    {
                    _RETADDRS[thread->raddr_cnt++] = CUR_PCOUNT;
			    }
				CUR_PCOUNT = _LABELS[_getval(var3t, var3, thread)];
			} 
			break;
		case _istr_ret:
            if (thread->raddr_cnt>0) {
			    CUR_PCOUNT = _RETADDRS[--thread->raddr_cnt];
            } else {
                // thread exit
                return SBI_THREAD_EXIT;
            }
			break;
		case _istr_debug:
			var1t = _getfch();
            var1 = _getfval(var1t);
			_debug(_getval(var1t, var1, thread));
			break;
		case _istr_error:
			var1t = _getfch();
            var1 = _getfval(var1t);
			_error(_getval(var1t, var1, thread));
			return SBI_PROG_ERROR;
			break;
        case _istr_print:
            { 
                uint16_t strLoc = _getfch();
                strLoc |= _getfch()<<8;
                if (rt->ctx->print) {
                    int slen, tmplen;
                    char *tmp, cur;
                    PCOUNT curp = CUR_PCOUNT; 
                    CUR_PCOUNT = strLoc;
                    slen = 0;
                    tmplen=0;
                    tmp = NULL;
                    cur;
                    do {
                        cur = _getfch();
                        if (tmplen==slen) {
                           tmp = (char*) realloc ( tmp, slen+20 ); 
                           slen += 20;
                           if (!tmp) return SBI_ALLOC_ERROR;
                        }
                        tmp[tmplen++] = cur;
                    } while (cur != 0);
                    _print(tmp);
                    free(tmp);
                    CUR_PCOUNT = curp; 
                }
            }
            break;
        case _istr_printd:
            {
                var1t = _getfch();
                var1 = _getfval(var1t);
                if (rt->ctx->printd)
                    _printd(_getval(var1t,var1,thread));
            }
            break;
		case _istr_sint:
			var1t = _getfch();
            var1 = _getfval(var1t);
			thread->_userfid=_getval(var1t, var1, thread);
			break;
		case _istr_int:
        case _istr_intr:
            {
	    	uint8_t argc;
		DTYPE *pvals=NULL, r;
                // NOTE should parameters be pushed on the stack instead?
                if (rd==_istr_intr) {
                    var1t=_getfch();
                    var1=_getfch();
                }
                argc = _getfch();
                if (argc>0) {
                    pvals = malloc(sizeof(DTYPE)*(argc));
                    if (!pvals) return SBI_ALLOC_ERROR; 
                }
                for (i=0;i<argc;++i)
                {
                    var2t = _getfch();
                    var2 = _getfval(var2t);
                    pvals[i] = _getval(var2t,var2,thread);
                }
		r = RT->ctx->sbi_user_funcs[thread->_userfid](argc,pvals);
                if (rd==_istr_intr) {
                    _setval(var1t,var1,r,thread);
                }
                if (pvals) free(pvals);
            }
			break;
        case _istr_thread:
            {
	    	int ret;
		uint8_t tId;
                var1t = _getfch();
                var1 = _getfch();
                var2t = _getfch();
                var2 = _getfch();
                ret = _sbi_new_thread_at(_LABELS[_getval(var1t,var1,thread)], rt);
                tId = !ret ? rt->new_threadid : 0;
                _setval( var2t, var2, tId, thread );
                if (ret) _error(ret);
                return ret;
            }
            break;
        case _istr_wait:
            {
	        DTYPE tId;
                var1t = _getfch();
                var1 = _getfch();
                tId = _getval(var1t,var1,thread);
                for (i=0;i<rt->thread_cnt;++i) {
                    if ( rt->_sbi_threads[i]->threadid == tId &&
                         rt->_sbi_threads[i]->status == RUNNING ) {
                         CUR_PCOUNT-=3; // rerun wait
                         break;
                    }
                }
            }
            break;
        case _istr_alive:
            {
	    DTYPE tId, val;
            var1t = _getfch();
            var1 = _getfch();
            var2t = _getfch();
            var2 = _getfch();
            tId = _getval(var1t,var1,thread);
            val = 0; // not running
            for (i=0;i<rt->thread_cnt;++i) {
               if (rt->_sbi_threads[i]->threadid == tId &&
                   rt->_sbi_threads[i]->status == RUNNING) {
                   val=1;
                   break;
               }
            }
            _setval(var2t,var2,val,thread);
            }
            break;
        case _istr_stop:
            {
	    DTYPE tId;
            var1t = _getfch();
            var1 = _getfch();
            tId = _getval(var1t,var1,thread);
            for (i=0;i<rt->thread_cnt;++i) {
               if (rt->_sbi_threads[i]->threadid == tId) {
                  rt->_sbi_threads[i]->status = STOPPED;
                  break;
               }
            }
            }
            break;
		case _istr_exit:
			return SBI_PROG_EXIT;
			break;
		case FOOTER_0:
			if (_getfch()==FOOTER_1) return SBI_PROG_EOF; else return SBI_INSTR_ERROR;
		default:
			_error(SBI_INSTR_ERROR);
            _error(rd);
            _error(CUR_PCOUNT-1);
            _ERR("Instruction error 0x%02x at pcount: 0x%02x thread %d\n", rd, CUR_PCOUNT-1, thread->threadid );
			return SBI_PROG_ERROR;
			break;
	}
	
	return SBI_NOERROR;
}


/*
	Creates a new thread from an SBI stream structure
	Remember to initialize the thread before starting executing it (_sbi_loadthread)
*/	
SBITHREAD* _sbi_createthread(PCOUNT p, sbi_runtime_t *rt)
{
	SBITHREAD *t;
	// Allocate memory for a new structure
	_TRACE( "_sbi_createthread require %d bytes\n", sizeof(SBITHREAD));
	t = (SBITHREAD*) malloc(sizeof(SBITHREAD));
    if (!t) return NULL;
	
	// Initialize thread
	t->p = p;
	t->status = STOPPED;
	t->_userfid = 0;
    t->raddr_cnt=0;
    t->stackp = 0;
    t->_t=rt->_t;
	
	// Return created structure
	return t;
}

/*
	Initializes the thread by reading the SBI stream header
	(load labels, interrupt addresses, etc...) and puts it into the threads list
	
	Returns: sbi_error_t
*/
sbi_error_t _sbi_loadthread(SBITHREAD* thread, sbi_runtime_t* rt)
{
	
	// Load thread into threads list
    if (rt->thread_cnt > THREADMAXNUM-1) {
       return SBI_THREAD_MAX;
    }
    rt->_sbi_threads[rt->thread_cnt++] = thread;
    do {
      thread->threadid = ++rt->new_threadid;
    } while (!thread->threadid); // ids wrap but skip 0
    thread->status=RUNNING;
	
	// Done
	return SBI_NOERROR;
}

void _sbi_removethread(SBITHREAD* thread, sbi_runtime_t* rt)
{
    int i;
    int remove=0;
    for ( i=0; i< rt->thread_cnt; ++i ) {
        if (rt->_sbi_threads[i] == thread) {
            free(thread);
            remove=1;
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
sbi_error_t sbi_step(void *rt)
{
    unsigned int ret=0;
    SBITHREAD *thread;
    if (!rt) return SBI_INVALID_RT;
    if (!RT->thread_cnt) return SBI_NOT_RUNNING;
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
        thread = RT->_sbi_threads[RT->_sbi_currentthreadn];
		if (thread)
		{
			if (thread->status == RUNNING)
			{
                if (RT->int_cnt>0) {
		    INTERRUPT id;
                    _RETADDRS[thread->raddr_cnt++] = CUR_PCOUNT;
                    id = RT->_intinqueue[--RT->int_cnt];
	                CUR_PCOUNT = RT->_interrupts[id]; // Set the program counter to interrupt's address
                }
				ret = _sbi_step_internal(thread,rt);
            } else if (thread->status == STOPPED) {
                ret = SBI_THREAD_EXIT;
            }

			if (ret)
			{
                _TRACE ( "Thread (%d) exit: %d\n", thread->threadid, ret );
                _sbi_removethread(thread,RT);
                thread=NULL;
                if (ret >= SBI_PROG_EXIT) {
                   // program exit
                   int i;
                   for ( i=0;i<RT->thread_cnt;++i ) RT->_sbi_threads[i]->status = STOPPED;
                } else {
                   // don't skip the next thread 
                   // set it to the previous threadn so the next thread
                   // is not starved.
                   RT->_sbi_currentthreadn =
                    RT->_sbi_currentthreadn == 0 ? 
                        RT->thread_cnt-1 :
                        RT->_sbi_currentthreadn-1;
                }
			}
		} else {
          // should not get this
          _error(SBI_INTERNAL_ERROR);
        }
		RT->_sbi_currentthreadn++;
		if (RT->_sbi_currentthreadn > (RT->thread_cnt - 1)) RT->_sbi_currentthreadn = 0;
	#endif

	return ret; 
}

unsigned int sbi_running(void* rt)
{
	int c = 0;
	int i;
	if (!rt) return 0;
	for (i=0; i<RT->thread_cnt; i++)
		if (RT->_sbi_threads[i])
			if (RT->_sbi_threads[i]->status == RUNNING)
				c++;
	return c;
}

void _sbi_stopthread(SBITHREAD* thread)
{
	thread->status = STOPPED;
}

sbi_error_t sbi_interrupt(const unsigned int id, void* rt)
{
	if (rt) {
        if (RT->int_cnt < INTQUEUE_SIZE-1) {
		    RT->_intinqueue[RT->int_cnt++] = id;
        } else
            return SBI_INT_LOST;
    } else 
        return SBI_INVALID_RT;

    return SBI_NOERROR;
}

void sbi_cleanup(void *rt) {
    int i;
	if (!rt) return;
    for (i=0;i<RT->thread_cnt;++i) {
      if (RT->_sbi_threads[i]) {
        free ( RT->_sbi_threads[i] );
      } else break;
    }
    free(RT->_interrupts);
    free(RT->_labels);
    free(rt);
}

