# Syscons:
- Design syscons to run common code for keyboard, mouse & terminal
	- keeping the drivers core independent 
- Implement common code infrastructure

## Keyboard:
- Remap kbd.c to fit planned syscons changes
- kbd changes:
	- remove need for the in-kernel virtual keyboard driver (used with FreeBSD's sysinit)
	- provide a generic keyboard layer with routines that can manage multiple.
	OR
	- At minimum a machine-independent keyboard driver with PS/2 compatability
	- cfdriver is only used with the driver not the generic keyboard layer

## Mouse:
- Add independent device operations
- Add cdevsw & cfdriver

## Terminal:
- Add independent device operations
- Add cdevsw & cfdriver
- Fix-up any missing tty components

