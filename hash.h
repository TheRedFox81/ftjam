/*
 * Copyright 1993, 1995 Christopher Seiwald.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

/*
 * hash.h - simple in-memory hashing routines
 *
 * 11/04/02 (seiwald) - const-ing for string literals
 */

/* technical explanation:
 *    each node of a hash table must start with a string pointer, which
 *    will be used as its key.
 */

typedef struct hashdata HASHDATA;

/* create a new hash table used to hold elements of 'datalen' bytes.
 * the table is identified by its 'name'
 */
struct hash *	hashinit( int datalen, const char *name );

/* retrieve an element from a hash table, find it through its key
 */
void*           hashget( struct hash *hp, const char*  key );

/* tries to put a new element into a hash table. If an element with the
 * same key is already there, it is returned, and *pcreate is set to 0
 *
 * if no element with the same key exists, the function creates a new
 * one, initializes it with the key and returns it. *pcreate is set to 1
 * and it is up to the caller to initialize the missing fields of the
 * new element.
 */
void*           hashput( struct hash *hp, const char*  key, int  *pcreate );

/* destroy a given hash table */
void 		hashdone( struct hash *hp );
