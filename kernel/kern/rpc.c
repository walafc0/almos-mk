#include<rpc.h>
#include<cpu.h>
#include<pmm.h>
#include<bits.h>
#include<atomic.h>
#include<kdmsg.h>
#include<thread.h>
#include<cluster.h>

void rpc_release(struct rpc_s *rpc);

#define rpc_debug(args...)

error_t rpc_listner_init(struct rpc_listner_s *rl)
{
	int i;
	error_t err;

	rl->count = 0;
	rl->pending = 0;
	atomic_init(&rl->handled, 0);
	err = 0;

	for(i=0; i < RPC_PRIO_NR; i++)
	{
		if((err = remote_fifo_init(&rl->fifo_tbl[i], 
				CONFIG_RPC_FIFO_SLOT_NR, 
				sizeof(struct rpc_ptr_s))))
		{
			PANIC("Initialisation of RPC failed.\n");
			return err;
		}
	}

	return 0;

}

/* FIXME: do some rpc work ... */
static void rpc_backoff()
{
	static uint32_t backoff;
	uint32_t retval;
	backoff = (backoff + cpu_get_id()*7) % 128;//?
	retval = cpu_time_stamp() + backoff;
	if(!cpu_yield())
		while(cpu_time_stamp() < retval)
			;
}

void __rpc_send(struct rpc_listner_s *rl, cid_t cid, gid_t lid, gid_t gid, 
					struct rpc_s *rpc, bool_t to_cid)
{
	uint_t isPending;
	struct rpc_ptr_s rpc_ptr;

	rpc_ptr.ptr = rpc;
	rpc_ptr.size = rpc->size;

	rpc_ptr.cid = current_cid;

	while(remote_fifo_put(&rl->fifo_tbl[rpc->prio], cid, (void *)&rpc_ptr))
		rpc_backoff();
	
	rpc_debug(INFO, "[%d] %s: RPC sent : %p with func %p\n", 
		cpu_get_id(), __FUNCTION__, rpc, rpc->handler);
	
		
	while(!(isPending=remote_lw((void*)&rl->pending, cid))) 
	{
		if(remote_atomic_cas((void*)&rl->pending, cid, isPending, 1))
		{
			rpc_debug(INFO, "[%d] RPCs in %d are no longer pending\n", cpu_get_id(), cid);

			if(rpc->prio < RPC_PRIO_LAZY)
			{
				if(to_cid)
				{
					if(cluster_in_kernel(cid)) return;
				}else 
					if(cpu_in_kernel(cid, lid)) return;
				
				rpc_debug(INFO, "[%d] sending IPI to %d ...\n", cpu_get_id(), cid);
				(void)arch_cpu_send_ipi(gid, cpu_get_id());
			}
		}
	}
}

//the same buffer is used for the commande and the response!
error_t rpc_send_sync(gid_t gid,  bool_t to_cid, size_t prio, void* func, 
			size_t nbret, size_t nbarg, 
			void* ret[], size_t ret_sz[], 
			struct rpc_s* rpc, size_t size,
			struct rpc_s* ret_rpc, size_t ret_size)
		
{
	struct rpc_listner_s *rl;
	size_t final_ret_sz;
	uint_t cid = arch_cpu_cid(gid);
	uint_t lid = arch_cpu_lid(gid);
	int i;
	
	if(to_cid)
		rl = &current_cluster->re_listner;
	else
		rl = &cpu_lid2ptr(lid)->re_listner;

	rpc->gid = cpu_get_id();
	rpc->prio = prio;
	rpc->size = size;
	rpc->args_nb = nbarg;
	rpc->handler = func;
	rpc->response = false;

	rpc->sender = current_thread; 
	rpc->ret_size = ret_size;
	rpc->ret_rpc = ret_rpc;

	rpc_debug
	//printk
	(INFO, "[cpu: %d, thread:%x, tid: %d] Sending rpc \n",\
		current_cpu->gid, current_thread, current_thread->ltid, __FUNCTION__, __LINE__);\

	__rpc_send(rl, cid, lid, gid, rpc, to_cid);


	if(prio < RPC_PRIO_LAZY)
	{
#ifdef RPC_THREAD_SLEEP
		//printk(INFO, "[%d]sleeping thread %p at %d\n",cpu_get_id(), current_thread, cpu_time_stamp()); 
		sched_sleep_check(current_thread);//potentiel race with wakeup ?
#endif
		while(!rpc->response)
			rpc_backoff();

		rpc_debug
		//printk
		(INFO, "[cpu: %d, thread:%x, tid: %d]  Received RPC %p responses \n", 
			current_cpu->gid, current_thread, current_thread->ltid, rpc);
	}else
		rpc_debug
		//printk
		(INFO, "[cpu: %d, thread:%x, tid: %d]  (%p) No response is waited for \n", 
			current_cpu->gid, current_thread, current_thread->ltid, rpc);
		



	for(i=0;i<nbret;i++)
	{
		final_ret_sz = MIN(ret_sz[i], rpc_get_arg_sz(ret_rpc, i));
		rpc_debug(INFO, "[%d] copiying ret arg %d, size %d\n", 
					cpu_get_id(), i, final_ret_sz);
		//if(!final_ret_sz) ret[i] = NULL;
		//else 
		memcpy(ret[i], rpc_get_arg(ret_rpc, i) , final_ret_sz);	
	}
	
	return 0;
}

error_t rppc_generic(gid_t gid, bool_t to_clst, size_t prio, void* func, 
					size_t nb_ret, size_t nb_arg, 
					void* rets[], size_t rets_sz[], 
					void* args[], size_t args_sz[]) 
{										
	size_t ret_size = 0;
	size_t size = 0;
	uint8_t *obuff;
	uint8_t *buff;
	error_t err;
	int i;
	
	//one for the the message and one for response
	size += sizeof(struct rpc_s);
	ret_size += sizeof(struct rpc_s);

	for(i = 0; i < nb_arg; i++)
		size+=ARROUND_UP(args_sz[i],RPC_ALIGN) + sizeof(uint32_t);
	for(i = 0; i < nb_ret; i++)
		ret_size+=ARROUND_UP(rets_sz[i],RPC_ALIGN) + sizeof(uint32_t);

	__RPC_ALLOCATE(buff, MAX(size, ret_size));

	obuff = buff;
	buff += sizeof(struct rpc_s);
	for(i = 0; i< nb_arg; i++)
		((uint32_t*)buff)[i] = args_sz[i];
	buff += nb_arg*4;
	for(i = 0; i< nb_arg; i++){
		memcpy(buff, args[i], args_sz[i]);
		buff += ARROUND_UP(args_sz[i], RPC_ALIGN);
	}
	err = rpc_send_sync(gid, to_clst, prio, func, nb_ret, nb_arg, 
				rets, rets_sz, 
				(struct rpc_s*) obuff, size, 
				(struct rpc_s*) obuff, ret_size);
	__RPC_FREE(obuff, MAX(size,ret_size));

	return err;
}

void rpc_response_notify(struct rpc_s *rpc);

//send responses by copying the an rpc directly in the sender rpc buffer
error_t rpc_send_rsp(struct rpc_s* sender_rpc, struct rpc_s* rpc, size_t size, uint32_t nb_ret)
{
	uint_t sender_cid; 
	uint_t sender_lid; 
	uint_t sender_gid;
	struct rpc_listner_s *sender_rl;
	struct rpc_s* ret_rpc;

	sender_gid = sender_rpc->gid;
	sender_cid = arch_cpu_cid(sender_gid);
	sender_lid = arch_cpu_lid(sender_gid);
	sender_rl = &cpu_lid2ptr(sender_lid)->re_listner;

	rpc->gid = cpu_get_id();
	rpc->prio = sender_rpc->prio;
	rpc->handler = rpc_response_notify;
	rpc->sender = sender_rpc->sender;
	rpc->size = size;
	rpc->args_nb = nb_ret;

#ifdef RPC_THREAD_SLEEP
	rpc->response = true;
#else
	rpc->response = false;// done later when all the data have been copied
#endif
	rpc->ret_size = 0;//no response expected
	rpc->ret_rpc = NULL;//no response expected

	ret_rpc = sender_rpc->ret_rpc;
	assert(size == sender_rpc->ret_size);

	rpc_debug(INFO, "[%d] sending response to RPC: origine %d, func %p\n", 
			cpu_get_id(), sender_rpc->gid, sender_rpc->handler);

	//release the local buffer allocated temporarly to cpy the remote rpc (if any)
	rpc_release(sender_rpc);

	//copy response rpc to the pre allocated buffer in sender_cid 
	remote_memcpy(ret_rpc, sender_cid, rpc, current_cid, size);
	cpu_wbflush();

#ifndef RPC_THREAD_SLEEP
	remote_sw((void*)&ret_rpc->response, sender_cid, true);
#endif

	if(rpc->prio < RPC_PRIO_LAZY)//otherwise no responses!
	//remotly wakeup the wating thread
	{
#ifdef RPC_THREAD_SLEEP
		rpc_debug(INFO, "[%d] RPC response wakening thread %x on cluster %d at %d\n", 
			cpu_get_id(), rpc->sender, sender_cid, cpu_time_stamp());
		
		remote_sw(&rpc->sender->state, sender_cid, S_READY);
		cpu_wbflush();
		if(!(remote_lw((void*)&sender_rl->pending, sender_cid))) 
		{
			if(cpu_in_kernel(sender_cid, sender_lid)) return 0;

			return 0;//FIXME: is it better to send an IPI ?
			rpc_debug(INFO, "[%d] sending IPI response to %d ...\n", cpu_get_id(), sender_cid);
			(void)arch_cpu_send_ipi(sender_gid, (uint32_t)-1);
		}
#endif
	}
	//no sleeping since no response is expected!
	
	return 0;
}

//rpc_response_notify: responses to the rpc_send_sync
//executed on the cluster of the sender
void rpc_response_notify(struct rpc_s *rpc)
{
	struct thread_s *to_wk;

	to_wk = rpc->sender;
	
	rpc_debug(INFO, "[%d]wakening thread %p\n",cpu_get_id(), to_wk); 
	sched_wakeup(to_wk);
	rpc_debug(INFO, "[%d]waked up thread %p\n",cpu_get_id(), to_wk); 
}

//TODO: send an rpc error to the sender
//to avoid allocations error add the necessary 
//fields [sender info (gid, sender ptr), error type] 
//in rpc_ptr
void rpc_send_rsp_err(struct rpc_ptr_s *rpc_ptr)
{
	printk(ERROR, "[%d] RPC error unhandled!!!!\n",cpu_get_id()); 
	while(1);
}

error_t rpc_ptr_copy(struct rpc_ptr_s *rpc_ptr, struct rpc_ptr_s *local)
{
	memcpy(local, rpc_ptr, sizeof(struct rpc_ptr_s));
	return 0;
}

void rpc_release(struct rpc_s *rpc)
{
	if(!rpc->response)
		__RPC_FREE((uint8_t*)rpc, rpc->size);
}

void *rpc_exec(void* _rpc)
{
	struct rpc_s *rpc;
	rpc = (struct rpc_s *) _rpc;
	rpc->handler(rpc);
	return NULL;
}

//FIXME: do a pool of thread
void rpc_thread_handle(struct rpc_s * rpc)
{
	struct thread_s *thread;
	error_t err;

	thread = kthread_create(current_task, &rpc_exec, 
				rpc, current_cpu->lid);

	if(thread == NULL)
		goto RPC_THREAD_ERROR;

	thread->task   = current_task;
	wait_queue_init(&thread->info.wait_queue, "RPC Thread");

	err = sched_register(thread);
	if(err) goto RPC_THREAD_ERROR;

	sched_add_created(thread);

	return;

RPC_THREAD_ERROR:
	printk(ERROR, "Failed to create RPC Thread in CPU %d\n", current_cpu->gid);
	if(thread) kthread_destroy(thread);
	else err = ENOMEM;

	//TODO send back err
}

void rpc_handle(struct rpc_ptr_s * rpc_ptr)
{
	struct rpc_s *rpc;
	uint8_t *buff;
	size_t size;

	size = rpc_ptr->size;
	
	__RPC_ALLOCATE(buff, size);//allocate on stack
	rpc = (struct rpc_s*) buff;

	rpc_debug(INFO, "[%d] handling RPC: origine %d, func %p, size %d\n", 
			cpu_get_id(), rpc->gid, rpc->handler, size);

	if(!rpc)
	{
		rpc_send_rsp_err(rpc_ptr);
		return;
	}

	remote_memcpy(rpc, current_cid, 
		rpc_ptr->ptr, rpc_ptr->cid, size); 
	

	rpc->handler(rpc);
}

//Called with irq disabled
error_t rpc_listner_notify(struct rpc_listner_s *rl, uint_t *irq_state)
{
	event_prio_t current_prio;
	struct rpc_ptr_s *rpc_ptr;
	struct rpc_ptr_s rpc_ptr_local;
	uint_t count;
	error_t err;

	count = 0;
	pmm_cache_flush_vaddr((vma_t)&rl->pending, PMM_DATA);
	if(!rpc_is_pending(rl))
		return 0;
		
	do
	{
		for(current_prio = RPC_PRIO_START; current_prio < RPC_PRIO_NR; current_prio++, count++)
		{
			while((err = remote_fifo_get(&rl->fifo_tbl[current_prio], (void**)&rpc_ptr)) == 0)
			{
				err = rpc_ptr_copy(rpc_ptr, &rpc_ptr_local);
				if(err) rpc_send_rsp_err(rpc_ptr);
				remote_fifo_release(&rl->fifo_tbl[current_prio]);
				cpu_restore_irq(*irq_state);
				rpc_handle(&rpc_ptr_local);
				cpu_disable_all_irq(irq_state);
				
				/* if we are no longer the handler return. This *
				 * can happen if we slept while traeting an RPC */
				/* FIXME: try to become handler ? */
				if(!rpc_check_handler(rl))
					goto NOTIFY_EXIT;
				//if(arch_cpu_cid(gid)!=current_cid)//this msg is not a reponse => signal that it has been handled
					//remote_fifo_release(&rl->fifo_tbl[current_prio].fifo, rpc);
			}
		}
		//if(!rl->pending) break;
		//if the cas succes check one last time that there's no new request
		//if it fail: no request and we have just rechecked
		rpc_debug(INFO, "[%d] %s finished loop pending ? %d\n", cpu_get_id(), __FUNCTION__, rl->pending);
	}while(atomic_cas((void*)&rl->pending, 1, 0));
		
NOTIFY_EXIT:
	rl->count += count;

	return 0;
}


void* rpc_get_arg(struct rpc_s *rpc, size_t args_nb)
{
	uint32_t i;
	uint32_t* arg_sz_tbl = (uint32_t*) (((uint32_t)rpc) + sizeof(*rpc));
	char* arg_tbl = (char*) arg_sz_tbl + 4*rpc->args_nb;

	if(args_nb >= rpc->args_nb)
	{
		printk(ERROR, 
		"RPC: the requested argument (%d) is not avalable\n", args_nb);
		rpc_print(rpc);
		while(1);//FIXME: a better exit
		//return NULL;
	}

	if(!arg_sz_tbl[args_nb])
		return NULL;

	for(i=0; i < args_nb; i++)
	{
		arg_tbl+=ARROUND_UP(arg_sz_tbl[i], sizeof(void*));
	}
	
	return (void*)arg_tbl;	
}


size_t rpc_get_arg_sz(struct rpc_s *rpc, size_t args_nb)
{
	uint32_t* arg_sz_tbl = (uint32_t*) (((uint32_t)rpc) + sizeof(*rpc));

	if(args_nb >= rpc->args_nb)
	{
		rpc_print(rpc);
		PANIC("RPC: the requested argument (%d) is not avalable\n", 
								args_nb);
		return 0;
	}

	return (size_t)arg_sz_tbl[args_nb];	
}

void rpc_print(struct rpc_s* rpc)
{
	printk(INFO, "%s: rpc %p, current cluster %x, current_cpu->lid %x:\n", 
				rpc, current_cid, current_cpu->lid);
	printk(INFO, "rpc gid %x, prio %d, sender %p, args_nb %d, size %d\n", 
			rpc->gid, rpc->prio, rpc->sender, rpc->args_nb, rpc->size);
}

/**********************************************************************************/
/********************************** RPC threads ***********************************/
/**********************************************************************************/

/* Called with irq disable to synchronise "rpc_working_thread" */
error_t rpc_thread_create(struct cpu_s *cpu, struct task_s *task)
{
	struct thread_s *new_thread;
	error_t err;

	new_thread = kthread_create(task, 
				&thread_rpc_manager, 
				NULL, 
				cpu->lid);

	if(new_thread == NULL)
	{
		return ENOMEM;
	}

	//rpc_debug
	printk(INFO, "INFO: Creating a new RPC thread (%d) on cpu %d\n", new_thread, cpu_get_id());

	wait_queue_init(&new_thread->info.wait_queue, "RPC Thread");

	new_thread->task   = task;

	if((err = sched_register(new_thread)))
		return err;

	sched_add_created(new_thread);

	cpu->rpc_working_thread++;

	/* RPC thread can not be preempted */
	thread_preempt_disable(new_thread);

	return 0;
}

void rpc_before_sleep(struct thread_s *thread)
{
	struct cpu_s *cpu;

	cpu  = current_cpu;	

	assert(cpu->rpc_working_thread > 0);

	if(!(--cpu->rpc_working_thread))
	{

		/* So new thread can handle rpcs */
		assert(thread->li);
		rpc_unset_handled(thread->li);

		if(!wait_queue_isEmpty(cpu->rpc_wq))
		{
			wakeup_one(cpu->rpc_wq, WAIT_FIRST);
		}
		else	/* create a new RPC thread */
		{
			/* FIXME: can we avoid systimatically creating 
			 * thread if we are handling a cluster RPC and 
			 * there is another proc that is in the kernel */
			if(rpc_thread_create(cpu, thread->task))
			{
				/* FIXME: send an event that will send *
				 * a response error back ? */
				printk(ERROR, 
				"[%d] Failed to create an RPC thread\n", 
				cpu->gid);
			}
		}
	}
}

void rpc_after_wakeup(struct thread_s *thread)
{
	struct cpu_s *cpu;
	/* Thread may be wakeup by another proc */
	cpu  = thread_current_cpu(thread);
	cpu->rpc_working_thread++;
}

void rpc_manager_init(struct cpu_s *cpu)
{
	kmem_req_t req;

	cpu->rpc_working_thread = 0;

	req.type  = KMEM_GENERIC;
	req.flags = AF_KERNEL;
	req.size  = sizeof(struct wait_queue_s);
	if((cpu->rpc_wq = kmem_alloc(&req)) == NULL)
		PANIC("Cannot allocate rpc wait_queue");

	wait_queue_init(cpu->rpc_wq, "RPC wait queue");
}

void rpc_check_notify(struct rpc_listner_s *li, uint_t *irq_state)
{
	uint_t handled;
	uint_t pending;

	handled = (uint_t) rpc_is_handled(li); 
	pending = rpc_is_pending(li);

	if(pending && !handled && rpc_set_handled(li))
	{
		current_thread->li = li;
		rpc_listner_notify(li, irq_state);
		rpc_unset_handled(li);
		current_thread->li = NULL;
	}

}

bool_t __rpc_check(struct rpc_listner_s *li)
{
	uint_t handled;
	uint_t pending;

	handled = (uint_t) rpc_is_handled(li); 
	pending = rpc_is_pending(li);

	if(pending && !handled)
		return true;

	return false;
}

bool_t rpc_check()
{
	if(__rpc_check(&current_cpu->re_listner))
		return true;
	if(__rpc_check(&current_cluster->re_listner))
		return true;

	return false;
}

void* thread_rpc_manager(void *arg)
{
	struct thread_s *this;
	struct cpu_s *cpu;
	uint_t irq_state;

	cpu_enable_all_irq(NULL);
  
	this = current_thread;
	cpu  = current_cpu;

	//Avoid that thread idle waken "this" a second time (since pending is set)...
	thread_preempt_disable(this);

	while(1)
	{
		rpc_debug(INFO, "INFO: RPC Handler on CPU %d, rpc is le pending %d [%d,%d]\n",
		       cpu->gid,
		       rpc_is_pending(&cpu->re_listner),
		       cpu_time_stamp(),
		       cpu_get_ticks(cpu)); 

		//Avoid the handling of IPI until we get to sleep (?)
		cpu_disable_all_irq(&irq_state);

		this->info.before_sleep = rpc_before_sleep;
		this->info.after_wakeup = rpc_after_wakeup;

		//check private RPC
		rpc_check_notify(&cpu->re_listner, &irq_state);
		
		//check shared RPC
		rpc_check_notify(&current_cluster->re_listner, &irq_state);

		this->info.before_sleep = NULL;
		this->info.after_wakeup = NULL;

		//TODO: If to much extra threads simply exit and don't wait+sleep ?
		wait_on(cpu->rpc_wq, WAIT_ANY);
		sched_sleep(this);

		cpu_restore_irq(irq_state);
	}

	return NULL;
}


/********************************************************************************/
/******************************* RPC TESTS **************************************/
/********************************************************************************/

RPC_DECLARE(__increment, RPC_RET(RPC_RET_PTR(uint32_t, res)), 
			 RPC_ARG(RPC_ARG_VAL(uint32_t, val)))
{
	*res = ++val;
}

error_t __test0(int nb)
{
	uint32_t nb_clstr;
	uint32_t rcount;
	uint32_t count;
	uint32_t result;
	uint32_t start;
	uint32_t end;
	int i;

	count = 0;
	rcount = 0;
	nb_clstr = current_cluster->clstr_wram_nr;
	
	rpc_debug(INFO, "[%d] Starting rpc test %d\n", cpu_get_id(), nb);

	start = cpu_time_stamp(); //cpu_get_ticks(current_cpu);
	for(i=0; i<nb_clstr; i++)
	{
		RCPC(i, RPC_PRIO_NRML, __increment,
				RPC_RECV(RPC_RECV_OBJ(rcount)), 
				RPC_SEND(RPC_SEND_OBJ(count)));
		if(rcount != ++count)
		{
			return -1;
		}
	}
	end = cpu_time_stamp(); //cpu_get_ticks(current_cpu);
	
	result = (end - start)/nb_clstr;

	printk(INFO, "[%d] RCPC average time (with %d cluster(s)) is %d cycles\n", cpu_get_id(), nb_clstr, result);

	return 0;
}

error_t __test1(int nb)
{
	uint32_t nb_cpu;
	uint32_t rcount;
	uint32_t count;
	uint32_t result;
	uint32_t start;
	uint32_t end;
	int i;

	count = 0;
	rcount = 0;
	nb_cpu = arch_onln_cpu_nr();
	
	rpc_debug(INFO, "[%d] Starting rpc test %d\n", cpu_get_id(), nb);

	start = cpu_time_stamp(); //cpu_get_ticks(current_cpu);
	for(i=0; i<nb_cpu; i++)
	{
		//if(i==0) continue;//skip
		RPPC(i, RPC_PRIO_NRML, __increment,
				RPC_RECV(RPC_RECV_OBJ(rcount)), 
				RPC_SEND(RPC_SEND_OBJ(count)));
		if(rcount != ++count)
		{
			return -1;
		}
	}
	end = cpu_time_stamp(); //cpu_get_ticks(current_cpu);

	result = (end - start)/nb_cpu;

	printk(INFO, "[%d] RPPC average time (with %d cpu(s)) is %d cycles\n", cpu_get_id(), nb_cpu, result);

	return 0;
}

typedef error_t (*rpc_test_func_t)();
struct rpc_test_s{
	rpc_test_func_t test_func;
	char test_name[10];
};
#define NB_TEST 4
static const rpc_test_func_t funcs[NB_TEST] = {__test0, __test1, __test1, __test0};

void* __rpc_test()
{
	int i;
	error_t err;

	//if(cpu_get_id() != 0) return 0;
	
	rpc_debug(INFO, "!!!!!!! [%d] STARTING RPC TEST !!!!!!!\n", cpu_get_id());

	for(i=0; i<NB_TEST; i++)
	{
		err = funcs[i](i);
		if(err)
		{ 
			printk(ERROR, "[%d] !!! ERROR in rpc test n°%d !!!\n", cpu_get_id(), i);
			return NULL;
		}
	}

	rpc_debug(INFO, "!!!!!!! [%d] RPC SUCCESS !!!!!!!\n", cpu_get_id());

	return NULL;
}

void rpc_test()
{
	struct thread_s *thread;
	error_t err;

	if(cpu_get_id()!=3) return;//trace only proc 3

	thread = kthread_create(current_task, 
				&__rpc_test, 
				NULL, 
				current_cpu->lid);
	thread->task  = current_task;
	wait_queue_init(&thread->info.wait_queue, "RPC_TEST");
	err           = sched_register(thread);
	assert(err == 0);
	sched_add_created(thread);
	rpc_debug(INFO,"INFO: rpc_test has been launched\n");
}
