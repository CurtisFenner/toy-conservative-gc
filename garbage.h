#pragma once

#include "stdint.h"
#include "stdlib.h"

// Begin execution of function `f` with a garbage collector automatically
// collecting all memory not referenced by f's stack
// NOTE: memory referenced only by globals WILL be collected
// RETURNS the return value of f(data)
void* gc_initialize(void* (*f)(void*), void* data);

// Search for and collect garbage. This is automatically invoked upon allocation
void gc_collect();

// (DEBUG ONLY)
// Record the name of the next allocation
// MUST NOT be called more than once before gc_allocate_bytes
void gc_setinfo(char const* msg);

// RETURNS a pointer to automatically-managed memory of at least size bytes
char* gc_allocate_bytes(size_t size);

// INTERNAL USE ONLY
// Releases memory allocated by gc_allocate_bytes
void gc_deallocate_bytes(char* bytes);

// Type-safe helper macro for allocating structs
#define NEW(T) ((T*)gc_allocate_bytes(sizeof(T)))

// Type-safe helper macro for allocating arrays
#define NEW_ARRAY(T, n) ((T*)gc_allocate_bytes(sizeof(T)*(n)))

struct gc_statistics {
	// The number of allocations being managed by the current collector
	size_t allocations;

	// The total size in bytes of all user memory managed by the current collector
	// This does NOT include overhead
	size_t bytes;
};

// RETURNS information about the current state of memory being managed by the
// current collector
struct gc_statistics gc_get_statistics();
