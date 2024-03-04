#ifndef APP_H
#define APP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#define FILE_NAME "/dev/adxl345-0"
#define WR_VALUE _IOW(10, 0, char)   // (type,nr,size) - The default major number of all the misc drivers is 10.
#define RWR_VALUE _IOWR(10, 2, char) // (type,nr,size) - The default major number of all the misc drivers is 10.
#define X_IOCTL 0
#define Y_IOCTL 1
#define Z_IOCTL 2

#define FINAL_VERSION
// #define IOCTL_V2
// #define DEBUG

#ifdef IOCTL_V2
struct ioctl_data
{
    char write_data[2];
    char read_data[2];
};
#endif

#endif