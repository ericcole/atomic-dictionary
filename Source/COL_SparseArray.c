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

//	32/4, 64/6, 64/7, 64/8
#define COLA_entries_per_node 4
#define COLA_bits_per_node 32

#define COLA_bits_per_hash 32
#define COLA_bits_per_mask 32
#define COLA_bits_per_math 32

#define COLA_atomic_casp ECC_atomic_compare_and_swap_ptr
#define COLA_atomic_addp ECC_atomic_add_ptr

#if COLA_bits_per_node == 64 && LONG_MAX >= 9223372036854775807L
typedef uint64_t COLA_casn_t;
#define COLA_atomic_casn ECC_atomic_compare_and_swap_64
#define COLA_ctzn ECC_count_trailing_zeros_64
#endif

#if COLA_bits_per_node == 32
typedef uint32_t COLA_casn_t;
#define COLA_atomic_casn ECC_atomic_compare_and_swap_32
#define COLA_ctzn ECC_count_trailing_zeros_32
#endif

#if COLA_bits_per_hash == 32
typedef uint32_t COLA_hash_t;
#define COLA_atomic_addh ECC_atomic_add_32
#endif

#if COLA_bits_per_mask == 32
typedef uint32_t COLA_mask_t;
#define COLA_atomic_andm ECC_atomic_and_32
#define COLA_atomic_orm  ECC_atomic_or_32

#define COLA_test_bit(_bits,_bit) (_bits[(_bit)/32] & (1<<((_bit)&31)))
#define COLA_set_bit(_bits,_bit) (_bits[(_bit)/32] |= (1<<((_bit)&31)))
#endif

#if COLA_bits_per_math == 32
typedef int32_t COLA_math_t;
#define COLA_atomic_casm ECC_atomic_compare_and_swap_32
#define COLA_atomic_addm ECC_atomic_add_32
#endif

#if COLA_bits_per_node == 32 && COLA_entries_per_node == 4
#define COLA_bits_per_level 2
#define COLA_bits_per_entry 8
#define COLA_maximum_depth 16	//	4^16 >= 2^32
#define COLA_entry_ones 0x01010101U

#define COLA_entry_index_for_depth(_hash, _depth) (((_hash) >> ((COLA_bits_per_hash - COLA_bits_per_level) - ((_depth) * COLA_bits_per_level))) & 3)
#endif

#if COLA_bits_per_node == 64 && COLA_entries_per_node == 8
#define COLA_bits_per_level 3
#define COLA_bits_per_entry 8
#define COLA_maximum_depth 11	//	8^11 >= 2^32
#define COLA_entry_ones 0x0101010101010101ULL

#define COLA_entry_index_for_depth(_hash, _depth) (((_hash) >> ((COLA_bits_per_hash - COLA_bits_per_level) - ((_depth) * COLA_bits_per_level))) & 7)
#endif

#if COLA_bits_per_node == 64 && COLA_entries_per_node == 7
#define COLA_bits_per_entry 9
#define COLA_maximum_depth 12	//	7^12 >= 2^32
#define COLA_entry_ones 0x0040201008040201ULL

const COLA_casn_t COLA__divisor_at_depth[] = {7*7*7*7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7*7,7*7*7*7*7*7*7*7,7*7*7*7*7*7*7,7*7*7*7*7*7,7*7*7*7*7,7*7*7*7,7*7*7,7*7,7,0};
#define COLA_entry_index_for_depth(_hash, _depth) (((_hash)/COLA__divisor_at_depth[_depth])%COLA_entries_per_node)
#endif

#if COLA_bits_per_node == 64 && COLA_entries_per_node == 6
#define COLA_bits_per_entry 10
#define COLA_maximum_depth 13	//	6^13 >= 2^32
#define COLA_entry_ones 0x0004010040100401ULL

const COLA_casn_t COLA__divisor_at_depth[] = {6*6*6*6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6*6,6*6*6*6*6*6*6*6,6*6*6*6*6*6*6,6*6*6*6*6*6,6*6*6*6*6,6*6*6*6,6*6*6,6*6,6,0};
#define COLA_entry_index_for_depth(_hash, _depth) (((_hash)/COLA__divisor_at_depth[_depth])%COLA_entries_per_node)
#endif

#pragma mark -

#define COLA_entry_limit (1<<COLA_bits_per_entry)
#define COLA_entry_mask (COLA_entry_limit-1)
#define COLA_leaf_count (COLA_entry_limit/4)
#define COLA_node_count (COLA_entry_limit-COLA_leaf_count)
#define COLA_bits_count (COLA_node_count / COLA_bits_per_node)

#define COLA_entry_for_index(_node, _index) (((_node) >> ((_index) * COLA_bits_per_entry)) & COLA_entry_mask)
#define COLA_node_entry_at_index(_entry, _index) ((_entry) << ((_index) * COLA_bits_per_entry))
#define COLA_node_with_entry_at_index(_node, _entry, _index) (((_node) & ~COLA_node_entry_at_index(COLA_entry_mask,_index)) | COLA_node_entry_at_index(_entry,_index))

#define COLA_entry_is_null(_entry) !(_entry)
#define COLA_entry_is_node(_entry) ((_entry)<COLA_node_count)
#define COLA_entry_leaf_index(_entry) ((_entry)-(COLA_node_count-1))
#define COLA_leaf_index_entry(_entry) ((_entry)+(COLA_node_count-1))

#define COLA_node_index_root (COLA_node_count - 1)
#define COLA_node_index_last (COLA_node_count - 2)
#define COLA_node_index_pool 0
#define COLA_leaf_index_pool 0

#define COLA_node_acquired (1<<30)
#define COLA_node_recycled (1<<29)
#define COLA_node_reserved (1<<8)

#pragma mark -

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




#ifndef COLA_warning
#if 1
#define COLA_warning(f,...) printf("" f "\n",__VA_ARGS__)
#else
#define COLA_warning(f,...)
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
	unsigned nodes[16];
	unsigned order, index, count = 0;
	
	if ( (prior ^ enter) >> 20 == 0 ) { return; }
	
	after = COLA_atomic_addm(&cola->leaves[0].references, -(1 << 10));
	
	if ( (after & 0x000FFC00) != 0 ) { return; }
	
	for ( order = 0 ; order < COLA_bits_count ; ++order ) {
		COLA_mask_t bits = recycle.bits[order];
		
		if ( bits ) { recycle.bits[order] = COLA_atomic_andm(&cola->recycle.bits[order], ~bits) & bits; }
	}
	
	for ( order = 0 ; order < COLA_bits_count ; ++order ) {
		COLA_mask_t bits = recycle.bits[order];
		
		if ( bits ) {
			for ( index = 0 ; index < 32 ; ++index ) {
				if ( bits & (1 << index) ) {
					nodes[count++] = index + (order * 32);
					
					if ( count == 16 ) { COLA_recycle_nodes(cola, nodes, count); count = 0; }
				}
			}
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
	COLA_assert(node < COLA_node_count, "COLA_acquire_node %08X", head);
	if ( !node ) { COLA_warning("COLA_acquire_node pool depleted %08X", head); return COLA_AcquireFailed; }
	
	peek = nodes[node];
	next = peek & COLA_pool_node_mask;
	salt = (head & COLA_pool_salt_mask) + COLA_pool_node_salt;
	if ( !COLA_atomic_casn(pool, head, next + salt) ) { return COLA_AcquireInterrupted; }
	
	COLA_assert(next < COLA_node_count, "COLA_acquire_node %08X", peek);
	COLA_assert(!COLA_test_bit(cola->recycle.bits, node), "COLA_acquire_node %02X", node);
	nodes[node] = salt;
	return node;
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
		COLA_assert(next < COLA_node_count, "COLA_recycle_nodes %08X", head);
		
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
	if ( !link ) { COLA_warning("COLA_acquire_leaf pool depleted %08X", head); return COLA_AcquireFailed; }
	COLA_assert(link <= COLA_leaf_count, "COLA_acquire_leaf head %08X", head);
	
	salt = head & ~COLA_entry_mask;
	peek = leaves[link].link;
	next = peek & COLA_entry_mask;
	COLA_assert(link != next, "COLA_acquire_leaf %02X next %08X", link, peek);
	if ( !COLA_atomic_casn(pool, head, next + salt + COLA_entry_limit) ) { return COLA_AcquireInterrupted; }
	COLA_assert(next <= COLA_leaf_count, "COLA_acquire_leaf next %08X", peek);
	
	leaves[link].link = 0;
	COLA_atomic_addm(&leaves[link].references, COLA_node_acquired - COLA_node_recycled);
	
	return link;
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
		
		COLA_assert(next != nodeIndex, "COLA_recycle_leaf %02X next %08X", nodeIndex, head);
		COLA_assert(next <= COLA_leaf_count, "COLA_recycle_leaf %02X next %08X", nodeIndex, head);
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
	
	return lone_branch;
}

unsigned COLA_reclaim_tomb(COLA cola, unsigned nodeIndex, COLA_node_t node, unsigned entryIndex, unsigned entry, unsigned recycleIndex) {
	COLA_node_t changeNode = COLA_node_with_entry_at_index(node, entry, entryIndex);
	
	COLA_assert(entry == 0 || entry >= COLA_node_count, "COLA_reclaim_tomb %u", entry);
	COLA_assert(entryIndex < COLA_entries_per_node, "COLA_reclaim_tomb %u", entryIndex);
	COLA_assert(nodeIndex > 0 && nodeIndex < COLA_node_count, "COLA_reclaim_tomb %u", nodeIndex);
	COLA_assert(recycleIndex > 0 && recycleIndex < COLA_node_count, "COLA_reclaim_tomb %u", recycleIndex);
	
	unsigned recycled = COLA_test_bit(cola->recycle.bits, nodeIndex);
	if ( COLA_atomic_casn(cola->nodes + nodeIndex, node, changeNode) ) {
		COLA_assert(!recycled, "COLA_reclaim_tomb %02X %08X ==> %08X", nodeIndex, node, changeNode);
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
					COLA_assert(!recycled, "COLA_remove %02X %08X ==> %08X", nodeIndex, node, changeNode);
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
				COLA_assert(!recycled, "COLA_assign %02X %08X ==> %08X", nodeIndex, node, changeNode);
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
	COLA_assert(depth < COLA_maximum_depth, "FC4_values %u", depth);
	COLA_assert(0 < nodeIndex && nodeIndex < COLA_node_count, "FC4_values %u", nodeIndex);
	
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
	COLA_node_t *nodes = cola->nodes;
	COLA_leaf_t *leaves = cola->leaves;
	COLA_leaf_t *pool = leaves + COLA_leaf_index_pool;
	
	unsigned used_nodes, unused_nodes, unrecycled_nodes;
	unsigned used_leaves, unused_leaves;
	uint32_t seen[COLA_entry_limit/32] = {0};
	
	seen[COLA_node_index_pool/32] |= 1<<(COLA_node_index_pool&31);
	seen[COLA_node_index_root/32] |= 1<<(COLA_node_index_root&31);
	
	unrecycled_nodes = 0;
	for ( unsigned order = 0 ; order < COLA_bits_count ; ++order ) {
		COLA_mask_t bits = cola->recycle.bits[order];
		for ( unsigned bit = 0 ; bit < COLA_bits_per_mask ; ++bit ) {
			if ( bits & (1 << bit) ) unrecycled_nodes += 1;
		}
		seen[order] |= bits;
	}
	
	COLA_node_t node = nodes[COLA_node_index_pool] & 0x00FF;
	for ( unused_nodes = 0 ; node > 0 && node <= COLA_node_count ; ++unused_nodes ) {
		if ( !(seen[node/32] ^= (1<<(node&31))) ) break;
		node = nodes[node] & 0x00FF;
	}
	
	COLA_link_t link = leaves[COLA_leaf_index_pool].link & 0x00FF;
	for ( unused_leaves = 0 ; link > 0 && link <= COLA_leaf_count ; ++unused_leaves ) {
		if ( !(seen[(link+191)/32] ^= (1<<((link+191)&31))) ) break;
		link = leaves[link].link & 0x00FF;
	}
	
	unused_nodes += 1;	//	pool
	used_nodes = COLA_verify_recurse(cola, COLA_node_index_root, seen);
	used_leaves = used_nodes >> 16;
	used_nodes &= 0x0000FFFF;
	
	for ( unsigned entry = 1 ; entry < COLA_entry_limit ; ++entry ) {
		if ( seen[entry/32] & (1<<(entry&31)) ) { continue; }
		
		if ( COLA_entry_is_node(entry) ) { COLA_assert(0, "COLA_verify unseen node %02X %08X", entry, nodes[entry]); }
		else { COLA_assert(0, "COLA_verify unseen leaf %02X L%08X R%08X H%08X", entry-191, leaves[entry-191].link, leaves[entry-191].references, leaves[entry-191].hash); }
	}
	
	COLA_assert(used_nodes + unused_nodes + unrecycled_nodes == COLA_node_count, "COLA_verify nodes %u + %u + %u", used_nodes, unused_nodes, unrecycled_nodes);
	COLA_assert(used_leaves + unused_leaves == COLA_capacity(cola), "COLA_verify leaves %u + %u", used_leaves, unused_leaves);
	COLA_assert(used_leaves == (pool->hash & 0x0FFF), "COLA_verify leaves %u != %u", used_leaves, pool->hash & 0x0FFF);
	COLA_assert((pool->references & 0x0FFFFF) == 0, "COLA_verify references %08X", pool->references);
	
	return
		(used_leaves + unused_leaves == COLA_capacity(cola) ? 0 : 1) |
		((used_nodes + unused_nodes + unrecycled_nodes == COLA_node_count) ? 0 : 2) |
		(unrecycled_nodes == 0 ? 0 : 4) |
		0;
}
