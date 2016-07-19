//
//  COL_SparseArray.h
//  AtomicDictionary
//
//  Created by Eric Cole on 7/15/16.
//  Copyright © 2016 Balance Software. All rights reserved.
//

#ifndef COL_SparseArray_h
#define COL_SparseArray_h


/**
	COLA
	
	concurrently operable lockless sparse array
	small static capacity with limited garbage collection
	
	The primary operations are insert, search for, and remove a value
	associated with a hash.  Additionally there are operations to count or get
	all value associations.  Any operation may be used concurrently from
	multiple threads without locks or other synchronization and will complete
	without locking or waiting.  Deallocation is not protected and depends
	on external safety mechanisms.
	
	Any operation will complete at an arbitrary point in time that is not
	strictly before or after any other operation.  Operations may be truly
	simultaneous.  Retrieving all values sees a snapshot that may not have
	existed at a distinct point in time.
	
	Each instance defines a small addressable space for all nodes that imposes
	a hard limit on the number of leaf and branch nodes available.  There are
	typically three times more branch nodes than leaf nodes but the worst case
	hashes can deplete the branch nodes before reaching the leaf capacity.
	
	Nodes that are removed are not available for insertion until all methods
	that interrupted or were interrupted by removal have completed.  Further
	removes before the first has settled pick up pending branch nodes.  Heavy
	concurrent use can temporarily deplete the node pool and prevent insertion.
	Heavy use varies by architecture but is typically dozens of concurrent
	removals and insertions within a microsecond.
	
	Search and replace never fail.  Remove never fails but does not immediately
	make resources available for insertion.  Insertion may fail if there are
	no nodes available.  Insertions that fail may succeed on a later attempt.
	
	Hashes that vary by only a few grouped bits may let insert and remove be
	more efficient.  Hashes that vary at both the most and least significant
	bits but are identical through the middle are least efficient.
*/

#include <stdlib.h>

typedef uint32_t COLA_hash_t;
typedef void *COLA_data_t;
typedef struct COLA_Opaque *COLA;

enum {
	/// COLA_assign completed successful insertion or replacement
	COLA_AssignSuccess = 0 ,
	/// COLA_assign honored the only replace option and found no existing value
	COLA_AssignMissingEntry = 1 ,
	/// COLA_assign honored the no replace option and returned the existing value
	COLA_AssignExistingEntry = 2 ,
	/// COLA_assign failed to insert a new value because all leaves were in use
	COLA_AssignNoLeaves = 3 ,
	/// COLA_assign failed to insert a new value because all nodes were in use
	COLA_AssignNoNodes = 4 ,
};

enum {
	/// COLA_assign will insert a new value or replace an existing value
	COLA_AssignNormal = 0 ,
	/// COLA_assign will replace an existing value but not insert a new value
	COLA_AssignOnlyReplace = 1 ,
	/// COLA_assign will insert a new value but not replace an existing value
	COLA_AssignNoReplace = 2 ,
	/// COLA_assign will insert a new value or add to an existing value
	COLA_AssignSum = 4 ,
};

/// COLA_attributes based on compile time options
unsigned COLA_attributes();

/// COLA_maximum_capacity that can be allocated or initialized
unsigned COLA_maximum_capacity();

/// COLA_size_for_elements returns the number of bytes needed to store elements
unsigned COLA_size_for_capacity(unsigned capacity);

/// COLA_allocate and initialize new structure
COLA COLA_allocate(unsigned capacity);

/// COLA_deallocate structure regardless of contents
void COLA_deallocate(COLA sca);

/// COLA_initialize structure before use if not using COLA_allocate
void COLA_initialize(COLA sca, unsigned capacity);


/// COLA_search get the data associoated with hash
COLA_data_t COLA_search(COLA sca, COLA_hash_t hash);

/// COLA_remove delete the association with hash and return the associated data
COLA_data_t COLA_remove(COLA sca, COLA_hash_t hash);

/// COLA_assign create or modify an association between hash and value
COLA_data_t COLA_assign(COLA sca, COLA_hash_t hash, COLA_data_t value, unsigned options, unsigned *outStatus);


/// COLA_values provides all current associated hashes and values
unsigned COLA_values(COLA sca, COLA_hash_t hashes[], COLA_data_t values[], unsigned capacity, unsigned reverse);

/// COLA_capacity maximum number of associations that can be assigned
unsigned COLA_capacity(COLA sca);

/// COLA_count approximate number of associations that are assigned
unsigned COLA_count(COLA sca);

/// COLA_is_empty returns true when there are no associations
unsigned COLA_is_empty(COLA sca);

/// COLA_remove_all removes all associations and returns the number removed
unsigned COLA_remove_all(COLA sca);

/// COLA_enumerator recieves each hash value association until returning a value other than zero
typedef unsigned (*COLA_enumerator)(COLA_hash_t, COLA_data_t, void *);

/// COLA_enumerate passes each hash value association to the enumerator and returns the first result other than zero
unsigned COLA_enumerate(COLA sca, COLA_enumerator enumerator, void *context);


#endif /* COL_SparseArray_h */
