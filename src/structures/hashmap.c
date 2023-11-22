//hasmap.c

#include "hashmap.h"

#include <stdlib.h>
#include <string.h>

//TODO: TEST TEST TEST

typedef struct HashEntry HashEntry;

struct HashEntry
{
	//WARN: two keys with same hash will result in bad things
	bool used;
	int hash;
	void *value;
	HashEntry *next;
};

struct Hashmap
{
	size_t capacity;
	size_t count;
	HashEntry *entries;
	int (*hasher)(void*);
	void (*deallocator)(void *);
};



Hashmap* hashmap_create(int (*hasher)(void*), void (*deallocator)(void *), size_t init_capacity)
{
	Hashmap *ret = malloc(sizeof(Hashmap));

	if (!ret)
		return NULL;

	ret->capacity = init_capacity;
	ret->count = 0;
	ret->entries = malloc(ret->capacity * sizeof(HashEntry));

	if (!ret->entries)
	{
		free(ret);
		return NULL;
	}

	memset(ret->entries, 0, ret->capacity * sizeof(HashEntry));
	ret->hasher = hasher;
	ret->deallocator = deallocator;
	return ret;
}

void hashmap_destroy(Hashmap *map)
{
	if (!map)
		return;

	for (size_t i = 0; i < map->capacity; i++)
	{
		if (!map->entries[i].used)
			continue;
		
		map->deallocator(map->entries[i].value);
	}

	free(map->entries);
	free(map);
}


//Collision resolution strategy: separate chaining
void *hashmap_get(Hashmap *map, void *key)
{
	if (!map)
		return NULL;

	int hash = map->hasher(key);
	int index = hash % map->capacity;
	HashEntry *entry = &map->entries[index];

	if (entry->used)
		return entry->value;

	while (entry->hash != hash)
		if ((entry = entry->next) == NULL)
			return NULL;

	return entry->value;
}

bool hashmap_add(Hashmap *map, void *key, void *value)
{
	if (!map)
		return false;

	int hash = map->hasher(key);
	int index = hash % map->capacity;
	HashEntry *entry = &map->entries[index], *last = entry;
	HashEntry newEntry = { .used = true, .hash = hash, .value = value, .next = NULL };

	if (entry->used)
	{
		while (entry->used)
		{
			last = entry;
			entry = entry->next;
		}

		entry = malloc(sizeof(HashEntry));

		if (!entry)
			return false; //TODO: Better error
	}

	last->next = entry;
	*entry = newEntry;
	map->count++; //TODO: Grow if needed
	return true;
}

bool hashmap_set(Hashmap *map, void *key, void *value)
{
	if (!map)
		return false; //TODO: Better error

	int hash = map->hasher(key);
	int index = hash % map->capacity;
	HashEntry *entry = &map->entries[index];

	if (!entry->used)
		return false;

	while (entry->hash != hash)
		if ((entry = entry->next) == NULL)
			return false;

	entry->value = value;
	return true;
}

bool hashmap_remove(Hashmap *map, void *key)
{
	if (!map)
		return false; //TODO: Better error

	int hash = map->hasher(key);
	int index = hash % map->capacity;
	HashEntry *entry = &map->entries[index], *last = entry;

	if (!entry->used)
		return false;

	while (entry->hash != hash)
	{
		if ((entry = entry->next) == NULL)
			return false;
		
		last = entry;
		entry = entry->next;
	}

	last->next = entry->next;
	map->deallocator(entry->value);
	free(entry);
	map->count--;

	return true;
}

bool hashmap_haskey(Hashmap *map, void *key)
{
	if (!map)
		return false; //TODO: Better error
	
	int hash = map->hasher(key);
	int index = hash % map->capacity;
	HashEntry *entry = &map->entries[index];

	if (entry->used)
		return true;

	while (entry->hash != hash)
		if ((entry = entry->next) == NULL)
			return false;

	return true;
}
