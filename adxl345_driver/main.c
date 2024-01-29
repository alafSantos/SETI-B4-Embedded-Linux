#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main()
{
    int dev;
    char buffer[2];
    ssize_t bytes_read;

    // Open the device file
    dev = open("/dev/adxl345-0", O_RDONLY);
    if (dev < 0)
    {
        perror("Failed to open the device file");
        return ENOENT;
    }

    // Read from the device
    bytes_read = read(dev, buffer, sizeof(buffer));
    if (bytes_read < 0)
    {
        perror("Failed to read from the device");
        close(dev);
        return EFAULT;
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
