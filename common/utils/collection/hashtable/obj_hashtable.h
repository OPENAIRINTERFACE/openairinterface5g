#ifndef _UTILS_COLLECTION_OBJ_HASH_TABLE_H_
#define _UTILS_COLLECTION_OBJ_HASH_TABLE_H_
#include<stdlib.h>
#include <stdint.h>
#include <stddef.h>

//#include "collection/hashtable/hashtable.h"
#include "hashtable.h"

typedef struct obj_hash_node_s {
    int                 key_size;
    void               *key;
    void               *data;
    struct obj_hash_node_s *next;
} obj_hash_node_t;

typedef struct obj_hash_table_s {
    hash_size_t         size;
    hash_size_t         num_elements;
    struct obj_hash_node_s **nodes;
    hash_size_t       (*hashfunc)(const void*, int);
    void              (*freekeyfunc)(void*);
    void              (*freedatafunc)(void*);
} obj_hash_table_t;

obj_hash_table_t   *obj_hashtable_create  (hash_size_t   size, hash_size_t (*hashfunc)(const void*, int ), void (*freekeyfunc)(void*), void (*freedatafunc)(void*));
hashtable_rc_t      obj_hashtable_destroy (obj_hash_table_t *hashtblP);
hashtable_rc_t      obj_hashtable_is_key_exists (obj_hash_table_t *hashtblP, void* keyP, int key_sizeP);
hashtable_rc_t      obj_hashtable_insert  (obj_hash_table_t *hashtblP,       void* keyP, int key_sizeP, void *dataP);
hashtable_rc_t      obj_hashtable_remove  (obj_hash_table_t *hashtblP, const void* keyP, int key_sizeP);
hashtable_rc_t      obj_hashtable_get     (obj_hash_table_t *hashtblP, const void* keyP, int key_sizeP, void ** dataP);
hashtable_rc_t      obj_hashtable_get_keys(obj_hash_table_t *hashtblP, void ** keysP, unsigned int *sizeP);
hashtable_rc_t      obj_hashtable_resize  (obj_hash_table_t *hashtblP, hash_size_t sizeP);

#endif

