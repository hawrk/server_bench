/*
 * mutex.h
 *
 *  Created on: 2012-1-2
 *      Author: Anakin
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include <pthread.h>

class Mutex
{
public:
	Mutex(pthread_mutex_t *mutex_t)
	{
		this->lock = mutex_t;

		pthread_mutex_lock(lock);
	}
	~Mutex()
	{
		pthread_mutex_unlock(lock);
	}
private:
	pthread_mutex_t * lock;
};

#endif /* MUTEX_H_ */
