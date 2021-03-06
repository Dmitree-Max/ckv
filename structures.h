#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <linux/slab.h> 
#include "global_vars.h"
#include "string_functions.h"

struct key
{
    char* key;
    char* value;
};

struct lock
{
    struct key* key;
    char* owner;
};

struct key_node
{
    struct key_node* next;
    struct key* data;
};

struct lock_node
{
    struct lock_node* next;
    struct lock* data;
};

struct key* create_key(char* key, char* value);

struct key_node* create_key_node(struct key* key);

struct lock_node* create_lock_node(struct lock* lock);
void delete_lock_node(struct lock_node* node);

void delete_lock(struct lock* lock);
void remove_lock(struct lock* lock);
struct lock* create_lock(struct key* key, char* owner);


#endif