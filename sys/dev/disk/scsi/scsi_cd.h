/*	$NetBSD: scsi_cd.h,v 1.6 1996/03/19 03:05:15 mycroft Exp $	*/

/*
 * Written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with 
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */
#ifndef	_SCSI_SCSI_CD_H
#define _SCSI_SCSI_CD_H 1

/*
 *	Define two bits always in the same place in byte 2 (flag byte)
 */
#define	CD_RELADDR	0x01
#define	CD_MSF		0x02

/*
 * SCSI command format
 */

struct scsi_read_capacity_cd {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t addr[4];
	u_int8_t unused[3];
	u_int8_t control;
};

struct scsi_pause {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[6];
	u_int8_t resume;
	u_int8_t control;
};
#define	PA_PAUSE	1
#define PA_RESUME	0

struct scsi_play_msf {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused;
	u_int8_t start_m;
	u_int8_t start_s;
	u_int8_t start_f;
	u_int8_t end_m;
	u_int8_t end_s;
	u_int8_t end_f;
	u_int8_t control;
};

struct scsi_play_track {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[2];
	u_int8_t start_track;
	u_int8_t start_index;
	u_int8_t unused1;
	u_int8_t end_track;
	u_int8_t end_index;
	u_int8_t control;
};

struct scsi_play {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t blk_addr[4];
	u_int8_t unused;
	u_int8_t xfer_len[2];
	u_int8_t control;
};

struct scsi_play_big {
	u_int8_t opcode;
	u_int8_t byte2;	/* same as above */
	u_int8_t blk_addr[4];
	u_int8_t xfer_len[4];
	u_int8_t unused;
	u_int8_t control;
};

struct scsi_play_rel_big {
	u_int8_t opcode;
	u_int8_t byte2;	/* same as above */
	u_int8_t blk_addr[4];
	u_int8_t xfer_len[4];
	u_int8_t track;
	u_int8_t control;
};

struct scsi_read_header {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t blk_addr[4];
	u_int8_t unused;
	u_int8_t data_len[2];
	u_int8_t control;
};

struct scsi_read_subchannel {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t byte3;
#define	SRS_SUBQ			0x40
	u_int8_t subchan_format;
	u_int8_t unused[2];
	u_int8_t track;
	u_int8_t data_len[2];
	u_int8_t control;
};

struct scsi_read_toc {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t unused[4];
	u_int8_t from_track;
	u_int8_t data_len[2];
	u_int8_t control;
};
;

struct scsi_read_cd_capacity {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t addr[4];
	u_int8_t unused[3];
	u_int8_t control;
};

/*
 * Opcodes
 */

#define READ_CD_CAPACITY	0x25	/* slightly different from disk */
#define READ_SUBCHANNEL		0x42	/* cdrom read Subchannel */
#define READ_TOC			0x43	/* cdrom read TOC */
#define READ_HEADER			0x44	/* cdrom read header */
#define PLAY				0x45	/* cdrom play  'play audio' mode */
#define PLAY_MSF			0x47	/* cdrom play Min,Sec,Frames mode */
#define PLAY_TRACK			0x48	/* cdrom play track/index mode */
#define PLAY_TRACK_REL		0x49	/* cdrom play track/index mode */
#define PAUSE				0x4b	/* cdrom pause in 'play audio' mode */
#define PLAY_BIG			0xa5	/* cdrom pause in 'play audio' mode */
#define PLAY_TRACK_REL_BIG	0xa9	/* cdrom play track/index mode */


struct scsi_read_cd_cap_data {
	u_int8_t addr[4];
	u_int8_t length[4];
};

/* mod pages common to scsi and atapi */
struct cd_audio_page {
	u_int8_t page_code;
#define	CD_PAGE_CODE		0x3F
#define	AUDIO_PAGE			0x0e
#define	CD_PAGE_PS			0x80
	u_int8_t param_len;
	u_int8_t flags;
#define	CD_PA_SOTC			0x02
#define	CD_PA_IMMED			0x04
	u_int8_t unused[2];
	u_int8_t format_lba; /* valid only for SCSI CDs */
#define	CD_PA_FORMAT_LBA 	0x0F
#define	CD_PA_APR_VALID		0x80
	u_int8_t lb_per_sec[2];
	struct port_control {
		u_int8_t channels;
#define	CHANNEL 			0x0F
#define	CHANNEL_0 			1
#define	CHANNEL_1 			2
#define	CHANNEL_2 			4
#define	CHANNEL_3 			8
#define	LEFT_CHANNEL		CHANNEL_0
#define	RIGHT_CHANNEL		CHANNEL_1
#define	MUTE_CHANNEL		0x0
#define	BOTH_CHANNEL		LEFT_CHANNEL | RIGHT_CHANNEL
		u_int8_t volume;
	} port[4];
#define	LEFT_PORT			0
#define	RIGHT_PORT			1
};

/* sense pages */
#define SCSI_CD_PAGE_CODE   			0x3F
#define SCSI_AUDIO_PAGE  				0x0e
#define SCSI_CD_PAGE_PS  				0x80

#define	SCSI_CD_WRITE_PARAMS_PAGE		0x05
#define	SCSI_CD_WRITE_PARAMS_PAGE_LEN	0x32
struct scsi_cd_write_params_page {
	u_int8_t page_code;
	u_int8_t page_len;
	u_int8_t write_type;
#define	WRITE_TYPE_DUMMY				0x10	/* do not actually write blocks */
#define	WRITE_TYPE_MASK					0x0f	/* session write type */
	u_int8_t track_mode;
#define	TRACK_MODE_MULTI_SESS			0xc0	/* multisession write type */
#define	TRACK_MODE_FP					0x20	/* fixed packet (if in packet mode) */
#define	TRACK_MODE_COPY					0x10	/* 1st higher gen of copy prot track */
#define	TRACK_MODE_PREEPMPASIS			0x01	/* audio w/ preemphasis (audio) */
#define	TRACK_MODE_INCREMENTAL			0x01	/* incremental data track (data) */
#define	TRACK_MODE_ALLOW_COPY			0x02	/* digital copy is permitted */
#define	TRACK_MODE_DATA					0x04	/* this is a data track */
#define	TRACK_MODE_4CHAN				0x08	/* four channel audio */
	u_int8_t dbtype;
#define	DBTYPE_MASK						0x0f	/* data block type */
	u_int8_t reserved1[2];
	u_int8_t host_appl_code;
#define	HOST_APPL_CODE_MASK				0x3f	/* host application code of disk */
	u_int8_t session_format;
	u_int8_t reserved2;
	u_int8_t packet_size[4];
	u_int8_t audio_pause_len[2];
	u_int8_t media_cat_number[16];
	u_int8_t isrc[14];
	u_int8_t sub_header[4];
	u_int8_t vendir_unique[4];
};

union scsi_cd_pages {
	struct scsi_cd_write_params_page 	write_params;
	struct cd_audio_page 				audio;
};

struct scsi_cd_mode_data  {
	struct scsi_mode_header header;
	struct scsi_blk_desc 	blk_desc;
	union scsi_cd_pages  	page;
};

#define AUDIOPAGESIZE \
	(sizeof(struct scsi_mode_header) + sizeof(struct scsi_blk_desc) \
	    + sizeof(struct cd_audio_page))
#endif /*_SCSI_SCSI_CD_H*/

