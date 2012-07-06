/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SR030PC50_H__
#define __SR030PC50_H__

#include <linux/types.h>
#include <mach/board.h>

#ifdef SR030PC50_DEBUG
#define CAM_DEBUG(fmt, arg...)	\
		do { printk(KERN_DEBUG "[%s:%d] " fmt "\033[0m\n",\
				__func__, __LINE__, ##arg); } \
		while (0)

#define cam_info(fmt, arg...)	\
		do { printk(KERN_INFO "[sr030pc50]" fmt "\n", ##arg); } \
		while (0)

#define cam_err(fmt, arg...)	\
		do { printk(KERN_ERR "[sr030pc50] %s:%d:" fmt "\n",\
				__func__, __LINE__, ##arg); } \
		while (0)

#else
#define CAM_DEBUG(fmt, arg...)
#define cam_info(fmt, arg...)
#define cam_err(fmt, arg...)
#endif

#define CAPTURE_FLASH	1
#define MOVIE_FLASH		2

/* level at or below which we need to enable flash when in auto mode */
#define LOW_LIGHT_LEVEL		0x20

/* preview size idx*/
#define PREVIEW_SIZE_VGA   0     /* 640x480 */
#define PREVIEW_SIZE_D1     1    /* 720x480 */
#define PREVIEW_SIZE_WVGA   2    /* 800x480 */
#define PREVIEW_SIZE_XGA    3    /* 1024x768*/
#define PREVIEW_SIZE_HD   4    /* 1280x720*/
#define PREVIEW_SIZE_FHD   5    /* 1920x1080*/
#define PREVIEW_SIZE_MMS 6        /* 176x144 */

/* DTP */
#define DTP_OFF		0
#define DTP_ON		1
#define DTP_OFF_ACK		2
#define DTP_ON_ACK		3

#define PREVIEW_MODE	0
#define MOVIE_MODE		1

#define SR030PC50_DELAY		0xFF00


struct sr030pc50_userset {
	unsigned int focus_mode;
	unsigned int focus_status;
	unsigned int continuous_af;

	unsigned int	metering;
	unsigned int	exposure;
	unsigned int		wb;
	unsigned int		iso;
	int	contrast;
	int	saturation;
	int	sharpness;
	int	brightness;
	int	scene;
	unsigned int zoom;
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int scenemode;
	unsigned int detectmode;
	unsigned int antishake;
	unsigned int fps;
	unsigned int flash_mode;
	unsigned int flash_state;
	unsigned int stabilize;	/* IS */
	unsigned int strobe;
	unsigned int jpeg_quality;
/*unsigned int preview_size;*/
/*struct m5mo_preview_size preview_size;*/
	unsigned int preview_size_idx;
	unsigned int capture_size;
	unsigned int thumbnail_size;
};

struct reg_struct_init {
/* --------PLL setting-------- */
	uint8_t pre_pll_clk_div;		/* 0x0305 */
	uint8_t plstatim;				/* 0x302b */
	uint8_t reg_3024;				/*ox3024*/
	uint8_t image_orientation;		/* 0x0101*/
	uint8_t vndmy_ablmgshlmt;		/*0x300a*/
	uint8_t y_opbaddr_start_di;		/*0x3014*/
	uint8_t reg_0x3015;				/*0x3015*/
	uint8_t reg_0x301c;				/*0x301c*/
	uint8_t reg_0x302c;				/*0x302c*/
	uint8_t reg_0x3031;				/*0x3031*/
	uint8_t reg_0x3041;				/* 0x3041 */
	uint8_t reg_0x3051;				/* 0x3051 */
	uint8_t reg_0x3053;				/* 0x3053 */
	uint8_t reg_0x3057;				/* 0x3057 */
	uint8_t reg_0x305c;				/* 0x305c */
	uint8_t reg_0x305d;				/* 0x305d */
	uint8_t reg_0x3060;				/* 0x3060 */
	uint8_t reg_0x3065;				/* 0x3065 */
	uint8_t reg_0x30aa;				/* 0x30aa */
	uint8_t reg_0x30ab;
	uint8_t reg_0x30b0;
	uint8_t reg_0x30b2;
	uint8_t reg_0x30d3;
	uint8_t reg_0x3106;
	uint8_t reg_0x310c;
	uint8_t reg_0x3304;
	uint8_t reg_0x3305;
	uint8_t reg_0x3306;
	uint8_t reg_0x3307;
	uint8_t reg_0x3308;
	uint8_t reg_0x3309;
	uint8_t reg_0x330a;
	uint8_t reg_0x330b;
	uint8_t reg_0x330c;
	uint8_t reg_0x330d;
	uint8_t reg_0x330f;
	uint8_t reg_0x3381;
};

struct reg_struct {
	uint8_t pll_multiplier;			/* 0x0307 */
	uint8_t frame_length_lines_hi;	/* 0x0340*/
	uint8_t frame_length_lines_lo;	/* 0x0341*/
	uint8_t y_addr_start;			/* 0x347 */
	uint8_t y_add_end;				/* 0x034b */
	uint8_t x_output_size_msb;		/* 0x034c */
	uint8_t x_output_size_lsb;		/* 0x034d */
	uint8_t y_output_size_msb;		/* 0x034e */
	uint8_t y_output_size_lsb;		/* 0x034f */
	uint8_t x_even_inc;				/* 0x0381 */
	uint8_t x_odd_inc;				/* 0x0383 */
	uint8_t y_even_inc;				/* 0x0385 */
	uint8_t y_odd_inc;				/* 0x0387 */
	uint8_t hmodeadd;				/* 0x3001 */
	uint8_t vmodeadd;				/* 0x3016 */
	uint8_t vapplinepos_start;		/*ox3069*/
	uint8_t vapplinepos_end;		/*306b*/
	uint8_t shutter;				/* 0x3086 */
	uint8_t haddave;				/* 0x30e8 */
	uint8_t lanesel;				/* 0x3301 */
};

struct sr030pc50_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
};

enum sr030pc50_test_mode_t {
	TEST_OFF,
	TEST_1,
	TEST_2,
	TEST_3
};

enum sr030pc50_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	INVALID_SIZE
};
enum sr030pc50_setting {
	RES_PREVIEW,
	RES_CAPTURE
};
enum sr030pc50_reg_update {
	/* Sensor egisters that need to be updated during initialization */
	REG_INIT,
	/* Sensor egisters that needs periodic I2C writes */
	UPDATE_PERIODIC,
	/* All the sensor Registers will be updated */
	UPDATE_ALL,
	/* Not valid update */
	UPDATE_INVALID
};
#endif /* __sr030pc50_H__ */
