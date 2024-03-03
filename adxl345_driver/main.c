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

// #define IOCTL_V2

#ifdef IOCTL_V2
struct ioctl_data
{
    char write_data[2];
    char read_data[2];
};
#endif

int main()
{
#ifndef IOCTL_V2
    int dev;
    int dev_change;
    char buffer[2];
    ssize_t bytes_read;
    char axis;
    char selected_axis;
    char samples = 20;

    printf("Choose an axis: ");
    scanf("%c", &axis);

    if (axis == 'x' || axis == 'X')
        selected_axis = X_IOCTL;
    else if (axis == 'y' || axis == 'Y')
        selected_axis = Y_IOCTL;
    else if (axis == 'z' || axis == 'Z')
        selected_axis = Z_IOCTL;
    else
    {
        printf("Failed to read an invalid axis");
        return -1;
    }

    // Open the device file
    dev = open(FILE_NAME, O_RDONLY);
    if (dev < 0)
    {
        perror("Failed to open the device file");
        return -ENOENT;
    }

    // Change the axis through ioctl system
    dev_change = ioctl(dev, WR_VALUE, selected_axis);
    if (dev_change < 0)
    {
        printf("Failed to set up the device file\n");
        close(dev);
        return -EIO;
    }

    for (int i = 0; i < samples; i++)
    {
        printf("------I%d------\n", i);
        // Read from the device
        bytes_read = read(dev, buffer, sizeof(buffer));
        if (bytes_read < 0)
        {
            perror("Failed to read from the device");
            close(dev);
            return -EFAULT;
        }

        // Print the data read from the device
        __int16_t result = buffer[0] | buffer[1] << 8;
        printf("buffer[0] = %x\n", buffer[0]);
        printf("buffer[1] = %x\n", buffer[1]);
        printf("Value = %d\n", result);

        usleep(100000);
    }

    // Close the device file
    close(dev);
#endif

#ifdef IOCTL_V2
    int dev;
    ssize_t bytes_read;
    char axis;
    char selected_axis = X_IOCTL;
    char samples = 4;
    char number_of_bytes = 2;

    struct ioctl_data wr;

    printf("Choose an axis: ");
    scanf("%c", &axis);

    if (axis == 'x' || axis == 'X')
        selected_axis = X_IOCTL;
    else if (axis == 'y' || axis == 'Y')
        selected_axis = Y_IOCTL;
    else if (axis == 'z' || axis == 'Z')
        selected_axis = Z_IOCTL;
    else
    {
        printf("Failed to read an invalid axis");
        return -1;
    }

    // Open the device file
    dev = open(FILE_NAME, O_RDONLY);
    if (dev < 0)
    {
        perror("Failed to open the device file");
        return -ENOENT;
    }

    for (int i = 0; i < samples; i++)
    {
        printf("------I%d------\n", i);

        wr.write_data[0] = selected_axis;
        wr.write_data[1] = number_of_bytes;

        //        printf("app side %d %d\n", wr.write_data[0], wr.write_data[1]);

        bytes_read = ioctl(dev, RWR_VALUE, &wr);

        if (bytes_read < 0)
        {
            printf("Failed to set up the device file\n");
            close(dev);
            return -EIO;
        }

        // Print the data read from the device
        __int16_t result = wr.read_data[0] | wr.read_data[1] << 8;
        printf("buffer[0] = %x\n", wr.read_data[0]);
        printf("buffer[1] = %x\n", wr.read_data[1]);
        printf("Value = %d\n", result);

        usleep(100000);
    }

    // Close the device file
    close(dev);
#endif

    return 0;
}
