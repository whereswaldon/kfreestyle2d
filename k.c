#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/uinput.h>

#define KINESIS_PACKET_SIZE 3

#define HID_UID_MUTE 0xE2
#define HID_UID_VOL_DOWN 0xEA
#define HID_UID_VOL_UP 0xE9
#define HID_UID_CALC 0x92

// Send the sequence of keypresses corresponding to pressing and releasing
// a single key
#define KINESIS_SEND_KEY(input_key, fd) emit((fd), (EV_KEY), (input_key), 1);\
    emit((fd), (EV_SYN), (SYN_REPORT), 0);\
    emit((fd), (EV_KEY), (input_key), 0);\
    emit((fd), (EV_SYN), (SYN_REPORT), 0)

// Send specific key actions
#define KINESIS_MUTE(fd) KINESIS_SEND_KEY(KEY_MUTE, (fd))
#define KINESIS_QUIETER(fd) KINESIS_SEND_KEY(KEY_VOLUMEDOWN, (fd))
#define KINESIS_LOUDER(fd) KINESIS_SEND_KEY(KEY_VOLUMEUP, (fd))
#define KINESIS_CALC(fd) KINESIS_SEND_KEY(KEY_CALC, (fd))

// Send a single uinput event
void emit(int fd, int type, int code, int val) {
    struct input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
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
    
    // Open the uinput device file for writing
    // Ensure that our writes do not block
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    // Enable needed keys
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, KEY_MUTE);
    ioctl(fd, UI_SET_KEYBIT, KEY_CALC);
    ioctl(fd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
    ioctl(fd, UI_SET_KEYBIT, KEY_VOLUMEUP);

    // Configure virtual device
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x058f; // Alcor Micro Corp. Same as real keyboard
    usetup.id.product = 0x9410; // Keyboard
    strcpy(usetup.name, "KB800 Kinesis Freestyle");

    // Create virtual device
    #if UINPUT_VERSION>=5
    ioctl(fd, UI_DEV_SETUP, &usetup);
    #else
    fprintf(stderr, "Warning: uinput is out of date, using old style initialization. This is unsupported and may be broken.\n")
    write(fd, &usetup, sizeof(usetup));
    #endif
    ioctl(fd, UI_DEV_CREATE);

    unsigned char data[KINESIS_PACKET_SIZE];
    memset(data, 0, sizeof(data));
    while (1) {
        read(rawfd, data, sizeof(data));
        switch (data[1]) {
            case HID_UID_MUTE:
                KINESIS_MUTE(fd);
                break;
            case HID_UID_VOL_DOWN:
                KINESIS_QUIETER(fd);
                break;
            case HID_UID_VOL_UP:
                KINESIS_LOUDER(fd);
                break;
            case HID_UID_CALC:
                KINESIS_CALC(fd);
                break;
            case 0:
                break;
            default:
                printf("Unknown HID Usage ID: 0x%x\n", data[1]);
        }
    }

    // Destroy virtual device
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
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

    watch(rawfd);

    close(rawfd);
    return 0;
}
