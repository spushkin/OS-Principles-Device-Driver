/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Siarhei Pushkin
* GitHub-Name:: spushkin
* Project:: Assignment 6 – Coding A Device Driver
*
* File:: Pushkin_Siarhei_HW6_main.c
*
* Description:: Test application for interacting with the 
* my_device_driver. This application opens the device, sets an 
* encryption key and mode using ioctl commands, and performs read 
* and write operations to demonstrate the device's encryption and 
* decryption capabilities. It serves as an example of how to use 
* the driver from user space.
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define DEVICE_PATH "/dev/my_device"
#define IOCTL_SET_ENCRYPTION_KEY _IOW('a', 'a', char*)
#define IOCTL_SET_OPERATION_MODE _IOW('a', 'b', int)

void set_encryption_key(int fd, const char *key) {
    if (ioctl(fd, IOCTL_SET_ENCRYPTION_KEY, key) == -1) {
        perror("Failed to set encryption key");
    }
}

void set_operation_mode(int fd, int mode) {
    if (ioctl(fd, IOCTL_SET_OPERATION_MODE, mode) == -1) {
        perror("Failed to set operation mode");
    }
}

void write_string(int fd, const char *str) {
    if (write(fd, str, strlen(str)) == -1) {
        perror("Failed to write string to device");
    }
}

void read_string(int fd, char *buffer, size_t len) {
    if (read(fd, buffer, len) == -1) {
        perror("Failed to read string from device");
    }
}

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device...");
        return -1;
    }

    set_encryption_key(fd, "my_secret_key"); // Set encryption key
    set_operation_mode(fd, 0);               // Set to encrypt mode

    char *input_str = "Hello, World!";
    write_string(fd, input_str);

    char encrypted_str[100] = {0}; // Initialize buffer
    read_string(fd, encrypted_str, sizeof(encrypted_str));
    printf("Encrypted string: %s\n", encrypted_str);

    set_operation_mode(fd, 1); // Set to decrypt mode
    write_string(fd, encrypted_str);

    char decrypted_str[100] = {0}; // Initialize buffer
    read_string(fd, decrypted_str, sizeof(decrypted_str));
    printf("Decrypted string: %s\n", decrypted_str);

    close(fd);
    return 0;
}
