//  serial_split.h
//  
//  Connects two serial ports and provides a psuedo terminal that allows
//  Third party access to the communications
//
//  Created by Joshua Owen on 12/10/18

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

typedef struct baud_struct
{
    speed_t baud;
    char *baudString;
} baud_struct;

static baud_struct baudRates[] = {
    {B50,     "50"},
    {B75,     "75"},
    {B110,    "110"},
    {B134,    "134"},
    {B150,    "150"},
    {B200,    "200"},
    {B300,    "300"},
    {B600,    "600"},
    {B1200,   "1200"},
    {B1800,   "1800"},
    {B2400,   "2400"},
    {B4800,   "4800"},
    {B9600,   "9600"},
    {B19200,  "19200"},
    {B38400,  "38400"},
    {B57600,  "57600"},
    {B115200, "115200"},
    {0,       NULL}
};

enum
{
    PARITY_NONE = 0,
    PARITY_ODD,
    PARITY_EVEN,
};

typedef struct dev_struct
{
    char*          name;
    int            fd;
    struct termios termOptions;
    speed_t        baudrate;
    int            parity; 
} dev_struct;

