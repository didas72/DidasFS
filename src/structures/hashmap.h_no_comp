//hashmap.h - 

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Hashmap Hashmap;

//TODO: Errors

Hashmap* hashmap_create(int (*hasher)(const void*), void (*deallocator)(void *), size_t init_capacity);
void hashmap_destroy(Hashmap *map);

void *hashmap_get(Hashmap *map, const void *key);
bool hashmap_add(Hashmap *map, void *key, void *value);
bool hashmap_set(Hashmap *map, void *key, void *value);
bool hashmap_remove(Hashmap *map, const void *key);
bool hashmap_has_key(Hashmap *map, const void *key);

size_t hashmap_get_count(Hashmap *map);
size_t hashmap_get_values(Hashmap *map, void **values, size_t capacity);

#endif
