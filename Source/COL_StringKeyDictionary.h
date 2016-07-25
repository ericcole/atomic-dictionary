//
//  COL_StringKeyDictionary.h
//  AtomicDictionary
//
//  Created by Eric Cole on 7/23/16.
//  Copyright © 2016 Balance Software. All rights reserved.
//

#ifndef COL_StringKeyDictionary_h
#define COL_StringKeyDictionary_h

#include "COL_Dictionary.h"

COLD COLD_allocate_for_string_keys(unsigned capacity);
COLD COLD_allocate_for_constant_string_keys(unsigned capacity);
void COLD_initialize_for_string_keys(COLD cold, unsigned capacity);
void COLD_initialize_for_constant_string_keys(COLD cold, unsigned capacity);

COLD_call_t COLD_string_key_calls;
COLD_call_t COLD_constant_string_key_calls;

#endif /* COL_StringKeyDictionary_h */
