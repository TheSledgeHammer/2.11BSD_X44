FreeBSD Port of evdev:

FreeBSD Evdev API's:
- uinput.h
- input.h
- input-event-codes.h
- freebsd_bitstring.h

TODO:
- Changes:
- Will No longer be a seperate character device.
	- evdev routines are sub-routines to the keyboard & mouse
	when configured.
- Evdev is a config option like FreeBSD & DragonFlyBSD (option EVDEV_SUPPORTED)
- Removes need for seperate keyboard & mouse mapping
- Removes duplicate methods between wscons and the evdev.
	

		