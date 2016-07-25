//
//  COL_StringKeyDictionary.c
//  AtomicDictionary
//
//  Created by Eric Cole on 7/23/16.
//  Copyright © 2016 Balance Software. All rights reserved.
//

#include "COL_StringKeyDictionary.h"


COLD COLD_allocate_for_string_keys(unsigned capacity) {
	return COLD_allocate(capacity, &COLD_string_key_calls);
}

void COLD_initialize_for_string_keys(COLD cold, unsigned capacity) {
	COLD_initialize(cold, capacity, &COLD_string_key_calls);
}

COLD COLD_allocate_for_constant_string_keys(unsigned capacity) {
	return COLD_allocate(capacity, &COLD_constant_string_key_calls);
}

void COLD_initialize_for_constant_string_keys(COLD cold, unsigned capacity) {
	COLD_initialize(cold, capacity, &COLD_constant_string_key_calls);
}


#pragma mark -

#ifndef _STRING_H_
#include <string.h>
#endif

unsigned COLD_equal_strings(COLD_data_t a, COLD_data_t b) {
	return 0 == strcmp((const char *)a, (const char *)b);
}

COLD_call_t COLD_constant_string_key_calls = {
	COLD_hash_bytes_null_terminated,
	COLD_equal_strings,
	NULL,
	NULL
};

COLD_call_t COLD_string_key_calls = {
	COLD_hash_bytes_null_terminated,
	COLD_equal_strings,
	(COLD_data_t (*)(COLD_data_t))strdup,
	(void (*)(COLD_data_t))free
};
