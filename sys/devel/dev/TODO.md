# Syscons:
- Design syscons to run common code for keyboard, mouse & terminal
	- keeping the drivers core independent 
- Implement common code infrastructure

## Keyboard:
- Remap kbd.c to fit planned syscons changes

## Mouse:
- Add independent device operations
- Add cdevsw & cfdriver

## Terminal:
- Add independent device operations
- Add cdevsw & cfdriver
- Fix-up any missing tty components