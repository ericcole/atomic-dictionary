//
//  COL_SparseArray.c
//  AtomicDictionary
//
//  Created by Eric Cole on 7/15/16.
//  Copyright © 2016 Balance Software. All rights reserved.
//

#include "COL_SparseArray.h"
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


#define COLA_32_4 36
#define COLA_64_6 70
#define COLA_64_7 71
#define COLA_64_8 72

#ifndef COLA_DISPOSITION
#define COLA_DISPOSITION COLA_32_4
#endif


#if COLA_DISPOSITION < 64

typedef uint32_t COLA_casn_t;
#define COLA_bits_per_node 32
#define COLA_atomic_casn ECC_atomic_compare_and_swap_32
#define COLA_ctzn ECC_count_trailing_zeros_32
#define COLA_NODE_FMT "%08X"

#else

typedef uint64_t COLA_casn_t;
#define COLA_bits_per_node 64
#define COLA_atomic_casn ECC_atomic_compare_and_swap_64
#define COLA_ctzn ECC_count_trailing_zeros_64
#define COLA_NODE_FMT "%016llX"

#endif


#if COLA_DISPOSITION == COLA_32_4
#define COLA_entries_per_node 4
#define COLA_bits_per_level 2
#define COLA_bits_per_entry 8
#define COLA_maximum_depth 16	//	4^16 >= 2^32
#define COLA_entry_ones 0x01010101U
#define COLA_ENTRY_FMT "%02X"

//#define COLA_entry_index_for_depth(_hash, _depth) (((_hash)>>((_depth)*2))&3)
#define COLA_entry_index_for_depth(_hash, _depth) (((_hash) >> ((COLA_bits_per_hash - 2) - ((_depth) * 2))) & 3)
#endif

#if COLA_DISPOSITION == COLA_64_8
#define COLA_entries_per_node 8
#define COLA_bits_per_level 3
#define COLA_bits_per_entry 8
#define COLA_maximum_depth 11	//	8^11 >= 2^32
#define COLA_entry_ones 0x0101010101010101ULL
#define COLA_ENTRY_FMT "%02llX"

#define COLA_entry_index_for_depth(_hash, _depth) (((_hash)>>((_depth)*3))&7)
//#define COLA_entry_index_for_depth(_hash, _depth) (((_depth)<10)?(((_hash)>>((COLA_bits_per_hash-3)-((_depth)*3)))&7):((_hash)&3))
#endif

#if COLA_DISPOSITION == COLA_64_7
#define COLA_entries_per_node 7
#define COLA_bits_per_entry 9
#define COLA_maximum_depth 12	//	7^12 >= 2^32
#define COLA_entry_ones 0x0040201008040201ULL
#define COLA_ENTRY_FMT "%03llX"

const COLA_casn_t COLA_divisor_at_depth[12] = {1,7,7*7,7*7*7,7*7*7*7,7*7*7*7*7,7*7*7*7*7*7,7*7*7*7*7*7*7,7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7*7*7U};
#define COLA_entry_index_for_depth(_hash, _depth) (((_hash)/COLA_divisor_at_depth[_depth])%7)
//#define COLA_entry_index_for_depth(_hash, _depth) (((_depth)<11)?(((_hash)/(3*COLA_divisor_at_depth[10-(_depth)]))%7):((_hash)%3))
#endif

#if COLA_DISPOSITION == COLA_64_6
#define COLA_entries_per_node 6
#define COLA_bits_per_entry 10
#define COLA_maximum_depth 13	//	6^13 >= 2^32
#define COLA_entry_ones 0x0004010040100401ULL
#define COLA_ENTRY_FMT "%03llX"

const COLA_casn_t COLA_divisor_at_depth[13] = {1,6,6*6,6*6*6,6*6*6*6,6*6*6*6*6,6*6*6*6*6*6,6*6*6*6*6*6*6,6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6*6*6U};
#define COLA_entry_index_for_depth(_hash, _depth) (((_hash)/COLA_divisor_at_depth[_depth])%6)
//#define COLA_entry_index_for_depth(_hash, _depth) (((_depth)<12)?(((_hash)/(2*COLA_divisor_at_depth[11-(_depth)]))%6):((_hash)%2))
#endif

#if COLA_DISPOSITION == COLA_64_5
#define COLA_entries_per_node 5
#define COLA_bits_per_entry 12
#define COLA_maximum_depth 14	//	5^14 >= 2^32
#define COLA_entry_ones 0x0001001001001001ULL
#define COLA_ENTRY_FMT "%03llX"

const COLA_casn_t COLA_divisor_at_depth[14] = {1,5,5*5,5*5*5,5*5*5*5,5*5*5*5*5,5*5*5*5*5*5,5*5*5*5*5*5*5,5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5*5*5,5*5*5*5*5*5*5*5*5*5*5*5*5U};
#define COLA_entry_index_for_depth(_hash, _depth) (((_hash)/COLA_divisor_at_depth[_depth])%5)
//#define COLA_entry_index_for_depth(_hash, _depth) (((_depth)<13)?(((_hash)/(4*COLA_divisor_at_depth[12-(_depth)]))%5):((_hash)%4))
#endif


#pragma mark -


#define COLA_atomic_casp ECC_atomic_compare_and_swap_ptr
#define COLA_atomic_addp ECC_atomic_add_ptr

#if 1
typedef uint32_t COLA_hash_t;
#define COLA_bits_per_hash 32
#define COLA_atomic_addh ECC_atomic_add_32
#endif

#if 1
typedef uint32_t COLA_mask_t;
#define COLA_bits_per_mask 32
#define COLA_atomic_andm ECC_atomic_and_32
#define COLA_atomic_orm  ECC_atomic_or_32

#define COLA_test_bit(_bits,_bit) (_bits[(_bit)/32] & (1<<((_bit)&31)))
#define COLA_set_bit(_bits,_bit) (_bits[(_bit)/32] |= (1<<((_bit)&31)))
#endif

#if 1
typedef int32_t COLA_math_t;
#define COLA_bits_per_math 32
#define COLA_atomic_casm ECC_atomic_compare_and_swap_32
#define COLA_atomic_addm ECC_atomic_add_32
#endif


#pragma mark -


#define COLA_entry_limit (1<<COLA_bits_per_entry)
#define COLA_entry_mask (COLA_entry_limit-1)
#define COLA_leaf_count (COLA_entry_limit/4)
#define COLA_node_count (COLA_entry_limit-COLA_leaf_count)
#define COLA_bits_count (COLA_node_count / COLA_bits_per_mask)

#define COLA_entry_for_index(_node, _index) (((_node) >> ((_index) * COLA_bits_per_entry)) & COLA_entry_mask)
#define COLA_node_entry_at_index(_entry, _index) ((COLA_node_t)(_entry) << ((_index) * COLA_bits_per_entry))
#define COLA_node_with_entry_at_index(_node, _entry, _index) (((_node) & ~COLA_node_entry_at_index(COLA_entry_mask,_index)) | COLA_node_entry_at_index(_entry,_index))

#define COLA_entry_is_null(_entry) !(_entry)
#define COLA_entry_is_node(_entry) ((_entry)<COLA_node_count)
#define COLA_entry_leaf_index(_entry) ((_entry)-(COLA_node_count-1))
#define COLA_leaf_index_entry(_index) ((_index)+(COLA_node_count-1))

#define COLA_node_index_root (COLA_node_count - 1)
#define COLA_node_index_last (COLA_node_count - 2)
#define COLA_node_index_pool 0
#define COLA_leaf_index_pool 0

#define COLA_node_acquired (1<<30)
#define COLA_node_recycled (1<<29)
#define COLA_node_reserved (1<<8)


#pragma mark - COLA


typedef COLA_casn_t COLA_node_t;
typedef COLA_casn_t COLA_link_t;
typedef void *COLA_data_t;

typedef struct COLA_Bits {
	COLA_mask_t bits[COLA_bits_count];
} COLA_bits_t;

typedef struct COLA_Leaf {
	COLA_link_t link;
	COLA_hash_t hash;
	COLA_math_t references;
	COLA_math_t reserved;
	COLA_data_t value;
} COLA_leaf_t;

typedef struct COLA_Opaque {
	COLA_node_t nodes[COLA_node_count];
	COLA_bits_t recycle;
	COLA_leaf_t leaves[8];
} COLA_root_t;


#pragma mark -


#ifndef COLA_warning
#if 1
#define COLA_warning(f,...) printf("" f "\n",__VA_ARGS__)
#else
#define COLA_warning(f,...)
#endif
#endif

#ifndef COLA_trace
#if 0
#define COLA_trace(f,...) printf("" f "\n",__VA_ARGS__)
#else
#define COLA_trace(f,...)
#endif
#endif

#ifndef COLA_assert
#if 1
void COLA_asserted(int value) {
	value = 0;
}

#define COLA_assert(condition,f,...) if(!(condition))COLA_asserted(printf("ASSERT(#%u) !(%s): " f "\n",__LINE__,#condition,__VA_ARGS__))
#else
#define COLA_assert(condition,f,...)
#endif
#endif


#pragma mark -


unsigned COLA_attributes() {
	return COLA_DISPOSITION;
}

unsigned COLA_size_for_elements(unsigned elements) {
	return sizeof(COLA_root_t) + sizeof(COLA_leaf_t) * (elements - 7);
}

COLA COLA_allocate(unsigned elements) {
	if ( elements > COLA_leaf_count ) { return NULL; }
	
	unsigned capacity = elements < COLA_leaf_count ? elements < 8 ? 7 : elements : COLA_leaf_count;
	unsigned size = COLA_size_for_elements(capacity);
	
	COLA cola = malloc(size);
	
	if ( cola ) {
		COLA_initialize(cola, capacity);
	}
	
	return cola;
}

void COLA_deallocate(COLA cola) {
	free(cola);
}

void COLA_initialize(COLA cola, unsigned elements) {
	COLA_assert(elements >= 4 && elements <= COLA_leaf_count, "COLA_initialize %u", elements);
	unsigned i;
	
	for ( i = 0 ; i < COLA_node_index_last ; ++i ) {
		cola->nodes[i] = i + 1;
	}
	
	cola->nodes[COLA_node_index_last] = 0;
	cola->nodes[COLA_node_index_root] = 0;
	
	for ( i = 0 ; i <= elements ; ++i ) {
		cola->leaves[i].link = i + 1;
		cola->leaves[i].references = COLA_node_recycled;
		cola->leaves[i].hash = 0;
	}
	
	cola->leaves[0].references = 0;
	cola->leaves[0].hash = elements << (COLA_bits_per_hash/2);
	cola->leaves[0].value = NULL;
	cola->leaves[elements].link = 0;
}


#pragma mark -


void COLA_recycle_nodes(COLA cola, unsigned nodeIndices[], unsigned count);

COLA_math_t COLA_enter(COLA cola) {
	COLA_math_t token = COLA_atomic_addm(&cola->leaves[0].references, 1);
	COLA_assert(token & 0x03FF, "COLA_enter %08X", token);
	return token;
}

void COLA_touch(COLA cola) {
	COLA_math_t references, old = cola->leaves[0].references;
	COLA_math_t enter = old;
	
	/*
		references >>  0 & 0x03FF : normal referencse
		references >> 10 & 0x03FF : owning references
		references >> 20 & 0x0FFF : change token
		
		methods begin with normal reference status and have no responsibilities
		when leaving
		
		methods that remove nodes or are interrupted by another method that
		removes nodes beome owning references and must recycle the removed nodes
		when exiting if they are the last owning reference
		
		touch increments the change token and makes all current references into
		owning references
		
		there can be a maximum of 2^10 conncurrent callers of enter and leave
		
		if the change token wraps as the final owning reference is released
		then nodes will not be recycled until after another call to touch
		
		may early out if interrupted by another call to touch
	*/
	
	do {
		references = (old & ~0x000FFC00) + ((old & 0x03FF) << 10) + (1 << 20);
		if ( COLA_atomic_casm(&cola->leaves[0].references, old, references) ) { break; }
		old = cola->leaves[0].references;
	} while ( (old ^ enter) >> 20 == 0 );	//	zero until touched
}

void COLA_leave(COLA cola, COLA_math_t enter) {
	COLA_bits_t recycle = cola->recycle;
	COLA_math_t after, prior = COLA_atomic_addm(&cola->leaves[0].references, -1);
	COLA_mask_t bits;
	unsigned nodes[16];
	unsigned order, index, count = 0;
	
	if ( (prior ^ enter) >> 20 == 0 ) { return; }
	
	after = COLA_atomic_addm(&cola->leaves[0].references, -(1 << 10));
	
	if ( (after & 0x000FFC00) != 0 ) { return; }
	
	for ( order = 0 ; order < COLA_bits_count ; ++order ) {
		bits = recycle.bits[order];
		if ( !bits ) { continue; }
		
		recycle.bits[order] = COLA_atomic_andm(&cola->recycle.bits[order], ~bits) & bits;
	}
	
	for ( order = 0 ; order < COLA_bits_count ; ++order ) {
		bits = recycle.bits[order];
		if ( !bits ) { continue; }
		
		for ( index = 0 ; index < 32 ; ++index ) {
			if ( !((bits >> index) & 1) ) { continue; }
			
			nodes[count++] = index + (order * 32);
			
			if ( count == 16 ) { COLA_recycle_nodes(cola, nodes, count); count = 0; }
		}
	}
	
	if ( count > 0 ) {
		COLA_recycle_nodes(cola, nodes, count);
	}
}


#pragma mark -


/*
	nodes have five states
	
	available nodes are linked in the node pool
	
	acquired nodes have been removed from the node pool and may be modified
	before being made visible from the root
	
	referenced nodes are visible from the root and must always be valid
	
	released nodes are no longer visible from the root but may still be
	visible to some concurrent operation and must always be valid
	
	recycling a node after release makes it available in the node pool again
	
	an acquired node that was never referenced may be recycled immediately
*/

#define COLA_AcquireInterrupted COLA_entry_limit
#define COLA_AcquireFailed 0

#define COLA_pool_dead_node COLA_node_entry_at_index(COLA_node_index_root,1)
#define COLA_pool_node_mask COLA_entry_mask
#define COLA_pool_node_salt COLA_pool_dead_node
#define COLA_pool_salt_mask ~(COLA_pool_increment-1)
#define COLA_pool_increment (1<<(COLA_bits_per_node - (COLA_bits_per_entry * (COLA_entries_per_node - 2))))

unsigned COLA_acquire_node(COLA cola) {
	COLA_node_t volatile *nodes = cola->nodes;
	COLA_node_t volatile *pool = nodes + 0;
	COLA_node_t node, next, head, salt, peek;
	
	head = *pool;
	node = head & COLA_pool_node_mask;
	COLA_assert(node < COLA_node_count, "COLA_acquire_node " COLA_NODE_FMT, head);
	if ( !node ) { COLA_warning("COLA_acquire_node pool depleted " COLA_NODE_FMT, head); return COLA_AcquireFailed; }
	
	peek = nodes[node];
	next = peek & COLA_pool_node_mask;
	salt = (head & COLA_pool_salt_mask) + COLA_pool_node_salt;
	if ( !COLA_atomic_casn(pool, head, next + salt) ) { return COLA_AcquireInterrupted; }
	
	COLA_assert(next < COLA_node_count, "COLA_acquire_node " COLA_NODE_FMT, peek);
	COLA_assert(!COLA_test_bit(cola->recycle.bits, node), "COLA_acquire_node " COLA_ENTRY_FMT, node);
	nodes[node] = salt;
	
	return (unsigned)node;
}

void COLA_recycle_nodes(COLA cola, unsigned nodeIndices[], unsigned count) {
	COLA_assert(count > 0 && count < COLA_node_count, "COLA_recycle_nodes %u", count);
	COLA_node_t volatile *nodes = cola->nodes;
	COLA_node_t volatile *pool = nodes + 0;
	COLA_node_t next, head, salt;
	unsigned index, nodeIndex = 0;
	
	for ( index = 0 ; index < count ; ++index ) {
		nodeIndex = nodeIndices[index];
		COLA_assert(nodeIndex > 0 && nodeIndex < COLA_node_count, "COLA_recycle_nodes %02X", nodeIndex);
		COLA_assert(!COLA_test_bit(cola->recycle.bits, nodeIndex), "COLA_recycle_nodes %02X", nodeIndex);
		nodes[nodeIndex] = COLA_pool_dead_node;
	}
	
	do {
		head = *pool;
		next = head & COLA_pool_node_mask;
		salt = (head & COLA_pool_salt_mask) + COLA_pool_node_salt + COLA_pool_increment;
		COLA_assert(next < COLA_node_count, "COLA_recycle_nodes " COLA_NODE_FMT , head);
		
		for ( index = 0 ; index < count ; ++index ) {
			nodeIndex = nodeIndices[index];
			nodes[nodeIndex] = next + salt;
			next = nodeIndex;
		}
	} while ( !COLA_atomic_casn(pool, head, nodeIndex + salt) );
}

void COLA_release_nodes(COLA cola, unsigned nodeIndices[], unsigned count) {
	COLA_assert(count > 0 && count < COLA_node_count, "COLA_release_nodes %u", count);
	COLA_bits_t recycle = {0};
	
	for ( unsigned index = 0 ; index < count ; ++index ) {
		unsigned nodeIndex = nodeIndices[index];
		COLA_assert(nodeIndex > 0 && nodeIndex < COLA_node_count, "COLA_release_nodes %02X", nodeIndex);
		COLA_assert(!COLA_test_bit(cola->recycle.bits, nodeIndex), "COLA_release_nodes %02X", nodeIndex);
		COLA_set_bit(recycle.bits, nodeIndex);
	}
	
	// touch between removing nodes and marking nodes for recycling
	COLA_touch(cola);
	
	for ( unsigned order = 0 ; order < COLA_bits_count ; ++order ) {
		COLA_mask_t bits = recycle.bits[order];
		
		if ( bits ) { COLA_atomic_orm(&cola->recycle.bits[order], bits); }
	}
}


#pragma mark -


unsigned COLA_acquire_leaf(COLA cola) {
	COLA_leaf_t volatile *leaves = cola->leaves;
	COLA_link_t volatile *pool = &leaves[COLA_leaf_index_pool].link;
	COLA_link_t link, next, head, salt, peek;
	
	head = *pool;
	link = head & COLA_entry_mask;
	if ( !link ) { COLA_warning("COLA_acquire_leaf pool depleted " COLA_NODE_FMT, head); return COLA_AcquireFailed; }
	COLA_assert(link <= COLA_leaf_count, "COLA_acquire_leaf head " COLA_NODE_FMT, head);
	
	salt = head & ~COLA_entry_mask;
	peek = leaves[link].link;
	next = peek & COLA_entry_mask;
	COLA_assert(link != next, "COLA_acquire_leaf " COLA_ENTRY_FMT " next " COLA_NODE_FMT, link, peek);
	if ( !COLA_atomic_casn(pool, head, next + salt + COLA_entry_limit) ) { return COLA_AcquireInterrupted; }
	COLA_assert(next <= COLA_leaf_count, "COLA_acquire_leaf next " COLA_NODE_FMT, peek);
	
	leaves[link].link = 0;
	COLA_atomic_addm(&leaves[link].references, COLA_node_acquired - COLA_node_recycled);
	
	return (unsigned)link;
}

void COLA_recycle_leaf(COLA cola, unsigned nodeIndex) {
	COLA_leaf_t volatile *leaves = cola->leaves;
	COLA_link_t volatile *pool = &leaves[COLA_leaf_index_pool].link;
	COLA_link_t next, head, salt;
	
	if ( !COLA_atomic_casm(&leaves[nodeIndex].references, 0, COLA_node_recycled) ) { return; }
	
	leaves[nodeIndex].hash = 0;
	leaves[nodeIndex].value = NULL;
	
	do {
		head = *pool;
		next = head & COLA_entry_mask;
		salt = (head & ~COLA_entry_mask) + COLA_entry_limit;
		
		COLA_assert(next != nodeIndex, "COLA_recycle_leaf %02X next " COLA_NODE_FMT, nodeIndex, head);
		COLA_assert(next <= COLA_leaf_count, "COLA_recycle_leaf %02X next " COLA_NODE_FMT, nodeIndex, head);
		leaves[nodeIndex].link = next;
	} while ( !COLA_atomic_casn(pool, head, nodeIndex + salt) );
}

void COLA_dispose_leaf(COLA cola, unsigned nodeIndex) {
	COLA_assert(nodeIndex > 0 && nodeIndex <= COLA_leaf_count, "COLA_dispose_leaf %02X", nodeIndex);
	if ( 0 == COLA_atomic_addm(&cola->leaves[nodeIndex].references, -COLA_node_acquired) ) {
		COLA_recycle_leaf(cola, nodeIndex);
	}
}

void COLA_release_leaf(COLA cola, unsigned nodeIndex) {
	COLA_assert(nodeIndex > 0 && nodeIndex <= COLA_leaf_count, "COLA_release_leaf %02X", nodeIndex);
	if ( 0 == COLA_atomic_addm(&cola->leaves[nodeIndex].references, -COLA_node_reserved) ) {
		COLA_recycle_leaf(cola, nodeIndex);
	}
}

unsigned COLA_reserve_leaf(COLA cola, unsigned nodeIndex) {
	COLA_assert(nodeIndex > 0 && nodeIndex <= COLA_leaf_count, "COLA_reserve_leaf %02X", nodeIndex);
	if ( COLA_node_acquired & COLA_atomic_addm(&cola->leaves[nodeIndex].references, COLA_node_reserved) ) {
		return 1;
	} else {
		COLA_release_leaf(cola, nodeIndex);
		return 0;
	}
}


#pragma mark -


#define COLA_node_not_entombed 1

#define COLA_entry_low_bits (COLA_entry_ones*((1<<(COLA_bits_per_entry-1))-1))
#define COLA_entry_high_bit (COLA_entry_ones*(1<<(COLA_bits_per_entry-1)))
#define COLA_node_branch_mask(_node) ((COLA_entry_low_bits+((_node)&COLA_entry_low_bits)|(_node))&COLA_entry_high_bit)

unsigned COLA_node_entombed_entry(COLA_node_t node) {
	if ( !node ) { return 0; }
	
	COLA_node_t branch_mask = COLA_node_branch_mask(node);
	if ( branch_mask & (branch_mask - 1) ) { return COLA_node_not_entombed; }
	
	unsigned index = COLA_ctzn(branch_mask) / COLA_bits_per_entry;
	COLA_node_t lone_branch = COLA_entry_for_index(node, index);
	if ( COLA_entry_is_node(lone_branch) ) { return COLA_node_not_entombed; }
	
	return (unsigned)lone_branch;
}

unsigned COLA_reclaim_tomb(COLA cola, unsigned nodeIndex, COLA_node_t node, unsigned entryIndex, unsigned entry, unsigned recycleIndex) {
	COLA_node_t changeNode = COLA_node_with_entry_at_index(node, entry, entryIndex);
	
	COLA_assert(entry == 0 || entry >= COLA_node_count, "COLA_reclaim_tomb %u", entry);
	COLA_assert(entryIndex < COLA_entries_per_node, "COLA_reclaim_tomb %u", entryIndex);
	COLA_assert(nodeIndex > 0 && nodeIndex < COLA_node_count, "COLA_reclaim_tomb %u", nodeIndex);
	COLA_assert(recycleIndex > 0 && recycleIndex < COLA_node_count, "COLA_reclaim_tomb %u", recycleIndex);
	
	unsigned recycled = COLA_test_bit(cola->recycle.bits, nodeIndex);
	if ( COLA_atomic_casn(cola->nodes + nodeIndex, node, changeNode) ) {
		COLA_trace("COLA_reclaim_tomb [%u] node[%02X] " COLA_NODE_FMT " ==> " COLA_NODE_FMT, entryIndex, nodeIndex, node, changeNode);
		COLA_assert(!recycled, "COLA_reclaim_tomb %02X " COLA_NODE_FMT " ==> " COLA_NODE_FMT, nodeIndex, node, changeNode);
		COLA_release_nodes(cola, &recycleIndex, 1);
		return 1;
	}
	
	return 0;
}


#pragma mark -


COLA_data_t COLA_search(COLA cola, COLA_hash_t hash) {
	COLA_data_t result = NULL;
	COLA_math_t enter = COLA_enter(cola);
	COLA_node_t volatile *nodes = cola->nodes;
	COLA_leaf_t volatile *leaves = cola->leaves;
	COLA_node_t next, node;
	
	unsigned entry, entryIndex, index, tomb;
	unsigned nodeIndex = COLA_node_index_root;
	node = nodes[nodeIndex];
	
	for ( unsigned depth = 0 ; depth < COLA_maximum_depth ; ++depth ) {
		entryIndex = COLA_entry_index_for_depth(hash, depth);
		entry = COLA_entry_for_index(node, entryIndex);
		
		if ( COLA_entry_is_null(entry) ) {
			break;
		} else if ( COLA_entry_is_node(entry) ) {
			next = nodes[entry];
			tomb = COLA_node_entombed_entry(next);
			
			if ( COLA_node_not_entombed == tomb ) {
				nodeIndex = entry;
				node = next;
				continue;
			}
			
			COLA_reclaim_tomb(cola, nodeIndex, node, entryIndex, tomb, entry);
			
			if ( COLA_entry_is_null(tomb) ) {
				break;
			}
			
			index = COLA_entry_leaf_index(tomb);
		} else {
			index = COLA_entry_leaf_index(entry);
		}
		
		if ( !COLA_reserve_leaf(cola, index) ) {
			break;
		}
		
		if ( leaves[index].hash == hash ) {
			result = leaves[index].value;
		}
		
		COLA_release_leaf(cola, index);
		break;
	}
	
	COLA_leave(cola, enter);
	
	return result;
}


#pragma mark -


COLA_data_t COLA_remove(COLA cola, COLA_hash_t hash) {
	COLA_data_t result = NULL;
	COLA_math_t enter = COLA_enter(cola);
	COLA_node_t volatile *nodes = cola->nodes;
	COLA_leaf_t volatile *leaves = cola->leaves;
	COLA_node_t next, node, changeNode;
	unsigned entry, entryIndex, index, tomb;
	unsigned retry, count, ascend, nodeIndex;
	unsigned recycleNodes[COLA_maximum_depth];
	unsigned nodeIndices[COLA_maximum_depth];
	
	do {
		retry = 0;
		ascend = 0;
		nodeIndex = COLA_node_index_root;
		node = nodes[nodeIndex];
		
		for ( unsigned depth = 0 ; depth < COLA_maximum_depth ; ++depth ) {
			entryIndex = COLA_entry_index_for_depth(hash, depth);
			entry = COLA_entry_for_index(node, entryIndex);
			
			nodeIndices[ascend = depth] = nodeIndex;
			
			if ( COLA_entry_is_null(entry) ) {
				break;
			} else if ( COLA_entry_is_node(entry) ) {
				next = nodes[entry];
				tomb = COLA_node_entombed_entry(next);
				
				if ( COLA_node_not_entombed == tomb ) {
					nodeIndex = entry;
					node = next;
					continue;
				}
				
				COLA_reclaim_tomb(cola, nodeIndex, node, entryIndex, tomb, entry);
				
				if ( tomb ) {
					retry = 1;
				}
				
				break;
			} else {
				index = COLA_entry_leaf_index(entry);
			}
			
			if ( !COLA_reserve_leaf(cola, index) ) {
				break;
			}
			
			if ( leaves[index].hash == hash ) {
				changeNode = COLA_node_with_entry_at_index(node, 0, entryIndex);
				
				unsigned recycled = COLA_test_bit(cola->recycle.bits, nodeIndex);
				if ( COLA_atomic_casn(nodes + nodeIndex, node, changeNode) ) {
					COLA_trace("COLA_remove %08X @%2u[%u] node[%02X] " COLA_NODE_FMT " ==> " COLA_NODE_FMT, hash, depth, entryIndex, nodeIndex, node, changeNode);
					COLA_assert(!recycled, "COLA_remove %02X " COLA_NODE_FMT " ==> " COLA_NODE_FMT, nodeIndex, node, changeNode);
					COLA_atomic_addh(&leaves[COLA_leaf_index_pool].hash, -1);
					result = leaves[index].value;
					COLA_dispose_leaf(cola, index);
				} else {
					retry = 1;
				}
			}
			
			COLA_release_leaf(cola, index);
			break;
		}
	} while ( retry );
	
	for ( count = 0 ; ascend > 0 ; --ascend ) {
		entry = nodeIndices[ascend];
		next = nodes[entry];
		tomb = COLA_node_entombed_entry(next);
		if ( COLA_node_not_entombed == tomb ) { break; }
		
		nodeIndex = nodeIndices[ascend - 1];
		node = nodes[nodeIndex];
		entryIndex = COLA_entry_index_for_depth(hash, ascend - 1);
		if ( COLA_entry_for_index(node, entryIndex) != entry ) { break; }
		
		changeNode = COLA_node_with_entry_at_index(node, tomb, entryIndex);
		if ( !COLA_atomic_casn(nodes + nodeIndex, node, changeNode) ) { break; }
		COLA_trace("COLA_ascend %08X @%2u[%u] node[%02X] " COLA_NODE_FMT " ==> " COLA_NODE_FMT, hash, ascend - 1, entryIndex, nodeIndex, node, changeNode);
		
		recycleNodes[count++] = entry;
	}
	
	if ( count > 0 ) {
		COLA_release_nodes(cola, recycleNodes, count);
	}
	
	COLA_leave(cola, enter);
	
	return result;
}

COLA_data_t COLA_assign(COLA cola, COLA_hash_t hash, COLA_data_t value, unsigned options, unsigned *outStatus) {
	enum { Descend = 0, Insert = 1, Replace = 2, Convert = 3 };
	
	COLA_data_t result = NULL;
	COLA_math_t enter = COLA_enter(cola);
	COLA_node_t volatile *nodes = cola->nodes;
	COLA_leaf_t volatile *leaves = cola->leaves;
	COLA_node_t next, node, changeNode = 0;
	COLA_hash_t otherHash = 0;
	unsigned entry, entryIndex, index, tomb, action, status;
	unsigned retry, ascend, nodeIndex, releaseIndex;
	unsigned assignLeaf, disposeLeaf = 0;
	unsigned nodesAssigned, nodesAcquired = 0;
	unsigned recycleNodes[COLA_maximum_depth];
	
	do {
		retry = 0;
		ascend = 0;
		status = 0;
		releaseIndex = 0;
		nodesAssigned = 0;
		nodeIndex = COLA_node_index_root;
		node = nodes[nodeIndex];
		
		for ( unsigned depth = 0 ; depth < COLA_maximum_depth ; ++depth ) {
			entryIndex = COLA_entry_index_for_depth(hash, depth);
			entry = COLA_entry_for_index(node, entryIndex);
			
			if ( COLA_entry_is_null(entry) ) {
				action = Insert;
				index = 0;
			} else if ( COLA_entry_is_node(entry) ) {
				next = nodes[entry];
				tomb = COLA_node_entombed_entry(next);
				
				if ( COLA_node_not_entombed == tomb ) {
					nodeIndex = entry;
					node = next;
					continue;
				}
				
				COLA_reclaim_tomb(cola, nodeIndex, node, entryIndex, tomb, entry);
				
				retry = 1;
				break;
			} else {
				index = COLA_entry_leaf_index(entry);
				
				if ( !COLA_reserve_leaf(cola, index) ) {
					retry = 1; break;
				}
				
				releaseIndex = index;
				otherHash = leaves[index].hash;
				action = ( otherHash == hash ) ? Replace : Convert;
			}
			
			if ( Replace == action ) {
				result = leaves[index].value;
				
				if ( options & COLA_AssignSum ) {
					result = COLA_atomic_addp(&leaves[index].value, value);
				} else if ( options & COLA_AssignNoReplace ) {
					status = COLA_AssignExistingEntry;
				} else if ( result == value ) {
					break;
				} else if ( COLA_atomic_casp(&leaves[index].value, result, value) ) {
					COLA_trace("COLA_assign %08X @%2u[%u] leaf[%2u] %p ==> %p", hash, depth, entryIndex, index, result, value);
					//	own value
				} else {
					retry = 1; break;
				}
				
				break;
			}
			
			if ( options & COLA_AssignOnlyReplace ) {
				status = COLA_AssignMissingEntry;
				break;
			}
			
			if ( disposeLeaf ) {
				assignLeaf = disposeLeaf;
			} else {
				assignLeaf = COLA_acquire_leaf(cola);
				
				if ( COLA_AcquireInterrupted == assignLeaf ) { retry = 1; break; }
				else if ( COLA_AcquireFailed == assignLeaf ) { status = COLA_AssignNoLeaves; break; }
				else { disposeLeaf = assignLeaf; }
			}
			
			if ( Insert == action ) {
				changeNode = COLA_node_with_entry_at_index(node, COLA_leaf_index_entry(assignLeaf), entryIndex);
			} else {
				unsigned acquired, previousEntry = 0, previousIndex = 0;
				
				for ( unsigned reach = depth + 1 ; reach < COLA_maximum_depth ; ++reach ) {
					if ( nodesAssigned < nodesAcquired ) {
						acquired = recycleNodes[nodesAssigned++];
					} else {
						acquired = COLA_acquire_node(cola);
						
						if ( COLA_AcquireInterrupted == acquired ) { retry = 1; break; }
						else if ( COLA_AcquireFailed == acquired ) { status = COLA_AssignNoNodes; break; }
						else { recycleNodes[nodesAcquired++] = acquired; nodesAssigned = nodesAcquired; }
					}
					
					if ( previousEntry ) {
						nodes[previousEntry] = COLA_node_entry_at_index(acquired, previousIndex);
					} else {
						changeNode = COLA_node_with_entry_at_index(node, acquired, entryIndex);
					}
					
					unsigned entryIndexA = COLA_entry_index_for_depth(otherHash, reach);
					unsigned entryIndexB = COLA_entry_index_for_depth(hash, reach);
					
					if ( entryIndexA != entryIndexB ) {
						nodes[acquired] = COLA_node_entry_at_index(COLA_leaf_index_entry(index), entryIndexA)
							| COLA_node_entry_at_index(COLA_leaf_index_entry(assignLeaf), entryIndexB);
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
			
			leaves[assignLeaf].hash = hash;
			leaves[assignLeaf].value = value;
			
			unsigned recycled = COLA_test_bit(cola->recycle.bits, nodeIndex);
			if ( COLA_atomic_casn(nodes + nodeIndex, node, changeNode) ) {
				COLA_trace("COLA_insert %08X @%2u[%u] node[%02X] " COLA_NODE_FMT " ==> " COLA_NODE_FMT " leaf[%2u] %p", hash, depth, entryIndex, nodeIndex, node, changeNode, assignLeaf, value);
				COLA_assert(!recycled, "COLA_assign %02X " COLA_NODE_FMT " ==> " COLA_NODE_FMT, nodeIndex, node, changeNode);
				COLA_atomic_addh(&leaves[COLA_leaf_index_pool].hash, 1);
				//	own value
				disposeLeaf = 0;
			} else {
				retry = 1;
			}
			
			break;
		}
		
		if ( releaseIndex ) { COLA_release_leaf(cola, releaseIndex); }
	} while ( retry );
	
	if ( disposeLeaf ) {
		COLA_dispose_leaf(cola, disposeLeaf);
	}
	
	if ( nodesAssigned < nodesAcquired ) {
		COLA_recycle_nodes(cola, recycleNodes + nodesAssigned, nodesAcquired - nodesAssigned);
	}
	
	COLA_leave(cola, enter);
	
	if ( outStatus ) {
		*outStatus = status;
	}
	
	return result;
}


#pragma mark -


unsigned COLA_values_recurse(COLA cola, unsigned depth, unsigned nodeIndex, unsigned used, COLA_hash_t hashes[], COLA_data_t values[], unsigned capacity, unsigned reverse) {
	COLA_assert(depth < COLA_maximum_depth, "COLA_values %u", depth);
	COLA_assert(0 < nodeIndex && nodeIndex < COLA_node_count, "COLA_values %u", nodeIndex);
	
	unsigned result = used;
	COLA_node_t volatile *nodes = cola->nodes;
	COLA_leaf_t volatile *leaves = cola->leaves;
	COLA_node_t next, node = nodes[nodeIndex];
	
	for ( unsigned order = 0 ; order < COLA_entries_per_node && result < capacity ; ++order ) {
		unsigned entryIndex = reverse ? ( COLA_entries_per_node - 1 - order ) : order;
		unsigned index, entry = COLA_entry_for_index(node, entryIndex);
		unsigned tomb;
		
		if ( COLA_entry_is_null(entry) ) {
			continue;
		} else if ( COLA_entry_is_node(entry) ) {
			next = nodes[entry];
			tomb = COLA_node_entombed_entry(next);
			
			if ( COLA_node_not_entombed == tomb ) {
				result = COLA_values_recurse(cola, depth + 1, entry, result, hashes, values, capacity, reverse);
				continue;
			}
			
			COLA_reclaim_tomb(cola, nodeIndex, node, entryIndex, tomb, entry);
			
			if ( COLA_entry_is_null(tomb) ) {
				continue;
			}
			
			index = COLA_entry_leaf_index(tomb);
		} else {
			index = COLA_entry_leaf_index(entry);
		}
		
		if ( !COLA_reserve_leaf(cola, index) ) {
			continue;
		}
		
		if ( NULL != hashes ) { hashes[result] = leaves[index].hash; }
		if ( NULL != values ) { values[result] = leaves[index].value; }
		result += 1;
		
		COLA_release_leaf(cola, index);
	}
	
	return result;
}

unsigned COLA_values(COLA cola, COLA_hash_t hashes[], COLA_data_t values[], unsigned capacity, unsigned reverse) {
	unsigned result = 0;
	
	COLA_math_t enter = COLA_enter(cola);
	
	result = COLA_values_recurse(cola, 0, COLA_node_index_root, 0, hashes, values, capacity, reverse);
	
	COLA_leave(cola, enter);
	
	return result;
}


#pragma mark -


unsigned COLA_enumerate(COLA cola, COLA_enumerator enumerator, void *context) {
	unsigned result = 0;
	COLA_hash_t hashes[COLA_leaf_count];
	COLA_data_t values[COLA_leaf_count];
	unsigned index, count = COLA_values(cola, hashes, values, COLA_leaf_count, 0);
	
	for ( index = 0 ; index < count && !result ; ++index ) {
		result = enumerator(hashes[index], values[index], context);
	}
	
	return result;
}

unsigned COLA_capacity(COLA cola) {
	return cola->leaves[COLA_leaf_index_pool].hash >> (COLA_bits_per_hash/2);
}

unsigned COLA_count(COLA cola) {
	return cola->leaves[COLA_leaf_index_pool].hash & ((1 << (COLA_bits_per_hash/2)) - 1);
	//return COLA_values(cola, NULL, NULL, COLA_leaf_count, 0);
}

unsigned COLA_is_empty(COLA cola) {
	return (cola->leaves[COLA_leaf_index_pool].hash & ((1 << (COLA_bits_per_hash/2)) - 1)) == 0;
	//return COLA_values(cola, NULL, NULL, 1, 0) == 0;
}

unsigned COLA_remove_all(COLA cola) {
	COLA_hash_t hashes[COLA_leaf_count];
	unsigned index, count = COLA_values(cola, hashes, NULL, COLA_leaf_count, 0);
	
	for ( index = 0 ; index < count ; ++index ) {
		COLA_remove(cola, hashes[index]);
	}
	
	return count;
}


#pragma mark -


unsigned COLA_verify_recurse(COLA cola, unsigned nodeIndex, uint32_t seen[]) {
	unsigned result = 1;
	COLA_node_t node = cola->nodes[nodeIndex];
	
	for ( unsigned order = 0 ; order < COLA_entries_per_node ; ++order ) {
		unsigned entry = COLA_entry_for_index(node, order);
		
		if ( COLA_entry_is_null(entry) ) { continue; }
		else if ( COLA_entry_is_node(entry) ) { result += COLA_verify_recurse(cola, entry, seen); }
		else { result += 1 << 16; }
		
		seen[entry/32] |= 1 << (entry&31);
 	}
	
	return result;
}

unsigned COLA_verify(COLA cola) {
	unsigned unverified = 0;
	unsigned used_leaves = 0;
	
	COLA_node_t *nodes = cola->nodes;
	COLA_leaf_t *leaves = cola->leaves;
	COLA_leaf_t *pool = leaves + COLA_leaf_index_pool;
	
	uint32_t nodes_recycled[COLA_entry_limit/32] = {0};
	uint32_t nodes_acquired[COLA_entry_limit/32] = {0};
	
	COLA_verify_recurse(cola, COLA_node_index_root, nodes_acquired);
	
	COLA_node_t node = nodes[COLA_node_index_pool] & COLA_pool_node_mask;
	while ( node > 0 && node <= COLA_node_count ) {
		nodes_recycled[node/32] |= (1<<(node&31));
		node = nodes[node] & COLA_pool_node_mask;
	}
	
	COLA_link_t link = leaves[COLA_leaf_index_pool].link & COLA_entry_mask;
	while ( link > 0 && link <= COLA_leaf_count ) {
		COLA_link_t entry = COLA_leaf_index_entry(link);
		nodes_recycled[entry/32] |= (1<<(entry&31));
		link = leaves[link].link & COLA_entry_mask;
	}
	
	for ( unsigned entry = 1 ; entry <= COLA_node_index_last ; ++entry ) {
		unsigned is_acquired = (nodes_acquired[entry/32] >> (entry&31)) & 1;
		unsigned is_recycled = (nodes_recycled[entry/32] >> (entry&31)) & 1;
		unsigned is_released = COLA_test_bit(cola->recycle.bits, entry) ? 1 : 0;
		
		if ( is_recycled + is_released + is_acquired == 1 ) { continue; }
		
		COLA_assert(0, "COLA_verify %c%c%c branch node %02X " COLA_NODE_FMT,
			is_acquired ? 'a' : '-', is_released ? 'r' : '-', is_recycled ? 'p' : '-',
			entry, nodes[entry]);
		
		unverified += 1;
	}
	
	for ( unsigned leaf = 1 ; leaf <= COLA_leaf_count ; ++leaf ) {
		COLA_link_t entry = COLA_leaf_index_entry(leaf);
		COLA_math_t references = leaves[leaf].references;
		unsigned is_acquired = (nodes_acquired[entry/32] >> (entry&31)) & 1;
		unsigned is_recycled = (nodes_recycled[entry/32] >> (entry&31)) & 1;
		unsigned is_released = (references & (COLA_node_acquired | COLA_node_recycled)) ? 0 : 1;
		
		if ( is_acquired ) used_leaves += 1;
		if ( is_recycled + is_released + is_acquired == 1 ) { continue; }
		
		COLA_assert(0, "COLA_verify %c%c%c leaf node %02X L" COLA_NODE_FMT " R%08X H%08X",
			is_acquired ? 'a' : '-', is_released ? 'r' : '-', is_recycled ? 'p' : '-',
			leaf, leaves[leaf].link, references, leaves[leaf].hash);
		
		unverified += 1;
	}
	
	COLA_assert((pool->references & 0x0FFFFF) == 0, "COLA_verify references %08X", pool->references);
	COLA_assert(used_leaves == (pool->hash & 0x0FFF), "COLA_verify leaves %u != %u", used_leaves, pool->hash & 0x0FFF);
	
	return unverified;
}
