# 16-Bit Support (Hardware and Software):
- 2.11BSD_X44 will openly support 16-Bit hardware if capable.
- Although 16-Bit platforms are not the primary focus.
	- 32-Bit/64-Bit is the project's intended focus.

## Things to Consider:
- Memory Constraints:
	- 2.11BSD on the PDP-11 for example from my understanding 
	was pushing the boundaries of the PDP-11 hardware.
- Performance: 
	- Converting between 16-bit <-> 32-bit <-> 64-bit is costly.
- Library Support:
	- How much support exists? or does it require new tools?
- Security:
	- Does it open up 2.11BSD_X44 to vulnerabilities and exploits.
- Does the software support need to be apart of 2.11BSD_X44 or can it be
a separate module/package?

The above doesn't just apply to 16-Bit, but also 32-Bit and 64-Bit.

## Planned:
- 2.11BSD's PCC compiler:
	- Patching and updating PCC so it will compile and work as intended. Has been in the
works since very early on.
	- This includes subsequent libraries like Fortran.
