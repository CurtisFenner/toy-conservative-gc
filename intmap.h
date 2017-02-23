#include "stdint.h"

// IntMap is a key/value store for uintptr_t
struct IntMap;

// RETURNS an empty IntMap
struct IntMap* IntMap_new();

// MODIFIES the IntMap such that subsequent calls to
// IntMap_get(key) returns value.
void IntMap_insert(struct IntMap*, uintptr_t key, uintptr_t value);

// RETURNS the value associated with the key (the value most recently inserted
// to that key)
// RETURNS 0 if this key has not yet been inserted.
uintptr_t IntMap_get(struct IntMap const*, uintptr_t key);

// MODIFIES the IntMap so that all keys map to 0.
void IntMap_clear(struct IntMap*);

// CALLS f(key, value, data) for each key/value pair with non-zero value.
// MODIFIES IntMap to use the return value of f() as the new value for the
// supplied key.
void IntMap_foreach(struct IntMap*, uintptr_t (*f)(uintptr_t, uintptr_t, void*), void* data);
