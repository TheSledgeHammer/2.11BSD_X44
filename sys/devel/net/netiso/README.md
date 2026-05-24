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