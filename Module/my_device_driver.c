/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Siarhei Pushkin
* GitHub-Name:: spushkin
* Project:: Assignment 6 – Coding A Device Driver
*
* File:: my_device_driver.c
*
* Description:: A simple Linux device driver for encrypting and 
* decrypting strings using a Caesar cipher. This module creates 
* a character device that supports reading, writing, and ioctl 
* operations for setting encryption keys and operation modes. 
* It demonstrates basic kernel module functionality, including 
* registering and unregistering a device, and handling user-space
* interactions via file operations.
**************************************************************/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/device.h>

#define DEVICE_NAME "my_device"
#define CLASS_NAME  "my_device_class"
#define IOCTL_SET_ENCRYPTION_KEY _IOW('a', 'a', char*)
#define IOCTL_SET_OPERATION_MODE _IOW('a', 'b', int)

static int major_number;
static char *encryption_key = NULL;
static int operation_mode = 0; // 0 = encrypt, 1 = decrypt
static struct class *my_device_class = NULL;
static struct device *my_device_device = NULL;

// Kernel buffer for data transfer
static char kernel_buffer[100]; 

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static long device_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
};

// Simple Caesar cipher shift amount
#define CAESAR_SHIFT 3

static void caesar_cipher(char *data, size_t len, int encrypt) {
    size_t i;
    for (i = 0; i < len; i++) {
        if (encrypt) {
            // Encrypt by shifting forward
            data[i] = data[i] + CAESAR_SHIFT; 
        } else {
            // Decrypt by shifting backward
            data[i] = data[i] - CAESAR_SHIFT; 
        }
    }
}

static int __init my_device_init(void) {

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register device: %d\n", major_number);
        return major_number;
    }
    printk(KERN_INFO "Device registered %d\n", major_number);

    my_device_class = class_create(CLASS_NAME);

    if (IS_ERR(my_device_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create device class\n");
        return PTR_ERR(my_device_class);
    }

    my_device_device = device_create(my_device_class, NULL, 
                                        MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(my_device_device)) {
        class_destroy(my_device_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create device\n");
        return PTR_ERR(my_device_device);
    }

    return 0;
}

static void __exit my_device_exit(void) {
    device_destroy(my_device_class, MKDEV(major_number, 0));
    class_unregister(my_device_class);
    class_destroy(my_device_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "Device unregistered\n");
    kfree(encryption_key);  // Free the allocated encryption key
}

static int device_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int device_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    printk(KERN_INFO "Device read\n");

    // Adjust read length to prevent overflow
    size_t msg_len = strlen(kernel_buffer);
    if (len > msg_len) {
        len = msg_len;
    }

    // Copy the data from kernel buffer to user buffer
    if (copy_to_user(buffer, kernel_buffer, len)) {
        printk(KERN_ALERT "Failed to send data to user\n");
        return -EFAULT;
    }

    return len; // Return the number of bytes read
}

static ssize_t device_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    printk(KERN_INFO "Device write\n");

    // Ensure the buffer does not overflow
    if (len > sizeof(kernel_buffer) - 1) {
        len = sizeof(kernel_buffer) - 1;
    }

    // Copy the data from user buffer to kernel buffer
    if (copy_from_user(kernel_buffer, buffer, len)) {
        printk(KERN_ALERT "Failed to receive data from user\n");
        return -EFAULT;
    }
    kernel_buffer[len] = '\0'; // Null-terminate the string

    // Perform encryption or decryption based on the mode
    caesar_cipher(kernel_buffer, len, operation_mode == 0); // Pass mode for encryption/decryption

    return len; // Return the number of bytes written
}

static long device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    char user_key_buffer[256];  // Buffer to store the user-provided key

    switch (cmd) {
        case IOCTL_SET_ENCRYPTION_KEY:
            // Free any previously allocated key
            kfree(encryption_key);
            encryption_key = NULL;

            // Copy the key from user space
            if (copy_from_user(user_key_buffer, (char __user *)arg, sizeof(user_key_buffer) - 1)) {
                printk(KERN_ALERT "Failed to copy encryption key from user space\n");
                return -EFAULT;
            }

            // Null-terminate to prevent overflow
            user_key_buffer[sizeof(user_key_buffer) - 1] = '\0';

            // Allocate memory for the new key
            encryption_key = kmalloc(strlen(user_key_buffer) + 1, GFP_KERNEL);
            if (encryption_key == NULL) {
                printk(KERN_ALERT "Failed to allocate memory for encryption key\n");
                return -ENOMEM;
            }

            // Copy the key to kernel space
            strcpy(encryption_key, user_key_buffer);

            printk(KERN_INFO "Encryption key set: %s\n", encryption_key);
            break;

        case IOCTL_SET_OPERATION_MODE:
            if (arg != 0 && arg != 1) {
                printk(KERN_ALERT "Invalid operation mode: %lu\n", arg);
                return -EINVAL;
            }
            operation_mode = arg;
            printk(KERN_INFO "Operation mode set to: %d\n", operation_mode);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siarhei Pushkin");
MODULE_DESCRIPTION("Super simple Caesar cipher Linux device driver");
