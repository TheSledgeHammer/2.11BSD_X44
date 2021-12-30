FreeBSD Port of evdev:

FreeBSD Evdev API's:
- uinput.h
- input.h
- input-event-codes.h
- freebsd_bitstring.h

TODO:
- replace evdev client buffer with the wsevent buffer: 
	- fix EVDEV_CLIENT_SIZEQ
	- evdev client buffer_size: not accounted for
- open, close, read, write: should be fixed once above is in place.
- input_events to wscon_events: handled on the fly (hopefully! if all goes well)
- keyboard & mouse: evdev input mapping to wscons mapping 
	- Two approaches:
		- 1) Two seperate keyboard & mouse maps. The default (wscons) and an evdev map.
		Toggling wscons with evdev mappings using configs or wscons userspace controls.
		- 2) Allow wscons existing maps to import/translate evdev maps.
		Both evdev and wscons use similar methods implement their mappings.
