/**
 * @file   ebbchar.c
 * @author Derek Molloy
 * @date   7 April 2015
 * @version 0.1
 * @brief   An introductory character driver to support the second article of my series on
 * Linux loadable kernel module (LKM) development. This module maps to /dev/ebbchar and
 * comes with a helper C program that can be run in Linux user space to communicate with
 * this the LKM.
 * @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
 */

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Derek Molloy");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for the BBB");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char message[256] = {0};   ///< Memory for the string that is passed from userspace
//static char getmsg[256] = {0};
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

struct object{
   int id;
   char name[256];
   struct hlist_node node;
};

DEFINE_HASHTABLE(htable, 3);

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init ebbchar_init(void){
   printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "EBBChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);
   
   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");

   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
   //message = kmalloc(256*sizeof(char), GFP_KERNEL);
   //hash_init(a);
   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ebbchar_exit(void){
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ebbcharClass);                          // unregister the device class
   class_destroy(ebbcharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   int k;
   int dec=0;
  int bucket;
struct object *traverse;// = (struct object *)kmalloc(sizeof(struct object), GFP_KERNEL);
   for(k=4;k<strlen(message);k++){
	printk(KERN_INFO "READ FUNCTION: characters of message = %c", message[k]);
	dec = dec*10 + (message[k]-'0');	
}
//dec = 19009;
   printk(KERN_INFO "READ FUNCTION: Convert string %s to number=%d\n", message, dec);
   bucket = dec;
  printk(KERN_INFO "LKM is in READ with bucket value=%d\n", bucket);
  //traverse->node = (struct)kmalloc(sizeof(struct hlist_node), GFP_KERNEL);
  hash_for_each_possible(htable,traverse,node,bucket){
 //        if(traverse == NULL){
//		printk(KERN_INFO "Traverse is null");	
//	}
 
    //  printk(KERN_INFO "This is the key=%d", traverse->id); 
    //	printk(KERN_INFO "This is the bucket=%d", bucket);  
	if(traverse->id == bucket){
      //        printk(KERN_INFO "Found the object\n");
		strcpy(message, "");
		//printk(KERN_INFO "%s\n", message);
		strcpy(message, traverse->name);
                //printk(KERN_INFO "%s\n", message);
		//capture_it = traverse->name;
	}

  };

//   traverse=NULL;
   //strcpy(message, "");
   printk(KERN_INFO "The message is %s\n", message);
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, strlen(message));
   //strcpy(message, "");
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static const char *get_func(int find){

struct object *traverse;
int bucket;
bucket = find;
printk(KERN_INFO "Before hash each possible");
hash_for_each_possible(htable, traverse, node, bucket){
	if(traverse->id == bucket){
		return traverse->name;
	}
}
return NULL;
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
  // sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
int error;
int bkt;
struct object *curr;
struct object obj1;
char key[5];
char value[256];
char shubhamwat[256] = {0};
int flag = 0;
int ctr1 = 0;
int ctr2 = 0;
int i;
int j;
int k;
int num = 0;
//struct mystruct *first = (struct mystruct*)kmalloc(sizeof(struct mystruct), GFP_KERNEL);
//first->hash_list = (struct hlist_node *)kmalloc(sizeof(struct hlist_node), GFP_KERNEL);
printk(KERN_INFO "WRITE FUNCTION: The message at the beginning is %s", message); 
//printk(KERN_INFO "WRITE FUNCTION: The buffer says = %s", buffer);
//error = copy_from_user(shubhamwat, buffer, len);
error = copy_from_user(message, buffer, len);
//strcpy(message, shubhamwat);
printk(KERN_INFO "WRITE FUNCTION: The message after copy is %s", message);
printk(KERN_INFO "WRITE FUNCTION: The 'wat' string is now = %s", shubhamwat);
//printk(KERN_INFO "%s", message);
//if(strcmp(message, "Put")==0){
//printk(KERN_INFO "%s", message);
//strcpy(message, "");
//error = copy_from_user(message, buffer, len);
if(message[0]=='G' && message[1]=='e' && message[2]=='t'){
//curr = kmalloc(sizeof(struct object),GFP_KERNEL);
//	for(k=4;k<strlen(message);k++){
//		num = num*10 + (message[k]-'0');
//	}
//bkt=num;
//get_func(bkt);
/*
hash_for_each_possible(htable, curr, node, bkt){
	if(curr->id == bkt){
		printk(KERN_INFO "GET: Found it key=%d value=%s\n", bkt, curr->name);
	}		
}
*/
printk(KERN_INFO "WRITE FUNCTION: In Get option and string before returning is = %s", message);
return len;
}
for(i=4;i<strlen(message);i++){
	if(message[i] == ' ' || message[i]=='\0'){
	flag = 1;
	}	
       else if(flag == 0){
	key[ctr1] = message[i];
	ctr1++;
	}
	else if(flag == 1){
	value[ctr2] = message[i];
	ctr2++;
        }

}
key[ctr1] = '\0';
value[ctr2] = '\0';

for(j=0;j<strlen(key);j++){
	num = num*10 + (key[j] - '0');
}

//copy_to_user(buffer, "hi user space", strlen(message));
printk(KERN_INFO "Shubham says the message is key=%d and value=%s", num, value); 
obj1.id = num;
strcpy(obj1.name, value);
bkt = num;

//struct mystruct *first = (struct mystruct *)kmalloc(sizeof(struct mystruct), GFP_KERNEL);
//first->data = message[0] - '0';
//first->hash_list = (struct hlist_node *)kmalloc(sizeof(struct hlist_node), GFP_KERNEL);
//get_func(bkt);
hash_add_rcu(htable, &(obj1.node), obj1.id);

hash_for_each_possible(htable,curr,node,bkt){
	if(curr->id == bkt){
		printk(KERN_INFO "key=%d => %s\n", bkt, curr->name);
	}
}

//}
//else{
//	strcpy(message, "");
//	error = copy_from_user(message, buffer, len);	     
//	for(j=0;j<strlen(message);j++){
//		num = num*10 + (message[j]-'0');
//	}     
//	bkt = num;
//	hash_for_each_possible(htable, curr, node, bkt){
//		if(curr->id == bkt){
//			strcpy(message, "");
//			strcpy(message, curr->name);
//			copy_to_user(buffer,message, strlen(message));
//		}
//	}
//}
size_of_message = strlen(message);                 // store the length of the stored message
   printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
strcpy(message, "");
printk(KERN_INFO "WRITE FUNCTION: In Put option and string before returning = %s", message);
   return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(ebbchar_init);
module_exit(ebbchar_exit);
