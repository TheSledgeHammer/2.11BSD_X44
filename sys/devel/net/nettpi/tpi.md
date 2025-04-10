# Transport Protocol Interface:

- Based/derived directly from the BSD's network stack. In particular the netiso tp implementation.
- Analogous to the XTI/TLI. While not conforming to the XTI/TLI standard.
- It does aim to achieve a similar goal.
- Allowing multiple network protocols across a unified interface.
- With minimal modifications to BSD's existing network stack.


tpi -> ipv4
tpi -> ipv6
tpi -> iso
tpi -> xns
tpi -> x25
tpi -> atm
