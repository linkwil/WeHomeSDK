#if defined(_WINDOWS) || defined(WIN32) || defined(STANDARDSHELL_UI_MODEL)
	#if !defined(__INCLUDED_MYPTHREAD_H_)
	#define __INCLUDED_MYPTHREAD_H_

	#include <process.h>		// _begin_thread
	#include <Windows.h>		// CRITICAL_SESSION

	#define pthread_t			int
	#define pthread_mutex_t		CRITICAL_SECTION
	#define pthread_attr_t		void
	#define pthread_mutexattr_t	void	

	#define pthread_mutex_lock(mutex)	::EnterCriticalSection(mutex)
	#define pthread_mutex_unlock(mutex)	::LeaveCriticalSection(mutex)
	#define pthread_mutex_init(mutex, attr)	::InitializeCriticalSection(mutex)
	#define pthread_mutex_destroy(mutex)	::DeleteCriticalSection(mutex);
	#define  pthread_func_t			void
	#define  PTHREAD_FUNC_NULL		

	int PthreadCreate(pthread_t *thread, const int stack_size_kbyte,  pthread_func_t (*start_routine) (void *), void *arg);

	int pthread_create(pthread_t *thread, const pthread_attr_t *attr,  pthread_func_t (*start_routine) (void *), void *arg);

	int pthread_detach(pthread_t thread_id);
	pthread_t pthread_self();

	#define usleep(x)		(x/1000 > 0 ? Sleep(x/1000) : Sleep(1))

	#endif
#else
    #include <stddef.h>
	#include "pthread.h"
	#define  pthread_func_t void*
#endif
