CC = c99
CFLAGS = -g -Wall -Wextra -pedantic
OBJS = k.o

PREFIX = /usr/local/share
INSTALLPATH = $(PREFIX)/kfreestyle2d/

kfreestyle2d: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o kfreestyle2d

systemd: kfreestyle2d@.service.template
	cat kfreestyle2d@.service.template | sed 's|<<<PREFIX>>>|$(PREFIX)|g' \
	| sed 's|<<<GROUP>>>|$(GROUP)|g' > /etc/systemd/system/kfreestyle2d@.service

systemd-uinput: uinput-load.service.template
	cat uinput-load.service.template > /etc/systemd/system/uinput-load.service

# Create a copy of the udev rules 
udev-rule: ./60-kfreestyle2d.rules
	cp 60-kfreestyle2d.rules /etc/udev/rules.d/60-kfreestyle2d.rules

# Ensure the existence of a directory within the prefix location
directory:
	mkdir $(INSTALLPATH) || true # ensure doesn't crash if already exists

# Copy the binary to its new home. Unlink any existing file first in case the
# service is already running.
binary: directory kfreestyle2d
	rm -f $(INSTALLPATH)/kfreestyle2d
	cp kfreestyle2d $(INSTALLPATH)/kfreestyle2d
	chgrp $(GROUP) $(INSTALLPATH)/kfreestyle2d

# Make systemd and udev notice their new configurations
refresh:
	systemctl enable uinput-load
	systemctl daemon-reload
	udevadm control --reload
	udevadm trigger

# Insert the uinput kernel module and ensure that it is inserted on startup
module:
	grep -e "uinput" /etc/modules > /dev/null 2>&1 || echo "uinput" >> /etc/modules
	modprobe uinput
	
install: group systemd systemd-uinput udev-rule binary module refresh
	
