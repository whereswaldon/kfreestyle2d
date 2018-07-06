#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/uinput.h>

#define KINESIS_PACKET_SIZE 3

#define MUTE KEY_MUTE
#define VDWN KEY_VOLUMEDOWN
#define VUP  KEY_VOLUMEUP
#define CALC KEY_CALC
#define EJCT KEY_EJECTCLOSECD
#define PREV KEY_PREVIOUSSONG
#define PLAY KEY_PLAYPAUSE
#define NEXT KEY_NEXTSONG

// Maps from events received on the raw hid device to events sent out on uinput device
const unsigned char mapping[256] = {
    /*         0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    /* 0 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 1 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 2 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 3 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 4 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 5 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 6 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 7 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 8 */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 9 */    0,    0, CALC,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* A */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* B */    0,    0,    0, NEXT, PREV,    0,    0,    0, EJCT,    0,    0,    0,    0,    0,    0,    0,
    /* C */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, PLAY,    0,    0,
    /* D */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* E */    0,    0, MUTE,    0,    0,    0,    0,    0,    0,  VUP, VDWN,    0,    0,    0,    0,    0,
    /* F */    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

// Send the sequence of keypresses corresponding to pressing and releasing
// a single key
#define KINESIS_SEND_KEY(input_key, fd) emit((fd), (EV_KEY), (input_key), 1);\
    emit((fd), (EV_SYN), (SYN_REPORT), 0);\
    emit((fd), (EV_KEY), (input_key), 0);\
    emit((fd), (EV_SYN), (SYN_REPORT), 0)

// Send a single uinput event
void emit(int fd, int type, int code, int val) {
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    if (write(fd, &ie, sizeof(ie)) < 0) {
        perror("Error emitting keypress");
    }
}

// Assume fd is the file descriptor of a raw hid file for
// a Kinesis Freestyle 2
// Use UDEV4 style initialization
void watch(int rawfd) {

    // If we're on UInput 5, then we can use the new style initialization. Both structures have the
    // same fields that we care about, so they can beuse interchangeably here.
    #if UINPUT_VERSION>=5
    struct uinput_setup usetup;
    #else
    struct uinput_user_dev usetup;
    #endif

    int ioctl_err;
    // Open the uinput device file for writing
    // Ensure that our writes do not block
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Unable to open uinput");
        exit(4);
    }

    // Enable needed keys
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
        perror("Unable to configure virtual keyboard");
        exit(8);
    }

    for (int i = 0; i < 256; i++) {
        if (mapping[i] && ioctl(fd, UI_SET_KEYBIT, mapping[i]) < 0) {
            fprintf(stderr, "Unable to configure virtual key ID 0x%x: %s\n", mapping[i], strerror(errno));
            exit(8);
        }
    }

    // Configure virtual device
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x058f; // Alcor Micro Corp. Same as real keyboard
    usetup.id.product = 0x9410; // Keyboard
    strcpy(usetup.name, "KB800 Kinesis Freestyle");

    // Create virtual device
    #if UINPUT_VERSION>=5
    ioctl_err = ioctl(fd, UI_DEV_SETUP, &usetup);
    #else
    fprintf(stderr, "Warning: uinput is out of date, using old style initialization. This is unsupported and may be broken.\n");
    ioctl_err = write(fd, &usetup, sizeof(usetup));
    #endif
    if (ioctl_err < 0) {
        perror("Unable to finalize virtual device configuration");
        exit(16);
    }
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("Unable to create virtual device");
        exit(32);
    }

    unsigned char data[KINESIS_PACKET_SIZE];
    unsigned char key;

    memset(data, 0, sizeof(data));
    while (1) {
        if (read(rawfd, data, sizeof(data)) < 0) {
            perror("Error reading raw device");
            break;
        }

        key = mapping[data[1]];
        if (key) {
            KINESIS_SEND_KEY(key, fd);
        }
        else if (data[1]) {
            printf("Unknown HID Usage ID: 0x%x\n", data[1]);
        }
    }

    // Destroy virtual device
    if (ioctl(fd, UI_DEV_DESTROY) < 0) {
        perror("Unable to destroy virtual device");
        exit(32);
    }
    if (close(fd) < 0) {
        perror("Error closing uinput file");
        exit(32);
    }
}

int main(int argc, char* argv[]) {
    char * devpath;
    int rawfd;
    printf("Unofficial kinesis userspace driver\n");
    if (argc < 2) {
        printf("Usage: %s <device>\n", argv[0]);
        exit(1);
    }
    devpath = argv[1];

    rawfd = open(devpath, O_RDONLY);
    if (rawfd < 0) {
        perror("Unable to open raw device file");
        exit(2);
    }

    watch(rawfd);

    if (close(rawfd) < 0) {
        perror("Error closing raw device file");
        exit(2);
    }
    return 0;
}
