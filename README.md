# kfreestyle2d
Unofficial Kinesis Freestyle 2 Userspace Linux Driver

**SECURITY NOTICE**: This program should not run as root! Read the Install instructions for details on how to set the correct permissions on the device files that the driver opens so that it can do so with normal user permissions.

The Kinesis Freestyle 2 keyboard is a fantastic split keyboard for people
who don't want RSI in their wrists. It has four multimedia keys on
F8-F11 that do not work in Linux. This is a simple program that enables
those keys. If you care how it works, keep reading. If not, skip to the
Build instructions.

## Origin

I spent a weekend digging into how this keyboard sent input for these multimedia
keys. I documented the process in a series of blog posts:

- [Part 1 - Debugging the problem](https://waldon.blog/2017/10/31/kinesis-freestyle-2-and-linux-part-1-debugging/)
- [Part 2 - Fix it in userspace](https://waldon.blog/2017/10/31/kinesis-freestyle-2-and-linux-part-2-the-userspace-fix/)
- [Part 3 - Permissions](https://waldon.blog/2017/11/15/kinesis-freestyle-2-and-linux-part-3-permissions/)
- [Part 4 - Automation](https://waldon.blog/2018/02/20/kinesis-freestyle-2-and-linux-part-4-automation/)

## Supported Systems

`kfreestyle2d` itself only depends on the Linux kernel. If you run Linux with
`uinput` version 4 or later, you should be fine.

The automatic execution of `kfreestyle2d` currently depends on `udev` and `systemd`.
If you run a system that uses a different init system or device manager, we'd love
some help with automatic installation procedures on those systems.

## How it works

The multimedia keys seem not to be understood properly by the `hid-generic`
kernel module that usually interprets events from HID (Human Interface Devices).
They are understood by the `hidusb` kernel module though, so you can read the
raw HID file to get the stream of bytes coming from the USB device.

This program uses a special kernel interface called `uinput` that allows drivers
for input devices to run as user programs. kfreestyle2d simply opens the raw byte
stream from the keyboard's special function keys. When it reads a keypress, it
translates the HID Usage ID of the key that it read into a keypress that the
`uinput` system understands. This allows the kernel to interpret the keypress
correctly and dispatch it to userspace programs like Xorg properly.

## Build

`make` will create the binary `kfreestyle2d` in the current directory. If you want to try
it without installing it, skip to the Manual Run section below.

## Install

Please check the makefile before installing. You may wish to adjust the value of `PREFIX` or
one of the other macros.

To properly install `kfreestyle2d`, you should be able to simply run `sudo make install`.
This will:

- Create a directory (default is /usr/local/share/kfreestyle2d) to store the executable and script.
- Create some udev rules in /etc/udev/rules.d/99-kfreestyle2d.rules
- Create a new systemd service file in /etc/systemd/system/kfreestyle2d.service

Once those things are done, the driver should run automatically when you plug in the keyboard
and should stop when you unplug it. If you have any problems running this command, please open an
issue.

Please note that the current setup only supports one Kinesis Freestyle 2. If you want to
plug multiple Kinesis Freestyle 2 keyboards into the same computer, you'll need to 
customize the automatic execution accordingly.

## Manual Run

This procedure will work to test whether the userspace driver is working without
the `udev` and `systemd` automation in place. If you already followed the Install
directions, you shouldn't need to do this.

First, ensure that the `uinput` kernel module is loaded with `lsmod | grep uinput`. If
that does not return a line containing uinput, run `sudo modprobe uinput` to load
the userspace input module into your kernel.

The driver expects to be given the path to an HID raw input device
that corresponds to the special function keys of a Kinesis Freestyle 2.

To find the correct HID raw input device, unplug your keyboard and
`ls /dev/hidraw*`. Then plug it in and `ls /dev/hidraw*`.

There should be two new `/dev/hidrawX` files the second time.

Try `sudo head -c 1 /dev/hidrawA` where A is the first new raw input file.
Then type on your keyboard. If the command exits, it just read normal keyboard
input from that file, which means that it is **not** the special function key
raw input file. If the command does not exit, try typing one of the special
keys (mute, volumedown, volumeup, calculator). Remember to press the function
key before trying to use these. If the command exits when you type a special
key, this is the file you're looking for.

To start the driver, run:
`./kfreestyle2d /dev/hidrawX` where X is the number of the raw input device
for the special keys.

When the driver is running, it should create a new `/dev/input/eventXX` file,
where XX is some number with 1-3 digits. You can run `sudo evtest /dev/input/eventXX`
on that file to see the output generated by the driver when you press those keys.

If the driver isn't generating output, feel free to create an issue.

If it is, you should now be able to configure your programs to handle these
keys using ordinary X or Wayland key configuration.

## Contribute

Suggestions and PRs welcome!
