#ifndef GLOBAL_VARS
#define GLOBAL_VARS



extern char* this_machine_id;

extern struct key_node* key_head;
extern struct key_node* key_tail;

extern struct lock_node* lock_head;
extern struct lock_node* lock_tail;

extern char device_buffer[];
extern char temp_buffer[];

#endif