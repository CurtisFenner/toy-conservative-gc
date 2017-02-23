#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "garbage.h"

#include "intmap.h"

static struct {
	uintptr_t* stackBegin;
	char const* info;

	struct IntMap* allocationSize;
	struct IntMap* infoMap;
} collector;

// (DEBUG ONLY)
// Record the name of the next allocation
// MUST NOT be called more than once before gc_allocate_bytes
void gc_setinfo(char const* msg) {
	assert(collector.info == NULL);
	collector.info = msg;
}

// Begin execution of function `f` with a garbage collector automatically
// collecting all memory not referenced by f's stack
// NOTE: memory referenced only by globals WILL be collected
// RETURNS the return value of f(data)
void* gc_initialize(void* (*f)(void*), void* data) {
#ifdef CWF_GC_DEBUG
	printf("# gc_initialize()\n");
#endif
	uintptr_t stack;

	// Create the map
	collector.allocationSize = IntMap_new();
	collector.infoMap = IntMap_new();

	// Mark the beginning of the stack
	collector.stackBegin = &stack;

	// Reset the debug info
	collector.info = NULL;

	void* result = f(data);
	gc_collect();
	return result;
}

struct addr_walk_info {
	struct IntMap* frontier;
	struct IntMap* visited;
	bool inserted;
};

// Helper for gc_collect
// Scans an allocation starting at base and recursively / via the
// output-frontier marks all objects reachable from base
static void visitAddress(uintptr_t base, void* vimpl) {
	struct addr_walk_info* impl = (struct addr_walk_info*)vimpl;

	// Check if this has-been / is-being visited
	if (IntMap_get(impl->visited, base)) {
		return;
	}

	// Mark this address as visited
	IntMap_insert(impl->visited, base, 1);

	// Scan through the body of this allocation, searching for more pointers
	size_t size = (size_t)IntMap_get(collector.allocationSize, base);
	for (uintptr_t* from = (uintptr_t*)base; from < (uintptr_t*)(base + size); from++) {
		if (IntMap_get(collector.allocationSize, *from)) {
			visitAddress(*from, impl);
		}
	}

	impl->inserted = true;
}

struct collection {
	// isVisited is a set of reachable allocations
	struct IntMap* isVisited;
};

// Deallocates any unmarked allocations
static uintptr_t collectUnvisited(uintptr_t key, uintptr_t size, void* vcollection) {
	struct collection* collection= (struct collection*)vcollection;

	if (!IntMap_get(collection->isVisited, key)) {
		// This address is not reachable
		gc_deallocate_bytes((char*)key);
		return 0;
	}
	return size;
}

struct allocationSearch {
	uintptr_t query;
	uintptr_t base;
};

// Helper for resolveBaseAddress
// Records in struct allocationSearch* vs ths base address of the allocation if
// vs->query points to part of or immediately after this allocation
static uintptr_t allocationContainsAddress(uintptr_t key, uintptr_t size, void* vs) {
	struct allocationSearch* s = (struct allocationSearch*)vs;

	if (key <= s->query && s->query <= key + size) {
		s->base = key;
	}

	return size;
}

// If query is a location in (or immediately after) the range of an allocation,
// RETURNS the base of that allocation
// RETURNS 0 otherwise.
// For example,
// int* foo = NEW_ARRAY(int, 10);
// resolveBaseAddress( (uintptr_t)(foo + 0) ) == (uintptr_t)foo
// resolveBaseAddress( (uintptr_t)(foo + 5) ) == (uintptr_t)foo
// resolveBaseAddress( (uintptr_t)(foo + 10) ) == (uintptr_t)foo
static uintptr_t resolveBaseAddress(uintptr_t query) {
	struct allocationSearch search;
	search.query = query;
	search.base = 0;

	IntMap_foreach(collector.allocationSize, &allocationContainsAddress, &search);

	// printf("$ %llu\t is resolved to %llu\n", (unsigned long long)query, (unsigned long long)search.base);
	return search.base;
}

// Search for and collect garbage. This is automatically invoked upon allocation
void gc_collect() {
	// Find the current position of the stack
	uintptr_t stack;
	uintptr_t* search = &stack;

	//printf("# <gc_collect>\n");

	// Begin with an empty "marked" set
	struct addr_walk_info info;
	info.frontier = IntMap_new();
	info.visited = IntMap_new();
	info.inserted = false;

	// Scan the stack
	for (uintptr_t* walk = (uintptr_t*)search; walk <= collector.stackBegin; walk++) {
		// XXX: `walk` moving outside of `search`'s allocation is undefined behavior
		// XXX: This dereference is undefined behavior
		uintptr_t pointer = *(uintptr_t*)walk;

		uintptr_t base = resolveBaseAddress(pointer);
		// Don't traverse non-managed pointers
		if (base == 0) {
			continue;
		}

		// Search this object
		visitAddress(base, &info);
	}

	// XXX: while info.frontier isn't empty...

	// Collect all unreachable memory
	struct collection collection;
	collection.isVisited = info.visited;
	IntMap_foreach(collector.allocationSize, &collectUnvisited, &collection);

	//printf("# </gc_collect>\n\n");
}

// RETURNS a pointer to automatically-managed memory of at least size bytes
char* gc_allocate_bytes(size_t size) {
	// Collect unreachable memory before allocating more
	gc_collect();

	//printf("# gc_allocate_bytes(%lld) -> ", (long long)size);
	char* allocated = (char*)malloc(size);

	// Record the allocation in the size map
	IntMap_insert(collector.allocationSize,
		(uintptr_t)allocated, (uintptr_t)size);

	if (collector.info != NULL) {
		// Record debug information
		IntMap_insert(collector.infoMap, (uintptr_t)allocated, (uintptr_t)collector.info);
		//printf("[%s] ", collector.info);
		collector.info = NULL;
	}

	//printf("%lld\n\n", (long long)(uintptr_t)allocated);
	return allocated;
}

// INTERNAL USE ONLY
// Releases memory allocated by gc_allocate_bytes
void gc_deallocate_bytes(char* bytes) {
#ifdef CWF_GC_DEBUG
	printf("# gc_deallocate_bytes(%lld)", (long long)(uintptr_t)bytes);
#endif
	uintptr_t info = IntMap_get(collector.infoMap, (uintptr_t)bytes);
	if (info) {
#ifdef CWF_GC_DEBUG
		printf(" [%s]", (char*)info);
#endif
		IntMap_insert(collector.infoMap, (uintptr_t)bytes, 0);
	}
	printf("\n");
	free(bytes);
}

// HELPER for gc_statistics
// Updates struct gc_statistics to include this allocation
static uintptr_t updateStatistics(uintptr_t key, uintptr_t size, void* vs) {
	struct gc_statistics* statistics = (struct gc_statistics*)vs;
	statistics->allocations++;
	statistics->bytes += size;

	return size;
}

// RETURNS information about the current state of memory being managed by the
// current collector
struct gc_statistics gc_get_statistics() {
	struct gc_statistics statistics;

	statistics.allocations = 0;
	statistics.bytes = 0;

	IntMap_foreach(collector.allocationSize, &updateStatistics, &statistics);

	return statistics;
}
