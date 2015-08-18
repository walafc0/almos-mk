/*
 * pthread_keys.c -  pthread specific keys related functions
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS.
 *
 * ALMOS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <cpu-syscall.h>
#include <pthread.h>
#include <stdio.h>

#define KEY_FREE 0
#define KEY_BUSY 1

typedef void (key_destr_func_t)(void*);

typedef struct key_entry_s
{
	int state;
	key_destr_func_t *destructor;
}key_entry_t;

static pthread_spinlock_t lock;
static key_entry_t keys_tbl[PTHREAD_KEYS_MAX];
static unsigned long next_key;

void __pthread_keys_init(void)
{
	key_entry_t *ptr = &keys_tbl[0];
	memset(ptr, 0, sizeof(keys_tbl));
	next_key = 0;
	pthread_spin_init(&lock, 0);
}

void __pthread_keys_destroy(void)
{
	__pthread_tls_t *tls;
	int iter, key;
	void *old_val;
	int hasVal, err;

	tls = cpu_get_tls();
 
	for(iter=0; iter < PTHREAD_DESTRUCTOR_ITERATIONS; iter++)
	{
		for(key=0, hasVal = 0; key < PTHREAD_KEYS_MAX; key++)
		{
			if(tls->values_tbl[key] == NULL)
				continue;
      
			old_val = tls->values_tbl[key];
			tls->values_tbl[key] = NULL;

			if((old_val == NULL) || (keys_tbl[key].destructor == NULL))
				continue;
      
			keys_tbl[key].destructor(old_val);
			hasVal = 1;
		}

		if(!hasVal) return;
	}
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
	int _key;
	int err;
  
	if(key == NULL)
		return EINVAL;
  
	err = 0;

	pthread_spin_lock(&lock);
  
	for(_key=next_key; (keys_tbl[_key].state != KEY_FREE) && (_key < PTHREAD_KEYS_MAX); _key++);
  
	if(_key != PTHREAD_KEYS_MAX)
	{
		keys_tbl[_key].state = KEY_BUSY;
		next_key = _key + 1;    
	}
	else
		err = EAGAIN;
  
	pthread_spin_unlock(&lock);
  
	if(err) return err;
  
	keys_tbl[_key].destructor = destructor;
	*key = _key;
	return 0;
}

int pthread_key_delete(pthread_key_t key)
{  
	if((key >= PTHREAD_KEYS_MAX) || (keys_tbl[key].state != KEY_BUSY))
		return EINVAL;
  
	keys_tbl[key].state = KEY_FREE;

	if(key < next_key)
	{
		pthread_spin_lock(&lock);
		next_key = (key < next_key) ? key : next_key;
		pthread_spin_unlock(&lock); 
	}

	return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
	__pthread_tls_t *tls;

	if((key >= PTHREAD_KEYS_MAX) || (keys_tbl[key].state != KEY_BUSY))
		return EINVAL;

	tls = cpu_get_tls();
	tls->values_tbl[key] = (void*)value;

	return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
	__pthread_tls_t *tls;

	if((key >= PTHREAD_KEYS_MAX) || (keys_tbl[key].state != KEY_BUSY))
		return NULL;

	tls = cpu_get_tls();
	return tls->values_tbl[key];
}


