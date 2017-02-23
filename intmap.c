#include "assert.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"

#include "intmap.h"

// IntMap is a key/value store for uintptr_t
struct IntMap {
	size_t capacity;
	uintptr_t* keyArray;
	uintptr_t* valueArray;
};

// RETURNS an empty IntMap
struct IntMap* IntMap_new() {
	struct IntMap* instance = (struct IntMap*)malloc(sizeof(struct IntMap));
	instance->capacity = 32;
	instance->keyArray = (uintptr_t*)malloc(32 * sizeof(uintptr_t));
	instance->valueArray = (uintptr_t*)malloc(32 * sizeof(uintptr_t));
	for (size_t i = 0; i < instance->capacity; i++) {
		instance->valueArray[i] = 0;
		instance->keyArray[i] = 0;
	}
	return instance;
}

// MODIFIES the IntMap such that subsequent calls to
// IntMap_get(key) returns value.
void IntMap_insert(struct IntMap* self, uintptr_t key, uintptr_t value) {
	assert(self != NULL);

	// Update existing key, value pair
	for (size_t i = 0; i < self->capacity; i++) {
		if (self->keyArray[i] == key) {
			self->valueArray[i] = value;
			return;
		}
	}

	// Inserting a non-existent value
	if (value == 0) {
		return;
	}

	while (1) {
		// Insert into empty space
		for (size_t i = 0; i < self->capacity; i++) {
			if (self->valueArray[i] == 0) {
				self->keyArray[i] = key;
				self->valueArray[i] = value;
				return;
			}
		}

		// Resize the backing array
		uintptr_t* newKeys = (uintptr_t*)malloc(2*self->capacity * sizeof(uintptr_t));
		uintptr_t* newValues = (uintptr_t*)malloc(2*self->capacity * sizeof(uintptr_t));
		for (size_t i = 0; i < 2*self->capacity; i++) {
			newKeys[i] = 0;
			newValues[i] = 0;
		}
		for (size_t i = 0; i < self->capacity; i++) {
			newKeys[i] = self->keyArray[i];
			newValues[i] = self->valueArray[i];
		}
		free(self->keyArray);
		free(self->valueArray);

		self->keyArray = newKeys;
		self->valueArray = newValues;
		self->capacity *= 2;
	}
}

// RETURNS the value associated with the key (the value most recently inserted
// to that key)
// RETURNS 0 if this key has not yet been inserted.
uintptr_t IntMap_get(struct IntMap const* self, uintptr_t key) {
	assert(self != NULL);

	for (size_t i = 0; i < self->capacity; i++) {
		if (self->keyArray[i] == key) {
			return self->valueArray[i];
		}
	}
	return 0;
}

// MODIFIES the IntMap so that all keys map to 0.
void IntMap_clear(struct IntMap* self) {
	assert(self != NULL);

	for (size_t i = 0; i < self->capacity; i++) {
		self->valueArray[i] = 0;
	}
}

// CALLS f(key, value, data) for each key/value pair with non-zero value.
// MODIFIES IntMap to use the return value of f() as the new value for the
// supplied key.
void IntMap_foreach(struct IntMap* self, uintptr_t (*f)(uintptr_t, uintptr_t, void*), void* data) {
	for (size_t i = 0; i < self->capacity; i++) {
		if (self->valueArray[i] != 0) {
			self->valueArray[i] = f(self->keyArray[i], self->valueArray[i], data);
		}
	}
}
