FreeBSD Port of evdev:

FreeBSD Evdev API's:
- uinput.h
- input.h
- input-event-codes.h
- freebsd_bitstring.h /* Only needed if using FreeBSD evdev API's */

TODO:
- Check wsmouse & wskbd evdev support is correct
- fix config param opt_evdev, causes conflicts in wscons when enabled.
	

		