// ordered_array.h -- Interface for creating, inserting and deleting
//                    from ordered arrays.
//                    Written for JamesM's kernel development tutorials.

#ifndef ORDERED_ARRAY
#define ORDERED_ARRAY

#include "system.h"

/**
   This array is insertion sorted - it always remains in a sorted state (between calls).
   It can store anything that can be cast to a void* -- so a unsigned int, or any pointer.
**/
typedef void* type_t;
/**
   A predicate should return nonzero if the first argument is less than the second. Else
   it should return zero.
**/
typedef char (*lessthan_predicate_t)(type_t,type_t);
typedef struct
{
    type_t *array;
    unsigned int size;
    unsigned int max_size;
    lessthan_predicate_t less_than;
} ordered_array_t;

/**
   A standard less than predicate.
**/
char standard_lessthan_predicate(type_t a, type_t b);

/**
   Create an ordered array.
**/
ordered_array_t create_ordered_array(unsigned int max_size, lessthan_predicate_t less_than);
ordered_array_t place_ordered_array(void *addr, unsigned int max_size, lessthan_predicate_t less_than);

/**
   Destroy an ordered array.
**/
void destroy_ordered_array(ordered_array_t *array);

/**
   Add an item into the array.
**/
void insert_ordered_array(type_t item, ordered_array_t *array);

/**
   Lookup the item at index i.
**/
type_t lookup_ordered_array(unsigned int i, ordered_array_t *array);

/**
   Deletes the item at location i from the array.
**/
void remove_ordered_array(unsigned int i, ordered_array_t *array);

#endif
