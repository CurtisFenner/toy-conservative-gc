# Toy Conservative Garbage Collector

The `garbage.h` header defines several functions that allow you to get
automatically managed memory in C!

* `gc_initialize` has to be used to start the whole process: give a function
  to run and a parameter to run it with.
* `NEW(type)` returns a freshly allocated (and uninitialized) `type*`
* `NEW_ARRAY(type, size)` returns a freshly allocated (and uninitialized)
  `type*` with enough space to fit `size` elements

Objects returned by `NEW` and `NEW_ARRAY` (or `gc_allocate_bytes`) automatically
get collected after you stop referencing them, so you don't need to do any
manual `free`s!

## How do I use this?

Include the library:

```c
#include "garbage.h"
```

Define a procedure that uses `NEW` or `NEW_ARRAY`

```c

void* program(void* data) {
    int* someInts = NEW_ARRAY(int, 100);
    
    for (int i = 0; i < 100; i++) {
        someInts[i] = *(int*)data;
    }
}
```

Start the procedure using `gc_initialize`:

```c
int main() {
    int arrayValue = 5;
    gc_initialize(program, (void*)&init);
}
```

## Should I use this for my <>?

Nope.

## How does it work?

Whenever you request memory to be allocated, the garbage collector tries to find
anything that can be collected.

It does this by searching through the stack (in `gc_collect`) and looking for
words in memory that look like they might be addresses returned by
`gc_allocate_bytes` (in `resolveBaseAddress`).

These addresses are recursively marked as reachable (in `visitAddress`), and the
allocated object is searched for more pointers.

After completing the search, all unmarked objects get collected.

## Limitations

* The collector is extremely slow. On every allocation, essentially all of
reachable memory is scanned.
    * Several pieces of the collector could be much faster. `resolveBaseAddress`
      could be implemented as a segment tree or other balance tree so that it takes
      essentially constant time rather than requiring a scan of all allocated
      addresses.

* The collector isn't strictly portable, since it invokes *undefined behavior*.
  Most of the undefined behavior is confined to `gc_collect`:
    * doing pointer arithmetic beyond the `stack` integer variable
    * dereferencing the pointer beyond the `stack` integer variable
  However, it appears that GCC (at least with `-O0`) does not harm the function,
  and it behaves as intended.

* Because this is a *conservative* collector, it's possible that the collector will
  see a word in memory that *looks* like a pointer, but actually isn't.

* Since the collector needs to be able to search pointed objects, it will *only*
  be able to search objects created with `gc_allocate_bytes` (or the `NEW` or
  `NEW_ARRAY` macros). This means you can't safely mix this library with `new` or
  `malloc`.

* I made no attempt to make this thread-safe.
