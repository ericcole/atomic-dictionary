//
//  COL_Dictionary.h
//  AtomicDictionary
//
//  Created by Eric Cole on 7/18/16.
//  Copyright © 2016 Balance Software. All rights reserved.
//

#ifndef COL_Dictionary_h
#define COL_Dictionary_h

#ifndef _STDLIB_H_
#include <stdlib.h>
#endif

typedef uint32_t COLD_hash_t;
typedef void const *COLD_data_t;
typedef struct COLD_Opaque *COLD;

///	return zero if the two values are not equal
typedef unsigned (*COLD_equal_function)(COLD_data_t, COLD_data_t);
///	return the hash of the given value
typedef COLD_hash_t (*COLD_hash_function)(COLD_data_t);
///	return an immutable copy of key or retain value
typedef COLD_data_t (*COLD_retain_function)(COLD_data_t);
///	release or free value
typedef void (*COLD_release_function)(COLD_data_t);

typedef struct COLD_ValueCallbacks {
	COLD_retain_function retain;
	COLD_release_function release;
} COLD_hold_t;

typedef struct COLD_KeyCallbacks {
	COLD_retain_function retain;
	COLD_release_function release;
	COLD_hash_function hash;
	COLD_equal_function equal;
} COLD_call_t;

/*
	Callbacks must be able to handle any value that may be passed
	to assign, remove or search.  If NULL may be passed then the
	callbacks must handle NULL.
	
	Keys must be immutable with respect to the hash callback.  If
	the key changes such that it would generate a different hash it
	will equal nothing.  The hash is only calculated when the key is
	assigned.
	
	When retain and release callbacks are provided then all keys
	and values will be held while an association exists and may
	be held for a short time before or after an association exists.
	
	When a retain callback is provided for values
		all assigned values are retained
		all removed values are released
		all results from search, remove and assign are retained
		keys are retained and released when values are replaced
	
	If there is no equality callback then the hash callback will not
	be used.  The hashes of two equal values must be equal.  When the
	hashes of two unequal values are equal it causes a hash collision
	and degrades performance.
	
	Using the COLD_AssignSum option with value callbacks is undefined
*/


enum {
	/// COLD_assign completed successful insertion or replacement
	COLD_AssignSuccess = 0 ,
	/// COLD_assign honored the only replace option and found no existing value
	COLD_AssignMissingEntry = 1 ,
	/// COLD_assign honored the no replace option and returned the existing value
	COLD_AssignExistingEntry = 2 ,
	/// COLD_assign failed to insert a new value because all leaves were in use
	COLD_AssignNoLeaves = 3 ,
	/// COLD_assign failed to insert a new value because all nodes were in use
	COLD_AssignNoNodes = 4 ,
};

enum {
	/// COLD_assign will insert a new value or replace an existing value
	COLD_AssignNormal = 0 ,
	/// COLD_assign will replace an existing value but not insert a new value
	COLD_AssignOnlyReplace = 1 ,
	/// COLD_assign will insert a new value but not replace an existing value
	COLD_AssignNoReplace = 2 ,
	/// COLD_assign will add to an existing value instead of replacing the value
	COLD_AssignSum = 4 ,
};


/// COLD_attributes based on compile time options
unsigned COLD_attributes();

/// COLD_maximum_capacity that can be allocated or initialized
unsigned COLD_maximum_capacity();

/// COLD_size_for_capacity returns the number of bytes needed to store elements
unsigned COLD_size_for_capacity(unsigned capacity);
unsigned COLD_capacity_for_size(unsigned size);

/// COLD_allocate and initialize new structure
COLD COLD_allocate(unsigned capacity, COLD_call_t const *keyCalls, COLD_hold_t const *valueCalls);

/// COLD_deallocate structure regardless of contents
void COLD_deallocate(COLD cold);

/// COLD_initialize structure before use if not using COLD_allocate
void COLD_initialize(COLD cold, unsigned capacity, COLD_call_t const *keyCalls, COLD_hold_t const *valueCalls);


/// COLD_search return true if an association with key exists and optionally copy the associated value
unsigned COLD_search(COLD cold, COLD_data_t key, COLD_data_t *copyValueFound);

/// COLD_remove return true if an association with key was removed and optionally copy the associated value
unsigned COLD_remove(COLD cold, COLD_data_t key, COLD_data_t *copyValueRemoved);

/// COLD_assign return the status of inserting or replacing an association and optionally copy the replaced value
unsigned COLD_assign(COLD cold, COLD_data_t key, COLD_data_t value, unsigned options, COLD_data_t *copyValueReplaced);

/// COLD_copy_value is COLD_search that returns the copy of value
COLD_data_t COLD_copy_value(COLD cold, COLD_data_t key);

/// COLD_release_value will balance the retain from a search, remove or assign copy of value
void COLD_release_value(COLD const cold, COLD_data_t value);


/// COLD_capacity maximum number of associations that can be assigned
unsigned COLD_capacity(COLD const cold);

/// COLD_count approximate number of associations that are assigned
unsigned COLD_count(COLD const cold);

/// COLD_is_empty returns true when there are no associations
unsigned COLD_is_empty(COLD const cold);

/// COLD_remove_all removes all associations and returns the number removed
unsigned COLD_remove_all(COLD cold);

/// COLD_enumerator recieves each key value association until returning a value other than zero
typedef unsigned (*COLD_enumerator)(COLD_data_t key, COLD_data_t value, void *context, COLD_hash_t hash);

/// COLD_enumerate passes each key value association to the enumerator and returns the first result other than zero
unsigned COLD_enumerate(COLD cold, COLD_enumerator enumerator, void *context);

/// COLD_enumerate_next passes one key value association to the enumerator or returns zero when there are no more
unsigned COLD_enumerate_next(COLD const cold, unsigned prior, COLD_enumerator enumerator, void *context);


/// COLD_hash_bytes quickly hashes data with the given length
COLD_hash_t COLD_hash_bytes(COLD_data_t key, unsigned length);

/// COLD_hash_bytes quickly hashes data with length up to the first null byte
COLD_hash_t COLD_hash_bytes_null_terminated(COLD_data_t key);


/// COLD_print internal structure
void COLD_print(COLD cold, char const *format_key_value_hash);

#endif /* COL_Dictionary_h */
