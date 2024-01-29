#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#define FILE_NAME "/dev/adxl345-0"
#define WR_VALUE _IOW(10, 0, char)
#define X_IOCTL 0
#define Y_IOCTL 1
#define Z_IOCTL 2

int main()
{
    int dev;
    int dev_change;
    char buffer[2];
    ssize_t bytes_read;
    char axis;
    char selected_axis;

    printf("Choose an axis: ");
    scanf("%c", &axis);

    if (axis == 'x' || axis == 'X')
        selected_axis = X_IOCTL;
    else if (axis == 'y' || axis == 'Y')
        selected_axis = Y_IOCTL;
    else if (axis == 'z' || axis == 'Z')
        selected_axis = Z_IOCTL;
    else{
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
        return -EIO;
    }

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

    // Close the device file
    close(dev);

    return 0;
}
