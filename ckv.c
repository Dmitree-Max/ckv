#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

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

static struct key_node* key_head;
static struct key_node* key_tail;

static struct lock_node* lock_head;
static struct lock_node* lock_tail;

static dev_t first;       // Global variable for the first device number
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl;  // Global variable for the device class

#define BUFFER_SIZE 1024
static char device_buffer[BUFFER_SIZE];
static char temp_buffer[BUFFER_SIZE];

static int my_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: open()\n");
  return 0;
}

static int my_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: close()\n");
  return 0;
}

// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
static int fill_buffer_with_keys(char* buffer, int length, int* offset)
{
    int offset_controller = 0;
    struct key_node* cur_node = key_head;
    int bytes_in_buffer = 0;
	char separator[] = " = ";
	int max_bytes_to_copy;

    while (cur_node != NULL)
    {
		int key_string_length = strlen(cur_node->data->key) + strlen(cur_node->data->value) + strlen(separator) + 1; // 1 for \n
		if (offset_controller + key_string_length > *offset)
		{
			strcpy(temp_buffer, cur_node->data->key);
			strcat(temp_buffer, separator);
			strcat(temp_buffer, cur_node->data->value);
			strcat(temp_buffer, "\n");

			// not copy more then buffer
			max_bytes_to_copy = length - bytes_in_buffer;

			if (offset_controller < *offset)
			{
				strncpy(buffer, temp_buffer + (*offset - offset_controller), max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer + (*offset - offset_controller)), max_bytes_to_copy);
			}
			else
			{
				strncpy(buffer + bytes_in_buffer, temp_buffer, max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer), max_bytes_to_copy);
			}

		}
		offset_controller += key_string_length;
        cur_node = cur_node->next;

        // overload of buffer
        if (bytes_in_buffer > length)
        {
            printk(KERN_INFO "charDev : Error! Buffer overload\n");
        }

        // full buffer
        if (bytes_in_buffer == length)
        {
        	break;
        }
    }

    *offset -= offset_controller;
    *offset = max(*offset, 0);
    return bytes_in_buffer;
}



// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
static int fill_buffer_with_locks(char* buffer, int length, int* offset)
{
    int offset_controller = 0;
    struct lock_node* cur_node = lock_head;
    int bytes_in_buffer = 0;
	char separator1[] = " (";
	char separator2[] = ")";
	int max_bytes_to_copy;
	int lock_string_length;

    while (cur_node != NULL)
    {
		lock_string_length = strlen(cur_node->data->key->key) + strlen(cur_node->data->owner) +\
				strlen(separator1) + strlen(separator2) + 1; // 1 for \n
		if (offset_controller + lock_string_length > *offset)
		{
			strcpy(temp_buffer, cur_node->data->key->key);
			strcat(temp_buffer, separator1);
			strcat(temp_buffer, cur_node->data->owner);
			strcat(temp_buffer, separator2);
			strcat(temp_buffer, "\n");

			// not copy more then buffer
			max_bytes_to_copy = length - bytes_in_buffer;

			if (offset_controller < *offset)
			{
				strncpy(buffer, temp_buffer + (*offset - offset_controller), max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer + (*offset - offset_controller)), max_bytes_to_copy);
			}
			else
			{
				strncpy(buffer + bytes_in_buffer, temp_buffer, max_bytes_to_copy);
				bytes_in_buffer += MIN(strlen(temp_buffer), max_bytes_to_copy);
			}

		}
		offset_controller += lock_string_length;
        cur_node = cur_node->next;

        // overload of buffer
        if (bytes_in_buffer > length)
        {
            printk(KERN_INFO "charDev : Error! Buffer overload\n");
        }

        // full buffer
        if (bytes_in_buffer == length)
        {
        	break;
        }
    }

    *offset -= offset_controller;
    *offset = max(*offset, 0);
    return bytes_in_buffer;
}


static int send_buffer(char *buff, int bytes_to_send, loff_t *ppos)
{
	int bytes_really_send;

	bytes_really_send = bytes_to_send - copy_to_user(buff, device_buffer, bytes_to_send);
    printk(KERN_INFO "charDev : Read: bytes send: %i\n", bytes_really_send);
    *ppos += bytes_really_send;
	return bytes_really_send;
}


// buffer to write result (device_buffer)
// length (bytes function can write in buffer)
// offset bytes to skip
// header -- string to write
static int write_header(char* buffer, int length, int *offset, char* header)
{
	int bytes_written = 0;
	char* symbol = header;
	int offset_controller = 0;
	int buffer_place = 0;

	while (*symbol != '\0')
	{
		if (offset_controller >= *offset)
		{
			buffer[buffer_place] = *symbol;
			buffer_place++;
			bytes_written++;
		}
		offset_controller++;
		if (buffer_place > length)
		{
			break;
		}
		symbol++;
	}
	*offset -= offset_controller;
    *offset = max(*offset, 0);
	return bytes_written;
}

static ssize_t device_read(struct file *fp, char *buff, size_t length, loff_t *ppos)
{
	    int size_in_min_buffer_left, offset, bytes_written;

		size_in_min_buffer_left = MIN(length, BUFFER_SIZE);
		offset = *ppos;

		bytes_written = write_header(device_buffer, size_in_min_buffer_left, &offset, "keys:\n");
		size_in_min_buffer_left -= bytes_written;

		if (size_in_min_buffer_left > 0)
		{
		    bytes_written += fill_buffer_with_keys(device_buffer + bytes_written, size_in_min_buffer_left, &offset);
			size_in_min_buffer_left -= bytes_written;
		}

		if (size_in_min_buffer_left > 0)
		{
		    bytes_written += write_header(device_buffer + bytes_written, size_in_min_buffer_left, &offset, "\nlocks:\n");
			size_in_min_buffer_left -= bytes_written;
		}

		if (size_in_min_buffer_left > 0)
		{
		    bytes_written += fill_buffer_with_locks(device_buffer + bytes_written, size_in_min_buffer_left, &offset);
			size_in_min_buffer_left -= bytes_written;
		}

        return send_buffer(buff, bytes_written, ppos);
}

static ssize_t device_write(struct file *fp, const char *buff, size_t length, loff_t *ppos)
{
        int maxbytes;           /* maximum bytes that can be read from ppos to BUFFER_SIZE*/
        int bytes_to_write;     /* gives the number of bytes to write*/
        int bytes_writen;       /* number of bytes actually writen*/
        maxbytes = BUFFER_SIZE - *ppos;
        if (maxbytes > length)
                bytes_to_write = length;
        else
                bytes_to_write = maxbytes;

        bytes_writen = bytes_to_write - copy_from_user(device_buffer + *ppos, buff, bytes_to_write);
        printk(KERN_INFO "charDev : device has been written %d\n", bytes_writen);
        *ppos += bytes_writen;
        return bytes_writen;
}

static struct file_operations pugs_fops =
{
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .read = device_read,
  .write = device_write
};


static struct key* create_key(char* key, char* value)
{
	 int key_length, value_length;
	 char* key_memory;
	 char* value_memeroy;
     struct key* new_key;

     new_key = kmalloc(sizeof(struct key), GFP_KERNEL);

     key_length = strlen(key) + 1;
     key_memory = kmalloc(sizeof(char) * key_length, GFP_KERNEL);
     memcpy(key_memory, key, sizeof(char) * key_length);
     new_key -> key = key_memory;

     value_length = strlen(value) + 1;
     value_memeroy = kmalloc(sizeof(char) * value_length, GFP_KERNEL);
     memcpy(value_memeroy, value, sizeof(char) * value_length);
     new_key -> value = value_memeroy;


     return new_key;
}


static struct lock* create_lock(struct key* key, char* owner)
{
	 int owner_length;
	 char* owner_memery;
     struct lock* new_lock;

     new_lock = kmalloc(sizeof(struct lock), GFP_KERNEL);

     owner_length = strlen(owner) + 1;
     owner_memery = kmalloc(sizeof(char) * owner_length, GFP_KERNEL);
     memcpy(owner_memery, owner, sizeof(char) * owner_length);
     new_lock -> owner = owner_memery;
     new_lock -> key   = key;

     return new_lock;
}


static struct key_node* create_key_node(struct key* key)
{
	struct key_node* new_node =  kmalloc(sizeof(struct key_node), GFP_KERNEL);
	new_node -> data = key;
	new_node -> next = NULL;
	return new_node;
}

static struct lock_node* create_lock_node(struct lock* lock)
{
	struct lock_node* new_node =  kmalloc(sizeof(struct lock_node), GFP_KERNEL);
	new_node -> data = lock;
	new_node -> next = NULL;
	return new_node;
}


static void insert_key(struct key* key)
{
    struct key_node* new_node = create_key_node(key);
    if (key_tail != NULL)
    {
       key_tail->next = new_node;
    }
    if (key_head == NULL)
    {
       key_head = new_node;
    }
    key_tail = new_node;
    return;
}


static void insert_lock(struct lock* lock)
{
    struct lock_node* new_node = create_lock_node(lock);
    if (lock_tail != NULL)
    {
       lock_tail->next = new_node;
    }
    if (lock_head == NULL)
    {
       lock_head = new_node;
    }
    lock_tail = new_node;
    return;
}

static void make_test_keys(void)
{
    char first_key[]             = "test_key";
    char first_key_value[]       = "serv";
    char second_key[]            = "test2_key";
    char second_key_value[]      = "123";
    insert_key(create_key(first_key, first_key_value));
    insert_key(create_key(second_key, second_key_value));
}

static void make_test_lock(struct key* key)
{
    char first_owner[]             = "Pentagon";

    insert_lock(create_lock(key, first_owner));
}

static int __init ofcd_init(void) /* Constructor */
{
  printk(KERN_INFO "Namaskar: ofcd registered");
  if (alloc_chrdev_region(&first, 0, 1, "Dmitree") < 0)
  {
    return -1;
  }
  
  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  if (device_create(cl, NULL, first, NULL, "ckv") == NULL)
  {
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  cdev_init(&c_dev, &pugs_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    return -1;
  }
  
  make_test_keys();
  make_test_lock(key_head->data);
  return 0;
}
 
static void __exit ofcd_exit(void) /* Destructor */
{
  cdev_del(&c_dev);
  device_destroy(cl, first);
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  printk(KERN_INFO "Alvida: ofcd unregistered");
}
 
module_init(ofcd_init);
module_exit(ofcd_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anil Kumar Pugalia <email_at_sarika-pugs_dot_com>");
MODULE_DESCRIPTION("Our First Character Driver");
