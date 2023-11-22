//hashmap.h - 

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Hashmap Hashmap;

//TODO: Errors yay

Hashmap* hashmap_create(int (*hasher)(void*), void (*deallocator)(void *), size_t init_capacity);
void hashmap_destroy(Hashmap *map);

void *hashmap_get(Hashmap *map, void *key);
bool hashmap_add(Hashmap *map, void *key, void *value);
bool hashmap_set(Hashmap *map, void *key, void *value);
bool hashmap_remove(Hashmap *map, void *key);
bool hashmap_haskey(Hashmap *map, void *key);

#endif
