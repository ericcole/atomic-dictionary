//
//  atomic.h
//  atomic
//
//  Created by Eric Cole on 7/7/16.
//  Copyright © 2016 Balance Software. All rights reserved.
//

#ifndef ECC_atomic_h
#define ECC_atomic_h

#if __APPLE__
#ifndef _OSATOMIC_H_
#include <libkern/OSAtomic.h>
#endif
#endif

#if _MSC_VER
#ifndef __INTRIN_H
#include <intrin.h>
#endif
#endif


/*
	atomic operations are performed in such a way that they will not be
	interrupted by other threads
	
	the compare and swap operation compares an expected value to the
	current value stored at a memory address and if they are equal then
	stores a new value to that address and return true otherwise no change
	is made
	
	the add operation stores the sum of the new and old values back to
	memory and returns the sum unlike other operations which return the
	original value
	
	the test and set, clear, flip operations test a single bit then modify
	the bit and return the original value of the bit
	
	the and, or, xor operations modify a word in memory and return the
	original value of the word
	
	these atomic operations always use the barrier or fence variants
	which flush instruction pipelines before executing
	
	all memory addresses should be natually aligned by size so the low few
	bits of the address are zero otherwise behavior may be undefined on
	some platforms
*/


#define ECC_interruptible_compare_and_swap(_address,_old,_new) (*(_address)==(_old)?*(_address)=(_new),1:0)
#define ECC_interruptible_add(_address,_add) (*(_address)+=(_add))
#define ECC_interruptible_test_and_set(_address,_bit) (((uint32_t *)(_address))[(_bit)/32]&(1<<((_bit)&31))?1:(((uint32_t *)(_address))[(_bit)/32]|=(1<<((_bit)&31)),0))
#define ECC_interruptible_test_and_clear(_address,_bit) (((uint32_t *)(_address))[(_bit)/32]&(1<<((_bit)&31))?(((uint32_t *)(_address))[(_bit)/32]^=(1<<((_bit)&31)),1):0)
#define ECC_interruptible_test_and_flip(_address,_bit) (((((uint32_t *)(_address))[(_bit)/32]^=(1<<((_bit)&31)))>>((_bit)&31))^1)


#ifdef _OSATOMIC_H_

#define ECC_atomic_compare_and_swap_ptr(_address, _old, _new) OSAtomicCompareAndSwapPtrBarrier(_old, _new, (void * volatile *)(_address))
#define ECC_atomic_compare_and_swap_32(_address, _old, _new)  OSAtomicCompareAndSwap32Barrier(_old, _new, (int32_t volatile *)(_address))
#define ECC_atomic_compare_and_swap_64(_address, _old, _new)  OSAtomicCompareAndSwap64Barrier(_old, _new, (int64_t volatile *)(_address))

#define ECC_atomic_add_ptr(_address,_add) ((void *)OSAtomicAdd64Barrier((intptr_t)_add,(int64_t volatile *)(_address)))
#define ECC_atomic_add_32(_address,_add) OSAtomicAdd32Barrier(_add,(int32_t volatile *)(_address))
#define ECC_atomic_add_64(_address,_add) OSAtomicAdd64Barrier(_add,(int64_t volatile *)(_address))

#define ECC_atomic_and_32(_address,_bits) OSAtomicAnd32OrigBarrier(_bits, (uint32_t volatile *)(_address))
#define ECC_atomic_or_32(_address,_bits)  OSAtomicOr32OrigBarrier(_bits, (uint32_t volatile *)(_address))
#define ECC_atomic_xor_32(_address,_bits) OSAtomicXor32OrigBarrier(_bits, (uint32_t volatile *)(_address))

#define ECC_atomic_test_and_set(_address,_bit)   (OSAtomicTestAndSetBarrier((_bit)^7,_address))
#define ECC_atomic_test_and_clear(_address,_bit) (OSAtomicTestAndClearBarrier((_bit)^7,_address))
#define ECC_atomic_test_and_flip(_address,_bit) ((OSAtomicXor32OrigBarrier(1<<((_bit)&31),((uint32_t volatile *)(_address))+(_bit)/32)>>((_bit)&31))&1)

#define ECC_atomic_barrier() OSMemoryBarrier()

#elif _MSC_VER

#define ECC_atomic_compare_and_swap_ptr(_address, _old, _new) (_InterlockedCompareExchangePointer((PVOID volatile *)(_address),_new,_old)==(_old))
#define ECC_atomic_compare_and_swap_32(_address, _old, _new)  (_InterlockedCompareExchange((LONG volatile *)(_address),_new,_old)==(_old))
#define ECC_atomic_compare_and_swap_64(_address, _old, _new)  (_InterlockedCompareExchange64((LONGLONG volatile *)(_address),_new,_old)==(_old))

#define ECC_atomic_add_ptr(_address,_add) ((void *)(_InterlockedExchangeAdd64((LONGLONG volatile *)(_address),(LONGLONG)_add)+(_add)))
#define ECC_atomic_add_32(_address,_add) (_InterlockedExchangeAdd((LONG volatile *)(_address),_add)+(_add))
#define ECC_atomic_add_64(_address,_add) (_InterlockedExchangeAdd64((LONGLONG volatile *)(_address),_add)+(_add))

#define ECC_atomic_and_32(_address,_bits) _InterlockedAnd(_bits, (LONG volatile *)(_address))
#define ECC_atomic_or_32(_address,_bits)  _InterlockedOr(_bits, (LONG volatile *)(_address))
#define ECC_atomic_xor_32(_address,_bits) _InterlockedXor(_bits, (LONG volatile *)(_address))

#define ECC_atomic_test_and_set(_address,_bit)   _interlockedbittestandset(((LONG volatile *)(_address))[(_bit)/32], (_bit)&31)
#define ECC_atomic_test_and_clear(_address,_bit) _interlockedbittestandreset(((LONG volatile *)(_address))[(_bit)/32], (_bit)&31)
#define ECC_atomic_test_and_flip(_address,_bit)   InterlockedBitTestAndComplement(((LONG volatile *)(_address))[(_bit)/32], (_bit)&31)

#define ECC_atomic_barrier() _ReadWriteBarrier()

#elif (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100

#define ECC_atomic_compare_and_swap_ptr(_address, _old, _new) __sync_bool_compare_and_swap(_address, _old, _new)
#define ECC_atomic_compare_and_swap_32(_address, _old, _new)  __sync_bool_compare_and_swap(_address, _old, _new)
#define ECC_atomic_compare_and_swap_64(_address, _old, _new)  __sync_bool_compare_and_swap(_address, _old, _new)

#define ECC_atomic_add_ptr(_address,_add) __sync_add_and_fetch(_address,_add)
#define ECC_atomic_add_32(_address,_add)  __sync_add_and_fetch(_address,_add)
#define ECC_atomic_add_64(_address,_add)  __sync_add_and_fetch(_address,_add)

#define ECC_atomic_and_32(_address,_bits) __sync_fetch_and_and(_address,_bits)
#define ECC_atomic_or_32(_address,_bits)  __sync_fetch_and_or(_address,_bits)
#define ECC_atomic_xor_32(_address,_bits) __sync_fetch_and_xor(_address,_bits)

#define ECC_atomic_test_and_set(_address,_bit)   ((__sync_fetch_and_or (((uint32_t volatile *)_address)+(_bit)/32, (1<<((_bit)&31)))>>((_bit)&31))&1)
#define ECC_atomic_test_and_clear(_address,_bit) ((__sync_fetch_and_and(((uint32_t volatile *)_address)+(_bit)/32,~(1<<((_bit)&31)))>>((_bit)&31))&1)
#define ECC_atomic_test_and_flip(_address,_bit)  ((__sync_fetch_and_xor(((uint32_t volatile *)_address)+(_bit)/32, (1<<((_bit)&31)))>>((_bit)&31))&1)

#define ECC_atomic_barrier() __sync_synchronize()

#else
#warning not atomic

#define ECC_atomic_compare_and_swap_ptr ECC_interruptible_compare_and_swap
#define ECC_atomic_compare_and_swap_32  ECC_interruptible_compare_and_swap
#define ECC_atomic_compare_and_swap_64  ECC_interruptible_compare_and_swap

#define ECC_atomic_add_ptr ECC_interruptible_add
#define ECC_atomic_add_32  ECC_interruptible_add
#define ECC_atomic_add_64  ECC_interruptible_add

static inline uint32_t ECC_interruptible_and_32(uint32_t *address, uint32_t bits) { uint32_t t = *address; *address = t & bits; return t; }
static inline uint32_t ECC_interruptible_or_32(uint32_t *address, uint32_t bits) { uint32_t t = *address; *address = t | bits; return t; }

#define ECC_atomic_and_32(_address, _bits) ECC_interruptible_and_32((uint32_t *)(_address), _bits)
#define ECC_atomic_or_32(_address, _bits)  ECC_interruptible_or_32((uint32_t *)(_address), _bits)
#define ECC_atomic_xor_32(_address, _bits) (((*(uint32_t *)(_address)) ^= (_bits)) ^ (_bits))

#define ECC_atomic_test_and_set   ECC_interruptible_test_and_set
#define ECC_atomic_test_and_clear ECC_interruptible_test_and_clear
#define ECC_atomic_test_and_flip  ECC_interruptible_test_and_flip

#define ECC_atomic_barrier()

#endif


/*
	bit counting intrinsics
	
	note that counting leading and trailing bits is undefined or platform
	dependent for the value zero even with software fallbacks
	
	software fallbacks are provided for missing intrinsics with credit to
	https://graphics.stanford.edu/~seander/bithacks.html
*/


#if __GNUC__

#define ECC_INTRINSICS 1

#define ECC_count_leading_zeros_32 __builtin_clz
#define ECC_count_trailing_zeros_32 __builtin_ctz
#define ECC_count_set_bits_32 __builtin_popcount

#if __LP64__
#define ECC_count_leading_zeros_64 __builtin_clzll
#define ECC_count_trailing_zeros_64 __builtin_ctzll
#define ECC_count_set_bits_64 __builtin_popcountll
#endif

#elif _MSC_VER

#define ECC_INTRINSICS 1

#define ECC_count_leading_zeros_32 __lzcnt
#define ECC_count_set_bits_32 __popcount

#if __LP64__
#define ECC_count_leading_zeros_64 __lzcnt64
#define ECC_count_set_bits_64 __popcount64
#endif

#endif


#ifndef ECC_count_trailing_zeros_32
#ifdef ECC_count_leading_zeros_32
#define ECC_count_trailing_zeros_32(_x) (31-ECC_count_leading_zeros_32((_x)&-(_x)))
#else
#define ECC_DeBruijnBitPosition_32 "\x00\x01\x1C\x02\x1D\x0E\x18\x03\x1E\x16\x14\x0F\x19\x11\x04\x08\x1F\x1B\x0D\x17\x15\x13\x10\x07\x1A\x0C\x12\x06\x0B\x05\x0A\x09"
#define ECC_count_trailing_zeros_32(_x) ECC_DeBruijnBitPosition_32[((uint32_t)(((_x)&-(_x))*0x077CB531U))>>27]
#endif
#endif

#ifndef ECC_count_leading_zeros_32
static inline unsigned ECC_count_leading_zeros_32(uint32_t x) {
	static const unsigned char kDeBruijnBitPosition[32] = {
		31, 22, 30, 21, 18, 10, 29, 2, 20, 17, 15, 13, 9, 6, 28, 1,
		23, 19, 11, 3, 16, 14, 7, 24, 12, 4, 8, 25, 5, 26, 27, 0
	};
	
	x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16;
	
	return kDeBruijnBitPosition[((uint32_t)(x*0x07C4ACDDU))>>27];
}
#endif

#ifndef ECC_count_set_bits_32
#define ECC_count_set_bits_8(_x) ((((((uint32_t)(_x)&0x00FFU)*0x08040201U)>>3)&0x11111111U)%0x0F)
#define ECC_count_set_bits_32(_x) (ECC_count_set_bits_8(_x)+ECC_count_set_bits_8((_x)>>8)+ECC_count_set_bits_8((_x)>>16)+ECC_count_set_bits_8((_x)>>24))
#endif

#if __LP64__
#ifndef ECC_count_trailing_zeros_64
#define ECC_count_trailing_zeros_64(_x) (((uint32_t)(_x))?ECC_count_trailing_zeros_32(_x):(32+ECC_count_trailing_zeros_32((_x)>>32)))
#endif

#ifndef ECC_count_leading_zeros_64
#define ECC_count_leading_zeros_64(_x) (((_x)>>32)?ECC_count_leading_zeros_32((_x)>>32):(32+ECC_count_leading_zeros_32(_x)))
#endif

#ifndef ECC_count_set_bits_64
#define ECC_count_set_bits_64(_x) (ECC_count_set_bits_32(_x)+ECC_count_set_bits_32((_x)>>32))
#endif
#endif


/*
	bit manipulation utilities
	
	0 <= n <= 127
*/


#define ECC_clear_least_significant_bit(_x) ((_x)&((_x)-1))
#define ECC_least_significant_bit(_x) ((_x)&-(_x))

#define ECC_bytes_less_than(_x,_n) (((~0U/255)*(127+(_n))-((_x)&((~0U/255)*127)))&~(_x)&((~0U/255)*128))
#define ECC_bytes_more_than(_x,_n) (((~0U/255)*(127-(_n))+((_x)&((~0U/255)*127)) |(_x))&((~0U/255)*128))

#define ECC_count_bytes_less_than(_x,_n) ((ECC_bytes_less_than(_x,_n)>>7)%255)
#define ECC_count_bytes_more_than(_x,_n) ((ECC_bytes_more_than(_x,_n)>>7)%255)


#endif /* ECC_atomic_h */
