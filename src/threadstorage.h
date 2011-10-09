/*
	Thread-local storage system
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef THREADSTORAGE_H

#define THREADSTORAGE_H

#include "config.h"
#include "map.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef mapkey_t tlskey_t;
/*! Callback type for TLS destructors.  Destructors are not permitted to set
    new TLS values unless that will ultimately result in the associated key
	being set to NULL.  Upon thread destruction, the destructor will iterate
	over all values and ensure that any new values are removed.  If a destructor
	always adds a new value, then the thread may infinitely loop while destroying
	TLS data.

	So, keep that in mind.
*/
typedef void (*tls_destructor_t)(tlskey_t key, void *value);

/** TLS depends on the memory and object systems */
void sys_tls_init();
void sys_tls_shutdown();

/*! \brief Places a pointer in thread-local storage.
	\param key The key to map the value to in TLS.
	\param value The value to be inserted into TLS.  If this value is NULL, the
	original value in TLS (if there is any) will be removed from thread-local storage.
	\param dtor A destructor routine for the key and/or value in thread-local storage
	when the value is either removed or thread-local storage is shut down.
*/
void tls_put(tlskey_t key, void *value, tls_destructor_t dtor);
void *tls_get(tlskey_t key);

#define threadstorage_remove(KEY) threadstorage_put((KEY), NULL, NULL)

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: THREADSTORAGE_H */
