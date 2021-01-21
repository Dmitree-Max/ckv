#include "commands.h"



static int lock(char* key_name);
static int unlock(char* key_name);


static int lock(char* key_name)
{
	struct key* key;
	if (is_there_lock(key_name))
	{
	    printk(KERN_INFO "charDev : try to lock locked key: %s\n", key_name);
		return -1;
	}
	else
	{
		key = find_key(key_name);
		if (key == NULL)
		{
		    printk(KERN_INFO "charDev : there is no such key: %s\n", key_name);
			return -1;
		}
		else
		{
		    insert_lock(create_lock(key, this_machine_id));
		    return 0;
		}
	}
}




static int unlock(char* key_name)
{
	struct lock* lock;
	lock = find_lock(key_name);
	if (lock == NULL)
	{
	    printk(KERN_INFO "charDev : there is no such lock: %s\n", key_name);
		return -1;
	}
	else
	{
		if (strcmp(lock->owner, this_machine_id) != 0)
		{
		    printk(KERN_INFO "charDev : %s tries to unlock %s, which owned by %s\n", this_machine_id, key_name, lock->owner);
			return -1;
		}
		else
		{
		    remove_lock(lock);
		    return 0;
		}
	}
}


int make_command(char* buff, int length)
{
	char* command;
	char* key_name;
	char* key_value;
	char* next = NULL;
	char* str = NULL;
    int length_left;
    str = buff;
    length_left = length;

	command = split(str, " ", &next, length_left);
	if (command == NULL)
	{
	    printk(KERN_INFO "charDev : no command %s\n", key_name);
	    return -1;
	}
	length_left -= next - str;
	str = next;
    printk(KERN_INFO "charDev : command %s\n", command);
	if (strcmp(command, "add-key") == 0)
	{
		key_name = split(str, " ", &next, length_left);
		if (key_name == NULL)
		{
		    printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		length_left -= next - str;
		str = next;
		key_value = split(str, " ", &next, length_left);
		if (key_value == NULL)
		{
		    printk(KERN_INFO "charDev : key value is NULL\n");
			return -1;
		}
	    return insert_key(create_key(key_name, key_value));
		return 0;
	}
	if (strcmp(command, "lock-key") == 0)
	{
		key_name = split(str, " ", &next, length_left);
		if (key_name == NULL)
		{
		    printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		return lock(key_name);
	}
	if (strcmp(command, "unlock-key") == 0)
	{
		key_name = split(str, " ", &next, length_left);
		if (key_name == NULL)
		{
		    printk(KERN_INFO "charDev : key name is NULL\n");
			return -1;
		}
		return unlock(key_name);
	}

    printk(KERN_INFO "charDev : no such command: %s\n", command);
	return -1;
}
