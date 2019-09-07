//Taken from registers taken FreeBSD
/*
 * Location of the users' stored
 * registers relative to R0 (EAX for the x86).
 * Usage is u.u_ar0[XX].
 */
//Segment Registers
#define   FS (0)
#define   ES (1)
#define   DS (2)
#define   SS (17)
#define   CS (14)
#define   GS (18)
#define   ISP (6)
#define   ERR (12)

//32-Bit Registers
#define   EAX (10)
#define   ECX (9)
#define   EDX (8)
#define   EBX (7)
#define   ESP (16)
#define   EBP (5)
#define   ESI (4)
#define   EDI (3)
#define   EIP (13)
#define   EFLAGS (15)
/*
//16-Bit Registers
#define   AX ()
#define   CX ()
#define   DX ()
#define   BX ()
#define   SP ()
#define   BP ()
#define   SI ()
#define   DI ()
#define   IP ()
#define   FLAGS ()

//64-Bit Registers
#define   RAX ()
#define   RCX ()
#define   RDX ()
#define   RBX ()
#define   RSP ()
#define   RBP ()
#define   RSI ()
#define   RDI ()
#define   RI ()

struct _reg32 {
  _uint32_t   r_fs;
  _uint32_t   r_es;
  _uint32_t   r_ds;
  _uint32_t   r_edi;
  _uint32_t   r_esi;
  _uint32_t   r_ebp;
  _uint32_t   r_isp;
  _uint32_t   r_ebx;
  _uint32_t   r_edx;
  _uint32_t   r_ecx;
  _uint32_t   r_eax;
  _uint32_t   r_trapno;
  _uint32_t   r_err;
  _uint32_t   r_eip;
  _uint32_t   r_cs;
  _uint32_t   r_eflags;
  _uint32_t   r_esp;
  _uint32_t   r_ss;
  _uint32_t   r_gs;
};

struct _reg64 {
  _int64_t   r_r15;
  _int64_t   r_r14;
  _int64_t   r_r13;
  _int64_t   r_r12;
  _int64_t   r_r11;
  _int64_t   r_r10;
  _int64_t   r_r9;
  _int64_t   r_r8;
  _int64_t   r_rdi;
  _int64_t   r_rsi;
  _int64_t   r_rbp;
  _int64_t   r_rbx;
  _int64_t   r_rdx;
  _int64_t   r_rcx;
  _int64_t   r_rax;
  _uint32_t  r_trapno;
  _uint16_t  r_fs;
  _uint16_t  r_gs;
  _uint32_t  r_err;
  _uint16_t  r_es;
  _uint16_t  r_ds;
  _int64_t   r_rip;
  _int64_t   r_cs;
  _int64_t   r_rflags;
  _int64_t   r_rsp;
  _int64_t   r_ss;
};
*/

#define	EAX	(0)
#define	ECX	(-1)
#define	EDX	(-2)
#define	EBX	(-3)
#define	ESP	(8)
#define	EBP	(-9)
#define	ESI	(-4)
#define	EDI	(-5)
#define	EIP	(5)
#define	EFL	(7)

#define	TBIT	0x100		/* EFLAGS trap flag */

struct trap {
    int dev;
    int pl;
    int edi;
    int esi;
    int ebx;
    int edx;
    int ecx;
    int eax;
    int ds;
    int es;
    int num;
    int err;
    int eip;
    int cs;
    int efl;
    int esp;
    int ss;
};
