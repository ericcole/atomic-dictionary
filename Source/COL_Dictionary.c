//
//  COL_Dictionary.c
//  AtomicDictionary
//
//  Created by Eric Cole on 7/18/16.
//  Copyright © 2016 Balance Software. All rights reserved.
//

#include "COL_Dictionary.h"
#include "ECC_atomic.h"

#ifndef	_STDIO_H_
#include <stdio.h>
#endif

#ifndef _STDLIB_H_
#include <stdlib.h>
#endif

#ifndef _LIMITS_H_
#include <limits.h>
#endif


#define COLD_4_in_32_bits 36
#define COLD_6_in_64_bits 70
#define COLD_7_in_64_bits 71
#define COLD_8_in_64_bits 72

#ifndef COLD_DISPOSITION
#if __LP64__
#define COLD_DISPOSITION COLD_7_in_64_bits
#else
#define COLD_DISPOSITION COLD_4_in_32_bits
#endif
#endif

#ifndef COLD_ORDER_BY_HASH
#define COLD_ORDER_BY_HASH 0
#endif


#if COLD_DISPOSITION < 64

typedef uint32_t COLD_casn_t;
#define COLD_bits_per_node 32
#define COLD_atomic_casn ECC_atomic_compare_and_swap_32
#define COLD_ctzn ECC_count_trailing_zeros_32
#define COLD_NODE_FMT "%08X"

#else

typedef uint64_t COLD_casn_t;
#define COLD_bits_per_node 64
#define COLD_atomic_casn ECC_atomic_compare_and_swap_64
#define COLD_ctzn ECC_count_trailing_zeros_64
#define COLD_NODE_FMT "%016llX"

#endif


#if COLD_DISPOSITION == COLD_4_in_32_bits
#define COLD_entries_per_node 4
#define COLD_bits_per_entry 8	//	4*8 <= 32
#define COLD_maximum_depth 16	//	4^16 >= 2^32
#define COLD_entry_ones 0x01010101U
#define COLD_ENTRY_FMT "%02X"

#if COLD_ORDER_BY_HASH
#define COLD_entry_index_for_depth(_hash, _depth) (((_hash) >> ((COLD_bits_per_hash - 2) - ((_depth) * 2))) & 3)
#else
#define COLD_entry_index_for_depth(_hash, _depth) (((_hash)>>((_depth)*2))&3)
#endif
#endif

#if COLD_DISPOSITION == COLD_8_in_64_bits
#define COLD_entries_per_node 8
#define COLD_bits_per_level 3
#define COLD_bits_per_entry 8	//	8*8 <= 64
#define COLD_maximum_depth 11	//	8^11 >= 2^32
#define COLD_entry_ones 0x0101010101010101ULL
#define COLD_ENTRY_FMT "%02llX"

#if COLD_ORDER_BY_HASH
#define COLD_entry_index_for_depth(_hash, _depth) (((_depth)<10)?(((_hash)>>((COLD_bits_per_hash-3)-((_depth)*3)))&7):((_hash)&3))
#else
#define COLD_entry_index_for_depth(_hash, _depth) (((_hash)>>((_depth)*3))&7)
#endif
#endif

#if COLD_DISPOSITION == COLD_7_in_64_bits
#define COLD_entries_per_node 7
#define COLD_bits_per_entry 9	//	7*9 <= 64
#define COLD_maximum_depth 12	//	7^12 >= 2^32
#define COLD_entry_ones 0x0040201008040201ULL
#define COLD_ENTRY_FMT "%03llX"

const COLD_casn_t COLD_divisor_at_depth[12] = {1,7,7*7,7*7*7,7*7*7*7,7*7*7*7*7,7*7*7*7*7*7,7*7*7*7*7*7*7,7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7*7*7U};
#if COLD_ORDER_BY_HASH
#define COLD_entry_index_for_depth(_hash, _depth) (((_depth)<11)?(((_hash)/(3*COLD_divisor_at_depth[10-(_depth)]))%7):((_hash)%3))
#else
#define COLD_entry_index_for_depth(_hash, _depth) (((_hash)/COLD_divisor_at_depth[_depth])%7)
#endif
#endif

#if COLD_DISPOSITION == COLD_6_in_64_bits
#define COLD_entries_per_node 6
#define COLD_bits_per_entry 10	//	6*10 <= 64
#define COLD_maximum_depth 13	//	6^13 >= 2^32
#define COLD_entry_ones 0x0004010040100401ULL
#define COLD_ENTRY_FMT "%03llX"

const COLD_casn_t COLD_divisor_at_depth[13] = {1,6,6*6,6*6*6,6*6*6*6,6*6*6*6*6,6*6*6*6*6*6,6*6*6*6*6*6*6,6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6*6*6U};
#if COLD_ORDER_BY_HASH
#define COLD_entry_index_for_depth(_hash, _depth) (((_depth)<12)?(((_hash)/(2*COLD_divisor_at_depth[11-(_depth)]))%6):((_hash)%2))
#else
#define COLD_entry_index_for_depth(_hash, _depth) (((_hash)/COLD_divisor_at_depth[_depth])%6)
#endif
#endif

#if COLD_DISPOSITION == COLD_5_in_64_bits
#define COLD_entries_per_node 5
#define COLD_bits_per_entry 12	//	5*12 <= 64
#define COLD_maximum_depth 14	//	5^14 >= 2^32
#define COLD_entry_ones 0x0001001001001001ULL
#define COLD_ENTRY_FMT "%03llX"

const COLD_casn_t COLD_divisor_at_depth[14] = {1,5,5*5,5*5*5,5*5*5*5,5*5*5*5*5,5*5*5*5*5*5,5*5*5*5*5*5*5,5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5*5*5*5U};
#if COLD_ORDER_BY_HASH
#define COLD_entry_index_for_depth(_hash, _depth) (((_depth)<13)?(((_hash)/(4*COLD_divisor_at_depth[12-(_depth)]))%5):((_hash)%4))
#else
#define COLD_entry_index_for_depth(_hash, _depth) (((_hash)/COLD_divisor_at_depth[_depth])%5)
#endif
#endif


#pragma mark - Atomic


#define COLD_atomic_casp ECC_atomic_compare_and_swap_ptr
#define COLD_atomic_addp ECC_atomic_add_ptr

#if 1
typedef uint32_t COLD_link_t;
#define COLD_bits_per_hash 32
#define COLD_bits_per_link 32
#define COLD_atomic_casl ECC_atomic_compare_and_swap_32
#define COLD_LINK_FMT "%08X"
#endif

#if 1
typedef uint32_t COLD_mask_t;
#define COLD_bits_per_mask 32
#define COLD_atomic_andm ECC_atomic_and_32
#define COLD_atomic_orm  ECC_atomic_or_32

#define COLD_test_bit(_bits,_bit) (_bits[(_bit)/32] & (1<<((_bit)&31)))
#define COLD_set_bit(_bits,_bit) (_bits[(_bit)/32] |= (1<<((_bit)&31)))
#endif

#if 1
typedef int32_t COLD_math_t;
#define COLD_bits_per_math 32
#define COLD_atomic_casm ECC_atomic_compare_and_swap_32
#define COLD_atomic_addm ECC_atomic_add_32
#endif


#pragma mark - Nodes


#define COLD_entry_limit (1<<COLD_bits_per_entry)
#define COLD_entry_mask (COLD_entry_limit-1)
#define COLD_leaf_count (COLD_entry_limit/4)
#define COLD_node_count (COLD_entry_limit-COLD_leaf_count)
#define COLD_bits_count (COLD_node_count / COLD_bits_per_mask)

#define COLD_entry_for_index(_node, _index) (((_node) >> ((_index) * COLD_bits_per_entry)) & COLD_entry_mask)
#define COLD_node_entry_at_index(_entry, _index) ((COLD_node_t)(_entry) << ((_index) * COLD_bits_per_entry))
#define COLD_node_with_entry_at_index(_node, _entry, _index) (((_node) & ~COLD_node_entry_at_index(COLD_entry_mask,_index)) | COLD_node_entry_at_index(_entry,_index))

#define COLD_entry_is_null(_entry) !(_entry)
#define COLD_entry_is_node(_entry) ((_entry)<COLD_node_count)
#define COLD_entry_leaf_index(_entry) ((_entry)-(COLD_node_count-1))
#define COLD_leaf_index_entry(_index) ((_index)+(COLD_node_count-1))

#define COLD_node_index_root (COLD_node_count - 1)
#define COLD_node_index_last (COLD_node_count - 2)
#define COLD_node_index_pool 0
#define COLD_leaf_index_pool 0

#define COLD_node_acquired (1<<30)
#define COLD_node_recycled (1<<29)
#define COLD_node_reserved (1<<8)
#define COLD_node_detached 1

#if COLD_entries_per_node * COLD_bits_per_entry < COLD_bits_per_node
#define COLD_node_is_list(_node) (((_node)>>(COLD_bits_per_node-1))&1)
#define COLD_empty_list_node ((COLD_node_t)1<<(COLD_bits_per_node-1))
#define COLD_first_list_entry 0
#else
#define COLD_node_is_list(_node) (COLD_entry_for_index(_node,0)==COLD_node_index_root)
#define COLD_empty_list_node COLD_node_entry_at_index(COLD_node_index_root,0)
#define COLD_first_list_entry 1
#endif


#pragma mark - Types


typedef COLD_casn_t COLD_node_t;
typedef void const *COLD_data_t;

typedef struct COLD_Bits {
	COLD_mask_t bits[COLD_bits_count];
} COLD_bits_t;

typedef struct COLD_Leaf {
	/// reference count of leaf node
	COLD_math_t references;
	/// link to next leaf in unused leaf pool or hash when leaf acquired
	union { COLD_link_t link; COLD_hash_t hash; };
	/// key used to test equality when hash matches
	COLD_data_t key;
	/// value associated with key
	COLD_data_t value;
} COLD_leaf_t;

/*
	it is typical to use pointers for key and value but there is no intrinsic
	limit on the size of a leaf node.
*/

typedef struct COLD_Opaque {
	COLD_math_t capacity, count;
	/// branch nodes both in unused node pool and visible from root
	COLD_node_t nodes[COLD_node_count];
	/// bit field of branch nodes which have been released but not recycled
	COLD_bits_t recycle;
	/// callbacks to manage and compare keys
	COLD_call_t keyCalls;
	/// callbacks to manage values
	COLD_hold_t valueCalls;
	/// variable sized array of leaves in unused leaf pool and visible from root
	COLD_leaf_t leaves[8];
} COLD_root_t;

/*
	The leaf at position zero is the head of the unused leaf pool and is never
	referenced by a branch node.  The properties of that leaf are special.
	
	leaves[0].link : first available leaf
	leaves[0].hash : capacity and approximate leaf count
	leaves[0].references : reference count of unrecycled branch nodes
*/


#define COLD_call_retain(_calls, _data) ((_calls).retain(_data))
#define COLD_call_release(_calls, _data) ((_calls).release(_data))
#define COLD_call_hash(_calls, _data) ((_calls).hash(_data))
#define COLD_call_equal(_calls, _a, _b) ((_calls).equal(_a, _b))


#pragma mark - Logging


#ifndef COLD_warning
#if 1
#define COLD_warning(f,...) printf("" f "\n",__VA_ARGS__)
#else
#define COLD_warning(f,...)
#endif
#endif

#ifndef COLD_trace
#if 0
#define COLD_trace(f,...) printf("" f "\n",__VA_ARGS__)
#else
#define COLD_trace(f,...)
#endif
#endif

#ifndef COLD_assert
#if 1
void COLD_asserted(int value) {
	value = 0;
}

#define COLD_assert(condition,f,...) if(!(condition))COLD_asserted(printf("ASSERT(#%u) !(%s): " f "\n",__LINE__,#condition,__VA_ARGS__))
#else
#define COLD_assert(condition,f,...)
#endif
#endif


#pragma mark - Initialization


COLD_hash_t COLD_hash_key(COLD_data_t key);
unsigned COLD_equal_keys(COLD_data_t a, COLD_data_t b);


unsigned COLD_attributes() {
#if COLD_ORDER_BY_HASH
	return COLD_DISPOSITION + 0x0100;
#else
	return COLD_DISPOSITION;
#endif
}

unsigned COLD_maximum_capacity() {
	return COLD_leaf_count;
}

unsigned COLD_size_for_capacity(unsigned capacity) {
	return sizeof(COLD_root_t) + sizeof(COLD_leaf_t) * (capacity - 7);
}

unsigned COLD_capacity_for_size(unsigned size) {
	return (size - sizeof(COLD_root_t)) / sizeof(COLD_leaf_t) + 7;
}

COLD COLD_allocate(unsigned request, COLD_call_t const *keyCalls, COLD_hold_t const *valueCalls) {
	unsigned capacity = request < COLD_leaf_count ? request < 8 ? 7 : request : COLD_leaf_count;
	unsigned size = COLD_size_for_capacity(capacity);
	
	COLD cold = malloc(size);
	
	if ( cold ) {
		COLD_initialize(cold, capacity, keyCalls, valueCalls);
	}
	
	return cold;
}

void COLD_deallocate(COLD cold) {
	COLD_remove_all(cold);
	free(cold);
}

void COLD_initialize(COLD cold, unsigned elements, COLD_call_t const *keyCalls, COLD_hold_t const *valueCalls) {
	COLD_assert(elements >= 4 && elements <= COLD_leaf_count, "COLD_initialize %u", elements);
	const COLD_leaf_t zeroLeaf = {COLD_node_recycled};
	const COLD_call_t zeroCall = {NULL};
	unsigned i;
	
	if ( elements > COLD_leaf_count ) { elements = COLD_leaf_count; }
	
	cold->count = 0;
	cold->capacity = elements;
	cold->keyCalls = keyCalls ? *keyCalls : zeroCall;
	cold->valueCalls = valueCalls ? *valueCalls : *(COLD_hold_t *)&zeroCall;
	
	if ( cold->keyCalls.hash == NULL || cold->keyCalls.equal == NULL ) { cold->keyCalls.hash = COLD_hash_key; }
	if ( cold->keyCalls.equal == NULL ) { cold->keyCalls.equal = COLD_equal_keys; }
	
	for ( i = 0 ; i < COLD_node_index_last ; ++i ) {
		cold->nodes[i] = i + 1;
	}
	
	cold->nodes[COLD_node_index_last] = 0;
	cold->nodes[COLD_node_index_root] = 0;
	
	for ( i = 0 ; i <= elements ; ++i ) {
		cold->leaves[i] = zeroLeaf;
		cold->leaves[i].link = i + 1;
	}
	
	cold->leaves[0].references = 0;
	cold->leaves[elements].link = 0;
}

void COLD_release_value(COLD cold, COLD_data_t value) {
	if ( cold->valueCalls.release ) { COLD_call_release(cold->valueCalls, value); }
}


#pragma mark - References


void COLD_recycle_nodes(COLD cold, unsigned nodeIndices[], unsigned count);

/// COLD_enter effectively reserves every branch node in the structure
COLD_math_t COLD_enter(COLD cold) {
	COLD_math_t token = COLD_atomic_addm(&cold->leaves[0].references, 1);
	COLD_assert(token & 0x03FF, "COLD_enter %08X", token);
	return token;
}

/// COLD_touch is called after releasing branch nodes that should be recycled
void COLD_touch(COLD cold) {
	COLD_math_t references, old = cold->leaves[0].references;
	COLD_math_t enter = old;
	
	/*
		references >>  0 & 0x03FF : normal referencse
		references >> 10 & 0x03FF : owning references
		references >> 20 & 0x0FFF : change token
	*/
	
	do {
		references = (old & ~0x000FFC00) + ((old & 0x03FF) << 10) + (1 << 20);
		if ( COLD_atomic_casm(&cold->leaves[0].references, old, references) ) { break; }
		old = cold->leaves[0].references;
	} while ( (old ^ enter) >> 20 == 0 );	//	zero until touched
}

/// COLD_leave may recycle any branch nodes released since COLD_enter was called
void COLD_leave(COLD cold, COLD_math_t enter) {
	COLD_bits_t recycle = cold->recycle;
	COLD_math_t after, prior = COLD_atomic_addm(&cold->leaves[0].references, -1);
	COLD_mask_t bits;
	unsigned nodes[16];
	unsigned order, index, count = 0;
	
	if ( (prior ^ enter) >> 20 == 0 ) { return; }
	
	after = COLD_atomic_addm(&cold->leaves[0].references, -(1 << 10));
	
	if ( (after & 0x000FFC00) != 0 ) { return; }
	
	for ( order = 0 ; order < COLD_bits_count ; ++order ) {
		bits = recycle.bits[order];
		if ( !bits ) { continue; }
		
		recycle.bits[order] = COLD_atomic_andm(&cold->recycle.bits[order], ~bits) & bits;
	}
	
	for ( order = 0 ; order < COLD_bits_count ; ++order ) {
		bits = recycle.bits[order];
		if ( !bits ) { continue; }
		
		for ( index = 0 ; index < 32 ; ++index ) {
			if ( !((bits >> index) & 1) ) { continue; }
			
			nodes[count++] = index + (order * 32);
			
			if ( count == 16 ) { COLD_recycle_nodes(cold, nodes, count); count = 0; }
		}
	}
	
	if ( count > 0 ) {
		COLD_recycle_nodes(cold, nodes, count);
	}
}


#pragma mark - Branches


#define COLD_AcquireInterrupted COLD_entry_limit
#define COLD_AcquireFailed 0

#define COLD_pool_dead_node COLD_node_entry_at_index(COLD_node_index_root,1)
#define COLD_pool_node_mask COLD_entry_mask
#define COLD_pool_node_salt COLD_pool_dead_node
#define COLD_pool_salt_mask ~(COLD_pool_increment-1)
#define COLD_pool_increment (1<<(COLD_bits_per_node - (COLD_bits_per_entry * (COLD_entries_per_node - 2))))

/// COLD_acquire_node acquires available node from tte pool of unused nodes
unsigned COLD_acquire_node(COLD cold) {
	COLD_node_t volatile *nodes = cold->nodes;
	COLD_node_t volatile *pool = nodes + 0;
	COLD_node_t node, next, head, salt, peek;
	
	head = *pool;
	node = head & COLD_pool_node_mask;
	COLD_assert(node < COLD_node_count, "COLD_acquire_node " COLD_NODE_FMT, head);
	if ( !node ) { COLD_warning("COLD_acquire_node pool depleted " COLD_NODE_FMT, head); return COLD_AcquireFailed; }
	
	peek = nodes[node];
	next = peek & COLD_pool_node_mask;
	salt = (head & COLD_pool_salt_mask) + COLD_pool_node_salt;
	if ( !COLD_atomic_casn(pool, head, next + salt) ) { return COLD_AcquireInterrupted; }
	
	COLD_assert(next < COLD_node_count, "COLD_acquire_node " COLD_NODE_FMT, peek);
	COLD_assert(!COLD_test_bit(cold->recycle.bits, node), "COLD_acquire_node " COLD_ENTRY_FMT, node);
	nodes[node] = salt;
	
	return (unsigned)node;
}

/// COLD_recycle_nodes restores nodes to the pool of unused nodes
void COLD_recycle_nodes(COLD cold, unsigned nodeIndices[], unsigned count) {
	COLD_assert(count > 0 && count < COLD_node_count, "COLD_recycle_nodes %u", count);
	COLD_node_t volatile *nodes = cold->nodes;
	COLD_node_t volatile *pool = nodes + 0;
	COLD_node_t next, head, salt;
	unsigned index, nodeIndex = 0;
	
	for ( index = 0 ; index < count ; ++index ) {
		nodeIndex = nodeIndices[index];
		COLD_assert(nodeIndex > 0 && nodeIndex < COLD_node_count, "COLD_recycle_nodes %02X", nodeIndex);
		COLD_assert(!COLD_test_bit(cold->recycle.bits, nodeIndex), "COLD_recycle_nodes %02X", nodeIndex);
		nodes[nodeIndex] = COLD_pool_dead_node;
	}
	
	do {
		head = *pool;
		next = head & COLD_pool_node_mask;
		salt = (head & COLD_pool_salt_mask) + COLD_pool_node_salt + COLD_pool_increment;
		COLD_assert(next < COLD_node_count, "COLD_recycle_nodes " COLD_NODE_FMT , head);
		
		for ( index = 0 ; index < count ; ++index ) {
			nodeIndex = nodeIndices[index];
			nodes[nodeIndex] = next + salt;
			next = nodeIndex;
		}
	} while ( !COLD_atomic_casn(pool, head, nodeIndex + salt) );
}

/// COLD_release_nodes marks nodes as needing to be recycled when methods leave
void COLD_release_nodes(COLD cold, unsigned nodeIndices[], unsigned count) {
	COLD_assert(count > 0 && count < COLD_node_count, "COLD_release_nodes %u", count);
	COLD_bits_t recycle = {0};
	
	for ( unsigned index = 0 ; index < count ; ++index ) {
		unsigned nodeIndex = nodeIndices[index];
		COLD_assert(nodeIndex > 0 && nodeIndex < COLD_node_count, "COLD_release_nodes %02X", nodeIndex);
		COLD_assert(!COLD_test_bit(cold->recycle.bits, nodeIndex), "COLD_release_nodes %02X", nodeIndex);
		COLD_set_bit(recycle.bits, nodeIndex);
	}
	
	// touch between removing nodes and marking nodes for recycling
	COLD_touch(cold);
	
	for ( unsigned order = 0 ; order < COLD_bits_count ; ++order ) {
		COLD_mask_t bits = recycle.bits[order];
		
		if ( bits ) { COLD_atomic_orm(&cold->recycle.bits[order], bits); }
	}
}


#pragma mark - Leaves


unsigned COLD_acquire_leaf(COLD cold) {
	COLD_leaf_t volatile *leaves = cold->leaves;
	COLD_link_t volatile *pool = &leaves[COLD_leaf_index_pool].link;
	COLD_link_t link, next, head, salt, peek;
	
	head = *pool;
	link = head & COLD_entry_mask;
	if ( !link ) { COLD_warning("COLD_acquire_leaf pool depleted " COLD_LINK_FMT, head); return COLD_AcquireFailed; }
	COLD_assert(link <= COLD_leaf_count, "COLD_acquire_leaf head " COLD_LINK_FMT, head);
	
	salt = head & ~COLD_entry_mask;
	peek = leaves[link].link;
	next = peek & COLD_entry_mask;
	COLD_assert(link != next, "COLD_acquire_leaf " COLD_LINK_FMT " next " COLD_LINK_FMT, link, peek);
	if ( !COLD_atomic_casl(pool, head, next + salt + COLD_entry_limit) ) { return COLD_AcquireInterrupted; }
	COLD_assert(next <= COLD_leaf_count, "COLD_acquire_leaf next " COLD_LINK_FMT, peek);
	
	leaves[link].link = 0;
	COLD_atomic_addm(&leaves[link].references, COLD_node_acquired - COLD_node_recycled + COLD_node_detached);
	
	return (unsigned)link;
}

void COLD_recycle_leaf(COLD cold, unsigned nodeIndex) {
	COLD_leaf_t volatile *leaves = cold->leaves;
	COLD_link_t volatile *pool = &leaves[COLD_leaf_index_pool].link;
	COLD_leaf_t volatile *leaf = leaves + nodeIndex;
	COLD_link_t next, head, salt;
	COLD_data_t key, value;
	
	if ( !COLD_atomic_casm(&leaves[nodeIndex].references, 0, COLD_node_recycled) ) { return; }
	
	key = leaf->key;
	value = leaf->value;
	
	leaf->link = 0;
	leaf->key = NULL;
	leaf->value = NULL;
	
	if ( cold->keyCalls.release ) { COLD_call_release(cold->keyCalls, key); }
	if ( cold->valueCalls.release ) { COLD_call_release(cold->valueCalls, value); }
	
	do {
		head = *pool;
		next = head & COLD_entry_mask;
		salt = (head & ~COLD_entry_mask) + COLD_entry_limit;
		
		COLD_assert(next != nodeIndex, "COLD_recycle_leaf %02X next " COLD_LINK_FMT, nodeIndex, head);
		COLD_assert(next <= COLD_leaf_count, "COLD_recycle_leaf %02X next " COLD_LINK_FMT, nodeIndex, head);
		leaf->link = next;
	} while ( !COLD_atomic_casl(pool, head, nodeIndex + salt) );
}

void COLD_dispose_leaf(COLD cold, unsigned nodeIndex, COLD_math_t balance) {
	COLD_assert(nodeIndex > 0 && nodeIndex <= COLD_leaf_count, "COLD_dispose_leaf %02X", nodeIndex);
	if ( 0 == COLD_atomic_addm(&cold->leaves[nodeIndex].references, -balance) ) {
		COLD_recycle_leaf(cold, nodeIndex);
	}
}

void COLD_release_leaf(COLD cold, unsigned nodeIndex) {
	COLD_dispose_leaf(cold, nodeIndex, COLD_node_reserved);
}

unsigned COLD_reserve_leaf(COLD cold, unsigned nodeIndex) {
	COLD_assert(nodeIndex > 0 && nodeIndex <= COLD_leaf_count, "COLD_reserve_leaf %02X", nodeIndex);
	if ( COLD_node_acquired & COLD_atomic_addm(&cold->leaves[nodeIndex].references, COLD_node_reserved) ) {
		return 1;
	} else {
		COLD_release_leaf(cold, nodeIndex);
		return 0;
	}
}


#pragma mark - Tombs


#define COLD_node_not_entombed COLD_node_index_root

#define COLD_entry_low_bits (COLD_entry_ones*((1<<(COLD_bits_per_entry-1))-1))
#define COLD_entry_high_bit (COLD_entry_ones*(1<<(COLD_bits_per_entry-1)))
#define COLD_node_branch_mask(_node) ((COLD_entry_low_bits+((_node)&COLD_entry_low_bits)|(_node))&COLD_entry_high_bit)

unsigned COLD_entombed_node_entry(COLD_node_t node, unsigned list) {
	COLD_node_t test = list ? (node - COLD_empty_list_node) : node;
	
	if ( !test ) { return 0; }
	
	COLD_node_t branch_mask = COLD_node_branch_mask(test);
	if ( branch_mask & (branch_mask - 1) ) { return COLD_node_not_entombed; }
	
	unsigned index = COLD_ctzn(branch_mask) / COLD_bits_per_entry;
	COLD_node_t lone_branch = COLD_entry_for_index(test, index);
	if ( COLD_entry_is_node(lone_branch) ) { return COLD_node_not_entombed; }
	
	return (unsigned)lone_branch;
}

unsigned COLD_reclaim_tomb(COLD cold, unsigned nodeIndex, COLD_node_t node, unsigned entryIndex, unsigned entry, unsigned recycleIndex) {
	COLD_node_t changeNode = COLD_node_with_entry_at_index(node, entry, entryIndex);
	
	COLD_assert(entry == 0 || entry >= COLD_node_count, "COLD_reclaim_tomb %u", entry);
	COLD_assert(entryIndex < COLD_entries_per_node, "COLD_reclaim_tomb %u", entryIndex);
	COLD_assert(nodeIndex > 0 && nodeIndex < COLD_node_count, "COLD_reclaim_tomb %u", nodeIndex);
	COLD_assert(recycleIndex > 0 && recycleIndex < COLD_node_count, "COLD_reclaim_tomb %u", recycleIndex);
	
	unsigned recycled = COLD_test_bit(cold->recycle.bits, nodeIndex);
	if ( COLD_atomic_casn(cold->nodes + nodeIndex, node, changeNode) ) {
		COLD_trace("COLD_reclaim_tomb [%u] node[%02X] " COLD_NODE_FMT " ==> " COLD_NODE_FMT, entryIndex, nodeIndex, node, changeNode);
		COLD_assert(!recycled, "COLD_reclaim_tomb %02X " COLD_NODE_FMT " ==> " COLD_NODE_FMT, nodeIndex, node, changeNode);
		COLD_release_nodes(cold, &recycleIndex, 1);
		return 1;
	}
	
	return 0;
}


#pragma mark - Search


unsigned COLD_search(COLD cold, COLD_data_t key, COLD_data_t *copyValueFound) {
	COLD_data_t result = NULL;
	COLD_math_t enter = COLD_enter(cold);
	COLD_hash_t hash = COLD_call_hash(cold->keyCalls, key);
	COLD_node_t volatile *nodes = cold->nodes;
	COLD_leaf_t volatile *leaves = cold->leaves;
	COLD_node_t next, node;
	
	unsigned status = 0;
	unsigned depth, order = 0, list = 0;
	unsigned entry, entryIndex, index, tomb;
	unsigned nodeIndex = COLD_node_index_root;
	node = nodes[nodeIndex];
	
	for ( depth = 0 ;; ) {
		if ( list ) {
			if ( order >= COLD_entries_per_node ) { break; }
			
			entryIndex = order;
			order += 1;
		} else {
			if ( depth >= COLD_maximum_depth ) { break; }
			
			entryIndex = COLD_entry_index_for_depth(hash, depth);
			depth += 1;
		}
		
		entry = COLD_entry_for_index(node, entryIndex);
		
		if ( COLD_entry_is_null(entry) ) {
			if ( list ) { continue; }
			else { break; }
		} if ( COLD_entry_is_node(entry) ) {
			next = nodes[entry];
			
			if ( list || COLD_node_is_list(next) ) {
				order = COLD_first_list_entry;
				list = 1;
			}
			
			tomb = COLD_entombed_node_entry(next, list);
			
			if ( COLD_node_not_entombed == tomb ) {
				nodeIndex = entry;
				node = next;
				continue;
			}
			
			COLD_reclaim_tomb(cold, nodeIndex, node, entryIndex, tomb, entry);
			
			if ( COLD_entry_is_null(tomb) ) {
				break;
			}
			
			index = COLD_entry_leaf_index(tomb);
		} else {
			index = COLD_entry_leaf_index(entry);
		}
		
		if ( !COLD_reserve_leaf(cold, index) ) {
			if ( list ) { continue; }
			else { break; }
		}
		
		if ( leaves[index].hash == hash && COLD_call_equal(cold->keyCalls, leaves[index].key, key) ) {
			result = leaves[index].value;
			
			if ( copyValueFound && cold->valueCalls.retain ) {
				result = COLD_call_retain(cold->valueCalls, result);
			}
			
			status = 1;
		}
		
		COLD_release_leaf(cold, index);
		
		if ( status || !list ) { break; }
	}
	
	COLD_leave(cold, enter);
	
	if ( copyValueFound ) {
		*copyValueFound = result;
	}
	
	return status;
}


COLD_data_t COLD_copy_value(COLD cold, COLD_data_t key) {
	COLD_data_t result = NULL;
	
	COLD_search(cold, key, &result);
	
	return result;
}


#pragma mark - Modify


unsigned COLD_remove(COLD cold, COLD_data_t key, COLD_data_t *copyValueRemoved) {
	COLD_data_t result = NULL;
	COLD_math_t enter = COLD_enter(cold);
	COLD_hash_t hash = COLD_call_hash(cold->keyCalls, key);
	COLD_node_t volatile *nodes = cold->nodes;
	COLD_leaf_t volatile *leaves = cold->leaves;
	COLD_node_t next, node, changeNode;
	unsigned status = 0;
	unsigned depth, order = 0, list;
	unsigned entry, entryIndex, index, tomb;
	unsigned retry, count, ascend, nodeIndex;
	unsigned recycleNodes[COLD_maximum_depth * 2];
	unsigned nodeIndices[COLD_maximum_depth * 2];
	
	do {
		list = 0;
		retry = 0;
		ascend = 0;
		nodeIndex = COLD_node_index_root;
		node = nodes[nodeIndex];
		nodeIndices[ascend = 0] = nodeIndex;
		
		for ( depth = 0 ;; ) {
			if ( list ) {
				if ( order >= COLD_entries_per_node ) { break; }
				
				entryIndex = order;
				order += 1;
			} else {
				if ( depth >= COLD_maximum_depth ) { break; }
				
				entryIndex = COLD_entry_index_for_depth(hash, depth);
				depth += 1;
			}
			
			entry = COLD_entry_for_index(node, entryIndex);
			
			if ( COLD_entry_is_null(entry) ) {
				if ( list ) { continue; }
				else { break; }
			} if ( COLD_entry_is_node(entry) ) {
				next = nodes[entry];
				
				if ( list || COLD_node_is_list(next) ) {
					order = COLD_first_list_entry;
					list = 1;
				}
				
				if ( ascend < COLD_maximum_depth * 2 ) {
					nodeIndices[++ascend] = entry;
				}
				
				tomb = COLD_entombed_node_entry(next, list);
				
				if ( COLD_node_not_entombed == tomb ) {
					nodeIndex = entry;
					node = next;
					continue;
				}
				
				COLD_reclaim_tomb(cold, nodeIndex, node, entryIndex, tomb, entry);
				
				if ( !COLD_entry_is_null(tomb) ) {
					retry = 1;
				}
				
				break;
			} else {
				index = COLD_entry_leaf_index(entry);
			}
			
			if ( !COLD_reserve_leaf(cold, index) ) {
				if ( list ) { continue; }
				else { break; }
			}
			
			if ( leaves[index].hash == hash && COLD_call_equal(cold->keyCalls, leaves[index].key, key) ) {
				changeNode = COLD_node_with_entry_at_index(node, 0, entryIndex);
				
				unsigned recycled = COLD_test_bit(cold->recycle.bits, nodeIndex);
				if ( COLD_atomic_casn(nodes + nodeIndex, node, changeNode) ) {
					COLD_trace("COLD_remove %08X @%2u[%u] node[%02X] " COLD_NODE_FMT " ==> " COLD_NODE_FMT, hash, depth, entryIndex, nodeIndex, node, changeNode);
					COLD_assert(!recycled, "COLD_remove %02X " COLD_NODE_FMT " ==> " COLD_NODE_FMT, nodeIndex, node, changeNode);
					COLD_atomic_addm(&cold->count, -1);
					
					if ( copyValueRemoved ) {
						result = leaves[index].value;
						
						if ( cold->valueCalls.retain ) { result = COLD_call_retain(cold->valueCalls, result); }
					}
					
					status = 1;
					COLD_dispose_leaf(cold, index, COLD_node_acquired);
				} else {
					retry = 1;
				}
				
				list = 0;	//	break after release leaf
			}
			
			COLD_release_leaf(cold, index);
			
			if ( !list ) { break; }
		}
	} while ( retry );
	
	for ( count = 0 ; ascend > 0 ; --ascend ) {
		entry = nodeIndices[ascend];
		next = nodes[entry];
		tomb = COLD_entombed_node_entry(next, ascend >= depth);
		if ( COLD_node_not_entombed == tomb ) { break; }
		
		nodeIndex = nodeIndices[ascend - 1];
		node = nodes[nodeIndex];
		entryIndex = ( ascend > depth ) ? COLD_entries_per_node - 1 : COLD_entry_index_for_depth(hash, ascend - 1);
		if ( COLD_entry_for_index(node, entryIndex) != entry ) { break; }
		
		changeNode = COLD_node_with_entry_at_index(node, tomb, entryIndex);
		if ( !COLD_atomic_casn(nodes + nodeIndex, node, changeNode) ) { break; }
		COLD_trace("COLD_ascend %08X @%2u[%u] node[%02X] " COLD_NODE_FMT " ==> " COLD_NODE_FMT, hash, ascend - 1, entryIndex, nodeIndex, node, changeNode);
		
		recycleNodes[count++] = entry;
	}
	
	if ( count > 0 ) {
		COLD_release_nodes(cold, recycleNodes, count);
	}
	
	COLD_leave(cold, enter);
	
	if ( copyValueRemoved ) {
		*copyValueRemoved = result;
	}
	
	return status;
}

unsigned COLD_assign(COLD cold, COLD_data_t key, COLD_data_t value, unsigned options, COLD_data_t *copyValueReplaced) {
	enum { Descend = 0, Insert = 1, Replace = 2, Convert = 3, Append = 4 };
	
	struct Rollback {
		COLD_node_t node;
		unsigned nodeIndex, entryIndex;
	};
	
	COLD_data_t result = NULL;
	COLD_math_t enter = COLD_enter(cold);
	COLD_hash_t hash = COLD_call_hash(cold->keyCalls, key);
	COLD_node_t volatile *nodes = cold->nodes;
	COLD_leaf_t volatile *leaves = cold->leaves;
	COLD_node_t next, node, changeNode = 0;
	COLD_hash_t otherHash = 0;
	struct Rollback insertion, splitting;
	unsigned depth, order = 0, list;
	unsigned entry, entryIndex, index, tomb, action, status;
	unsigned retry, ascend, nodeIndex, releaseIndex;
	unsigned assignLeaf, disposeLeaf = 0;
	unsigned nodesAssigned, nodesAcquired = 0;
	unsigned recycleNodes[COLD_maximum_depth];
	
	do {
		list = 0;
		retry = 0;
		ascend = 0;
		releaseIndex = 0;
		nodesAssigned = 0;
		nodeIndex = COLD_node_index_root;
		node = nodes[nodeIndex];
		status = COLD_AssignSuccess;
		action = Descend;
		
		splitting.node = 0;
		insertion.node = 0;
		
		for ( depth = 0 ;; ) {
			if ( list ) {
				if ( order >= COLD_entries_per_node ) { break; }
				
				entryIndex = order;
				order += 1;
			} else {
				if ( depth >= COLD_maximum_depth ) { break; }
				
				entryIndex = COLD_entry_index_for_depth(hash, depth);
				depth += 1;
			}
			
			entry = COLD_entry_for_index(node, entryIndex);
			
			if ( COLD_entry_is_null(entry) ) {
				index = 0;
				
				if ( !list ) {
					//	empty entry in branch node
					action = Insert;
				} else if ( entryIndex + 1 == COLD_entries_per_node ) {
					if ( insertion.node ) {
						node = insertion.node;
						nodeIndex = insertion.nodeIndex;
						entryIndex = insertion.entryIndex;
					}
					
					action = Insert;
				} else {
					//	empty entry in list node
					if ( !insertion.node ) {
						insertion.node = node;
						insertion.nodeIndex = nodeIndex;
						insertion.entryIndex = entryIndex;
					}
					
					continue;
				}
			} else if ( COLD_entry_is_node(entry) ) {
				next = nodes[entry];
				
				if ( list ) {
					order = COLD_first_list_entry;
				} else if ( COLD_node_is_list(next) ) {
					order = COLD_first_list_entry;
					list = 1;
					
					splitting.node = node;
					splitting.nodeIndex = nodeIndex;
					splitting.entryIndex = entryIndex;
				}
				
				tomb = COLD_entombed_node_entry(next, list);
				
				if ( COLD_node_not_entombed == tomb ) {
					nodeIndex = entry;
					node = next;
					continue;
				}
				
				COLD_reclaim_tomb(cold, nodeIndex, node, entryIndex, tomb, entry);
				
				retry = 1;
				break;
			} else {
				index = COLD_entry_leaf_index(entry);
				
				if ( !COLD_reserve_leaf(cold, index) ) {
					retry = 1; break;
				}
				
				releaseIndex = index;
				otherHash = leaves[index].hash;
				
				if ( otherHash != hash ) {
					if ( list ) {
						entryIndex = splitting.entryIndex;
						nodeIndex = splitting.nodeIndex;
						node = splitting.node;
						list = 0;
					}
					
					action = Convert;
				} else if ( COLD_call_equal(cold->keyCalls, leaves[index].key, key) ) {
					action = Replace;
				} else if ( !list ) {
					action = Append;
				} else if ( entryIndex + 1 == COLD_entries_per_node ) {
					if ( insertion.node ) {
						action = Insert;
						node = insertion.node;
						nodeIndex = insertion.nodeIndex;
						entryIndex = insertion.entryIndex;
					} else {
						action = Append;
					}
				} else {
					continue;
				}
			}
			
			if ( Replace == action ) {
				result = leaves[index].value;
				
				if ( options & COLD_AssignSum ) {
					result = COLD_atomic_addp(&leaves[index].value, value); break;
				} else if ( options & COLD_AssignNoReplace ) {
					status = COLD_AssignExistingEntry; break;
				} else if ( result == value ) {
					break;
				} else if ( cold->valueCalls.retain != NULL || cold->valueCalls.release != NULL ) {
					//	replace entire leaf
				} else if ( COLD_atomic_casp(&leaves[index].value, (void *)result, (void *)value) ) {
					COLD_trace("COLD_assign %08X @%2u[%u] leaf[%2u] %p ==> %p", hash, depth, entryIndex, index, result, value);
					break;
				} else {
					retry = 1; break;
				}
			} else if ( options & COLD_AssignOnlyReplace ) {
				status = COLD_AssignMissingEntry;
				break;
			}
			
			if ( disposeLeaf ) {
				assignLeaf = disposeLeaf;
			} else {
				assignLeaf = COLD_acquire_leaf(cold);
				
				if ( COLD_AcquireInterrupted == assignLeaf ) { retry = 1; break; }
				else if ( COLD_AcquireFailed == assignLeaf ) { status = COLD_AssignNoLeaves; break; }
				else { disposeLeaf = assignLeaf; }
				
				COLD_data_t useKey = ( Replace == action ) ? leaves[index].key : key;
				
				leaves[assignLeaf].key = cold->keyCalls.retain ? COLD_call_retain(cold->keyCalls, useKey) : useKey;
				leaves[assignLeaf].hash = hash;
				leaves[assignLeaf].value = cold->valueCalls.retain ? COLD_call_retain(cold->valueCalls, value) : value;
			}
			
			if ( Insert == action || Replace == action ) {
				changeNode = COLD_node_with_entry_at_index(node, COLD_leaf_index_entry(assignLeaf), entryIndex);
			} else if ( Append == action ) {
				unsigned acquired;
				
				if ( nodesAssigned < nodesAcquired ) {
					acquired = recycleNodes[nodesAssigned++];
				} else {
					acquired = COLD_acquire_node(cold);
					
					if ( COLD_AcquireInterrupted == acquired ) { retry = 1; break; }
					else if ( COLD_AcquireFailed == acquired ) { status = COLD_AssignNoNodes; break; }
					else { recycleNodes[nodesAcquired++] = acquired; nodesAssigned = nodesAcquired; }
				}
				
				nodes[acquired] = COLD_empty_list_node
					| COLD_node_entry_at_index(COLD_leaf_index_entry(index), COLD_first_list_entry)
					| COLD_node_entry_at_index(COLD_leaf_index_entry(assignLeaf), COLD_first_list_entry + 1);
				
				changeNode = COLD_node_with_entry_at_index(node, acquired, entryIndex);
			} else {
				unsigned acquired, previousEntry = 0, previousIndex = 0;
				
				for ( unsigned reach = depth ; reach < COLD_maximum_depth ; ++reach ) {
					if ( nodesAssigned < nodesAcquired ) {
						acquired = recycleNodes[nodesAssigned++];
					} else {
						acquired = COLD_acquire_node(cold);
						
						if ( COLD_AcquireInterrupted == acquired ) { retry = 1; break; }
						else if ( COLD_AcquireFailed == acquired ) { status = COLD_AssignNoNodes; break; }
						else { recycleNodes[nodesAcquired++] = acquired; nodesAssigned = nodesAcquired; }
					}
					
					if ( previousEntry ) {
						nodes[previousEntry] = COLD_node_entry_at_index(acquired, previousIndex);
					} else {
						changeNode = COLD_node_with_entry_at_index(node, acquired, entryIndex);
					}
					
					unsigned entryIndexA = COLD_entry_index_for_depth(otherHash, reach);
					unsigned entryIndexB = COLD_entry_index_for_depth(hash, reach);
					
					if ( entryIndexA != entryIndexB ) {
						nodes[acquired] = COLD_node_entry_at_index(COLD_leaf_index_entry(index), entryIndexA)
							| COLD_node_entry_at_index(COLD_leaf_index_entry(assignLeaf), entryIndexB);
						break;
					}
					
					previousIndex = entryIndexA;
					previousEntry = acquired;
				}
				
				if ( retry || status ) {
					nodesAssigned = 0;
					break;
				}
			}
			
			unsigned recycled = COLD_test_bit(cold->recycle.bits, nodeIndex);
			if ( COLD_atomic_casn(nodes + nodeIndex, node, changeNode) ) {
				COLD_atomic_addm(&leaves[assignLeaf].references, -COLD_node_detached);
				COLD_atomic_addm(&cold->count, 1);
				COLD_trace("COLD_insert %08X @%2u[%u] node[%02X] " COLD_NODE_FMT " ==> " COLD_NODE_FMT " leaf[%2u] %p", hash, depth, entryIndex, nodeIndex, node, changeNode, assignLeaf, value);
				COLD_assert(!recycled, "COLD_assign %02X " COLD_NODE_FMT " ==> " COLD_NODE_FMT, nodeIndex, node, changeNode);
				disposeLeaf = 0;
			} else {
				retry = 1;
			}
			
			break;
		}
		
		if ( Replace == action && copyValueReplaced && !retry && cold->valueCalls.retain ) { result = COLD_call_retain(cold->valueCalls, result); }
		if ( releaseIndex ) { COLD_release_leaf(cold, releaseIndex); }
	} while ( retry );
	
	if ( disposeLeaf ) {
		COLD_dispose_leaf(cold, disposeLeaf, COLD_node_acquired + COLD_node_detached);
	}
	
	if ( nodesAssigned < nodesAcquired ) {
		COLD_recycle_nodes(cold, recycleNodes + nodesAssigned, nodesAcquired - nodesAssigned);
	}
	
	COLD_leave(cold, enter);
	
	if ( copyValueReplaced ) {
		*copyValueReplaced = result;
	}
	
	return status;
}


#pragma mark - Query


unsigned COLD_capacity(COLD const cold) {
	return cold->capacity;
}

unsigned COLD_count(COLD const cold) {
	return cold->count;
}

unsigned COLD_is_empty(COLD const cold) {
	return cold->count == 0;
}

unsigned COLD_enumerate(COLD const cold, COLD_enumerator enumerator, void *context) {
	unsigned result = 0;
	unsigned index, capacity = COLD_capacity(cold);
	COLD_leaf_t volatile *leaves = cold->leaves;
	COLD_leaf_t volatile *leaf;
	
	for ( index = 1 ; index <= capacity && !result ; ++index ) {
		if ( !COLD_reserve_leaf(cold, index) ) { continue; }
		
		leaf = leaves + index;
		
		if ( !(leaf->references & COLD_node_detached) ) {
			result = enumerator(leaf->key, leaf->value, context, leaf->hash);
		}
		
		COLD_release_leaf(cold, index);
	}
	
	return result;
}

unsigned COLD_enumerate_next(COLD const cold, unsigned prior, COLD_enumerator enumerator, void *context) {
	unsigned result = 0;
	unsigned index, capacity = COLD_capacity(cold);
	COLD_leaf_t volatile *leaves = cold->leaves;
	COLD_leaf_t volatile *leaf;
	
	for ( index = 1 + prior ; index <= capacity && !result ; ++index ) {
		if ( !COLD_reserve_leaf(cold, index) ) { continue; }
		
		leaf = leaves + index;
		
		if ( !(leaf->references & COLD_node_detached) ) {
			enumerator(leaf->key, leaf->value, context, leaf->hash);
			result = index;
		}
		
		COLD_release_leaf(cold, index);
	}
	
	return result;
}

unsigned COLD_remove_all(COLD cold) {
	unsigned result = 0;
	unsigned index, capacity = COLD_capacity(cold);
	COLD_leaf_t volatile *leaves = cold->leaves;
	
	for ( index = 1 ; index <= capacity && !result ; ++index ) {
		if ( !COLD_reserve_leaf(cold, index) ) { continue; }
		
		COLD_remove(cold, leaves[index].key, NULL);
		COLD_release_leaf(cold, index);
		result += 1;
	}
	
	return result;
}


#pragma mark - Verify


unsigned COLD_verify_recurse(COLD const cold, unsigned nodeIndex, uint32_t seen[]) {
	unsigned result = 1;
	COLD_node_t node = cold->nodes[nodeIndex];
	unsigned list = COLD_node_is_list(node);
	
	for ( unsigned order = (list ? COLD_first_list_entry : 0) ; order < COLD_entries_per_node ; ++order ) {
		unsigned entry = COLD_entry_for_index(node, order);
		
		if ( COLD_entry_is_null(entry) ) { continue; }
		else if ( COLD_entry_is_node(entry) ) { result += COLD_verify_recurse(cold, entry, seen); }
		else { result += 1 << 16; }
		
		seen[entry/32] |= 1 << (entry&31);
 	}
	
	return result;
}

unsigned COLD_verify(COLD const cold) {
	unsigned unverified = 0;
	unsigned used_leaves = 0;
	unsigned capacity = COLD_capacity(cold);
	
	COLD_node_t *nodes = cold->nodes;
	COLD_leaf_t *leaves = cold->leaves;
	COLD_leaf_t *pool = leaves + COLD_leaf_index_pool;
	
	uint32_t nodes_recycled[COLD_entry_limit/32] = {0};
	uint32_t nodes_acquired[COLD_entry_limit/32] = {0};
	
	COLD_verify_recurse(cold, COLD_node_index_root, nodes_acquired);
	
	COLD_node_t node = nodes[COLD_node_index_pool] & COLD_pool_node_mask;
	while ( node > 0 && node <= COLD_node_count ) {
		nodes_recycled[node/32] |= (1<<(node&31));
		node = nodes[node] & COLD_pool_node_mask;
	}
	
	COLD_link_t link = leaves[COLD_leaf_index_pool].link & COLD_entry_mask;
	while ( link > 0 && link <= capacity ) {
		COLD_link_t entry = COLD_leaf_index_entry(link);
		nodes_recycled[entry/32] |= (1<<(entry&31));
		link = leaves[link].link & COLD_entry_mask;
	}
	
	for ( unsigned entry = 1 ; entry <= COLD_node_index_last ; ++entry ) {
		unsigned is_acquired = (nodes_acquired[entry/32] >> (entry&31)) & 1;
		unsigned is_recycled = (nodes_recycled[entry/32] >> (entry&31)) & 1;
		unsigned is_released = COLD_test_bit(cold->recycle.bits, entry) ? 1 : 0;
		
		if ( is_recycled + is_released + is_acquired == 1 ) { continue; }
		
		COLD_assert(0, "COLD_verify %c%c%c branch node %02X " COLD_NODE_FMT,
			is_acquired ? 'a' : '-', is_released ? 'r' : '-', is_recycled ? 'p' : '-',
			entry, nodes[entry]);
		
		unverified += 1;
	}
	
	for ( unsigned leaf = 1 ; leaf <= capacity ; ++leaf ) {
		COLD_link_t entry = COLD_leaf_index_entry(leaf);
		COLD_math_t references = leaves[leaf].references;
		unsigned is_acquired = (nodes_acquired[entry/32] >> (entry&31)) & 1;
		unsigned is_recycled = (nodes_recycled[entry/32] >> (entry&31)) & 1;
		unsigned is_released = (references & (COLD_node_acquired | COLD_node_recycled)) ? 0 : 1;
		
		if ( is_acquired ) used_leaves += 1;
		if ( is_recycled + is_released + is_acquired == 1 ) { continue; }
		
		COLD_assert(0, "COLD_verify %c%c%c leaf node %02X R%08X H%08X",
			is_acquired ? 'a' : '-', is_released ? 'r' : '-', is_recycled ? 'p' : '-',
			leaf, references, leaves[leaf].link);
		
		unverified += 1;
	}
	
	COLD_assert((pool->references & 0x0FFFFF) == 0, "COLD_verify references %08X", pool->references);
	COLD_assert(used_leaves == cold->count, "COLD_verify leaves %u != %u", used_leaves, cold->count);
	
	return unverified;
}


#pragma mark - Print


unsigned COLD_print_recurse(COLD cold, unsigned nodeIndex, unsigned depth, unsigned level, char const *format_key_value_hash) {
	unsigned result = 0;
	COLD_node_t node = cold->nodes[nodeIndex];
	
	if ( 0 == level ) { printf("---------- %p [%u of %u] ----------\n", cold, COLD_count(cold), COLD_capacity(cold)); }
	if ( 0 == depth ) { printf("%3u|", level); }
	if ( level == depth ) { printf(" " COLD_ENTRY_FMT "%s", (COLD_node_t)nodeIndex, COLD_node_is_list(node) ? ";" : ":"); }
	
	for ( unsigned order = 0 ; order < COLD_entries_per_node ; ++order ) {
		unsigned entry = COLD_entry_for_index(node, order);
		
		if ( level == depth ) { printf("%s" COLD_ENTRY_FMT "", order > 0 ? " " : "[", (COLD_node_t)entry); }
		
		if ( COLD_entry_is_null(entry) ) {
			continue;
		} else if ( COLD_entry_is_node(entry) ) {
			if ( depth < level ) {
				result += COLD_print_recurse(cold, entry, depth + 1, level, format_key_value_hash);
			} else if ( depth == level && entry != COLD_node_index_root ) {
				result += 1;
			}
		} else {
			if ( depth + 1 == level ) {
				COLD_leaf_t *leaf = cold->leaves + COLD_entry_leaf_index(entry);
				
				if ( NULL == format_key_value_hash || 0 == *format_key_value_hash ) {
					printf(" " COLD_ENTRY_FMT ":[%08X %p:%p]", (COLD_node_t)entry, leaf->hash, leaf->key, leaf->value);
				} else {
					printf(" " COLD_ENTRY_FMT ":", (COLD_node_t)entry);
					printf(format_key_value_hash, leaf->key, leaf->value, leaf->hash);
				}
			} else if ( depth == level ) {
				result += 1;
			}
		}
 	}
	
	if ( level == depth ) { printf("]"); }
	if ( 0 == depth ) { printf("\n"); }
	
	return result;
}

void COLD_print(COLD cold, char const *format_key_value_hash) {
	unsigned level = 0;
	
	//	recursively iterate to depth for each depth
	while ( COLD_print_recurse(cold, COLD_node_index_root, 0, level++, format_key_value_hash) && level < COLD_maximum_depth * 2 ) {}
}


#pragma mark - Hash


COLD_hash_t COLD_hash_key(COLD_data_t key) {
	intptr_t raw = (intptr_t)key;
	
	if ( sizeof(COLD_data_t) > 8 && 8 >= sizeof(COLD_hash_t) ) { raw ^= raw >> 64; }
	if ( sizeof(COLD_data_t) > 4 && 4 >= sizeof(COLD_hash_t) ) { raw ^= raw >> 32; }
	if ( sizeof(COLD_data_t) > 2 && 2 >= sizeof(COLD_hash_t) ) { raw ^= raw >> 16; }
	if ( sizeof(COLD_data_t) > 1 && 1 >= sizeof(COLD_hash_t) ) { raw ^= raw >>  8; }
	
	return (COLD_hash_t)raw;
}

unsigned COLD_equal_keys(COLD_data_t a, COLD_data_t b) {
	return a == b;
}


#define COLD_hash_prime 8765423

#if 32 == COLD_bits_per_hash
#define COLD_hash_rotate(_hash) ((_hash) << 13) | ((_hash) >> 19)
#endif
#if 64 == COLD_bits_per_hash
#define COLD_hash_rotate(_hash) ((_hash) << 47) | ((_hash) >> 17)
#endif

COLD_hash_t COLD_hash_bytes(COLD_data_t key, unsigned length) {
	COLD_hash_t result = 0;
	unsigned char const *data = key;
	unsigned i;
	
	for ( i = 0 ; i < length ; ++i ) {
		result ^= data[i] * COLD_hash_prime;
		result = COLD_hash_rotate(result);
	}
	
	return result ^ (length * COLD_hash_prime);
}

COLD_hash_t COLD_hash_bytes_null_terminated(COLD_data_t key) {
	COLD_hash_t result = 0;
	unsigned char const *data = key;
	unsigned char c;
	unsigned i = 0;
	
	while (( c = data[i++] )) {
		result ^= c * COLD_hash_prime;
		result = COLD_hash_rotate(result);
	}
	
	return result ^ ((i - 1) * COLD_hash_prime);
}

unsigned COLD_hash_enumerator(COLD_data_t key, COLD_data_t value, void *context, COLD_hash_t hash) {
	*(COLD_hash_t *)context ^= hash;
	return 0;
}

COLD_hash_t COLD_hash(COLD cold) {
	COLD_hash_t result = COLD_count(cold) * COLD_hash_prime;
	COLD_enumerate(cold, COLD_hash_enumerator, &result);
	return result;
}
