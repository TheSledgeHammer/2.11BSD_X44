/* $FreeBSD$ */
#ifndef _EFI_SER_H
#define _EFI_SER_H

/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    efiser.h

Abstract:

    EFI serial protocol

Revision History

--*/

/*
 * Serial protocol
 */

#define SERIAL_IO_PROTOCOL \
    { 0xBB25CF6F, 0xF1D4, 0x11D2, 0x9A, 0x0C, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0xFD }

INTERFACE_DECL(_SERIAL_IO_INTERFACE);

typedef enum {
    DefaultParity,      
    NoParity,           
    EvenParity,
    OddParity,
    MarkParity,
    SpaceParity
} EFI_PARITY_TYPE;

typedef enum {
	DefaultStopBits,
	OneStopBit,
	OneFiveStopBits,
	TwoStopBits
} EFI_STOP_BITS_TYPE;

#define EFI_SERIAL_CLEAR_TO_SEND                   0x0010
#define EFI_SERIAL_DATA_SET_READY                  0x0020
#define EFI_SERIAL_RING_INDICATE                   0x0040
#define EFI_SERIAL_CARRIER_DETECT                  0x0080
#define EFI_SERIAL_REQUEST_TO_SEND                 0x0002
#define EFI_SERIAL_DATA_TERMINAL_READY             0x0001
#define EFI_SERIAL_INPUT_BUFFER_EMPTY              0x0100
#define EFI_SERIAL_OUTPUT_BUFFER_EMPTY             0x0200
#define EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE        0x1000
#define EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE        0x2000
#define EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE    0x4000

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_RESET) (
    IN struct _SERIAL_IO_INTERFACE  *This
    );

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_SET_ATTRIBUTES) (
    IN struct _SERIAL_IO_INTERFACE  *This,
    IN UINT64                       BaudRate,
    IN UINT32                       ReceiveFifoDepth,
    IN UINT32                       Timeout,
    IN EFI_PARITY_TYPE              Parity,
    IN UINT8                        DataBits,
    IN EFI_STOP_BITS_TYPE           StopBits
    );

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_SET_CONTROL_BITS) (
    IN struct _SERIAL_IO_INTERFACE  *This,
    IN UINT32                       Control
    );

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_GET_CONTROL_BITS) (
    IN struct _SERIAL_IO_INTERFACE  *This,
    OUT UINT32                      *Control
    );

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_WRITE) (
    IN struct _SERIAL_IO_INTERFACE  *This,
    IN OUT UINTN                    *BufferSize,
    IN VOID                         *Buffer
    );

typedef
EFI_STATUS
(EFIAPI *EFI_SERIAL_READ) (
    IN struct _SERIAL_IO_INTERFACE  *This,
    IN OUT UINTN                    *BufferSize,
    OUT VOID                        *Buffer
    );

typedef struct {
    UINT32                  	ControlMask;

	/* current Attributes. */
    UINT32                  	Timeout;
    UINT64                  	BaudRate;
    UINT32                  	ReceiveFifoDepth;
    UINT32                  	DataBits;
    UINT32                  	Parity;
    UINT32                  	StopBits;
} SERIAL_IO_MODE;

#define SERIAL_IO_INTERFACE_REVISION    0x00010000

typedef struct _SERIAL_IO_INTERFACE {
    UINT32                       Revision;
    EFI_SERIAL_RESET             Reset;
    EFI_SERIAL_SET_ATTRIBUTES    SetAttributes;
    EFI_SERIAL_SET_CONTROL_BITS  SetControl;
    EFI_SERIAL_GET_CONTROL_BITS  GetControl;
    EFI_SERIAL_WRITE             Write;
    EFI_SERIAL_READ              Read;

    SERIAL_IO_MODE               *Mode;
} SERIAL_IO_INTERFACE;

#endif
