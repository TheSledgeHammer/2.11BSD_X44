FreeBSD Port of evdev:
Initil Port: (not modified)
- evdev.c:
- evdev_mt.c
- evdev_utils.c
- evdev_private.h

Changes:
- evdev events: 
	- all evdev events are managed via wscons event queue.
- evdev client:
	- keep buffer: (could be used in evdev_softc)
	- necessary? (api's or FreeBSD hooks?)
		- its main purpose is to recieve all evdev events invoked by the keyboard &
		mouse. which becomes reduntant if we're using the wscons event chain.	
- input:
	- necessary? (api's or FreeBSD hooks?)
		- duplicates: input_event, input_keymap_entry, input_absinfo
	- can map with wsconsio

Keep:
- bitstrings
- input event codes