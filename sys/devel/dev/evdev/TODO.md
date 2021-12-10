FreeBSD Port of evdev:
Initil Port: (not modified)
- evdev.c:
- evdev_mt.c
- evdev_utils.c
- evdev_private.h

- cdev.c: (aka evdev device routines)
	- rename: cdev is a poor name choice when we support both cdevs & bdevs

Fixes:
- uinput.c: mostly rewritten
	- probe, attach, open & close routines
	- detach & activate??

