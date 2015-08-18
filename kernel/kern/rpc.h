/*
 * kern/rpc.h - remote procedural call interface
 * 
 * Copyright (c) 2015 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS-kernel.
 *
 * ALMOS-kernel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS-kernel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS-kernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _RPC_H_
#define _RPC_H_

#include <errno.h>
#include <types.h>
#include <bits.h>
#include <atomic.h>
#include <remote_access.h>
#include <remote_fifo.h>

#define RPC_DEFAULT_PROC_HANDLER 0
#define RPC_ALLOCATE_ON_STACK_SZ 2048

#define RPC_PRIO_START 0
typedef enum
{
	RPC_PRIO_NRML = RPC_PRIO_START,			/* Fixed Priority */
	RPC_PRIO_LAZY,
	RPC_PRIO_NR
}rpc_prio_t;

#define RPC_PRIO_MAPPER         RPC_PRIO_NRML
#define RPC_PRIO_PPM            RPC_PRIO_NRML
#define RPC_PRIO_FS             RPC_PRIO_NRML
#define RPC_PRIO_FS_STAT        RPC_PRIO_NRML
#define RPC_PRIO_FS_LOOKUP      RPC_PRIO_NRML
#define RPC_PRIO_FS_WRITE       RPC_PRIO_NRML
#define RPC_PRIO_PID_ALLOC      RPC_PRIO_NRML
#define RPC_PRIO_SIG_RISE       RPC_PRIO_NRML
#define RPC_PRIO_PS             RPC_PRIO_NRML
#define RPC_PRIO_TSK_LOOKUP     RPC_PRIO_NRML
#define RPC_PRIO_EXEC           RPC_PRIO_NRML

/* RPC FIFO type */
struct remote_fifo_s;

struct rpc_s;
struct task_s;
struct cpu_s;

typedef void rpc_handler_t(struct rpc_s* rpc);


struct rpc_s
{
	gid_t gid;
	uint_t prio;// ?
	size_t size;
	struct rpc_s *ret_rpc;//pre allocated buffer for the response
	size_t ret_size;
	struct thread_s *sender;
	rpc_handler_t *handler;
	uint8_t args_nb;
	volatile bool_t response;//is this message a response ?
};

/** opaque events listner */
struct rpc_listner_s
{
	uint_t count;
	atomic_t handled;
	volatile uint_t pending;
	//volatile uint_t prio; //CACHELINE;
	struct remote_fifo_s fifo_tbl[RPC_PRIO_NR];
};

struct rpc_ptr_s
{
	struct rpc_s* ptr;
	size_t size;
	cid_t cid;
};

void* thread_rpc_manager(void *arg);
error_t rpc_listner_init(struct rpc_listner_s *rl);
error_t rpc_listner_notify(struct rpc_listner_s *rl, uint_t *irq_state);

//return ENOMEM in case...
error_t rppc_generic(gid_t gid, bool_t to_clst, size_t prio, void* func, 
				size_t nb_ret, size_t nb_arg, 
				void* rets[], size_t rets_sz[], 
				void* args[], size_t args_sz[]);

error_t rpc_send_sync(gid_t gid, bool_t to_clst, size_t prio, void* func, 
				size_t nbret, size_t nbarg, 
				void* ret[], size_t ret_sz[], 
				struct rpc_s* rpc, size_t size,
				struct rpc_s* ret_rpc, size_t ret_size);
//error_t rpc_send_async(struct rpc_s *rpc, void *ret);

void* rpc_get_arg(struct rpc_s *rpc, size_t arg_nb);
size_t rpc_get_arg_sz(struct rpc_s *rpc, size_t arg_nb);
void rpc_print(struct rpc_s* rpc);
error_t rpc_send_rsp(struct rpc_s* sender_rpc, struct rpc_s* rpc,
					size_t size, uint32_t nb_ret);
bool_t rpc_check();
error_t rpc_thread_create(struct cpu_s *cpu, struct task_s *task);

/* el must be local */
#define rpc_is_pending(el)	((el)->pending) 
#define rpc_is_handled(el)	(atomic_get(&(el)->handled))
#define rpc_set_handled(el)	(atomic_test_set(&(el)->handled, (uint_t) current_thread))
#define rpc_check_handler(el)	((void*)atomic_get(&(el)->handled) == current_thread)
#define rpc_unset_handled(el)	(atomic_init(&(el)->handled, 0))

/****************************************************************************************/

#define RPC_FUNC_NAME(_func) _func ##_main
#define RPC_FUNC_LOCAL(_func) _func ##_local
#define RPC_FUNC_DEMARSHALL(_func) _func ##_demarshall

#define RPC_ALIGN (sizeof(void*))

/*
 * RPC ALLOCATOR *
 */
#define __RPC_ALLOCATE_ON_HEAP(buff, size)	\
	do{buff= NULL; printk(ERROR, "TODO! RPC_ALLOC!\n");}while(1)

//Does the alignement really necessary
//Doesn't the compiler already align it ?
#define __RPC_ALLOCATE(buff, size)		\
	size_t on_stack_size = 0;		\
	if(size <= RPC_ALLOCATE_ON_STACK_SZ)	\
		on_stack_size = size;		\
	else __RPC_ALLOCATE_ON_HEAP(buff, size);\
	uint8_t sbuff[on_stack_size+RPC_ALIGN];	\
	if(size <= RPC_ALLOCATE_ON_STACK_SZ)	\
		buff = (uint8_t*)ARROUND_UP	\
		(((uint32_t)&sbuff[0]),RPC_ALIGN)

#define __RPC_FREE(buff, size)			\
	do{					\
	if(size > RPC_ALLOCATE_ON_STACK_SZ)	\
		printk(ERROR, "TODO! rpc_free\n");	\
	}while(0)

/*
 * RPC REMOTE CALL PROCEDURES *
 */

#define __RPPC_GENERIC(_gid, to_clst, _prio, _func_name, _nb_ret, _nb_arg,  _ret_ptr, _ret_sz, _args, _args_sz)\
	__rpc_ret = rppc_generic(_gid, to_clst, _prio, RPC_FUNC_DEMARSHALL(_func_name),  \
	_nb_ret, _nb_arg,  _ret_ptr, _ret_sz, _args, _args_sz); __rpc_ret

/* 
 * RPC calls								*
 * RPPCX (X is the number of return values and Y the number of		*
 * arguments) is require that a function to be remotely executed	*
 * on a processor gid							*
 * RCPCXY require that a function be remotely executed on		*
 * a cluster cid							*
 */

#define RPC_RECV_OBJ(_val) ((void*)&_val), sizeof(_val)

extern void* __rpc_null;
#define RPC_SEND_NULL __rpc_null, 0
#define RPC_SEND_OBJ(_val) ((void*)&_val), sizeof(_val)
#define RPC_SEND_MEM(_ptr, _size) ((void*)_ptr), _size


#define __RCPC_TABLE(type, name, ...) type name[__GET_NB_ARG(__VA_ARGS__)] = {__VA_ARGS__};
#define __RCPC_RET(...) __RCPC_TABLE(void*, rets, __VA_ARGS__)				
#define __RCPC_RET_SZ(...) __RCPC_TABLE(size_t, rets_sz, __VA_ARGS__)				
#define __RCPC_ARG(...) __RCPC_TABLE(void*, args, __VA_ARGS__)				
#define __RCPC_ARG_SZ(...) __RCPC_TABLE(size_t, args_sz, __VA_ARGS__)				

#define __RCPC__(_cid, _prio, _func_name, _nb_ret, _nb_arg, ...)	\
({									\
	if(_cid == current_cid)	RPC_FUNC_LOCAL(_func_name)(__VA_ARGS__);\
	else								\
	__RPPC_GENERIC(arch_cpu_gid(_cid, cpu_get_lid()), true, _prio,	\
					_func_name, _nb_ret, _nb_arg, 	\
					rets, rets_sz, args, args_sz);	\
})


#define RPC_SEND(...) __VA_ARGS__
#define RPC_RECV(...) __VA_ARGS__

#define __RCPC_GET_FIRST_(a, b, c)	a
#define __RCPC_GET_FIRST(...) FOR_EACH2(__RCPC_GET_FIRST_, __VA_ARGS__)

#define __RCPC_GET_SECND_(a, b, c)	b
#define __RCPC_GET_SECND(...) FOR_EACH2(__RCPC_GET_SECND_, __VA_ARGS__)



#define RCPC(_cid, _prio, _func_name, _rets, _args)	\
({									\
	error_t __rpc_ret = 0;						\
	__RCPC_RET(__RCPC_GET_FIRST(_rets))				\
	__RCPC_RET_SZ(__RCPC_GET_SECND(_rets))				\
	__RCPC_ARG(__RCPC_GET_FIRST(_args))				\
	__RCPC_ARG_SZ(__RCPC_GET_SECND(_args))				\
	__RCPC__(_cid, _prio, _func_name,				\
		(__GET_NB_ARG(_rets)/2), (__GET_NB_ARG(_args)/2),	\
		__RCPC_GET_FIRST(_rets), __RCPC_GET_FIRST(_args));	\
})

#define __RPPC__(_gid, _prio, _func_name, _nb_ret, _nb_arg, ...)\
	__RPPC(_gid, _prio, _func_name, _nb_ret, _nb_arg, ##__VA_ARGS__)

#define __RPPC(_gid, _prio, _func_name, _nb_ret, _nb_arg, ...)			\
({										\
	if(_gid == current_cpu->gid) RPC_FUNC_LOCAL(_func_name)(__VA_ARGS__);	\
	else								\
	__RPPC_GENERIC(_gid, false, _prio, _func_name,			\
				ID(_nb_ret), ID(_nb_arg),		\
				rets, rets_sz, args, args_sz);		\
})

#define COMMA_PLACE00(a, b) 
#define COMMA_PLACE01(a, b) ,b
#define COMMA_PLACE10(a, b) ,a 
#define COMMA_PLACE11(a, b) ,a, b
#define COMMA_PLACE2(a,b)	\
	PASTE3(COMMA_PLACE,IS_EMPTY(a),IS_EMPTY(b))(ID(a),ID(b))

//(_rets:IS_EMPTY(_rets)); (_args:IS_EMPTY(_args))
#define RPPC(_gid, _prio, _func_name, _rets, _args)	\
({									\
	error_t __rpc_ret = 0;						\
	__RCPC_RET(__RCPC_GET_FIRST(_rets))				\
	__RCPC_RET_SZ(__RCPC_GET_SECND(_rets))				\
	__RCPC_ARG(__RCPC_GET_FIRST(_args))				\
	__RCPC_ARG_SZ(__RCPC_GET_SECND(_args))				\
	__RPPC__(_gid, _prio, _func_name,				\
	(__GET_NB_ARG(_rets)/2), (__GET_NB_ARG(_args)/2)	\
	COMMA_PLACE2(__RCPC_GET_FIRST(_rets), __RCPC_GET_FIRST(_args)));		\
})

/* 
 * RPC function declaration					*
 * and RPC responses (to return the value of the functions)	*
 */
#define RPC_RSP_SEND(_sender_rpc, _rpc, size, _nb_ret )	\
({							\
	rpc_send_rsp(_sender_rpc, _rpc, size, _nb_ret);		\
})

#define __RPPC_RESPONSE_PREPARE(_nb_ret, _ret_sz, _ret, _rpc_rsp)	\
	size_t size = 0;				\
	uint8_t *buff;					\
	size_t align;					\
	int i;						\
	align= sizeof(void*);				\
	size+= sizeof(struct rpc_s);			\
	for(i = 0; i < _nb_ret; i++)			\
		size+=ARROUND_UP(_ret_sz[i], align)+4;	\
	__RPC_ALLOCATE(buff, size);			\
	_rpc_rsp = (struct rpc_s*)buff;			\
	_rpc_rsp->size = size;				\
	buff += sizeof(struct rpc_s);			\
	for(i = 0; i < _nb_ret; i++)			\
		((uint32_t*)buff)[i] = _ret_sz[i];	\
	buff += _nb_ret*4;				\
	for(i = 0; i < _nb_ret; i++){			\
		_ret[i] = (void*)buff;			\
		buff += ARROUND_UP(_ret_sz[i], align);	\
	}

#define __RPC_RESPOND_ALLOC(_nb, _ret_sz)				\
	struct rpc_s *rpc_rsp;						\
	void* ret[_nb];							\
	__RPPC_RESPONSE_PREPARE(_nb, _ret_sz, ret, rpc_rsp);		

#define __RPC_RESPOND(_rpc_sndr, _rpc_rsp, _nb_ret)	\
	RPC_RSP_SEND(_rpc_sndr, _rpc_rsp, size, _nb_ret);\
	__RPC_FREE(_rpc_rsp, size);


#define RPC_RET_PTR(_ret_type, _err) _ret_type, _err

//type to allocate, type to define, type to cast, arg name
//#define RPC_ARG_VAL(_arg_type, _arg) _arg_type,  *(_arg_type*), _arg
//#define RPC_ARG_PTR(_arg_type, _arg) _arg_type, (_arg_type*), _arg
#define RPC_ARG_VAL(_arg_type, _arg) _arg_type, _arg, _VAL
#define RPC_ARG_PTR(_arg_type, _arg) _arg_type, _arg, _PTR

#define	__FUNC_DECL(_func_name,...)	__FUNC_DECL_(_func_name, ##__VA_ARGS__)	
#define	__FUNC_DECL_(_func_name,...)	\
void RPC_FUNC_NAME(_func_name)(cid_t RPC_SENDER_CID, gid_t RPC_SENDER_GID, ##__VA_ARGS__)

#define __FUNC_LOCA(_func_name,...) __FUNC_LOCA_(_func_name, ##__VA_ARGS__)
#define __FUNC_LOCA_(_func_name,...) void RPC_FUNC_LOCAL(_func_name)(__VA_ARGS__){

#define	__FUNC_CALL(_func_name,...) __FUNC_CALL_(_func_name, ##__VA_ARGS__)
#define	__FUNC_CALL_(_func_name,...) RPC_FUNC_NAME(_func_name)(current_cid, cpu_get_id(), ##__VA_ARGS__);}

#define __RET_CAST(_ret_type, _n) (_ret_type*) ret[_n]

#define __ARG_CAST(_arg_cast, _n) _arg_cast rpc_get_arg(rpc, _n)

#define __SIZE_OF(item, n) sizeof(item)

#define __FUNC_DEMA1(_func_name,...)	__FUNC_DEMA1_(_func_name, ##__VA_ARGS__)
#define __FUNC_DEMA1_(_func_name,...)					\
	void RPC_FUNC_DEMARSHALL(_func_name)(struct rpc_s *rpc) {	\
	uint32_t __nb_ret = __GET_NB_ARG(__VA_ARGS__);			\
	uint32_t ret_sz[__GET_NB_ARG(__VA_ARGS__)] =			\
			{FOR_EACH(__SIZE_OF, __VA_ARGS__)};		\
	__RPC_RESPOND_ALLOC(__nb_ret, ret_sz);				\
	RPC_FUNC_NAME(_func_name)(arch_cpu_cid(rpc->gid), rpc->gid,	\
				FOR_EACH(__RET_CAST, __VA_ARGS__)	
#define __FUNC_DEMA2(...) ARG_CHOOSER2(,FOR_EACH(__ARG_CAST, __VA_ARGS__)));	\
			__RPC_RESPOND(rpc, rpc_rsp, __nb_ret);			\
	}								\

#define __GET_BOOL(...) __GET_NB_ARG_(empty, ##__VA_ARGS__, __GET_NB_ARG_BOOL_N())
#define IS_EMPTY(...) __GET_BOOL(__VA_ARGS__)

#define PASTE3(_0, _1, _2) PASTE3_(_0, _1, _2)
#define PASTE3_(_0, _1, _2) PASTE3__(_0, _1, _2)
#define PASTE3__(_0, _1, _2) _0##_1##_2

#define PASTE2(_0, _1) PASTE2_(_0, _1)
#define PASTE2_(_0, _1) PASTE2__(_0, _1)
#define PASTE2__(_0, _1)  _0##_1

#define ID(...) __VA_ARGS__

#define ARG_CHOOSER00(a, b, c) a
#define ARG_CHOOSER10(a, b, c) a, b
#define ARG_CHOOSER01(a, b, c) a, c
#define ARG_CHOOSER11(a, b, c) a, b, c
#define ARG_CHOOSER3(a,b,c) PASTE3(ARG_CHOOSER,IS_EMPTY(b),IS_EMPTY(c))(a,ID(b),ID(c))

#define ARG_CHOOSER0(a, b) a
#define ARG_CHOOSER1(a, b) a, b
#define ARG_CHOOSER2(a,b) PASTE2(ARG_CHOOSER,IS_EMPTY(b))(ID(a),ID(b))


#define RPC_RET(...) __VA_ARGS__
#define RPC_ARG(...) __VA_ARGS__

#define __GET_RET_VAL_(type, val, nb)	val
#define __GET_RET_VAL(...)	FOR_EACH2(__GET_RET_VAL_, __VA_ARGS__)

#define __GET_RET_TP_(type, val, nb)	type
#define __GET_RET_TP(...)	FOR_EACH2(__GET_RET_TP_, __VA_ARGS__)

#define __GET_RET_PTR_(type, val, nb)	type* val
#define __GET_RET_PTR(...)	FOR_EACH2(__GET_RET_PTR_, __VA_ARGS__) 

#define __GET_ARG_CAST_VAL(type)	*(type*)
#define __GET_ARG_CAST_PTR(type)	(type*)
#define __GET_ARG_CAST(type, vp)	CONCATENATE(__GET_ARG_CAST, vp)(type)

#define __GET_ARG_TYPE_VAL(type)	type
#define __GET_ARG_TYPE_PTR(type)	type*
#define __GET_ARG_TYPE(type, vp)	CONCATENATE(__GET_ARG_TYPE, vp)(type)

#define __GET_ARG_TV_(type, val, vp, nb) __GET_ARG_TYPE(type, vp) val
#define __GET_ARG_TV(...)	FOR_EACH3(__GET_ARG_TV_, ##__VA_ARGS__)

#define __GET_ARG_PTR_(type, val, vp, nb)	type* val
#define __GET_ARG_PTR(...)	FOR_EACH3(__GET_ARG_PTR_, __VA_ARGS__)

#define __GET_ARG_CASV_(type, val, vp, nb) __GET_ARG_CAST(type, vp) val
#define __GET_ARG_CASV(...)	FOR_EACH3(__GET_ARG_CASV_, __VA_ARGS__)

#define __GET_ARG_CASTS_(type, val, vp, nb) __GET_ARG_CAST(type, vp)	
#define __GET_ARG_CASTS(...)	FOR_EACH3(__GET_ARG_CASTS_, __VA_ARGS__)

#define RPC_DECLARE(_func_name, _ret, _arg)\
	__FUNC_DECL(ARG_CHOOSER3(_func_name, __GET_RET_PTR(_ret), __GET_ARG_TV(_arg)));		\
	__FUNC_LOCA(ARG_CHOOSER3(_func_name, __GET_RET_PTR(_ret), __GET_ARG_PTR(_arg)))		\
	__FUNC_CALL(ARG_CHOOSER3(_func_name, __GET_RET_VAL(_ret), __GET_ARG_CASV(_arg)))	\
	__FUNC_DEMA1(ARG_CHOOSER2(_func_name, __GET_RET_TP(_ret))) __FUNC_DEMA2(__GET_ARG_CASTS(_arg))	\
	__FUNC_DECL(ARG_CHOOSER3(_func_name, __GET_RET_PTR(_ret), __GET_ARG_TV(_arg)))			


#define STRINGIZE(arg)  STRINGIZE1(arg)
#define STRINGIZE1(arg) STRINGIZE2(arg)
#define STRINGIZE2(arg) #arg

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

//empty allow us to return 0 if no arg in __VA_ARGS__ avoid the trailing comma
#define __GET_NB_ARG(...) __GET_NB_ARG_(empty, ##__VA_ARGS__, __GET_NB_ARG_RSEQ_N())
#define __GET_NB_ARG_(empty, ...) __GET_NB_ARG_N(__VA_ARGS__) 
#define __GET_NB_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, N, ...) N 
#define __GET_NB_ARG_RSEQ_N() 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define __GET_NB_ARG_BOOL_N() 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0

#define FOR_EACH_0(what,...) 

#define FOR_EACH_1(what, N, x, ...) what(x, (N-1))
#define FOR_EACH_2(what, N, x, ...)\
  what(x, (N-2)),\
  FOR_EACH_1(what, N,  __VA_ARGS__)
#define FOR_EACH_3(what, N, x, ...)\
  what(x, (N-3)),\
  FOR_EACH_2(what, N, __VA_ARGS__)
#define FOR_EACH_4(what, N, x, ...)\
  what(x, (N-4)),\
  FOR_EACH_3(what, N,  __VA_ARGS__)
#define FOR_EACH_5(what, N, x, ...)\
  what(x, (N-5)),\
 FOR_EACH_4(what, N,  __VA_ARGS__)
#define FOR_EACH_6(what, N, x, ...)\
  what(x, (N-6)),\
  FOR_EACH_5(what, N,  __VA_ARGS__)
#define FOR_EACH_7(what, N, x, ...)\
  what(x, (N-7)),\
  FOR_EACH_6(what, N,  __VA_ARGS__)
#define FOR_EACH_8(what, N, x, ...)\
  what(x, (N-8)),\
  FOR_EACH_7(what, N,  __VA_ARGS__)

#define FOR_EACH_(N, what, ...) CONCATENATE(FOR_EACH_, N)(what, N, __VA_ARGS__)
#define FOR_EACH(what, ...) FOR_EACH_(__GET_NB_ARG(__VA_ARGS__), what, __VA_ARGS__)

#define FOR_EACH2_0(what, ...) /**/

#define FOR_EACH2_2(what, x, y, ...)\
  what(x, y, 1)
#define FOR_EACH2_4(what, x, y, ...)\
  what(x, y, 2),\
  FOR_EACH2_2(what,  __VA_ARGS__)
#define FOR_EACH2_6(what, x, y, ...)\
  what(x, y, 3),\
  FOR_EACH2_4(what,  __VA_ARGS__)
#define FOR_EACH2_8(what, x, y, ...)\
  what(x, y, 4),\
  FOR_EACH2_6(what,  __VA_ARGS__)
#define FOR_EACH2_10(what, x, y, ...)\
  what(x, y, 5),\
  FOR_EACH2_8(what,  __VA_ARGS__)


#define FOR_EACH2_(N, what, ...) CONCATENATE(FOR_EACH2_, N)(what, __VA_ARGS__)
#define FOR_EACH2(what, ...) FOR_EACH2_(__GET_NB_ARG(__VA_ARGS__), what, __VA_ARGS__)


#define FOR_EACH3_0(what, ...) /**/

#define FOR_EACH3_3(what, x, y, z, ...)\
  what(x, y, z, 1)
#define FOR_EACH3_6(what, x, y, z, ...)\
  what(x, y, z, 2),\
  FOR_EACH3_3(what,  __VA_ARGS__)
#define FOR_EACH3_9(what, x, y, z, ...)\
  what(x, y, z, 3),\
  FOR_EACH3_6(what,  __VA_ARGS__)
#define FOR_EACH3_12(what, x, y, z, ...)\
  what(x, y, z, 4),\
  FOR_EACH3_9(what,  __VA_ARGS__)
#define FOR_EACH3_15(what, x, y, z, ...)\
  what(x, y, z, 5),\
  FOR_EACH3_12(what,  __VA_ARGS__)

//At max five argument

#define FOR_EACH3_(N, what, ...) CONCATENATE(FOR_EACH3_, N)(what, __VA_ARGS__)
#define FOR_EACH3(what, ...) FOR_EACH3_(__GET_NB_ARG(__VA_ARGS__), what, __VA_ARGS__)

#endif
