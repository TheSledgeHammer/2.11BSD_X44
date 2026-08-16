# Netiso Planned Updates:

- Merge useful parts of nettpi into netiso:
	- iso_nsap.c, iso_tsap.c, iso_ssap.c, iso_psap.c
	- iso_sap.c, iso_sap.h, iso_nsap.h
- Implement ISODE-like TSAP management for TPDU's, TPKT.
	- Work is based on the above point.
- Add a libiso to user-space to provide access to said sap layers.
- Update various netiso components to use the new
  sap layers
- Improve tp_driver.c state handler.
	- Preferably replace with something more maintainable/readable.
	- Doesn't use hexadecimal values for all switch cases 
	(i.e. handlers, events, timers and actions).
		- Making it a guessing game of what they actually reference.
- Merge duplicate defines etc... (General code cleanup)

- Implement NLSP (Network Layer Security Protocol)
- and/or TLSP (Transport Layer Security Protocol)
	- Could existing security stacks be used??
		- pfkeyv2?, netkey?, pf?, ipsecs (ah? and esp?)
		

# tp_driver.c Breakdown

# xebec_index:
- The below macro controls the the state.
	- macro: (((ev_number) << 4) + (tp_state))
	- which are defined in tp_states.h and tp_events.h

Example:
- case:		ev_number:	tp_state:
- 0x102:	0x10 		0x2
- 0x104:	0x10 		0x4
- 0x144:	0x14 		0x4
- 0x162:	0x16 		0x2
- 0x172:	0x17 		0x2
- 0x174:	0x17 		0x4
- 0x177:	0x17 		0x7
- 0x188:	0x18		0x8

# xebex_action:
- Educated guess.
	- macro: (((tp_state) << 4) + (ev_number))
	- the above macro provides all the hex values, and some cases align
	with the given combination, but some also do not make any sense. 
