#ifndef ADXL345_TP
#define ADXL345_TP

// REGISTERS
#define BW_RATE 0x2C
#define INT_ENABLE 0x2E
#define DATA_FORMAT 0x31
#define FIFO_CTL 0x38
#define POWER_CTL 0x2D
#define DATAX1 0x33
#define DATAX0 0x32
#define DATAY0 0x34
#define DATAY1 0x35
#define DATAZ0 0x36
#define DATAZ1 0x37

#define RATE_CODE_3200 0x0F
#define RATE_CODE_1600 0x0E
#define RATE_CODE_0800 0x0D
#define RATE_CODE_0400 0x0C
#define RATE_CODE_0200 0x0B
#define RATE_CODE_0100 0x0A

#define WR_VALUE _IOW(20, 0, char) // (type,nr,size)
#define X_IOCTL 0
#define Y_IOCTL 1
#define Z_IOCTL 2

#endif