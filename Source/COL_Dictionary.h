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

typedef struct COLD_KeyCallbacks {
	/// hash contents of key
	COLD_hash_t (*hash)(COLD_data_t);
	/// compare two keys to see if they are equal
	unsigned (*equal)(COLD_data_t, COLD_data_t);
	/// retain or make immutable copy of key
	COLD_data_t (*retain)(COLD_data_t);
	/// release or free key
	void (*release)(COLD_data_t);
} COLD_call_t;


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

/// COLD_size_for_elements returns the number of bytes needed to store elements
unsigned COLD_size_for_capacity(unsigned capacity);

/// COLD_allocate and initialize new structure
COLD COLD_allocate(unsigned capacity, COLD_call_t const *keyCalls);

/// COLD_deallocate structure regardless of contents
void COLD_deallocate(COLD cold);

/// COLD_initialize structure before use if not using COLD_allocate
void COLD_initialize(COLD cold, unsigned capacity, COLD_call_t const *keyCalls);


/// COLD_search get the data associoated with hash
COLD_data_t COLD_search(COLD cold, COLD_data_t key);

/// COLD_remove delete the association with hash and return the associated data
COLD_data_t COLD_remove(COLD cold, COLD_data_t key);

/// COLD_assign create or modify an association between hash and value
COLD_data_t COLD_assign(COLD cold, COLD_data_t key, COLD_data_t value, unsigned options, unsigned *outStatus);


/// COLD_capacity maximum number of associations that can be assigned
unsigned COLD_capacity(COLD cold);

/// COLD_count approximate number of associations that are assigned
unsigned COLD_count(COLD cold);

/// COLD_is_empty returns true when there are no associations
unsigned COLD_is_empty(COLD cold);

/// COLD_remove_all removes all associations and returns the number removed
unsigned COLD_remove_all(COLD cold);

/// COLD_enumerator recieves each hash value association until returning a value other than zero
typedef unsigned (*COLD_enumerator)(COLD_data_t key, COLD_data_t value, void *context, COLD_hash_t hash);

/// COLD_enumerate passes each hash value association to the enumerator and returns the first result other than zero
unsigned COLD_enumerate(COLD cold, COLD_enumerator enumerator, void *context);


/// COLD_hash_bytes quickly hashes data with the given length
COLD_hash_t COLD_hash_bytes(COLD_data_t key, unsigned length);

/// COLD_hash_bytes quickly hashes data with length up to the first null byte
COLD_hash_t COLD_hash_bytes_null_terminated(COLD_data_t key);

#endif /* COL_Dictionary_h */
