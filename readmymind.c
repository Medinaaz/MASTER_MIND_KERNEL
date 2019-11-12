#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>   
#include <linux/slab.h>     
#include <linux/fs.h>     
#include <linux/errno.h>   
#include <linux/types.h>   
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/switch_to.h>     
#include <asm/uaccess.h>    
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include "readmymind_ioctl.h"

#define READMYMIND_MAJOR 0
#define READMYMIND_MAX_GUESS 4
#define READMYMIND_NUMBER_OF_DEVICES 2
#define READMYMIND_SECRET_NUMBER '1234'

int readmymind_major = READMYMIND_MAJOR;
int readmymind_minor = 0;
int readmymind_number_devices = READMYMIND_NUMBER_OF_DEVICES;
int readmymind_max_guess = READMYMIND_MAX_GUESS;
char *mmind_number = READMYMIND_SECRET_NUMBER;

module_param(readmymind_major, int, S_IRUGO);
module_param(readmymind_max_guess, int, S_IRUGO);
module_param(mmind_number, charp, S_IRUGO);

MODULE_AUTHOR("Mehmet Yakuphan Bilgic, Medina Zaganjori, Sabrina Cara");
MODULE_LICENSE("Dual BSD/GPL");

struct guess
{
    int minusNumber;
    int plusNumber;
    int guessInput;
    int guessOrder;
};

struct readmymind_dev
{
    struct guess **data; 
    struct cdev cdev;
    unsigned long size;
    int guessCount;
};

char *getGuessCount(struct readmymind_dev *obj){
    return obj->guessCount;
}

struct readmymind_dev *readmymind_devices;

char *intToChar(int input)
{
    switch(input){
        case 1:
            return '1';
        case 2:
            return '2';
        case 3:
            return '3';
        case 4:
            return '4';
        case 5:
            return '5';
        case 6:
            return '6';
        case 7:
            return '7';
        case 8:
            return '8';
        case 9:
            return '9';
        case 0:
            return '0';
    }

    return '0';
}

void deleteHistory(struct file *filp) 
{
    struct readmymind_dev *dev = filp->private_data;
    int index;
   
    int guessCountInt = dev->guessCount;

    for (index = 0; index < guessCountInt; index++)
    {
        kfree(dev->data[index]->minusNumber);
        kfree(dev->data[index]->plusNumber);
        kfree(dev->data[index]->guessInput);
    }

    kfree(dev->guessCount);
}

int readmymind_trim(struct readmymind_dev *dev)
{
    int i;
    int localGuessCount = dev->guessCount;

    if(dev->data){
        for(i = 0; i < localGuessCount; i++) {
            if(dev->data[i]){
                kfree(dev->data[i]);
            }
        }
        kfree(dev->data);
    }
    dev->data = NULL;
    dev->guessCount = 0;
    dev->size = readmymind_max_guess;
    return 0;   
}

int readmymind_open(struct inode *inode, struct file *filp)
{
    struct readmymind_dev *dev;
    dev = container_of(inode->i_cdev, struct readmymind_dev, cdev);
    filp->private_data = dev;
    
    return 0;
}

int readmymind_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t readmymind_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct readmymind_dev *dev = filp->private_data;
    struct guess *guessPtr = kmalloc(sizeof(struct guess), GFP_KERNEL);
    int guessPosition = (long)*f_pos;
    ssize_t retval = 0;
    int position = 0;
    int characterPosition = 0;
    int bufferPosition = 0;
    int intGuessCount;
    char *local_buffer;

    if (*f_pos >= dev->size)
        return retval;

    if (dev->data == NULL || ! dev->data[guessPosition])
        return retval;

        //256 * 16 = 4096
    local_buffer = kmalloc(256 * 16 * sizeof(char), GFP_KERNEL);
    memset(local_buffer, 0, 256 * 16 * sizeof(char));

    for (position = guessPosition; position < dev->guessCount; position++)
    {
        guessPtr = dev->data[position];

        char *plusNumberLocal = kmalloc(sizeof(char), GFP_KERNEL);
        plusNumberLocal = guessPtr->plusNumber + '0';

        int guessInputLocal = guessPtr->guessInput;
        int guessCountLocal = guessPtr->guessOrder;

        local_buffer[bufferPosition++] = intToChar(guessInputLocal / 1000);
        local_buffer[bufferPosition++] = intToChar((guessInputLocal % 1000) / 100);
        local_buffer[bufferPosition++] = intToChar((guessInputLocal % 100) / 10);
        local_buffer[bufferPosition++] = intToChar(guessInputLocal % 10);
        local_buffer[bufferPosition++] = ' ';
        local_buffer[bufferPosition++] = intToChar(guessPtr->plusNumber);
        local_buffer[bufferPosition++] = '+';
        local_buffer[bufferPosition++] = ' ';
        local_buffer[bufferPosition++] = intToChar(guessPtr->minusNumber);
        local_buffer[bufferPosition++] = '-';
        local_buffer[bufferPosition++] = ' ';
        local_buffer[bufferPosition++] = intToChar(guessCountLocal / 1000);
        local_buffer[bufferPosition++] = intToChar((guessCountLocal % 1000) / 100);
        local_buffer[bufferPosition++] = intToChar((guessCountLocal % 100) / 10);
        local_buffer[bufferPosition++] = intToChar(guessCountLocal % 10);
        local_buffer[bufferPosition++] = '\n';
    }
        
    if (copy_to_user(buf, local_buffer, bufferPosition)) {
        retval = -EFAULT;
        return retval;
    }

    *f_pos += bufferPosition;
    retval = bufferPosition;

    return retval;
}

ssize_t readmymind_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct readmymind_dev *dev = filp->private_data;    
    ssize_t retval = -ENOMEM;

    //not using f_pos to prevent overwrite
    int guessPosition =0;
    char *local_buffer = kmalloc(4 * sizeof(char), GFP_KERNEL);
    int *plusNumber = kmalloc(sizeof(int), GFP_KERNEL);
    int *minusNumber = kmalloc(sizeof(int), GFP_KERNEL);
    int guessInput;
    int localPlusCount = 0;
    int localMinusCount = 0;
    int i;
    int j;
    int localGuessCount;

    if(dev->size == dev->guessCount){
        return retval;
    }

    if (!dev->data) {
        dev->data = kmalloc(dev->size * sizeof(struct guess *), GFP_KERNEL);
        if (!dev->data){
            return retval;
        }
        else{
            memset(dev->data, 0, dev->size * sizeof(struct guess *));
        }
    }

    if (!dev->data[guessPosition]) {
        dev->data[guessPosition] = kmalloc(sizeof(struct guess), GFP_KERNEL);
        if (!dev->data[guessPosition]){
             return retval;
        }
    }

    local_buffer = kmalloc(4 * sizeof(char), GFP_KERNEL);

    if (copy_from_user(local_buffer, buf, count)) {
        retval = -EFAULT;
        return retval;    
    }
 
    for (i = 0; i < count; i++)
    {
        if(mmind_number[i] == local_buffer[i]){
            localPlusCount++;
        }
      
        for(j = 0; j < count; j++){
            if(mmind_number[i] == local_buffer[j]){
                if(i != j) {
                    localMinusCount++;
                }
            }
        }
    }

    minusNumber = localMinusCount;
    plusNumber= localPlusCount;

    int counter;
    int index;
    guessInput = 0;
    index = 3;
    counter = 1;
    
    for (index; index > -1; index--) {
        guessInput += counter * (local_buffer[index] - '0');
        counter = counter * 10;
    }
  
    localGuessCount = dev->guessCount;

    dev->data[localGuessCount] = kmalloc(sizeof(struct guess), GFP_KERNEL);
    
    dev->data[localGuessCount]->minusNumber = kmalloc(sizeof(int), GFP_KERNEL);
    dev->data[localGuessCount]->plusNumber = kmalloc(sizeof(int), GFP_KERNEL);
    dev->data[localGuessCount]->guessInput = kmalloc(sizeof(int), GFP_KERNEL);
    dev->data[localGuessCount]->guessOrder = kmalloc(sizeof(int), GFP_KERNEL);

    dev->data[localGuessCount]->minusNumber = minusNumber;
    dev->data[localGuessCount]->plusNumber = plusNumber;
    dev->data[localGuessCount]->guessInput= guessInput;
    dev->data[localGuessCount]->guessOrder = localGuessCount + 1;
    
    (*f_pos)++;
    
    dev->guessCount = localGuessCount + 1;

    retval = count;
    kfree(local_buffer);
  
    return retval;
}

long readmymind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    int retval = 0;
    struct readmymind_dev *dev ;
    dev = filp->private_data;


    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != READMYMIND_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > READMYMIND_IOC_MAXNR) return -ENOTTY;

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;

    switch(cmd) {
        case MMIND_REMAINING:
            retval = readmymind_max_guess - dev->guessCount;
            break;

        case MMIND_ENDGAME:
            deleteHistory(filp);
            break;

        case MMIND_NEWGAME:
            mmind_number = arg;
            break;
        //No need to have a default case since we checked cmd value before.            
    }

    return retval;
}

loff_t readmymind_llseek(struct file *filp, loff_t off, int whence)
{
    struct readmymind_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}

struct file_operations readmymind_fops = {
    .owner =    THIS_MODULE,
    .llseek =   readmymind_llseek,
    .read =     readmymind_read,
    .write =    readmymind_write,
    .unlocked_ioctl =  readmymind_ioctl,
    .open =     readmymind_open,
    .release =  readmymind_release
};

void readmymind_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(readmymind_major, readmymind_minor);

    if (readmymind_devices) {
        for (i = 0; i < readmymind_number_devices; i++) {
            readmymind_trim(readmymind_devices + i);
            cdev_del(&readmymind_devices[i].cdev);
        }
    kfree(readmymind_devices);
    }

    unregister_chrdev_region(devno, readmymind_number_devices);
}

int readmymind_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct readmymind_dev *dev;

    if (readmymind_major) {     
        devno = MKDEV(readmymind_major, readmymind_minor);
        result = register_chrdev_region(devno, readmymind_number_devices, "mastermind");
    } else {
        result = alloc_chrdev_region(&devno, readmymind_minor, readmymind_number_devices,
                                     "mastermind");
        readmymind_major = MAJOR(devno);
    }
    if (result < 0) {
        return result;
    }

    readmymind_devices = kmalloc(readmymind_number_devices * sizeof(struct readmymind_dev),
                            GFP_KERNEL);

    if (!readmymind_devices) {
        result = -ENOMEM;
        goto fail;
    }

    memset(readmymind_devices, 0, readmymind_number_devices * sizeof(struct readmymind_dev));

    for (i = 0; i < readmymind_number_devices; i++) {
        dev = &readmymind_devices[i];
        dev->size = readmymind_max_guess;
        dev->guessCount = 0;
        devno = MKDEV(readmymind_major, readmymind_minor + i);
        cdev_init(&dev->cdev, &readmymind_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &readmymind_fops;
        err = cdev_add(&dev->cdev, devno, 1);

        if (err){
            printk(KERN_NOTICE "Error %d adding mastermind%d", err, i);
        }
    }

    return 0;

  fail:
    readmymind_cleanup_module();
    return result;
}

module_init(readmymind_init_module);
module_exit(readmymind_cleanup_module); 