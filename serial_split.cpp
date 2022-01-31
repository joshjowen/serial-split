//  serial_split.cpp
//
//  Connects two serial ports  or a serial port and a pseudo terminal
//  and provides a pseudo terminal that allows third party access to the
//  communications.
//
//  Created by Joshua Owen on 12/10/18

#include "serial_split.h"

using namespace std;

dev_struct inputDevice, outputDevice, virtualDevice;

void print_usage()
{
    cout << "\nusage: serial_split [options]\n\noptions:"
         << "\n\t-i    - input device"
         << "\n\t-o    - output device (default: create virtual)"
         << "\n\t-s    - symlink location for output device (default: just use /dev/pts/[N])"
         << "\n\t-v    - symlink location for virtual device (default: just use /dev/pts/[N])"
         << "\n\t-b    - baudrate (default '9600')"
         << "\n\t-p    - parity (default 'none')"
         << "\n\t-d    - sends both outputs to virtual device (default 'input device only')"
         << "\n\t-c    - Don't intercept just connect 2 devices."
         << "\n\t-w    - Enable writing from virtual device to input device (default 'false'))"
         << "\n\n";
}

int open_port(dev_struct *device)
{
    device->fd = open(device->name, O_RDWR | O_NOCTTY | O_NDELAY); //  open serial connection like a file

    if (device->fd == -1) //  test if serial connection worked
    {
        perror("open_port: Unable to open specified port"); //  if connection failed this tells why
    }
    else
    {
        fcntl(device->fd, F_SETFL, 0); // set blocking read
        tcgetattr(device->fd, &device->termOptions);
        cfsetispeed(&device->termOptions, device->baudrate); // set input baudrate
        cfsetospeed(&device->termOptions, device->baudrate); // set output baudrate
        device->termOptions.c_cflag &= ~(PARENB | PARODD);
        device->termOptions.c_cflag |= CS8;

        if (device->parity)
            device->termOptions.c_cflag |= PARENB;

        if (device->parity == PARITY_ODD)
            device->termOptions.c_cflag |= PARODD;

        device->termOptions.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
        device->termOptions.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
        device->termOptions.c_oflag = ~OPOST;

        tcsetattr(device->fd, TCSAFLUSH, &device->termOptions);
    }

    return (device->fd);
}

int open_pty(dev_struct *device)
{
    if ((device->fd = getpt()) < 0 || grantpt(device->fd) < 0 || unlockpt(device->fd) < 0 || !(device->name = ptsname(device->fd)))
    {
        perror("open_pty: Unable to open a pseudo terminal");
        exit(-1);
    }
    fcntl(device->fd, F_SETFL, 0); // set blocking read
    tcgetattr(device->fd, &device->termOptions);

    device->termOptions.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
    device->termOptions.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
    device->termOptions.c_oflag = ~OPOST;

    tcsetattr(device->fd, TCSAFLUSH, &device->termOptions);

    return (device->fd);
}

void transmit(int infd, int outfd, int virtfd)
{
    unsigned char buffer[255];
    int readSize;
    int ignored __attribute__((unused));

    if ((readSize = read(infd, buffer, 255)) < 0)
    {
    }
    else
    {
        if (readSize > 0)
        {
            tcflush(outfd, TCOFLUSH);
            ignored = write(outfd, buffer, readSize);
            if (virtfd >= 0)
            {
                tcflush(virtfd, TCOFLUSH);
                ignored = write(virtfd, buffer, readSize);
            }
        }
        else
        {
            exit(1);
        }
    }
}

void close_ports(int s)
{
    cout << "Closing Ports" << endl;
    if (inputDevice.fd)
        close(inputDevice.fd);
    if (outputDevice.fd)
        close(outputDevice.fd);
    if (virtualDevice.fd)
        close(virtualDevice.fd);

    cout << "Ports Closed" << endl;
    exit(1);
}

int main(int argc, char *argv[])
{
    // Set default values for ports
    inputDevice.name = outputDevice.name = virtualDevice.name = NULL;
    inputDevice.fd = outputDevice.fd = virtualDevice.fd = -1;
    inputDevice.baudrate = outputDevice.baudrate = B9600;
    inputDevice.parity = PARITY_NONE;

    // set signal handler
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = close_ports;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    int optret; // return value for command line options
    int i;      // Counter for while loops

    bool dualWrite = false;
    bool bridge = false;
    bool writeBack = false;

    std::string symlink_name, vsymlink_name;

    // Get command line options
    while ((optret = getopt(argc, argv, "b:p:i:o:s:v:wdch?")) != -1)
    {
        switch (optret)
        {
        // Set baudrate
        case 'b':
            // Find the baud rate that corresponds to the argument value
            i = 0;
            while (baudRates[i].baudString && strcmp(optarg, baudRates[i].baudString))
                i++;

            if (baudRates[i].baudString)
            {
                inputDevice.baudrate = baudRates[i].baud;
                outputDevice.baudrate = baudRates[i].baud;
            }
            break;

        // Set parity
        case 'p':

            if (strcmp(optarg, "none") == 0)
            {
                inputDevice.parity = PARITY_NONE;
                outputDevice.parity = PARITY_NONE;
            }
            else if (strcmp(optarg, "even") == 0)
            {
                inputDevice.parity = PARITY_EVEN;
                outputDevice.parity = PARITY_EVEN;
            }
            else if (strcmp(optarg, "odd") == 0)
            {
                inputDevice.parity = PARITY_ODD;
                outputDevice.parity = PARITY_ODD;
            }
            break;

        // Set input device
        case 'i':
            inputDevice.name = optarg;
            break;

        // Set output device
        case 'o':
            outputDevice.name = optarg;
            break;
        case 's':
            symlink_name = std::string(optarg);
            break;
        case 'v':
            vsymlink_name = std::string(optarg);
            break;
        case 'd':
            dualWrite = true;
            break;
        case 'c':
            bridge = true;
            break;
        case 'w':
            writeBack = true;
            break;

        case 'h':
        case '?':
        default:
            print_usage();
            return -1;
        }
    }

    // exit if device names were not provided
    if (!inputDevice.name)
    {
        print_usage();
        return -1;
    }

    // open input device
    if (open_port(&inputDevice) < 0)
    {
        perror("Failed to open input device");
        return -1;
    }

    // open output device
    if (!outputDevice.name)
    {
        if (open_pty(&outputDevice) < 0)
        {
            perror("Failed to open virtual output device");
            return -1;
        }

        if (!symlink_name.empty())
        {
            remove(symlink_name.c_str());
            if (symlink(outputDevice.name, symlink_name.c_str()) < 0)
            {
                perror("Failed to create output device symlink");
                return -1;
            }

            cout << "Output: " << symlink_name.c_str() << endl;
        }
        else
        {
            cout << "Output: " << outputDevice.name << endl;
        }

    }
    else
    {
        if (open_port(&outputDevice) < 0)
        {
            perror("Failed to open output device");
            return -1;
        }
    }

    if (!bridge)
    {
        if (open_pty(&virtualDevice) < 0)
        {
            perror("Failed to open virtual device\n\n");
            return -1;
        }
        else
        {
            if (!vsymlink_name.empty())
            {
                remove(vsymlink_name.c_str());
                if (symlink(virtualDevice.name, vsymlink_name.c_str()) < 0)
                {
                    perror("Failed to create virtual device symlink");
                    return -1;
                }

                cout << "Virtual: " << vsymlink_name.c_str() << endl;
            }
            else
            {
                cout << "Virtual: " << virtualDevice.name << endl;
            }
        }
    }

    fd_set rset;
    int maxfd = max(inputDevice.fd, outputDevice.fd);
    int vfd = dualWrite ? virtualDevice.fd : -1;
    int ofd = bridge ? virtualDevice.fd : vfd;

    while (1)
    {
        FD_ZERO(&rset);
        FD_SET(inputDevice.fd, &rset);
        FD_SET(outputDevice.fd, &rset);
        if (writeBack && virtualDevice.fd > 0)
        {
            FD_SET(virtualDevice.fd, &rset);
            maxfd = max(maxfd, virtualDevice.fd);
        }

        select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(inputDevice.fd, &rset))
        {
            transmit(inputDevice.fd, outputDevice.fd, vfd);
        }
        if (FD_ISSET(outputDevice.fd, &rset))
        {
            transmit(outputDevice.fd, inputDevice.fd, ofd);
        }
        if (writeBack && FD_ISSET(virtualDevice.fd, &rset))
        {
            transmit(virtualDevice.fd, inputDevice.fd, -1);
        }
    }
    return 0;
}
