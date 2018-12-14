CC = c99
CFLAGS = -g -Wall -Wextra -pedantic
OBJS = k.o

PREFIX = /usr/local/share
INSTALLPATH = $(PREFIX)/kfreestyle2d/
GROUP = uinput
GROUPADD_PATH = /usr/sbin/groupadd

kfreestyle2d: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o kfreestyle2d

systemd: kfreestyle2d.service.template
	cat kfreestyle2d.service.template | sed 's|<<<PREFIX>>>|$(PREFIX)|g' \
	| sed 's|<<<GROUP>>>|$(GROUP)|g' > /etc/systemd/system/kfreestyle2d.service

# Create a copy of the udev rules 
udev-rule: ./99-kfreestyle2d.rules.template
	cat 99-kfreestyle2d.rules.template | sed 's|<<<GROUP>>>|$(GROUP)|g' \
	> /etc/udev/rules.d/99-kfreestyle2d.rules

# Create a copy of the script in the prefix directory
script: ./sort-and-run.sh.template directory
	cat ./sort-and-run.sh.template | sed 's|<<<PREFIX>>>|$(PREFIX)|g' \
	> $(INSTALLPATH)/sort-and-run.sh
	chmod +x $(INSTALLPATH)/sort-and-run.sh
	chgrp $(GROUP) $(INSTALLPATH)/sort-and-run.sh

# Add a uinput user to the system
group:
	$(GROUPADD_PATH) -f uinput
	id -u uinput > /dev/null 2>&1 || useradd --system --no-create-home --shell /bin/false -g uinput uinput > /dev/null

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
	systemctl daemon-reload
	udevadm control --reload
	udevadm trigger

# Insert the uinput kernel module and ensure that it is inserted on startup
module:
	grep -e "uinput" /etc/modules > /dev/null 2>&1 || echo "uinput" >> /etc/modules
	modprobe uinput
	
install: group systemd udev-rule script binary module refresh
	
