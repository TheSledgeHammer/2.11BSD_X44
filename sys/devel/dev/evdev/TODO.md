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

Keyboard:
- scancode -> ascii value
- decimal to hex

utf8 = hex

alpha: hex - 1 = scancode
numurical: hex + 1 = scancode
exception is 0

key scancode hex 	decimal
esc		0	  01	   1	

1		2	  31	  49
2  		3  	  32   	  50
3		4	  33	  51
4		5	  34
5		6	  35
6		7	  36
7		8     37
8		9	  38
9		10	  39
0		11	  30	  48

a		30	  61	  97
b			  62
c			  63
s		31	  73	  115