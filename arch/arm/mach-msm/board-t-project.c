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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c/sx150x.h>
#include <linux/i2c/isl9519.h>
#include <linux/gpio.h>
#include <linux/msm_ssbi.h>
#include <linux/regulator/gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8921-adc.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slimbus/slimbus.h>
#include <linux/bootmem.h>
#include <linux/msm_kgsl.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/cyttsp.h>
#include <linux/dma-mapping.h>
#include <linux/platform_data/qcom_crypto_device.h>
#include <linux/platform_data/qcom_wcnss_device.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/msm_tsens.h>
#include <linux/ks8851.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/hardware/gic.h>
#include <asm/mach/mmc.h>

#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_spi.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <linux/usb/android.h>
#include <mach/usbdiag.h>
#include <mach/socinfo.h>
#include <mach/rpm.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/dma.h>
#include <mach/msm_dsps.h>
#include <mach/msm_xo.h>
#include <mach/restart.h>
#include <mach/t-project-gpio.h>
#ifdef CONFIG_SENSORS_AK8975
#include <linux/i2c/ak8975.h>
#endif
#ifdef CONFIG_MPU_SENSORS_MPU6050B1
#include <linux/mpu6x.h>
#endif
#ifdef CONFIG_OPTICAL_GP2A
#include <linux/i2c/gp2a.h>
#endif
#ifdef CONFIG_OPTICAL_GP2AP020A00F
#include <linux/i2c/gp2ap020.h>
#endif

#ifdef CONFIG_WCD9310_CODEC
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9310/core.h>
#include <linux/mfd/wcd9310/pdata.h>
#endif
#ifdef CONFIG_KEYBOARD_GPIO
#include <linux/gpio_keys.h>
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
#include <linux/i2c/fsa9485.h>
#endif

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
#include <linux/sii9234.h>
#endif

#include <linux/power_supply.h>
#ifdef CONFIG_PM8921_CHARGER
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#endif
#ifdef CONFIG_BATTERY_MAX17040
#include <linux/max17040_battery.h>
#endif
#include <linux/i2c/mxt224e.h>
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
#include <linux/i2c/cypress_touchkey.h>
#endif
#include <linux/ion.h>
#include <mach/ion.h>

#include "timer.h"
#include "devices.h"
#include "devices-msm8x60.h"
#include "spm.h"
#include "board-msm8960.h"
#include "pm.h"
#include "cpuidle.h"
#include "rpm_resources.h"
#include "mpm.h"
#include "acpuclock.h"
#include "rpm_log.h"
#include "smd_private.h"
#ifdef CONFIG_NFC_PN544
#include <linux/pn544.h>
#endif /* CONFIG_NFC_PN544	*/
#include "mach/board-t-project-camera.h"
#include "pm-boot.h"

static struct platform_device msm_fm_platform_init = {
	.name = "iris_fm",
	.id   = -1,
};

#ifdef CONFIG_SEC_DEBUG
#include <mach/sec_debug.h>
#endif
unsigned int gpio_table[][GPIO_REV_MAX] = {
	/* GPIO_INDEX			Rev	{#00,#01 }, */
	/* VOLUME_UP	*/		{ 12, 80, 80 },
	/* VOLUME_DOWN	*/		{ 13, 81, 81 },
	/* TOUCH_KEY_I2C_SDA*/  { 12, 12, 71},
	/* TOUCH_KEY_I2C_SCL*/  { 13, 13, 72},
};

static int gpio_rev(unsigned int index)
{
	if (system_rev == BOARD_REV00)
		return gpio_table[index][BOARD_REV00];
	if (system_rev == BOARD_REV01)
		return gpio_table[index][BOARD_REV01];
	if (system_rev >= BOARD_REV02)
		return gpio_table[index][BOARD_REV02];

	return -EINVAL;
}

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8XXX_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= PM8921_GPIO_PM_TO_SYS(_gpio), \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

#define PM8XXX_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8XXX_GPIO_DISABLE(_gpio) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8XXX_GPIO_OUTPUT(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_INPUT(_gpio, _pull) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

/* Initial PM8921 GPIO configurations */
static struct pm8xxx_gpio_init pm8921_gpios[] __initdata = {
	PM8XXX_GPIO_INPUT(16,	    PM_GPIO_PULL_UP_30), /* SD_CARD_WP */
	PM8XXX_GPIO_OUTPUT(17,	    0),			 /* DISP 3.3 V Boost */
	PM8XXX_GPIO_OUTPUT(21,	    1),			 /* NFC_EN */
	PM8XXX_GPIO_OUTPUT_FUNC(24, 0, PM_GPIO_FUNC_2),	 /* Bl: Off, PWM mode */
	PM8XXX_GPIO_INPUT(26,	    PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_OUTPUT(43,	    1),			 /* DISP_RESET_N */
	PM8XXX_GPIO_OUTPUT(18,	    1),			 /* SPK_EN */
};

/* Initial PM8921 MPP configurations */
static struct pm8xxx_mpp_init pm8921_mpps[] __initdata = {
	/* External 5V regulator enable; shared by HDMI and USB_OTG switches. */
	PM8XXX_MPP_INIT(7, D_INPUT, PM8921_MPP_DIG_LEVEL_VPH, DIN_TO_INT),
#ifdef CONFIG_OPTICAL_GP2A
	PM8XXX_MPP_INIT(PM8921_AMUX_MPP_4, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH7,
		DOUT_CTRL_LOW),
#endif
	PM8XXX_MPP_INIT(PM8921_AMUX_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH8,
								DOUT_CTRL_LOW),
};

#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_INPUT_BMP180) || defined(CONFIG_OPTICAL_GP2A)
static void sensor_power_on_vdd(int onoff);

#endif

static void __init pm8921_gpio_mpp_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(pm8921_gpios); i++) {
		rc = pm8xxx_gpio_config(pm8921_gpios[i].gpio,
					&pm8921_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(pm8921_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8921_mpps[i].mpp,
					&pm8921_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
}

#define FPGA_CS_GPIO		14
#define KS8851_RST_GPIO		89
#define KS8851_IRQ_GPIO		90

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
enum {
	GPIO_EXPANDER_IRQ_BASE = (PM8921_IRQ_BASE + PM8921_NR_IRQS),
	GPIO_EXPANDER_GPIO_BASE = (PM8921_MPP_BASE + PM8921_NR_MPPS),
	/* CAM Expander */
	GPIO_CAM_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE,
	GPIO_CAM_GP_STROBE_READY = GPIO_CAM_EXPANDER_BASE,
	GPIO_CAM_GP_AFBUSY,
	GPIO_CAM_GP_STROBE_CE,
	GPIO_CAM_GP_CAM1MP_XCLR,
	GPIO_CAM_GP_CAMIF_RESET_N,
	GPIO_CAM_GP_XMT_FLASH_INT,
	GPIO_CAM_GP_LED_EN1,
	GPIO_CAM_GP_LED_EN2,

};
#endif

/* The SPI configurations apply to GSBI 1*/
static struct gpiomux_setting spi_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting spi_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gsbi3_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi3_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi5 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi7 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi10 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi12 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting audio_auxpcm[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_NONE,
	.drv = GPIOMUX_DRV_8MA,
	.func = GPIOMUX_FUNC_GPIO,
};
#endif

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};
static struct gpiomux_setting volkey = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

struct msm_gpiomux_config msm8960_gpiomux_configs[NR_GPIO_IRQS] = {
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	{
		.gpio = GPIO_KS8851_IRQ,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
	{
		.gpio = GPIO_KS8851_RST,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
	{
		.gpio = GPIO_FPGA_CS,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
	{
		.gpio = GPIO_VOLUME_UP,
		.settings = {
			[GPIOMUX_ACTIVE] = &volkey,
			[GPIOMUX_SUSPENDED] = &volkey,
		}
	},
	{
		.gpio = GPIO_VOLUME_DN,
		.settings = {
			[GPIOMUX_ACTIVE] = &volkey,
			[GPIOMUX_SUSPENDED] = &volkey,
		}
	},
#endif
};

static struct msm_gpiomux_config msm8960_gsbi_configs[] __initdata = {
	{
		.gpio      = 6,		/* GSBI1 QUP SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
	{
		.gpio      = 7,		/* GSBI1 QUP SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
	{
		.gpio      = 8,		/* GSBI1 QUP SPI_CS_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
#if !defined(CONFIG_VIDEO_MHL_V1) && !defined(CONFIG_VIDEO_MHL_V2)
	{
		.gpio      = 9,		/* GSBI1 QUP SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
#endif
	{
		.gpio      = 16,	/* GSBI3 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 17,	/* GSBI3 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 22,	/* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi5,
		},
	},
	{
		.gpio      = 23,	/* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi5,
		},
	},
	{
		.gpio      = 24,	/* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi5,
		},
	},
	{
		.gpio      = 25,	/* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi5,
		},
	},
	{
		.gpio      = 32,	/* GSBI7 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7,
		},
	},
	{
		.gpio      = 33,	/* GSBI7 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7,
		},
	},
	{
		.gpio      = 44,	/* GSBI12 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 45,	/* GSBI12 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 73,	/* GSBI10 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{
		.gpio      = 74,	/* GSBI10 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
};

static struct msm_gpiomux_config msm8960_slimbus_config[] __initdata = {
	{
		.gpio	= 60,		/* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio	= 61,		/* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

static struct msm_gpiomux_config msm8960_audio_codec_configs[] __initdata = {
	{
		.gpio = 59,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
};

static struct msm_gpiomux_config msm8960_audio_auxpcm_configs[] __initdata = {
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
};

static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};
static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 1*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 2*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 3*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_5, /*active 4*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_6, /*active 5*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 6*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_3, /*active 7*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*i2c suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},

};

static struct msm_gpiomux_config msm8960_cam_common_configs[] = {
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 5,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 76,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
};

static struct msm_gpiomux_config msm8960_cam_2d_configs[] = {
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
};

static struct gpiomux_setting cyts_resout_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cyts_resout_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cyts_sleep_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cyts_sleep_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cyts_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cyts_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8960_cyts_configs[] __initdata = {
	{	/* TS INTERRUPT */
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_int_sus_cfg,
		},
	},
	{	/* TS SLEEP */
		.gpio = 50,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_sleep_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_sleep_sus_cfg,
		},
	},
	{	/* TS RESOUT */
		.gpio = 52,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_resout_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_resout_sus_cfg,
		},
	},
};

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
enum {
	SX150X_CAM,
};

static struct sx150x_platform_data sx150x_data[] = {
	[SX150X_CAM] = {
		.gpio_base         = GPIO_CAM_EXPANDER_BASE,
		.oscio_is_gpo      = false,
		.io_pullup_ena     = 0x0,
		.io_pulldn_ena     = 0xc0,
		.io_open_drain_ena = 0x0,
		.irq_summary       = -1,
	},
};

#endif

#ifdef CONFIG_I2C

#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI10_QUP_I2C_BUS_ID 10

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)

static struct i2c_board_info cam_expander_i2c_info[] = {
	{
		I2C_BOARD_INFO("sx1508q", 0x22),
		.platform_data = &sx150x_data[SX150X_CAM]
	},
};

static struct msm_cam_expander_info cam_expander_info[] = {
	{
		cam_expander_i2c_info,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	},
};
#endif
#endif

static struct gpiomux_setting sec_ts_ldo_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sec_ts_ldo_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8960_sec_ts_configs[] = {
	{	/* TS LDO EN */
		.gpio = 10,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sec_ts_ldo_act_cfg,
			[GPIOMUX_SUSPENDED] = &sec_ts_ldo_sus_cfg,
		},
	},
};

#define MSM_PMEM_KERNEL_EBI1_SIZE  0x110C000
#define MSM_PMEM_ADSP_SIZE         0x3800000
#define MSM_PMEM_AUDIO_SIZE        0x28B000
#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
#define MSM_PMEM_SIZE 0x4000000 /* 64 Mbytes */
#else
#define MSM_PMEM_SIZE 0x1800000 /* 24 Mbytes */
#endif

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define MSM_ION_EBI_SIZE	MSM_PMEM_SIZE
#define MSM_ION_ADSP_SIZE	MSM_PMEM_ADSP_SIZE
#define MSM_ION_HEAP_NUM	4
#else
#define MSM_ION_HEAP_NUM	2
#endif

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
static unsigned pmem_kernel_ebi1_size = MSM_PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_size = MSM_PMEM_SIZE;
static int __init pmem_size_setup(char *p)
{
	pmem_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_size", pmem_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};
#endif
static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};
#endif

#define DSP_RAM_BASE_8960 0x8da00000
#define DSP_RAM_SIZE_8960 0x1800000
static int dspcrashd_pdata_8960 = 0xDEADDEAD;

static struct resource resources_dspcrashd_8960[] = {
	{
		.name   = "msm_dspcrashd",
		.start  = DSP_RAM_BASE_8960,
		.end    = DSP_RAM_BASE_8960 + DSP_RAM_SIZE_8960,
		.flags  = IORESOURCE_DMA,
	},
};

struct platform_device msm_device_dspcrashd_8960 = {
	.name           = "msm_dspcrashd",
	.num_resources  = ARRAY_SIZE(resources_dspcrashd_8960),
	.resource       = resources_dspcrashd_8960,
	.dev = { .platform_data = &dspcrashd_pdata_8960 },
};

static struct memtype_reserve msm8960_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_pdata.size = pmem_size;
#endif
	android_pmem_audio_pdata.size = MSM_PMEM_AUDIO_SIZE;
#endif
}

static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm8960_reserve_table[p->memory_type].size += p->size;
}

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_pdata);
#endif
	reserve_memory_for(&android_pmem_audio_pdata);
	msm8960_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
}

static int msm8960_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

#ifdef CONFIG_ION_MSM
struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = {
		{
			.id	= ION_HEAP_SYSTEM_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_KMALLOC_HEAP_NAME,
		},
		{
			.id	= ION_HEAP_SYSTEM_CONTIG_ID,
			.type	= ION_HEAP_TYPE_SYSTEM_CONTIG,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		{
			.id	= ION_HEAP_EBI_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_EBI1_HEAP_NAME,
			.size	= MSM_ION_EBI_SIZE,
			.memory_type = ION_EBI_TYPE,
		},
		{
			.id	= ION_HEAP_ADSP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_ADSP_HEAP_NAME,
			.size	= MSM_ION_ADSP_SIZE,
			.memory_type = ION_EBI_TYPE,
		},
#endif
	}
};

struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

static void reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	msm8960_reserve_table[MEMTYPE_EBI1].size += MSM_ION_EBI_SIZE;
	msm8960_reserve_table[MEMTYPE_EBI1].size += MSM_ION_ADSP_SIZE;
#endif
}
static void __init msm8960_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_ion_memory();
}

static struct reserve_info msm8960_reserve_info __initdata = {
	.memtype_reserve_table = msm8960_reserve_table,
	.calculate_reserve_sizes = msm8960_calculate_reserve_sizes,
	.paddr_to_memtype = msm8960_paddr_to_memtype,
};

static int msm8960_memory_bank_size(void)
{
	return 1<<29;
}

static void __init locate_unstable_memory(void)
{
	struct membank *mb = &meminfo.bank[meminfo.nr_banks - 1];
	unsigned long bank_size;
	unsigned long low, high;

	bank_size = msm8960_memory_bank_size();
	low = meminfo.bank[0].start;
	high = mb->start + mb->size;
	low &= ~(bank_size - 1);

	if (high - low <= bank_size)
		return;
	msm8960_reserve_info.low_unstable_address = low + bank_size;
	msm8960_reserve_info.max_unstable_size = high - low - bank_size;
	msm8960_reserve_info.bank_size = bank_size;
	pr_info("low unstable address %lx max size %lx bank size %lx\n",
		msm8960_reserve_info.low_unstable_address,
		msm8960_reserve_info.max_unstable_size,
		msm8960_reserve_info.bank_size);
}

static void __init place_movable_zone(void)
{
	movable_reserved_start = msm8960_reserve_info.low_unstable_address;
	movable_reserved_size = msm8960_reserve_info.max_unstable_size;
	pr_info("movable zone start %lx size %lx\n",
		movable_reserved_start, movable_reserved_size);
}

static void __init msm8960_early_memory(void)
{
	reserve_info = &msm8960_reserve_info;
	locate_unstable_memory();
	place_movable_zone();
}

static void __init msm8960_reserve(void)
{
	msm_reserve();
}

static int msm8960_change_memory_power(u64 start, u64 size,
	int change_type)
{
	return soc_change_memory_power(start, size, change_type);
}

#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
static void cypress_power_onoff(int onoff)
{
	if (system_rev >= BOARD_REV02) {
		if (onoff) {
			gpio_direction_output(10, 1);
			msleep(40);
		} else
			gpio_direction_output(10, 0);
	} else {
		int ret;
		static struct regulator *reg_l17;

		if (!reg_l17) {
			reg_l17 = regulator_get(NULL, "8921_l17");
			if (IS_ERR(reg_l17)) {
				pr_err("could not get 8921_l17, rc = %ld\n",
					PTR_ERR(reg_l17));
				return;
		}

		ret = regulator_set_voltage(reg_l17, 3300000, 3300000);
		if (ret) {
			pr_err("%s: unable to set ldo17 voltage to 3.3V\n",
				__func__);
			return;
		}
		}

		if (onoff) {
			ret = regulator_enable(reg_l17);
			if (ret) {
				pr_err("enable l17 failed, rc=%d\n", ret);
				return;
			}
			pr_info("cypress_power_on is finished.\n");
		} else {
			ret = regulator_disable(reg_l17);
			if (ret) {
				pr_err("disable l17 failed, rc=%d\n", ret);
				return;
			}
		pr_info("cypress_power_off is finished.\n");
		}
	}
}

static u8 touchkey_keycode[] = {KEY_MENU, KEY_BACK};

static struct cypress_touchkey_platform_data cypress_touchkey_pdata = {
	.gpio_int = GPIO_TOUCH_KEY_INT,
	.gpio_led_en = GPIO_KEY_LED_EN,
	.touchkey_keycode = touchkey_keycode,
	.power_onoff = cypress_power_onoff,
};

static struct i2c_board_info touchkey_i2c_devices_info[] __initdata = {
	{
		I2C_BOARD_INFO("cypress_touchkey", 0x20),
		.platform_data = &cypress_touchkey_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_TOUCH_KEY_INT),
	},
};

static struct i2c_gpio_platform_data  cypress_touchkey_i2c_gpio_data = {
	.sda_pin		= NULL,
	.scl_pin		= NULL,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device touchkey_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_TOUCHKEY_I2C_BUS_ID,
	.dev.platform_data	= &cypress_touchkey_i2c_gpio_data,
};

#endif

#ifdef CONFIG_USB_SWITCH_FSA9485
static enum cable_type_t set_cable_status;

static void fsa9485_usb_cb(bool attached)
{
	union power_supply_propval value;
	int ret = 0;

	struct power_supply *psy = power_supply_get_by_name("battery");
	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	pr_info("fsa9485_usb_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}

	if (psy) {
		if (!ret) {
			ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
				&value);
		} else {
			pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
				__func__, ret);
		}
	} else {
		pr_err("%s : psy is null!\n", __func__);
	}
}

static void fsa9485_charger_cb(bool attached)
{
	union power_supply_propval value;
	int ret = 0;

	struct power_supply *psy = power_supply_get_by_name("battery");
	if (!psy) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	pr_info("fsa9480_charger_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_AC:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}
	if (psy) {
		if (!ret) {
			ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
				&value);
		} else {
			pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
				__func__, ret);
		}
	} else {
		pr_err("%s : psy is null!\n", __func__);
	}
}

static void msm8960_mhl_gpio_init(void)
{
	int ret;

	ret = gpio_request(GPIO_MHL_SEL, "mhl_sel");
	if (ret < 0)
		pr_err("mhl_sel gpio_request is failed\n");

	return;
}

static void cfg_mhl_sel(bool onoff)
{
	gpio_direction_output(GPIO_MHL_SEL, onoff ? 1 : 0);
	pr_info("mhl_sel :%d in fsa9485\n",
				gpio_get_value_cansleep(GPIO_MHL_SEL));
}

static struct i2c_gpio_platform_data fsa_i2c_gpio_data = {
	.sda_pin		= GPIO_USB_I2C_SDA,
	.scl_pin		= GPIO_USB_I2C_SCL,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device fsa_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_FSA9485_I2C_BUS_ID,
	.dev.platform_data	= &fsa_i2c_gpio_data,
};

static struct fsa9485_platform_data fsa9485_pdata = {
	.mhl_cb = NULL,
	.mhl_sel = cfg_mhl_sel,
	.usb_cb = fsa9485_usb_cb,
	.charger_cb = fsa9485_charger_cb,
};

static struct i2c_board_info micro_usb_i2c_devices_info[] __initdata = {
	{
		I2C_BOARD_INFO("fsa9485", 0x4A >> 1),
		.platform_data = &fsa9485_pdata,
		.irq = MSM_GPIO_TO_INT(14),
	},
};

#endif

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
static struct i2c_gpio_platform_data mhl_i2c_gpio_data = {
	.sda_pin   = GPIO_MHL_SDA,
	.scl_pin    = GPIO_MHL_SCL,
	.udelay    = 3,/*(i2c clk speed: 500khz / udelay)*/
};

static struct platform_device mhl_i2c_gpio_device = {
	.name       = "i2c-gpio",
	.id     = MSM_MHL_I2C_BUS_ID,
	.dev        = {
		.platform_data  = &mhl_i2c_gpio_data,
	},
};
/*
gpio_interrupt pin is very changable each different h/w_rev or  board.
*/
int get_mhl_int_irq(void)
{
	return  MSM_GPIO_TO_INT(GPIO_MHL_INT);
}

int n_pre_cfg_pw_state = 1;

static void sii9234_hw_onoff(bool onoff)
{
	int rc;
	/*VPH_PWR : mhl_power_source
	VMHL_3.3V, VSIL_A_1.2V, VMHL_1.8V
	just power control with HDMI_EN pin*/

	if (n_pre_cfg_pw_state) {
		gpio_request(GPIO_MHL_EN, NULL);
		n_pre_cfg_pw_state = 0;
	}

	if (onoff)
		gpio_direction_output(GPIO_MHL_EN, 1);
	else {
		gpio_direction_output(GPIO_MHL_EN, 0);
		usleep_range(10000, 20000);

		if (gpio_direction_output(GPIO_MHL_RST, 0))
			printk(KERN_ERR "%s error in making GPIO_MHL_RST Low\n"
			, __func__);
	}

	return;
}

static void sii9234_hw_reset(void)
{
	gpio_request(GPIO_MHL_RST, NULL);

	usleep_range(10000, 20000);
	if (gpio_direction_output(GPIO_MHL_RST, 1))
		printk(KERN_ERR "%s error in making GPIO_MHL_RST HIGH\n",
			 __func__);

	usleep_range(5000, 20000);
	if (gpio_direction_output(GPIO_MHL_RST, 0))
		printk(KERN_ERR "%s error in making GPIO_MHL_RST Low\n",
			 __func__);

	usleep_range(10000, 20000);
	if (gpio_direction_output(GPIO_MHL_RST, 1))
		printk(KERN_ERR "%s error in making GPIO_MHL_RST HIGH\n",
			 __func__);
	msleep(30);
}

struct sii9234_platform_data sii9234_pdata = {
	.get_irq = get_mhl_int_irq,
	.hw_onoff = sii9234_hw_onoff,
	.hw_reset = sii9234_hw_reset,
	.gpio = GPIO_MHL_SEL,
};

static struct i2c_board_info mhl_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("sii9234_mhl_tx", 0x72>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_tpi", 0x7A>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_hdmi_rx", 0x92>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_cbus", 0xC8>>1),
		.platform_data = &sii9234_pdata,
	},
};
#endif

#ifdef CONFIG_BATTERY_MAX17040
void max17040_hw_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_FUEKGAUGE_I2C_SCL, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_FUELGAUGE_I2C_SDA,  0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
	gpio_set_value(GPIO_FUEKGAUGE_I2C_SCL, 1);
	gpio_set_value(GPIO_FUELGAUGE_I2C_SDA, 1);

	gpio_tlmm_config(GPIO_CFG(GPIO_FUEL_INT,  0, GPIO_CFG_INPUT,
	 GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
}

static int max17040_low_batt_cb(void)
{
	pr_err("%s: Low battery alert\n", __func__);
}

static struct max17040_platform_data max17043_pdata = {
	.hw_init = max17040_hw_init,
	.low_batt_cb = max17040_low_batt_cb,
	.rcomp_value = 0xe71f,
};

static struct i2c_gpio_platform_data fuelgauge_i2c_gpio_data = {
	.sda_pin		= GPIO_FUELGAUGE_I2C_SDA,
	.scl_pin		= GPIO_FUEKGAUGE_I2C_SCL,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device fuelgauge_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= MSM_FUELGAUGE_I2C_BUS_ID,
	.dev.platform_data	= &fuelgauge_i2c_gpio_data,
};

static struct i2c_board_info fuelgauge_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("max17040", (0x6D >> 1)),
		.platform_data = &max17043_pdata,
		.irq		= MSM_GPIO_TO_INT(GPIO_FUEL_INT),
	}
};
#endif

#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
static struct i2c_gpio_platform_data opt_i2c_gpio_data = {
	.sda_pin = GPIO_SENSOR_ALS_SDA,
	.scl_pin = GPIO_SENSOR_ALS_SCL,
	.udelay = 5,
};

static struct platform_device opt_i2c_gpio_device = {
	.name = "i2c-gpio",
	.id = MSM_OPT_I2C_BUS_ID,
	.dev = {
		.platform_data = &opt_i2c_gpio_data,
	},
};

static struct i2c_board_info opt_i2c_borad_info[] = {
	{
#if defined(CONFIG_OPTICAL_GP2A)
		I2C_BOARD_INFO("gp2a", 0x88>>1),
#elif defined(CONFIG_OPTICAL_GP2AP020A00F)
		I2C_BOARD_INFO("gp2a", 0x72>>1),
#endif
	},
};

static void gp2a_led_onoff(int);

#if defined(CONFIG_OPTICAL_GP2A)
static struct opt_gp2a_platform_data opt_gp2a_data = {
	.gp2a_led_on	= gp2a_led_onoff,
	.power_on = sensor_power_on_vdd,
	.irq = MSM_GPIO_TO_INT(PMIC_GPIO_ALS_INT),
	.ps_status = PMIC_GPIO_ALS_INT,
};
#elif defined(CONFIG_OPTICAL_GP2AP020A00F)
static struct gp2a_platform_data opt_gp2a_data = {
	.gp2a_led_on	= gp2a_led_onoff,
	.p_out = PMIC_GPIO_ALS_INT,
};
#endif

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
	.dev        = {
		.platform_data  = &opt_gp2a_data,
	},
};
#endif

#ifdef CONFIG_MPU_SENSORS_MPU6050B1
#define SENSOR_MPU_NAME			"mpu6050B1"
static struct mpu_platform_data mpu_data = {
	.int_config = 0x12,
	.orientation = {0, 1, 0,
			1, 0, 0,
			0, 0, -1},
	/* accel */
	.accel = {
		  .get_slave_descr = mantis_get_slave_descr,
		  .adapt_num = MSM_SNS_I2C_BUS_ID,
		  .bus = EXT_SLAVE_BUS_SECONDARY,
		  .address = 0x68,
		  .orientation = {0, 1, 0,
				  1, 0, 0,
				  0, 0, -1},
		  },
	/* compass */
	.compass = {
		    .get_slave_descr = ak8975_get_slave_descr,
		    .adapt_num = MSM_SNS_I2C_BUS_ID,
		    .bus = EXT_SLAVE_BUS_PRIMARY,
		    .address = 0x0C,
		    .orientation = {0, -1, 0,
				    -1, 0, 0,
				    0, 0, -1},
		    },
};

static struct mpu_platform_data mpu_data_00 = {
	.int_config = 0x12,
	.orientation = {1, 0, 0,
			0, 1, 0,
			0, 0, 1},
	/* accel */
	.accel = {
		  .get_slave_descr = mantis_get_slave_descr,
		  .adapt_num = MSM_SNS_I2C_BUS_ID,
		  .bus = EXT_SLAVE_BUS_SECONDARY,
		  .address = 0x68,
		  .orientation = {1, 0, 0,
				  0, 1, 0,
				  0, 0, 1},
		  },
	/* compass */
	.compass = {
		    .get_slave_descr = ak8975_get_slave_descr,
		    .adapt_num = MSM_SNS_I2C_BUS_ID,
		    .bus = EXT_SLAVE_BUS_PRIMARY,
		    .address = 0x0C,
		    .orientation = {0, -1, 0,
				    1, 0, 0,
				    0, 0, 1},
		    },
};
#endif /*CONFIG_MPU_SENSORS_MPU6050B1 */

#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || defined(CONFIG_INPUT_BMP180)

#ifdef CONFIG_SENSORS_AK8975
static struct akm8975_platform_data akm8975_pdata = {
	.gpio_data_ready_int = GPIO_MSENSE_RST,
};
#endif

static struct i2c_gpio_platform_data sns_i2c_gpio_data = {
	.sda_pin = GPIO_SENSOR_SNS_SDA,
	.scl_pin = GPIO_SENSOR_SNS_SCL,
	.udelay = 5,
};

static struct platform_device sns_i2c_gpio_device = {
	.name = "i2c-gpio",
	.id = MSM_SNS_I2C_BUS_ID,
	.dev = {
		.platform_data  = &sns_i2c_gpio_data,
	},
};

static struct i2c_board_info sns_i2c_borad_info[] = {
#ifdef CONFIG_MPU_SENSORS_MPU6050B1
	{
	 I2C_BOARD_INFO(SENSOR_MPU_NAME, 0x68),
	 .irq = MSM_GPIO_TO_INT(GPIO_MPU3050_INT),
	 .platform_data = &mpu_data,
	 },
#endif
#ifdef CONFIG_SENSORS_AK8975
	{
		I2C_BOARD_INFO("ak8975", 0x0C),
		.platform_data = &akm8975_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_MSENSE_RST),
	},
#endif
#ifdef CONFIG_INPUT_BMP180
	{
		I2C_BOARD_INFO("bmp180", 0x77),
	},
#endif

};
#endif

#ifdef CONFIG_MPU_SENSORS_MPU6050B1
static void mpl_init(void)
{
	gpio_request(GPIO_MPU3050_INT, "MPUIRQ");
	gpio_direction_input(GPIO_MPU3050_INT);
	if (system_rev < BOARD_REV01)
		mpu_data = mpu_data_00;
}
#endif

#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
static void opt_init(void)
{
	gpio_tlmm_config(GPIO_CFG(PMIC_GPIO_ALS_INT, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_ALS_SDA, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_ALS_SCL, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_request(PMIC_GPIO_ALS_INT, "PSVOUT");
	gpio_direction_input(PMIC_GPIO_ALS_INT);
	gpio_request(GPIO_PS_EN, "PS_EN");

#if defined(CONFIG_OPTICAL_GP2AP020A00F)
	gpio_direction_output(GPIO_PS_EN, 1);
#else
	gpio_direction_output(GPIO_PS_EN, 0);
#endif
	gpio_free(PMIC_GPIO_ALS_INT);

}
#endif
#ifdef CONFIG_MSM_CAMERA

static uint16_t msm_cam_gpio_2d_tbl[] = {
	5, /*CAMIF_MCLK*/
	20, /*CAMIF_I2C_DATA*/
	21, /*CAMIF_I2C_CLK*/
};

static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_tbl = msm_cam_gpio_2d_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_2d_tbl),
};

struct msm_camera_sensor_strobe_flash_data strobe_flash_xenon = {
	.flash_trigger = GPIO_VFE_CAMIF_TIMER2,
	.flash_charge = GPIO_VFE_CAMIF_TIMER1,
	.flash_charge_done = GPIO_VFE_CAMIF_TIMER3_INT,
	.flash_recharge_duration = 50000,
	.irq = MSM_GPIO_TO_INT(GPIO_VFE_CAMIF_TIMER3_INT),
};

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = GPIO_CAM_GP_LED_EN1,
	._fsrc.ext_driver_src.led_flash_en = GPIO_CAM_GP_LED_EN2,
#if defined(CONFIG_I2C) && (defined(CONFIG_GPIO_SX150X) || \
			defined(CONFIG_GPIO_SX150X_MODULE))
	._fsrc.ext_driver_src.expander_info = cam_expander_info,
#endif
};
#endif

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 27648000,
		.ib  = 110592000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 140451840,
		.ib  = 561807360,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 274423680,
		.ib  = 1097694720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 302071680,
		.ib  = 1208286720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};

#ifdef CONFIG_IMX074_ACT
static struct i2c_board_info imx074_actuator_i2c_info = {
	I2C_BOARD_INFO("imx074_act", 0x11),
};

static struct msm_actuator_info imx074_actuator_info = {
	.board_info     = &imx074_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable     = 1,
};
#endif

#ifdef CONFIG_IMX074
static struct msm_camera_sensor_flash_data flash_imx074 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src
#endif
};

static struct msm_camera_sensor_platform_info sensor_board_info_imx074 = {
	.mount_angle	= 90,
	.sensor_reset	= 107,
	.sensor_pwd	= 85,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
};

static struct msm_camera_sensor_info msm_camera_sensor_imx074_data = {
	.sensor_name	= "imx074",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx074,
	.strobe_flash_data = &strobe_flash_xenon,
	.sensor_platform_info = &sensor_board_info_imx074,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_IMX074_ACT
	.actuator_info = &imx074_actuator_info
#endif
};

struct platform_device msm8960_camera_sensor_imx074 = {
	.name	= "msm_camera_imx074",
	.dev	= {
		.platform_data = &msm_camera_sensor_imx074_data,
	},
};
#endif
#ifdef CONFIG_OV2720
static struct msm_camera_sensor_flash_data flash_ov2720 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov2720 = {
	.mount_angle	= 0,
	.sensor_reset	= 76,
	.sensor_pwd	= 85,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov2720_data = {
	.sensor_name	= "ov2720",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_ov2720,
	.sensor_platform_info = &sensor_board_info_ov2720,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_ov2720 = {
	.name	= "msm_camera_ov2720",
	.dev	= {
		.platform_data = &msm_camera_sensor_ov2720_data,
	},
};
#endif

static struct msm_camera_sensor_flash_data flash_qs_mt9p017 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
};

static struct msm_camera_sensor_platform_info sensor_board_info_qs_mt9p017 = {
	.mount_angle	= 270,
	.sensor_reset	= 107,
	.sensor_pwd	= 85,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
};

static struct msm_camera_sensor_info msm_camera_sensor_qs_mt9p017_data = {
	.sensor_name	= "qs_mt9p017",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_qs_mt9p017,
	.sensor_platform_info = &sensor_board_info_qs_mt9p017,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_3D,
};

struct platform_device msm8960_camera_sensor_qs_mt9p017 = {
	.name	= "msm_camera_qs_mt9p017",
	.dev	= {
		.platform_data = &msm_camera_sensor_qs_mt9p017_data,
	},
};

static struct regulator *l11, *l12, *l18, *l29;
/* CAM power
	CAM_SENSOR_A_2.8		:  GPIO_CAM_A_EN(GPIO 46)
	CAM_SENSOR_IO_1.8		: VREG_L29		: l29
	CAM_AF_2.8				: VREG_L11		: l11
	CAM_SENSOR_CORE1.2		: VREG_L12		: l12
	CAM_ISP_CORE_1.2		: CAM_CORE_EN(GPIO 6)

	CAM_DVDD_1.5		: VREG_L18		: l18
*/

static void cam_ldo_power_on(int mode)
{
	int ret = 0;
	cam_info("cam_ldo_power_on\n");

/*5M Core 1.2V - CAM_ISP_CORE_1P2*/
	gpio_set_value_cansleep(CAM_CORE_EN, 1);
	usleep(1*1000);

/*VT core 1.2V - CAM_DVDD_1P2V*/
	l12 = regulator_get(NULL, "8921_l12");
	ret = regulator_set_voltage(l12, 1200000, 1200000);
	if (ret)
		cam_err("error setting voltage\n");
	ret = regulator_enable(l12);
	if (ret)
		cam_err("error enabling regulator\n");
	usleep(1*1000);

	l18 = regulator_get(NULL, "8921_l18");
	ret = regulator_set_voltage(l18, 1200000, 1200000);
	if (ret)
		cam_err("error setting voltage\n");
	ret = regulator_enable(l18);
	if (ret)
		cam_err("error enabling regulator\n");
	usleep(1*1000);

/*Sensor AVDD 2.8V - CAM_SENSOR_A2P8 */
	gpio_set_value_cansleep(GPIO_CAM_A_EN, 1);
	usleep(1*1000);

/*Sensor IO 1.8V -CAM_SENSOR_IO_1P8  */
	l29 = regulator_get(NULL, "8921_l29");
	ret = regulator_set_voltage(l29, 1800000, 1800000);
	if (ret)
		cam_err("error setting voltage\n");
	ret = regulator_enable(l29);
	if (ret)
		cam_err("error enabling regulator\n");
	usleep(1*1000);

/*Sensor AF 2.8V -CAM_AF_2P8  */
	if (!mode) {
		l11 = regulator_get(NULL, "8921_l11");
		ret = regulator_set_voltage(l11, 2800000, 2800000);
		if (ret)
			cam_err("error setting voltage\n");
		ret = regulator_enable(l11);
		if (ret)
			cam_err("error enabling regulator\n");
	}
}

static void cam_ldo_power_off(int mode)
{
	int ret = 0;
	cam_info("cam_ldo_power_off");

/*Sensor AF 2.8V -CAM_AF_2P8  */
	if (!mode) {
		if (l11) {
			ret = regulator_disable(l11);
			if (ret)
				cam_err("error disabling regulator\n");
		}
		usleep(1*1000);
	}

/*Sensor IO 1.8V -CAM_SENSOR_IO_1P8  */
	if (l29) {
		ret = regulator_disable(l29);
		if (ret)
			cam_err("error disabling regulator\n");
	}
	usleep(1*1000);

/*Sensor AVDD 2.8V - CAM_SENSOR_A2P8 */
	gpio_set_value_cansleep(GPIO_CAM_A_EN, 0);
	usleep(1*1000);

/*VT core 1.2 - CAM_DVDD_1P2V*/
	if (l18) {
		ret = regulator_disable(l18);
		if (ret)
			cam_err("error disabling regulator\n");
	}
	usleep(1*1000);

	if (l12) {
		ret = regulator_disable(l12);
		if (ret)
			cam_err("error disabling regulator\n");
	}
	usleep(1*1000);

/*5M Core 1.2V - CAM_ISP_CORE_1P2*/
	gpio_set_value_cansleep(CAM_CORE_EN, 0);
	usleep(1*1000);

}

#ifdef CONFIG_ISX012
static struct msm_camera_sensor_flash_data flash_isx012 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
};

static struct msm_camera_sensor_platform_info sensor_board_info_isx012 = {
	.mount_angle	= 90,
	.sensor_reset	= 107,
	.sensor_stby	= 4,
	.vt_sensor_reset	= 76,
	.vt_sensor_stby	= 18,
	.flash_en	= 3,
	.flash_set	= 2,
	.mclk	= 5,
	.sensor_pwd	= 6,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
	.sensor_power_on = cam_ldo_power_on,
	.sensor_power_off = cam_ldo_power_off,
};

static struct msm_camera_sensor_info msm_camera_sensor_isx012_data = {
	.sensor_name	= "isx012",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_isx012,
	.sensor_platform_info = &sensor_board_info_isx012,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_isx012 = {
	.name	= "msm_camera_isx012",
	.dev	= {
		.platform_data = &msm_camera_sensor_isx012_data,
	},
};
#endif
#ifdef CONFIG_S5K6AA
static struct msm_camera_sensor_flash_data flash_s5k6aa = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k6aa = {
	.mount_angle	= 90,
	.sensor_reset	= 107,
	.sensor_pwd	= 6,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6aa_data = {
	.sensor_name	= "s5k6aa",
	.pdata	= &msm_camera_csi_device_data[1],
	.sensor_platform_info = &sensor_board_info_s5k6aa,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_s5k6aa = {
	.name	= "msm_camera_s5k6aa",
	.dev	= {
		.platform_data = &msm_camera_sensor_s5k6aa_data,
	},
};
#endif

#ifdef CONFIG_S5K8AAY
static struct msm_camera_sensor_flash_data flash_s5k8aay = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k8aay = {
	.mount_angle	= 90,
	.sensor_reset	= CAM1_RST_N,
	.sensor_pwd	= CAM_CORE_EN,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
	.sensor_power_on = cam_ldo_power_on,
	.sensor_power_off = cam_ldo_power_off,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k8aay_data = {
	.sensor_name	= "s5k8aay",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k8aay,
	.sensor_platform_info = &sensor_board_info_s5k8aay,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
};

struct platform_device msm8960_camera_sensor_s5k8aay = {
	.name	= "msm_camera_s5k8aay",
	.dev	= {
		.platform_data = &msm_camera_sensor_s5k8aay_data,
	},
};
#endif
static struct platform_device *cam_dev[] = {
#ifdef CONFIG_ISX012
		&msm8960_camera_sensor_isx012,
#endif
#ifdef CONFIG_S5K6AA
		&msm8960_camera_sensor_s5k6aa,
#endif
#ifdef CONFIG_S5K8AAY
		&msm8960_camera_sensor_s5k8aay,
#endif
	};

static struct msm8960_privacy_light_cfg privacy_light_info = {
	.mpp = PM8921_MPP_PM_TO_SYS(12),
};

static void __init msm8960_init_cam(void)
{
	int i;
	struct msm_camera_sensor_info *s_info;

	msm8960_kenos_cam_init();

	gpio_tlmm_config(GPIO_CFG(CAM_CORE_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_CAM_A_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_VFE_CAMIF_TIMER1, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_VFE_CAMIF_TIMER2, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	/*Main cam reset */
	gpio_tlmm_config(GPIO_CFG(CAM1_RST_N, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	/*Main cam stby*/
	gpio_tlmm_config(GPIO_CFG(GPIO_VFE_CAMIF_TIMER3_INT, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	/*Front cam reset*/
	gpio_tlmm_config(GPIO_CFG(CAM2_RST_N, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	/*Front cam stby*/
	gpio_tlmm_config(GPIO_CFG(GPIO_VFE_CAMIF_TIMER_9A, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	/*Falsh Enable*/
	gpio_tlmm_config(GPIO_CFG(GPIO_VFE_CAMIF_TIMER1, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
	/*Flash Set*/
	gpio_tlmm_config(GPIO_CFG(GPIO_VFE_CAMIF_TIMER2, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);

	for (i = 0; i < ARRAY_SIZE(cam_dev); i++) {
		s_info = cam_dev[i]->dev.platform_data;
		msm_get_cam_resources(s_info);
		platform_device_register(cam_dev[i]);
	}
}
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
/* prim = 540 x 960 x 4(bpp) x 3(pages) */
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 3)
#elif CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT_PANEL
#define MSM_FB_PRIM_BUF_SIZE (540 * 960 * 4 * 3)
#else
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 3)
#endif
#else
/* prim = 540 x 960 x 4(bpp) x 2(pages) */
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 2)
#elif CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT_PANEL
#define MSM_FB_PRIM_BUF_SIZE (540 * 960 * 4 * 2)
#else
#define MSM_FB_PRIM_BUF_SIZE (480 * 800 * 4 * 2)
#endif
#endif

#ifdef CONFIG_FB_MSM_MIPI_DSI
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT_PANEL
/* 480 x 800 x 3 x 2 */
#define MIPI_DSI_WRITEBACK_SIZE (480 * 800 * 3 * 2)
#elif CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_CMD_QHD_PT_PANEL
/* 540 x 960 x 3 x 2 */
#define MIPI_DSI_WRITEBACK_SIZE (540 * 960 * 3 * 2)
#endif
#else
#define MIPI_DSI_WRITEBACK_SIZE 0
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
/* hdmi = 1920 x 1088 x 2(bpp) x 1(page) */
#define MSM_FB_EXT_BUF_SIZE 0x3FC000
#elif defined(CONFIG_FB_MSM_TVOUT)
/* tvout = 720 x 576 x 2(bpp) x 2(pages) */
#define MSM_FB_EXT_BUF_SIZE 0x195000
#else /* CONFIG_FB_MSM_HDMI_MSM_PANEL */
#define MSM_FB_EXT_BUF_SIZE 0
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_FB_MSM_OVERLAY_WRITEBACK
/* width x height x 3 bpp x 2 frame buffer */
#define MSM_FB_WRITEBACK_SIZE (1376 * 768 * 3 * 2)
#define MSM_FB_WRITEBACK_OFFSET  \
		(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE)
#else
#define MSM_FB_WRITEBACK_SIZE   0
#define MSM_FB_WRITEBACK_OFFSET 0
#endif

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
/* 4 bpp x 2 page HDMI case */
#define MSM_FB_SIZE roundup((1920 * 1088 * 4 * 2), 4096)
#else
/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE + \
				MSM_FB_WRITEBACK_SIZE, 4096)
#endif

static int writeback_offset(void)
{
	return MSM_FB_WRITEBACK_OFFSET;
}
/*
 * The GPIO_MDP_VSYNC should be GPIO 0.
 * But a current device is using GPIO 19 instead of it.
 * So GPIO 19 is used as GPIO_MDP_VSYNC for the device.
 * It will be changed to GPIO 0 again on next version of device.
 */
#define PANEL_NAME_MAX_LEN	30
#define MIPI_CMD_NOVATEK_QHD_PANEL_NAME	"mipi_cmd_novatek_qhd"
#define MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME	"mipi_video_novatek_qhd"
#define MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME	"mipi_video_toshiba_wsvga"
#define MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME	"mipi_video_chimei_wxga"
#define MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME	"mipi_video_simulator_vga"
#define MIPI_CMD_RENESAS_FWVGA_PANEL_NAME	"mipi_cmd_renesas_fwvga"
#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (machine_is_msm8960_liquid()) {
		if (!strncmp(name, MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
	} else {
		if (!strncmp(name, MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

#ifndef CONFIG_FB_MSM_MIPI_PANEL_DETECT
		if (!strncmp(name, MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
				strnlen(MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
#endif
	}

	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	if (!strncmp(name, TVOUT_PANEL_NAME,
			strnlen(TVOUT_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static bool n_dsi_power_on = true;

/**
 * LiQUID panel on/off
 *
 * @param on
 *
 * @return int
 */
static int mipi_dsi_liquid_panel_power(int on)
{
	static struct regulator *reg_l2, *reg_ext_3p3v;
	static int gpio21, gpio24, gpio43;
	int rc;

	pr_info("%s: on=%d\n", __func__, on);

	gpio21 = PM8921_GPIO_PM_TO_SYS(21); /* disp power enable_n */
	gpio43 = PM8921_GPIO_PM_TO_SYS(43); /* Displays Enable (rst_n)*/
	gpio24 = PM8921_GPIO_PM_TO_SYS(24); /* Backlight PWM */

	if (n_dsi_power_on) {

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		reg_ext_3p3v = regulator_get(&msm_mipi_dsi1_device.dev,
			"vdd_lvds_3p3v");
		if (IS_ERR(reg_ext_3p3v)) {
			pr_err("could not get reg_ext_3p3v, rc = %ld\n",
			       PTR_ERR(reg_ext_3p3v));
		    return -ENODEV;
		}

		rc = gpio_request(gpio21, "disp_pwr_en_n");
		if (rc) {
			pr_err("request gpio 21 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio24, "disp_backlight_pwm");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		n_dsi_power_on = false;
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_ext_3p3v);
		if (rc) {
			pr_err("enable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}

		/* set reset pin before power enable */
		gpio_set_value_cansleep(gpio43, 0); /* disp disable (resx=0) */

		gpio_set_value_cansleep(gpio21, 0); /* disp power enable_n */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 0); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
	} else {
		gpio_set_value_cansleep(gpio43, 0);
		gpio_set_value_cansleep(gpio21, 1);

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_ext_3p3v);
		if (rc) {
			pr_err("disable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}

static int mipi_dsi_cdp_panel_power(int on)
{
	static struct regulator *reg_l8, *reg_l23, *reg_l2;
	static int gpio43;
	int rc;

	struct pm_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 1,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	pr_info("%s: state : %d\n", __func__, on);

	gpio43 = PM8921_GPIO_PM_TO_SYS(43);
	if (n_dsi_power_on) {

		reg_l8 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdc");
		if (IS_ERR(reg_l8)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8));
			return -ENODEV;
		}

		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l8, 3000000, 3000000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		/*
		 * turn VDD3.0, VDD1.8 on
		 */
		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		msleep(20);
		rc = regulator_enable(reg_l8);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = pm8xxx_gpio_config(gpio43, &gpio43_param);
		if (rc) {
			pr_err("gpio_config 43 failed (3), rc=%d\n", rc);
			return -EINVAL;
		}

		gpio_direction_output(gpio43, 1);

		n_dsi_power_on = false;

		return 0;
	}

	if (on) {
		/*
		 * turn VDD3.0, VDD1.8 on
		 */
		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		msleep(20);
		rc = regulator_enable(reg_l8);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		/*
		 * Wait 25ms
		 */
		msleep(25);
		/*
		 * Active Reset
		 */
		gpio_set_value_cansleep(gpio43, 1);
		msleep(20);
		gpio_set_value_cansleep(gpio43, 0);
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1);
		msleep(20);
	} else {
		gpio_set_value_cansleep(gpio43, 0);

		rc = regulator_disable(reg_l8);
		if (rc) {
			pr_err("disable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}


		rc = regulator_set_optimum_mode(reg_l8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

	}
	return 0;
}

static int mipi_dsi_panel_power(int on)
{
	int ret;

	pr_info("%s: on=%d\n", __func__, on);

	if (machine_is_msm8960_liquid())
		ret = mipi_dsi_liquid_panel_power(on);
	else
		ret = mipi_dsi_cdp_panel_power(on);

	return ret;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = GPIO_MDP_VSYNC,
	.dsi_power_save = mipi_dsi_panel_power,
};

#ifdef CONFIG_MSM_BUS_SCALING

static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
static struct msm_bus_vectors hdmi_as_primary_vectors[] = {
	/* If HDMI is used as primary */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 2000000000,
		.ib = 2000000000,
	},
};
static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
};
#else
static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};
#endif

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

#endif

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
int mdp_core_clk_rate_table[] = {
	200000000,
	200000000,
	200000000,
	200000000,
};
#else
int mdp_core_clk_rate_table[] = {
	85330000,
	85330000,
	160000000,
	200000000,
};
#endif

static struct msm_panel_common_pdata mdp_pdata =  {
	.gpio = GPIO_MDP_VSYNC,
#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
	.mdp_core_clk_rate = 200000000,
#else
	.mdp_core_clk_rate = 85330000,
#endif
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_42,
	.writeback_offset = writeback_offset,
};

#if 1
static struct platform_device mipi_dsi_renesas_panel_device = {
	.name = "mipi_renesas",
	.id = 0,
};

static struct platform_device mipi_dsi_simulator_panel_device = {
	.name = "mipi_simulator",
	.id = 0,
};

#define LPM_CHANNEL0 0
static int toshiba_gpio[] = {LPM_CHANNEL0};

static struct mipi_dsi_panel_platform_data toshiba_pdata = {
	.gpio = toshiba_gpio,
};

static struct platform_device mipi_dsi_toshiba_panel_device = {
	.name = "mipi_toshiba",
	.id = 0,
	.dev = {
		.platform_data = &toshiba_pdata,
	}
};

static int dsi2lvds_gpio[2] = {
	0,/* Backlight PWM-ID=0 for PMIC-GPIO#24 */
	0x1F08 /* DSI2LVDS Bridge GPIO Output, mask=0x1f, out=0x08 */
	};

static struct msm_panel_common_pdata mipi_dsi2lvds_pdata = {
	.gpio_num = dsi2lvds_gpio,
};

static struct mipi_dsi_phy_ctrl dsi_novatek_cmd_mode_phy_db = {

/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
	{0x0F, 0x0a, 0x04, 0x00, 0x20},	/* regulator */
	/* timing   */
	{0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
	0x0c, 0x03, 0x04, 0xa0},
	{0x5f, 0x00, 0x00, 0x10},	/* phy ctrl */
	{0xff, 0x00, 0x06, 0x00},	/* strength */
	/* pll control */
	{0x40, 0xf9, 0x30, 0xda, 0x00, 0x40, 0x03, 0x62,
	0x40, 0x07, 0x03,
	0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

static struct mipi_dsi_panel_platform_data novatek_pdata = {
	.phy_ctrl_settings = &dsi_novatek_cmd_mode_phy_db,
};

static struct platform_device mipi_dsi_novatek_panel_device = {
	.name = "mipi_novatek",
	.id = 0,
	.dev = {
		.platform_data = &novatek_pdata,
	}
};

static struct platform_device mipi_dsi2lvds_bridge_device = {
	.name = "mipi_tc358764",
	.id = 0,
	.dev.platform_data = &mipi_dsi2lvds_pdata,
};
static struct platform_device mipi_dsi_samsung_oled_panel_device = {
	.name = "mipi_samsung_oled",
	.id = 0,
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};
#ifdef CONFIG_SAMSUNG_HDMI_ENABLE_POWER
static int hdmi_gpio_enable(void);
static int hdmi_gpio_enable(void)
{
	int rc;
	rc = gpio_request(GPIO_HDMI_EN1, "GPIO_HDMI_EN1");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"GPIO_HDMI_EN1", 0, rc);
			goto error1;
		}
		rc = gpio_request(GPIO_HDMI_EN2, "GPIO_HDMI_EN2");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"GPIO_HDMI_EN2", 1, rc);
			goto error2;
		}

	gpio_set_value(GPIO_HDMI_EN1, 1);
	gpio_set_value(GPIO_HDMI_EN2, 1);
	return 0;

error1:
	gpio_free(GPIO_HDMI_EN1);
error2:
	gpio_free(GPIO_HDMI_EN2);
}
#endif
static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
#ifdef CONFIG_SAMSUNG_HDMI_ENABLE_POWER
	.hdmi_enable = hdmi_gpio_enable,
#endif
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 2000000000,
		.ib = 2000000000,
	},
};
#else
static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};
#endif

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
};
#endif

static void __init msm_fb_add_devices(void)
{
	struct platform_device *ptr = NULL;

	if (machine_is_msm8960_liquid())
		ptr = &mipi_dsi2lvds_bridge_device;
	else
		ptr = &mipi_dsi_samsung_oled_panel_device;
	platform_add_devices(&ptr, 1);

	if (machine_is_msm8x60_rumi3()) {
		msm_fb_register_device("mdp", NULL);
		mipi_dsi_pdata.target_type = 1;
	} else
		msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_MSM_BUS_SCALING
	msm_fb_register_device("dtv", &dtv_pdata);
#endif
}

static struct gpiomux_setting mdp_vsync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8960_mdp_vsync_configs[] __initdata = {
	{
		.gpio = GPIO_MDP_VSYNC,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdp_vsync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_vsync_suspend_cfg,
		},
	}
};

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

static void samsung_sys_class_init(void)
{
	pr_info("samsung sys class init.\n");

	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	pr_info("samsung sys class end.\n");
};

#ifdef CONFIG_USB_SWITCH_FSA9485
static struct gpiomux_setting fsa9485_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting fsa9485_suspend_1_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
static struct gpiomux_setting fsa9485_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8960_fsa9485_configs[] __initdata = {
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_ACTIVE]	= &fsa9485_active_cfg,
			[GPIOMUX_SUSPENDED] = &fsa9485_suspend_cfg,
		},
	},

	{
		.gpio = 73,
		.settings = {
			[GPIOMUX_ACTIVE]	= &fsa9485_suspend_1_cfg,
			[GPIOMUX_SUSPENDED] = &fsa9485_suspend_cfg,
		},
	},

	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_ACTIVE]	= &fsa9485_suspend_1_cfg,
			[GPIOMUX_SUSPENDED] = &fsa9485_suspend_cfg,
		},
	},
};
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_suspend_1_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting hdmi_active_3_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	/*.pull = GPIOMUX_PULL_UP,*/
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8960_hdmi_configs[] __initdata = {
#ifdef CONFIG_SAMSUNG_HDMI_ENABLE_POWER
	{

		.gpio = 0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
	{
		.gpio = 1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
#endif
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 101,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
};

#if defined(CONFIG_NFC_PN544)
static int __init pn544_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_NFC_IRQ, 0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);

	return 0;
}
#endif

#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || defined(CONFIG_INPUT_BMP180)

static int __init sensor_device_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_SNS_SDA, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_SNS_SCL, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	sensor_power_on_vdd(1);

	return 0;
}
#endif

#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || \
	defined(CONFIG_INPUT_BMP180) || defined(CONFIG_OPTICAL_GP2A)
static struct regulator *vsensor_2p85, *vsensor_1p8;
static int sensor_power_2p85_cnt;

static void sensor_power_on_vdd(int onoff)
{
	int ret;

	if (vsensor_2p85 == NULL) {
		vsensor_2p85 = regulator_get(NULL, "8921_l9");
		if (IS_ERR(vsensor_2p85))
			return ;

		ret = regulator_set_voltage(vsensor_2p85, 2850000, 2850000);
		if (ret)
			pr_err("%s: error vsensor_2p85 setting voltage ret=%d\n",
				__func__, ret);
	}
	if (vsensor_1p8 == NULL) {
		vsensor_1p8 = regulator_get(NULL, "8921_lvs4");
		if (IS_ERR(vsensor_1p8))
			return ;

		ret = regulator_set_voltage(vsensor_1p8, 1800000, 1800000);
		if (ret)
			pr_err("%s: error vsensor_1p8 setting voltage ret=%d\n",
				__func__, ret);
	}
	if (onoff) {
		sensor_power_2p85_cnt++;
		ret = regulator_enable(vsensor_2p85);
		if (ret)
			pr_err("%s: error enabling regulator\n", __func__);

		ret = regulator_enable(vsensor_1p8);
		if (ret)
			pr_err("%s: error enabling regulator\n", __func__);
	} else {
		sensor_power_2p85_cnt--;
		if (regulator_is_enabled(vsensor_2p85)) {
			ret = regulator_disable(vsensor_2p85);
			if (ret)
				pr_err("%s: error vsensor_2p85 enabling regulator\n",
				__func__);
		}
		if (regulator_is_enabled(vsensor_1p8)) {
			ret = regulator_disable(vsensor_1p8);
			if (ret)
				pr_err("%s: error vsensor_1p8 enabling regulator\n",
				__func__);
		}
	}
}

#endif

#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
static void gp2a_led_onoff(int onoff)
{
	static struct regulator *reg_8921_leda;
	static int prev_on;
	int rc;

	if (system_rev < BOARD_REV01) {
		gpio_set_value(GPIO_PS_EN, onoff ? 1 : 0);
	} else {
		if (onoff == prev_on)
			return;

		if (!reg_8921_leda) {
			reg_8921_leda = regulator_get(NULL, "8921_l16");
			rc = regulator_set_voltage(reg_8921_leda,
				3000000, 3000000);
			if (rc)
				pr_err("%s: error reg_8921_leda setting  ret=%d\n",
					__func__, rc);
		}

		if (onoff) {
			rc = regulator_enable(reg_8921_leda);
			if (rc) {
				pr_err("'%s' regulator enable failed, rc=%d\n",
					"reg_8921_leda", rc);
				return;
			}
			pr_debug("%s(on): success\n", __func__);
		} else {
			rc = regulator_disable(reg_8921_leda);
			if (rc) {
				pr_err("'%s' regulator disable failed, rc=%d\n",
					"reg_8921_leda", rc);
				return;
			}
			pr_debug("%s(off): success\n", __func__);
		}
		prev_on = onoff;
	}
}

#endif

static int hdmi_enable_5v(int on)
{
	/* TBD: PM8921 regulator instead of 8901 */
	static struct regulator *reg_8921_hdmi_mvs;	/* HDMI_5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8921_hdmi_mvs)
		reg_8921_hdmi_mvs = regulator_get(&hdmi_msm_device.dev,
			"hdmi_mvs");

	if (on) {
		rc = regulator_enable(reg_8921_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_l23, *reg_8921_s4;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_8921_l23) {
		reg_8921_l23 = regulator_get(&hdmi_msm_device.dev, "hdmi_avdd");
		if (IS_ERR(reg_8921_l23)) {
			pr_err("could not get reg_8921_l23, rc = %ld\n",
				PTR_ERR(reg_8921_l23));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_l23, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev, "hdmi_vcc");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_8921_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_8921_l23);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_avdd", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vcc", rc);
			return rc;
		}
		rc = gpio_request(100, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 100, rc);
			goto error1;
		}
		rc = gpio_request(101, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 101, rc);
			goto error2;
		}
		rc = gpio_request(102, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 102, rc);
			goto error3;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(100);
		gpio_free(101);
		gpio_free(102);

		rc = regulator_disable(reg_8921_l23);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_8921_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

error3:
	gpio_free(101);
error2:
	gpio_free(100);
error1:
	regulator_disable(reg_8921_l23);
	regulator_disable(reg_8921_s4);
	return rc;
}

static int hdmi_cec_power(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(99, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", 99, rc);
			goto error;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(99);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

static void __init msm8960_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));

}
#ifdef CONFIG_WCD9310_CODEC

#define TABLA_INTERRUPT_BASE (NR_MSM_IRQS + NR_GPIO_IRQS + NR_PM8921_IRQS)

/* Micbias setting is based on 8660 CDP/MTP/FLUID requirement
 * 4 micbiases are used to power various analog and digital
 * microphones operating at 1800 mV. Technically, all micbiases
 * can source from single cfilter since all microphones operate
 * at the same voltage level. The arrangement below is to make
 * sure all cfilters are exercised. LDO_H regulator ouput level
 * does not need to be as high as 2.85V. It is choosen for
 * microphone sensitivity purpose.
 */
static struct tabla_pdata tabla_platform_data = {
	.slimbus_slave_device = {
		.name = "tabla-slave",
		.e_addr = {0, 0, 0x10, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_TABLA_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(38),
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 1800,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	}
};

static struct slim_device msm_slim_tabla = {
	.name = "tabla-slim",
	.e_addr = {0, 1, 0x10, 0, 0x17, 2},
	.dev = {
		.platform_data = &tabla_platform_data,
	},
};
#endif

static struct slim_boardinfo msm_slim_devices[] = {
#ifdef CONFIG_WCD9310_CODEC
	{
		.bus_num = 1,
		.slim_slave = &msm_slim_tabla,
	},
#endif
	/* add more slimbus slaves as needed */
};

#define MSM_WCNSS_PHYS	0x03000000
#define MSM_WCNSS_SIZE	0x280000

static struct resource resources_wcnss_wlan[] = {
	{
		.start	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.end	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.name	= "wcnss_wlanrx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.end	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.name	= "wcnss_wlantx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_WCNSS_PHYS,
		.end	= MSM_WCNSS_PHYS + MSM_WCNSS_SIZE - 1,
		.name	= "wcnss_mmio",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 84,
		.end	= 88,
		.name	= "wcnss_gpios_5wire",
		.flags	= IORESOURCE_IO,
	},
};

static struct qcom_wcnss_opts qcom_wcnss_pdata = {
	.has_48mhz_xo	= 1,
};

static struct platform_device msm_device_wcnss_wlan = {
	.name		= "wcnss_wlan",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_wcnss_wlan),
	.resource	= resources_wcnss_wlan,
	.dev		= {.platform_data = &qcom_wcnss_pdata},
};

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000

#define QCE_HW_KEY_SUPPORT	0
#define QCE_SHA_HMAC_SUPPORT	1
#define QCE_SHARE_CE_RESOURCE	1
#define QCE_CE_SHARED		0

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
};

static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif


static int __init gpiomux_init(void)
{
	int rc;

	rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err(KERN_ERR "msm_gpiomux_init failed %d\n", rc);
		return rc;
	}

	msm_gpiomux_install(msm8960_cam_common_configs,
			ARRAY_SIZE(msm8960_cam_common_configs));

	msm_gpiomux_install(msm8960_gpiomux_configs,
			ARRAY_SIZE(msm8960_gpiomux_configs));

	msm_gpiomux_install(msm8960_gsbi_configs,
			ARRAY_SIZE(msm8960_gsbi_configs));

	msm_gpiomux_install(msm8960_cyts_configs,
			ARRAY_SIZE(msm8960_cyts_configs));

	msm_gpiomux_install(msm8960_sec_ts_configs,
			ARRAY_SIZE(msm8960_sec_ts_configs));

	msm_gpiomux_install(msm8960_slimbus_config,
			ARRAY_SIZE(msm8960_slimbus_config));

	msm_gpiomux_install(msm8960_audio_codec_configs,
			ARRAY_SIZE(msm8960_audio_codec_configs));
/* Samsung not use auxpcm(external codec) */
/*
	msm_gpiomux_install(msm8960_audio_auxpcm_configs,
			ARRAY_SIZE(msm8960_audio_auxpcm_configs));
*/
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	msm_gpiomux_install(msm8960_hdmi_configs,
			ARRAY_SIZE(msm8960_hdmi_configs));
#endif

	msm_gpiomux_install(msm8960_mdp_vsync_configs,
			ARRAY_SIZE(msm8960_mdp_vsync_configs));

	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));

#ifdef CONFIG_USB_SWITCH_FSA9485
	msm_gpiomux_install(msm8960_fsa9485_configs,
			ARRAY_SIZE(msm8960_fsa9485_configs));
#endif
	return 0;
}

#define MSM_SHARED_RAM_PHYS 0x80000000

static struct pm8921_adc_amux pm8921_adc_channels_data[] = {
	{"vcoin", CHANNEL_VCOIN, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vbat", CHANNEL_VBAT, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"dcin", CHANNEL_DCIN, CHAN_PATH_SCALING4, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ichg", CHANNEL_ICHG, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vph_pwr", CHANNEL_VPH_PWR, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ibat", CHANNEL_IBAT, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"batt_therm", CHANNEL_BATT_THERM, CHAN_PATH_SCALING1, AMUX_RSV2,
		ADC_DECIMATION_TYPE2, ADC_SCALE_BATT_THERM},
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"usbin", CHANNEL_USBIN, CHAN_PATH_SCALING3, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pmic_therm", CHANNEL_DIE_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PMIC_THERM},
	{"625mv", CHANNEL_625MV, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"125v", CHANNEL_125V, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"chg_temp", CHANNEL_CHG_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pa_therm1", ADC_MPP_1_AMUX8, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"pa_therm0", ADC_MPP_1_AMUX3, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
};

static struct pm8921_adc_properties pm8921_adc_data = {
	.adc_vdd_reference	= 1800, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar                = 0,
};

static struct pm8921_adc_platform_data pm8921_adc_pdata = {
	.adc_channel		= pm8921_adc_channels_data,
	.adc_num_board_channel	= ARRAY_SIZE(pm8921_adc_channels_data),
	.adc_prop		= &pm8921_adc_data,
	.adc_mpp_base		= PM8921_MPP_PM_TO_SYS(1),
};

static void __init msm8960_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8960_io();

	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");
#ifdef CONFIG_SEC_DEBUG
	sec_getlog_supply_meminfo(0x40000000, 0x80000000, 0x00, 0x00);
#endif

}

static void __init msm8960_init_irq(void)
{
	unsigned int i;

	msm_mpm_irq_extn_init();
	gic_init(0, GIC_PPI_START, MSM_QGIC_DIST_BASE,
						(void *)MSM_QGIC_CPU_BASE);

	/* Edge trigger PPIs except AVS_SVICINT and AVS_SVICINTSWDONE */
	writel_relaxed(0xFFFFD7FF, MSM_QGIC_DIST_BASE + GIC_DIST_CONFIG + 4);

	writel_relaxed(0x0000FFFF, MSM_QGIC_DIST_BASE + GIC_DIST_ENABLE_SET);
	mb();

	/* FIXME: Not installing AVS_SVICINT and AVS_SVICINTSWDONE yet
	 * as they are configured as level, which does not play nice with
	 * handle_percpu_irq.
	 */
	for (i = GIC_PPI_START; i < GIC_SPI_START; i++) {
		if (i != AVS_SVICINT && i != AVS_SVICINTSWDONE)
			irq_set_handler(i, handle_percpu_irq);
	}
}

/* MSM8960 have 5 SDCC controllers */
enum sdcc_controllers {
	SDCC1,
	SDCC2,
	SDCC3,
	SDCC4,
	SDCC5,
	MAX_SDCC_CONTROLLER
};

/* All SDCC controllers requires VDD/VCC voltage */
static struct msm_mmc_reg_data mmc_vdd_reg_data[MAX_SDCC_CONTROLLER] = {
	/* SDCC1 : eMMC card connected */
	[SDCC1] = {
		.name = "sdc_vdd",
		.set_voltage_sup = 1,
		.high_vol_level = 2950000,
		.low_vol_level = 2950000,
		.always_on = 1,
		.lpm_sup = 1,
		.lpm_uA = 9000,
		.hpm_uA = 200000, /* 200mA */
	},
	/* SDCC3 : External card slot connected */
	[SDCC3] = {
		.name = "sdc_vdd",
		.set_voltage_sup = 1,
		.high_vol_level = 2950000,
		.low_vol_level = 2950000,
		.hpm_uA = 600000, /* 600mA */
	}
};

/* Only slots having eMMC card will require VCCQ voltage */
static struct msm_mmc_reg_data mmc_vccq_reg_data[1] = {
	/* SDCC1 : eMMC card connected */
	[SDCC1] = {
		.name = "sdc_vccq",
		.set_voltage_sup = 1,
		.always_on = 1,
		.high_vol_level = 1800000,
		.low_vol_level = 1800000,
		.hpm_uA = 200000, /* 200mA */
	}
};

/* All SDCC controllers may require voting for VDD PAD voltage */
static struct msm_mmc_reg_data mmc_vddp_reg_data[MAX_SDCC_CONTROLLER] = {
	/* SDCC3 : External card slot connected */
	[SDCC3] = {
		.name = "sdc_vddp",
		.set_voltage_sup = 1,
		.high_vol_level = 2950000,
		.low_vol_level = 1850000,
		.always_on = 1,
		.lpm_sup = 1,
		/* Max. Active current required is 16 mA */
		.hpm_uA = 16000,
		/*
		 * Sleep current required is ~300 uA. But min. vote can be
		 * in terms of mA (min. 1 mA). So let's vote for 2 mA
		 * during sleep.
		 */
		.lpm_uA = 2000,
	}
};

static struct msm_mmc_slot_reg_data mmc_slot_vreg_data[MAX_SDCC_CONTROLLER] = {
	/* SDCC1 : eMMC card connected */
	[SDCC1] = {
		.vdd_data = &mmc_vdd_reg_data[SDCC1],
		.vccq_data = &mmc_vccq_reg_data[SDCC1],
	},
	/* SDCC3 : External card slot connected */
	[SDCC3] = {
		.vdd_data = &mmc_vdd_reg_data[SDCC3],
		.vddp_data = &mmc_vddp_reg_data[SDCC3],
	}
};

/* SDC1 pad data */
static struct msm_mmc_pad_drv sdc1_pad_drv_on_cfg[] = {
	{TLMM_HDRV_SDC1_CLK, GPIO_CFG_16MA},
	{TLMM_HDRV_SDC1_CMD, GPIO_CFG_10MA},
	{TLMM_HDRV_SDC1_DATA, GPIO_CFG_10MA}
};

static struct msm_mmc_pad_drv sdc1_pad_drv_off_cfg[] = {
	{TLMM_HDRV_SDC1_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC1_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC1_DATA, GPIO_CFG_2MA}
};

static struct msm_mmc_pad_pull sdc1_pad_pull_on_cfg[] = {
	{TLMM_PULL_SDC1_CLK, GPIO_CFG_NO_PULL},
	{TLMM_PULL_SDC1_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC1_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_mmc_pad_pull sdc1_pad_pull_off_cfg[] = {
	{TLMM_PULL_SDC1_CLK, GPIO_CFG_NO_PULL},
	{TLMM_PULL_SDC1_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC1_DATA, GPIO_CFG_PULL_DOWN}
};

/* SDC3 pad data */
static struct msm_mmc_pad_drv sdc3_pad_drv_on_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_8MA}
};

static struct msm_mmc_pad_drv sdc3_pad_drv_off_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_2MA}
};

static struct msm_mmc_pad_pull sdc3_pad_pull_on_cfg[] = {
	{TLMM_PULL_SDC3_CLK, GPIO_CFG_NO_PULL},
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_mmc_pad_pull sdc3_pad_pull_off_cfg[] = {
	{TLMM_PULL_SDC3_CLK, GPIO_CFG_NO_PULL},
	/*
	 * SDC3 CMD line should be PULLed UP otherwise fluid platform will
	 * see transitions (1 -> 0 and 0 -> 1) on card detection line,
	 * which would result in false card detection interrupts.
	 */
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	/*
	 * Keeping DATA lines status to PULL UP will make sure that
	 * there is no current leak during sleep if external pull up
	 * is connected to DATA lines.
	 */
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};

struct msm_mmc_pad_pull_data mmc_pad_pull_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.on = sdc1_pad_pull_on_cfg,
		.off = sdc1_pad_pull_off_cfg,
		.size = ARRAY_SIZE(sdc1_pad_pull_on_cfg)
	},
	[SDCC3] = {
		.on = sdc3_pad_pull_on_cfg,
		.off = sdc3_pad_pull_off_cfg,
		.size = ARRAY_SIZE(sdc3_pad_pull_on_cfg)
	},
};

struct msm_mmc_pad_drv_data mmc_pad_drv_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.on = sdc1_pad_drv_on_cfg,
		.off = sdc1_pad_drv_off_cfg,
		.size = ARRAY_SIZE(sdc1_pad_drv_on_cfg)
	},
	[SDCC3] = {
		.on = sdc3_pad_drv_on_cfg,
		.off = sdc3_pad_drv_off_cfg,
		.size = ARRAY_SIZE(sdc3_pad_drv_on_cfg)
	},
};

struct msm_mmc_pad_data mmc_pad_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.pull = &mmc_pad_pull_data[SDCC1],
		.drv = &mmc_pad_drv_data[SDCC1]
	},
	[SDCC3] = {
		.pull = &mmc_pad_pull_data[SDCC3],
		.drv = &mmc_pad_drv_data[SDCC3]
	},
};

struct msm_mmc_pin_data mmc_slot_pin_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.pad_data = &mmc_pad_data[SDCC1],
	},
	[SDCC3] = {
		.pad_data = &mmc_pad_data[SDCC3],
	},
};

static unsigned int sdc1_sup_clk_rates[] = {
	400000, 24000000, 48000000
};

static unsigned int sdc3_sup_clk_rates[] = {
	400000, 24000000, 48000000, 96000000
};

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct mmc_platform_data msm8960_sdc1_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.sup_clk_table	= sdc1_sup_clk_rates,
	.sup_clk_cnt	= ARRAY_SIZE(sdc1_sup_clk_rates),
	.pclk_src_dfab	= 1,
	.nonremovable	= 1,
	.sdcc_v4_sup	= true,
	.vreg_data	= &mmc_slot_vreg_data[SDCC1],
	.pin_data	= &mmc_slot_pin_data[SDCC1]
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm8960_sdc3_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.sup_clk_table	= sdc3_sup_clk_rates,
	.sup_clk_cnt	= ARRAY_SIZE(sdc3_sup_clk_rates),
	.pclk_src_dfab	= 1,
#ifdef CONFIG_MMC_MSM_SDC3_WP_SUPPORT
	.wpswitch_gpio	= PM8921_GPIO_PM_TO_SYS(16),
#endif
	.sdcc_v4_sup	= true,
	.vreg_data	= &mmc_slot_vreg_data[SDCC3],
	.pin_data	= &mmc_slot_pin_data[SDCC3],
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status_gpio	= PM8921_GPIO_PM_TO_SYS(26),
	.status_irq	= PM8921_GPIO_IRQ(PM8921_IRQ_BASE, 26),
	.irq_flags	= IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.xpc_cap	= 1,
	.uhs_caps	= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
			MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_DDR50 |
			MMC_CAP_MAX_CURRENT_600)
};
#endif

static void __init msm8960_init_mmc(void)
{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	/* SDC1 : eMMC card connected */
	msm_add_sdcc(1, &msm8960_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	/* SDC3: External card slot */
	msm_add_sdcc(3, &msm8960_sdc3_data);
#endif
}

static void __init msm8960_init_buses(void)
{
#ifdef CONFIG_MSM_BUS_SCALING
	msm_bus_rpm_set_mt_mask();
	msm_bus_8960_apps_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_sys_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_mm_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_sys_fpb_pdata.rpm_enabled = 1;
	msm_bus_8960_cpss_fpb_pdata.rpm_enabled = 1;
	msm_bus_apps_fabric.dev.platform_data = &msm_bus_8960_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_8960_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_8960_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_8960_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_8960_cpss_fpb_pdata;
	
#endif
}

static struct msm_spi_platform_data msm8960_qup_spi_gsbi1_pdata = {
	.max_clock_speed = 15060000,
};

#ifdef CONFIG_USB_MSM_OTG_72K
static struct msm_otg_platform_data msm_otg_pdata;
#else
static void msm_hsusb_vbus_power(bool on)
{
	int rc;
	static bool vbus_is_on;
	static struct regulator *mvs_otg_switch;
	struct pm_gpio param = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 1,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength	= PM_GPIO_STRENGTH_MED,
		.function	= PM_GPIO_FUNC_NORMAL,
	};

	if (vbus_is_on == on)
		return;

	if (on) {
		mvs_otg_switch = regulator_get(&msm8960_device_otg.dev,
					       "vbus_otg");
		if (IS_ERR(mvs_otg_switch)) {
			pr_err("Unable to get mvs_otg_switch\n");
			return;
		}

		rc = gpio_request(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN),
						"usb_5v_en");
		if (rc < 0) {
			pr_err("failed to request usb_5v_en gpio\n");
			goto put_mvs_otg;
		}

		if (regulator_enable(mvs_otg_switch)) {
			pr_err("unable to enable mvs_otg_switch\n");
			goto free_usb_5v_en;
		}

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN),
				&param);
		if (rc < 0) {
			pr_err("failed to configure usb_5v_en gpio\n");
			goto disable_mvs_otg;
		}
		vbus_is_on = true;
		return;
	} else {
		gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN),
				0);
#ifdef CONFIG_USB_SWITCH_FSA9485
		fsa9485_otg_detach();
#endif
	}
disable_mvs_otg:
		regulator_disable(mvs_otg_switch);
free_usb_5v_en:
		gpio_free(PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_OTG_EN));
put_mvs_otg:
		regulator_put(mvs_otg_switch);
		vbus_is_on = false;
}

#ifdef CONFIG_USB_HOST_NOTIFY
static void msm_hsusb_set_autosw_pba()
{
#ifdef CONFIG_USB_SWITCH_FSA9485
	fsa9485_otg_set_autosw_pda();
#endif
}
#endif

static struct msm_otg_platform_data msm_otg_pdata = {
	.mode			= USB_OTG,
	.otg_control		= OTG_NO_CONTROL,
	.phy_type		= SNPS_28NM_INTEGRATED_PHY,
	.pclk_src_name		= "dfab_usb_hs_clk",
	.pmic_id_irq		= PM8921_USB_ID_IN_IRQ(PM8921_IRQ_BASE),
	.vbus_power		= msm_hsusb_vbus_power,
	.power_budget		= 750,
#ifdef CONFIG_USB_HOST_NOTIFY
	.set_autosw_pba   = msm_hsusb_set_autosw_pba,
#endif
};
#endif

#define PID_MAGIC_ID		0x71432909
#define SERIAL_NUM_MAGIC_ID	0x61945374
#define SERIAL_NUMBER_LENGTH	127
#define DLOAD_USB_BASE_ADD	0x2A03F0C8

struct magic_num_struct {
	uint32_t pid;
	uint32_t serial_num;
};

struct dload_struct {
	uint32_t	reserved1;
	uint32_t	reserved2;
	uint32_t	reserved3;
	uint16_t	reserved4;
	uint16_t	pid;
	char		serial_number[SERIAL_NUMBER_LENGTH];
	uint16_t	reserved5;
	struct magic_num_struct magic_struct;
};

static int usb_diag_update_pid_and_serial_num(uint32_t pid, const char *snum)
{
	struct dload_struct __iomem *dload = 0;

	dload = ioremap(DLOAD_USB_BASE_ADD, sizeof(*dload));
	if (!dload) {
		pr_err("%s: cannot remap I/O memory region: %08x\n",
					__func__, DLOAD_USB_BASE_ADD);
		return -ENXIO;
	}

	pr_debug("%s: dload:%p pid:%x serial_num:%s\n",
				__func__, dload, pid, snum);
	/* update pid */
	dload->magic_struct.pid = PID_MAGIC_ID;
	dload->pid = pid;

	/* update serial number */
	dload->magic_struct.serial_num = 0;
	if (!snum) {
		memset(dload->serial_number, 0, SERIAL_NUMBER_LENGTH);
		goto out;
	}

	dload->magic_struct.serial_num = SERIAL_NUM_MAGIC_ID;
	strncpy(dload->serial_number, snum, SERIAL_NUMBER_LENGTH);
	dload->serial_number[SERIAL_NUMBER_LENGTH - 1] = '\0';
out:
	iounmap(dload);
	return 0;
}

static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

static uint8_t spm_wfi_cmd_sequence[] __initdata = {
			0x03, 0x0f,
};

static uint8_t spm_power_collapse_without_rpm[] __initdata = {
			0x00, 0x24, 0x54, 0x10,
			0x09, 0x03, 0x01,
			0x10, 0x54, 0x30, 0x0C,
			0x24, 0x30, 0x0f,
};

static uint8_t spm_power_collapse_with_rpm[] __initdata = {
			0x00, 0x24, 0x54, 0x10,
			0x09, 0x07, 0x01, 0x0B,
			0x10, 0x54, 0x30, 0x0C,
			0x24, 0x30, 0x0f,
};

static struct msm_spm_seq_entry msm_spm_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_MODE_CLOCK_GATING,
		.notify_rpm = false,
		.cmd = spm_wfi_cmd_sequence,
	},
	[1] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = false,
		.cmd = spm_power_collapse_without_rpm,
	},
	[2] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = spm_power_collapse_with_rpm,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SECURE] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
		.reg_init_values[MSM_SPM_REG_SAW2_VCTL] = 0x9C,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020202,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0060009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x0000001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_seq_list),
		.modes = msm_spm_seq_list,
	},
	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SECURE] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
		.reg_init_values[MSM_SPM_REG_SAW2_VCTL] = 0x9C,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020202,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0060009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x0000001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_seq_list),
		.modes = msm_spm_seq_list,
	},
};

static uint8_t l2_spm_wfi_cmd_sequence[] __initdata = {
			0x00, 0x20, 0x03, 0x20,
			0x00, 0x0f,
};

static uint8_t l2_spm_gdhs_cmd_sequence[] __initdata = {
			0x00, 0x20, 0x34, 0x64,
			0x48, 0x07, 0x48, 0x20,
			0x50, 0x64, 0x04, 0x34,
			0x50, 0x0f,
};
static uint8_t l2_spm_power_off_cmd_sequence[] __initdata = {
			0x00, 0x10, 0x34, 0x64,
			0x48, 0x07, 0x48, 0x10,
			0x50, 0x64, 0x04, 0x34,
			0x50, 0x0F,
};

static struct msm_spm_seq_entry msm_spm_l2_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_L2_MODE_RETENTION,
		.notify_rpm = false,
		.cmd = l2_spm_wfi_cmd_sequence,
	},
	[1] = {
		.mode = MSM_SPM_L2_MODE_GDHS,
		.notify_rpm = true,
		.cmd = l2_spm_gdhs_cmd_sequence,
	},
	[2] = {
		.mode = MSM_SPM_L2_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = l2_spm_power_off_cmd_sequence,
	},
};


static struct msm_spm_platform_data msm_spm_l2_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW_L2_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SECURE] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020202,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x00A000AE,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A00020,
		.modes = msm_spm_l2_seq_list,
		.num_modes = ARRAY_SIZE(msm_spm_l2_seq_list),
	},
};

#ifdef CONFIG_NFC_PN544
static struct pn544_i2c_platform_data pn544_pdata = {
	.irq_gpio = GPIO_NFC_IRQ,
	.ven_gpio = PM8921_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN),
	.firm_gpio = GPIO_NFC_FIRMWARE,
};

static struct i2c_board_info pn544_info[] __initdata = {
	{
		I2C_BOARD_INFO("pn544", 0x2b),
		.irq = MSM_GPIO_TO_INT(GPIO_NFC_IRQ),
		.platform_data = &pn544_pdata,
	},
};
#endif /* CONFIG_NFC_PN544	*/

/*atmel_mxt224E*/
static void mxt224_power_onoff(int onoff)
{
	if (system_rev >= BOARD_REV02) {
		int ret;
		static struct regulator *reg_l17;

		if (!reg_l17) {
			reg_l17 = regulator_get(NULL, "8921_l17");
			if (IS_ERR(reg_l17)) {
				pr_err("could not get 8921_l17, rc = %ld\n",
					PTR_ERR(reg_l17));
				return;
			}

			ret = regulator_set_voltage(reg_l17, 3300000, 3300000);
			if (ret) {
				pr_err("%s: unable to set ldo17 voltage to 3.3V\n",
					__func__);
				return;
			}
		}

		if (onoff) {
			ret = regulator_enable(reg_l17);
			if (ret) {
				pr_err("enable l17 failed, rc=%d\n", ret);
				return;
			}
			pr_info("mxt224_power_on is finished.\n");



			pr_info("mxt224_power_on is finished.\n");
		} else {
			ret = regulator_disable(reg_l17);
			if (ret) {
				pr_err("disable l17 failed, rc=%d\n", ret);
				return;
			}

			pr_info("mxt224_power_off is finished.\n");
		}
	} else {
		if (onoff) {
			gpio_direction_output(10, 1);
			msleep(40);
		} else
			gpio_direction_output(10, 0);
	}
}

static void mxt224_register_callback(void *function)
{
	printk(KERN_ERR "mxt224_register_callback\n");
}

static void mxt224_read_ta_status(bool *ta_status)
{
	/* *ta_status = is_cable_attached; */
}

#define MXT224_MAX_MT_FINGERS		10

/*
	Configuration for MXT224
*/
#define MXT224_THRESHOLD_BATT		40
#define MXT224_THRESHOLD_CHRG		70
#define MXT224_ATCHCALST		9
#define MXT224_ATCHCALTHR		30

static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				64, 255, 15};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				10, 0, 5, 0, 0, 0,
				 MXT224_ATCHCALST, MXT224_ATCHCALTHR};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				131, 0, 0, 19, 11, 0, 32,
				MXT224_THRESHOLD_BATT, 2, 1, 0, 10, 1,
				11, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 0, 0, 0, 0, 143, 55, 143, 90, 18};

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 1};
static u8 t20_config[] = {PROCI_GRIPFACESUPPRESSION_T20,
				7, 0, 0, 0, 0, 0, 0, 30, 20, 4, 15, 10};
static u8 t22_config[] = {PROCG_NOISESUPPRESSION_T22,
				13, 0, 0, 0, 0, 0, 0, 3, 30, 0, 0,  29, 34, 39,
				49, 58, 3};
static u8 t28_config[] = {SPT_CTECONFIG_T28,
				0, 0, 3, 16, 19, 60};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t18_config,
	t20_config,
	t22_config,
	t28_config,
	end_config,
};

/*
	Configuration for MXT224-E
*/
#define MXT224E_THRESHOLD_BATT		50
#define MXT224E_THRESHOLD_CHRG		40
#define MXT224E_CALCFG_BATT		64
#define MXT224E_CALCFG_CHRG		82
#define MXT224E_ATCHFRCCALTHR_NORMAL		40
#define MXT224E_ATCHFRCCALRATIO_NORMAL		55

static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
				48, 255, 25};

static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				27, 0, 5, 1, 0, 0, 4, 35,
				MXT224E_ATCHFRCCALTHR_NORMAL,
				MXT224E_ATCHFRCCALRATIO_NORMAL};

static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 32,
				MXT224E_THRESHOLD_BATT, 2, 1, 10, 3, 1,
				46, MXT224_MAX_MT_FINGERS, 5, 40, 10, 191, 3,
				27, 2, 10, 10, 10, 10, 143, 40, 143, 80, 18,
				15, 50, 50, 0};

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t18_config_e[] = {SPT_COMCONFIG_T18,
				0, 0};

static u8 t19_config_e[] = {SPT_GPIOPWM_T19,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t23_config_e[] = {TOUCH_PROXIMITY_T23,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t25_config_e[] = {SPT_SELFTEST_T25,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40,
				0, 0, 0, 0, 0};

static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42,
				0, 0, 0, 0, 0, 0, 0, 0};

static u8 t46_config_e[] = {SPT_CTECONFIG_T46,
				0, 3, 24, 35, 0, 0, 1, 0};

static u8 t47_config_e[] = {PROCI_STYLUS_T47,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
				3, 132, MXT224E_CALCFG_BATT,
				0, 0, 0, 0, 0, 10, 20, 0, 0, 0,
				6, 6, 0, 0, 64, 4, 64, 10,
				0, 20, 5, 0, 38, 0, 5,
				0, 0, 0, 0, 0, 0, 32, MXT224E_THRESHOLD_BATT,
				2, 3, 1, 46, MXT224_MAX_MT_FINGERS,
				5, 40, 10, 10, 10, 10, 143, 40, 143,
				80, 18, 15, 0 };

static u8 t48_config_chrg_e[] = {PROCG_NOISESUPPRESSION_T48,
				3, 132, MXT224E_CALCFG_CHRG,
				0, 0, 0, 0, 0, 10, 20, 0, 0, 0, 6,
				6, 0, 0, 64, 4, 64, 10, 0, 20, 5, 0, 38, 0, 20,
				0, 0, 0, 0, 0, 0, 0, MXT224E_THRESHOLD_CHRG,
				2, 5, 2, 47, MXT224_MAX_MT_FINGERS, 5, 40, 10,
				0, 10, 10, 143, 40, 143, 80, 18, 15, 0 };

static u8 end_config_e[] = {RESERVED_T255};

static const u8 *mxt224e_config[] = {
	t7_config_e,
	t8_config_e,
	t9_config_e,
	t15_config_e,
	t18_config_e,
	t19_config_e,
	t23_config_e,
	t25_config_e,
	t40_config_e,
	t42_config_e,
	t46_config_e,
	t47_config_e,
	t48_config_e,
	end_config_e,
};

static struct mxt224_platform_data mxt224_data = {
	.max_finger_touches = MXT224_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_CYTTSP_TS_IRQ,
	.config = mxt224_config,
	.config_e = mxt224e_config,
	.min_x = 0,
	.max_x = 539,
	.min_y = 0,
	.max_y = 959,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.atchcalst = MXT224_ATCHCALST,
	.atchcalsthr = MXT224_ATCHCALTHR,
	.tchthr_batt = MXT224_THRESHOLD_BATT,
	.tchthr_charging = MXT224_THRESHOLD_CHRG,
	.tchthr_batt_e = MXT224E_THRESHOLD_BATT,
	.tchthr_charging_e = MXT224E_THRESHOLD_CHRG,
	.calcfg_batt_e = MXT224E_CALCFG_BATT,
	.calcfg_charging_e = MXT224E_CALCFG_CHRG,
	.atchfrccalthr_e = MXT224E_ATCHFRCCALTHR_NORMAL,
	.atchfrccalratio_e = MXT224E_ATCHFRCCALRATIO_NORMAL,
	.t48_config_batt_e = t48_config_e,
	.t48_config_chrg_e = t48_config_chrg_e,
	.power_onoff = mxt224_power_onoff,
	.register_cb = mxt224_register_callback,
	.read_ta_status = mxt224_read_ta_status,
};

/* I2C2 */
static struct i2c_board_info mxt224_info[] __initdata = {
	{
		I2C_BOARD_INFO(MXT224_DEV_NAME, 0x4a),
		.platform_data = &mxt224_data,
		.irq = MSM_GPIO_TO_INT(GPIO_CYTTSP_TS_IRQ),
	},
};

static void  mxt224_init(void)
{
	int ret;
	static struct regulator *reg_l17;

	printk(KERN_ERR " :  %s: ++:\n", __func__);
	reg_l17 = regulator_get(NULL, "8921_l17");
	if (IS_ERR(reg_l17)) {
		pr_err("could not get 8921_l17, rc = %ld\n",
			PTR_ERR(reg_l17));
		return;
	}
	ret = regulator_set_voltage(reg_l17, 3300000, 3300000);
	if (ret) {
		pr_err("%s: unable to set ldo17 voltage to 3.3V\n", __func__);
		return;
	}
	ret = regulator_enable(reg_l17);
	if (ret) {
		pr_err("enable l17 failed, rc=%d\n", ret);
		return;
	}

	ret = gpio_request(10, "tsp_ldo_en");
	if (ret != 0) {
		pr_err("tsp ldo request failed, ret=%d", ret);
		return;
	}
}

/* configuration data */
static const u8 mxt_config_data[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	11, 1, 0, 20, 10, 11, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T7 Object */
	100, 16, 50,
	/* T8 Object */
	8, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T9 Object */
	131, 0, 0, 26, 42, 0, 32, 60, 2, 5,
	0, 5, 5, 34, 10, 10, 10, 10, 255, 2,
	85, 5, 18, 18, 18, 18, 0, 0, 5, 20,
	0, 5, 45, 46,
	/* T15 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* T22 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 30, 0,
	0, 0, 255, 255, 255, 255, 0,
	/* T24 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T25 Object */
	3, 0, 188, 52, 52, 33, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T27 Object */
	0, 0, 0, 0, 0, 0, 0,
	/* T28 Object */
	0, 0, 0, 8, 8, 60,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T41 Object */
	0, 0, 0, 0, 0, 0,
	/* T43 Object */
	0, 0, 0, 0, 0, 0,
};

static void mxt_init_hw_liquid(void)
{
	int rc;

	rc = gpio_request(GPIO_MXT_TS_IRQ, "mxt_ts_irq_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_ts_irq gpio [%d]\n",
				__func__, GPIO_MXT_TS_IRQ);
		return;
	}

	rc = gpio_direction_input(GPIO_MXT_TS_IRQ);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_ts_irq gpio [%d]\n",
				__func__, GPIO_MXT_TS_IRQ);
		goto err_irq_gpio_req;
	}

	rc = gpio_request(GPIO_MXT_TS_LDO_EN, "mxt_ldo_en_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_ldo_en gpio [%d]\n",
				__func__, GPIO_MXT_TS_LDO_EN);
		goto err_irq_gpio_req;
	}

	rc = gpio_direction_output(GPIO_MXT_TS_LDO_EN, 1);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_ldo_en gpio [%d]\n",
				__func__, GPIO_MXT_TS_LDO_EN);
		goto err_ldo_gpio_req;
	}

	rc = gpio_request(GPIO_MXT_TS_RESET, "mxt_reset_gpio");
	if (rc) {
		pr_err("%s: unable to request mxt_reset gpio [%d]\n",
				__func__, GPIO_MXT_TS_RESET);
		goto err_ldo_gpio_set_dir;
	}

	rc = gpio_direction_output(GPIO_MXT_TS_RESET, 1);
	if (rc) {
		pr_err("%s: unable to set_direction for mxt_reset gpio [%d]\n",
				__func__, GPIO_MXT_TS_RESET);
		goto err_reset_gpio_req;
	}

	return;

err_reset_gpio_req:
	gpio_free(GPIO_MXT_TS_RESET);
err_ldo_gpio_set_dir:
	gpio_set_value(GPIO_MXT_TS_LDO_EN, 0);
err_ldo_gpio_req:
	gpio_free(GPIO_MXT_TS_LDO_EN);
err_irq_gpio_req:
	gpio_free(GPIO_MXT_TS_IRQ);
}

static struct mxt_platform_data mxt_platform_data = {
	.config			= mxt_config_data,
	.config_length		= ARRAY_SIZE(mxt_config_data),
	.x_size			= 1365,
	.y_size			= 767,
	.irqflags		= IRQF_TRIGGER_FALLING,
	.i2c_pull_up		= true,
};

static struct i2c_board_info mxt_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x5b),
		.platform_data = &mxt_platform_data,
		.irq = MSM_GPIO_TO_INT(GPIO_MXT_TS_IRQ),
	},
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
}

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi4_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi3_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi7_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi10_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi12_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_rpm_platform_data msm_rpm_data = {
	.reg_base_addrs = {
		[MSM_RPM_PAGE_STATUS] = MSM_RPM_BASE,
		[MSM_RPM_PAGE_CTRL] = MSM_RPM_BASE + 0x400,
		[MSM_RPM_PAGE_REQ] = MSM_RPM_BASE + 0x600,
		[MSM_RPM_PAGE_ACK] = MSM_RPM_BASE + 0xa00,
	},

	.irq_ack = RPM_APCC_CPU0_GP_HIGH_IRQ,
	.irq_err = RPM_APCC_CPU0_GP_LOW_IRQ,
	.irq_vmpm = RPM_APCC_CPU0_GP_MEDIUM_IRQ,
	.msm_apps_ipc_rpm_reg = MSM_APCS_GCC_BASE + 0x008,
	.msm_apps_ipc_rpm_val = 4,
};

static struct ks8851_pdata spi_eth_pdata = {
	.irq_gpio = KS8851_IRQ_GPIO,
	.rst_gpio = KS8851_RST_GPIO,
};

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias               = "ks8851",
		.irq                    = MSM_GPIO_TO_INT(KS8851_IRQ_GPIO),
		.max_speed_hz           = 19200000,
		.bus_num                = 0,
		.chip_select            = 0,
		.mode                   = SPI_MODE_0,
		.platform_data		= &spi_eth_pdata
	},
};

static struct platform_device msm_device_saw_core0 = {
	.name          = "saw-regulator",
	.id            = 0,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s5,
	},
};

static struct platform_device msm_device_saw_core1 = {
	.name          = "saw-regulator",
	.id            = 1,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s6,
	},
};

static struct tsens_platform_data msm_tsens_pdata  = {
		.slope			= 910,
		.tsens_factor		= 1000,
		.hw_type		= MSM_8960,
		.tsens_num_sensor	= 5,
};

static struct platform_device msm_tsens_device = {
	.name	= "tsens8960-tm",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_tsens_pdata,
	},
};

#ifdef CONFIG_MSM_FAKE_BATTERY
static struct platform_device fish_battery_device = {
	.name = "fish_battery",
};
#endif

static struct platform_device msm8960_device_ext_5v_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= PM8921_MPP_PM_TO_SYS(7),
	.dev	= {
		.platform_data = &msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_5V],
	},
};

static struct platform_device msm8960_device_ext_l2_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= 91,
	.dev	= {
		.platform_data = &msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_L2],
	},
};

static struct platform_device msm8960_device_ext_3p3v_vreg __devinitdata = {
	.name	= GPIO_REGULATOR_DEV_NAME,
	.id	= PM8921_GPIO_PM_TO_SYS(17),
	.dev	= {
		.platform_data =
			&msm_gpio_regulator_pdata[GPIO_VREG_ID_EXT_3P3V],
	},
};
#ifdef CONFIG_KEYBOARD_GPIO
static struct gpio_keys_button gpio_keys_button[] = {
	{
		.code			= KEY_VOLUMEUP,
		.type			= EV_KEY,
		.gpio			= NULL,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5, /* ms */
		.desc			= "Vol Up",
	},
	{
		.code			= KEY_VOLUMEDOWN,
		.type			= EV_KEY,
		.gpio			= NULL,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5, /* ms */
		.desc			= "Vol Down",
	},
	{
		.code			= KEY_HOME,
		.type			= EV_KEY,
		.gpio			= 52,
		.active_low		= 1,
		.wakeup			= 0,
		.debounce_interval	= 5, /* ms */
		.desc			= "home",
	},

};
static struct gpio_keys_platform_data gpio_keys_platform_data = {
	.buttons	= gpio_keys_button,
	.nbuttons	= ARRAY_SIZE(gpio_keys_button),
	.rep		= 0,
};

static struct platform_device msm8960_gpio_keys_device = {
	.name	= "sec_keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_keys_platform_data,
	}
};
#endif

static struct platform_device msm8960_device_rpm_regulator __devinitdata = {
	.name	= "rpm-regulator",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_regulator_pdata,
	},
};

static struct msm_rpm_log_platform_data msm_rpm_log_pdata = {
	.phys_addr_base = 0x0010C000,
	.reg_offsets = {
		[MSM_RPM_LOG_PAGE_INDICES] = 0x00000080,
		[MSM_RPM_LOG_PAGE_BUFFER]  = 0x000000A0,
	},
	.phys_size = SZ_8K,
	.log_len = 4096,		  /* log's buffer length in bytes */
	.log_len_mask = (4096 >> 2) - 1,  /* length mask in units of u32 */
};

static struct platform_device msm_rpm_log_device = {
	.name	= "msm_rpm_log",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_log_pdata,
	},
};

static struct platform_device *common_devices[] __initdata = {
	&msm8960_device_dmov,
	&msm_device_smd,
	&msm8960_device_uart_gsbi5,
	&msm_device_uart_dm6,
	&msm_device_saw_core0,
	&msm_device_saw_core1,
	&msm8960_device_ext_5v_vreg,
	&msm8960_device_ext_l2_vreg,
	&msm8960_device_ssbi_pm8921,
/*	&msm8960_device_qup_spi_gsbi1, */
	&msm8960_device_qup_i2c_gsbi3,
	&msm8960_device_qup_i2c_gsbi4,
	&msm8960_device_qup_i2c_gsbi7,
	&msm8960_device_qup_i2c_gsbi10,
#ifndef CONFIG_MSM_DSPS
	&msm8960_device_qup_i2c_gsbi12,
#endif
	&msm_slim_ctrl,
	&msm_device_wcnss_wlan,
#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&msm_device_sps,
#ifdef CONFIG_MSM_FAKE_BATTERY
	&fish_battery_device,
#endif
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	&android_pmem_device,
	&android_pmem_adsp_device,
#endif
	&android_pmem_audio_device,
#endif
#ifdef CONFIG_KEYBOARD_GPIO
	&msm8960_gpio_keys_device,
#endif
	&msm_fb_device,
	&msm_device_vidc,
	&msm_device_bam_dmux,
	&msm_fm_platform_init,

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
	&msm_rpm_device,
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
	&msm_rpm_log_device,
#ifdef CONFIG_MSM_QDSS
	&msm_etb_device,
	&msm_tpiu_device,
	&msm_funnel_device,
	&msm_ptm_device,
#endif
	&msm_device_dspcrashd_8960,
};

static struct platform_device *sim_devices[] __initdata = {
	&msm8960_device_otg,
	&msm8960_device_gadget_peripheral,
	&msm_device_hsusb_host,
	&msm_device_hsic_host,
	&android_usb_device,
	&msm_device_vidc,
	&mipi_dsi_simulator_panel_device,
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
	&msm_pcm,
	&msm_pcm_routing,
	&msm_cpudai0,
	&msm_cpudai1,
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,
	&msm_cpu_fe,
	&msm_stub_codec,
	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif
};

static struct platform_device *rumi3_devices[] __initdata = {
	&msm_kgsl_3d0,
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
	&mipi_dsi_renesas_panel_device,
#ifdef CONFIG_MSM_GEMINI
	&msm8960_gemini_device,
#endif
};

static struct platform_device *cdp_devices[] __initdata = {
	&msm8960_device_otg,
	&msm8960_device_gadget_peripheral,
	&msm_device_hsusb_host,
	&msm_device_hsic_host,
	&android_usb_device,
	&msm_pcm,
	&msm_pcm_routing,
	&msm_cpudai0,
	&msm_cpudai1,
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,
	&msm_cpu_fe,
	&msm_stub_codec,
	&msm_kgsl_3d0,
#ifdef CONFIG_MSM_KGSL_2D
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#endif
	&mipi_dsi_novatek_panel_device,
#ifdef CONFIG_MSM_GEMINI
	&msm8960_gemini_device,
#endif
	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,
	&msm_cpudai_afe_01_rx,
	&msm_cpudai_afe_01_tx,
	&msm_cpudai_afe_02_rx,
	&msm_cpudai_afe_02_tx,
	&msm_pcm_afe,
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	&hdmi_msm_device,
#endif
	&msm_pcm_hostless,
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
	&msm_tsens_device,
};

static struct platform_device *t_project_devices[] __initdata = {
	&msm8960_device_otg,
	&msm8960_device_gadget_peripheral,
	&msm_device_hsusb_host,
	&android_usb_device,
	&msm_pcm,
	&msm_pcm_routing,
	&msm_cpudai0,
	&msm_cpudai1,
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
/* Add platform device for AUX PCM Interface */
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,

	&msm_cpu_fe,
	&msm_stub_codec,
	&msm_kgsl_3d0,
#ifdef CONFIG_MSM_KGSL_2D
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#endif
#ifdef CONFIG_MSM_GEMINI
	&msm8960_gemini_device,
#endif

	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,
/* Add platform device for ALSA Audio RT Proxy Driver */
	&msm_cpudai_afe_01_rx,
	&msm_cpudai_afe_01_tx,
	&msm_cpudai_afe_02_rx,
	&msm_cpudai_afe_02_tx,
	&msm_pcm_afe,

#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	&mhl_i2c_gpio_device,
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
	&fsa_i2c_gpio_device,
#endif
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
	&touchkey_i2c_gpio_device,
#endif
#ifdef CONFIG_BATTERY_MAX17040
	&fuelgauge_i2c_gpio_device,
#endif
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	&hdmi_msm_device,
#endif
	&msm_pcm_hostless,
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || defined(CONFIG_INPUT_BMP180)
	&sns_i2c_gpio_device,
#endif
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
	&opt_i2c_gpio_device,
	&opt_gp2a,
#endif

};

static void __init msm8960_i2c_init(void)
{
	msm8960_device_qup_i2c_gsbi4.dev.platform_data =
					&msm8960_i2c_qup_gsbi4_pdata;
	msm8960_device_qup_i2c_gsbi7.dev.platform_data =
					&msm8960_i2c_qup_gsbi7_pdata;

	msm8960_device_qup_i2c_gsbi3.dev.platform_data =
					&msm8960_i2c_qup_gsbi3_pdata;

	msm8960_device_qup_i2c_gsbi10.dev.platform_data =
					&msm8960_i2c_qup_gsbi10_pdata;

	msm8960_device_qup_i2c_gsbi12.dev.platform_data =
					&msm8960_i2c_qup_gsbi12_pdata;
}

static void __init msm8960_gfx_init(void)
{
	uint32_t soc_platform_version = socinfo_get_version();
	if (SOCINFO_VERSION_MAJOR(soc_platform_version) == 1) {
		struct kgsl_device_platform_data *kgsl_3d0_pdata =
				msm_kgsl_3d0.dev.platform_data;
		kgsl_3d0_pdata->pwr_data.pwrlevel[0].gpu_freq =
				320000000;
		kgsl_3d0_pdata->pwr_data.pwrlevel[1].gpu_freq =
				266667000;
	}
}

static struct pm8xxx_irq_platform_data pm8xxx_irq_pdata __devinitdata = {
	.irq_base		= PM8921_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(104),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8xxx_gpio_pdata __devinitdata = {
	.gpio_base	= PM8921_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_mpp_platform_data pm8xxx_mpp_pdata __devinitdata = {
	.mpp_base	= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_rtc_platform_data pm8xxx_rtc_pdata __devinitdata = {
	.rtc_write_enable       = false,
	.rtc_alarm_powerup	= false,
};

static struct pm8xxx_pwrkey_platform_data pm8xxx_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 970,
	.wakeup			= 1,
};

/* Rotate lock key is not available so use F1 */
#define KEY_ROTATE_LOCK KEY_F1

static const unsigned int keymap_liquid[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(1, 3, KEY_ROTATE_LOCK),
	KEY(1, 4, KEY_HOME),
};

static struct matrix_keymap_data keymap_data_liquid = {
	.keymap_size    = ARRAY_SIZE(keymap_liquid),
	.keymap         = keymap_liquid,
};

static struct pm8xxx_keypad_platform_data keypad_data_liquid = {
	.input_name             = "keypad_8960_liquid",
	.input_phys_device      = "keypad_8960/input0",
	.num_rows               = 2,
	.num_cols               = 5,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data_liquid,
};

static const unsigned int keymap[] = {
/* taing.sim */
	KEY(0, 0, KEY_SEARCH),
	KEY(0, 1, KEY_BACK),
	KEY(0, 2, KEY_HOME),
	KEY(0, 3, KEY_MENU),
};

static struct matrix_keymap_data keymap_data = {
	.keymap_size    = ARRAY_SIZE(keymap),
	.keymap         = keymap,
};

static struct pm8xxx_keypad_platform_data keypad_data = {
	.input_name             = "keypad_8960",
	.input_phys_device      = "keypad_8960/input0",
	.num_rows               = 1,
	.num_cols               = 5,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data,
};

static const unsigned int keymap_sim[] = {
	KEY(0, 0, KEY_7),
	KEY(0, 1, KEY_DOWN),
	KEY(0, 2, KEY_UP),
	KEY(0, 3, KEY_RIGHT),
	KEY(0, 4, KEY_ENTER),
	KEY(0, 5, KEY_L),
	KEY(0, 6, KEY_BACK),
	KEY(0, 7, KEY_M),

	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_SEND),
	KEY(1, 2, KEY_1),
	KEY(1, 3, KEY_4),
	KEY(1, 4, KEY_CLEAR),
	KEY(1, 5, KEY_MSDOS),
	KEY(1, 6, KEY_SPACE),
	KEY(1, 7, KEY_COMMA),

	KEY(2, 0, KEY_6),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_8),
	KEY(2, 3, KEY_3),
	KEY(2, 4, KEY_NUMERIC_STAR),
	KEY(2, 5, KEY_UP),
	KEY(2, 6, KEY_DOWN),
	KEY(2, 7, KEY_LEFTSHIFT),

	KEY(3, 0, KEY_9),
	KEY(3, 1, KEY_NUMERIC_POUND),
	KEY(3, 2, KEY_0),
	KEY(3, 3, KEY_2),
	KEY(3, 4, KEY_SLEEP),
	KEY(3, 5, KEY_F1),
	KEY(3, 6, KEY_F2),
	KEY(3, 7, KEY_F3),

	KEY(4, 0, KEY_BACK),
	KEY(4, 1, KEY_HOME),
	KEY(4, 2, KEY_MENU),
	KEY(4, 3, KEY_VOLUMEUP),
	KEY(4, 4, KEY_VOLUMEDOWN),
	KEY(4, 5, KEY_F4),
	KEY(4, 6, KEY_F5),
	KEY(4, 7, KEY_F6),

	KEY(5, 0, KEY_R),
	KEY(5, 1, KEY_T),
	KEY(5, 2, KEY_Y),
	KEY(5, 3, KEY_LEFTALT),
	KEY(5, 4, KEY_KPENTER),
	KEY(5, 5, KEY_Q),
	KEY(5, 6, KEY_W),
	KEY(5, 7, KEY_E),

	KEY(6, 0, KEY_F),
	KEY(6, 1, KEY_G),
	KEY(6, 2, KEY_H),
	KEY(6, 3, KEY_CAPSLOCK),
	KEY(6, 4, KEY_PAGEUP),
	KEY(6, 5, KEY_A),
	KEY(6, 6, KEY_S),
	KEY(6, 7, KEY_D),

	KEY(7, 0, KEY_V),
	KEY(7, 1, KEY_B),
	KEY(7, 2, KEY_N),
	KEY(7, 3, KEY_MENU),
	KEY(7, 4, KEY_PAGEDOWN),
	KEY(7, 5, KEY_Z),
	KEY(7, 6, KEY_X),
	KEY(7, 7, KEY_C),

	KEY(8, 0, KEY_P),
	KEY(8, 1, KEY_J),
	KEY(8, 2, KEY_K),
	KEY(8, 3, KEY_INSERT),
	KEY(8, 4, KEY_LINEFEED),
	KEY(8, 5, KEY_U),
	KEY(8, 6, KEY_I),
	KEY(8, 7, KEY_O),

	KEY(9, 0, KEY_4),
	KEY(9, 1, KEY_5),
	KEY(9, 2, KEY_6),
	KEY(9, 3, KEY_7),
	KEY(9, 4, KEY_8),
	KEY(9, 5, KEY_1),
	KEY(9, 6, KEY_2),
	KEY(9, 7, KEY_3),

	KEY(10, 0, KEY_F7),
	KEY(10, 1, KEY_F8),
	KEY(10, 2, KEY_F9),
	KEY(10, 3, KEY_F10),
	KEY(10, 4, KEY_FN),
	KEY(10, 5, KEY_9),
	KEY(10, 6, KEY_0),
	KEY(10, 7, KEY_DOT),

	KEY(11, 0, KEY_LEFTCTRL),
	KEY(11, 1, KEY_F11),
	KEY(11, 2, KEY_ENTER),
	KEY(11, 3, KEY_SEARCH),
	KEY(11, 4, KEY_DELETE),
	KEY(11, 5, KEY_RIGHT),
	KEY(11, 6, KEY_LEFT),
	KEY(11, 7, KEY_RIGHTSHIFT),
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(0, 2, KEY_CAMERA_SNAPSHOT),
	KEY(0, 3, KEY_CAMERA_FOCUS),
};

static struct matrix_keymap_data keymap_data_sim = {
	.keymap_size    = ARRAY_SIZE(keymap_sim),
	.keymap         = keymap_sim,
};

static struct pm8xxx_keypad_platform_data keypad_data_sim = {
	.input_name             = "keypad_8960",
	.input_phys_device      = "keypad_8960/input0",
	.num_rows               = 12,
	.num_cols               = 8,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data_sim,
};

static int pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	325,
};

static struct pm8921_charger_platform_data pm8921_chg_pdata __devinitdata = {
	.safety_time		= 180,
	.update_time		= 60000,
	.max_voltage		= 4200,
	.min_voltage		= 3200,
	.resume_voltage		= 4100,
	.term_current		= 100,
	.cool_temp		= 10,
	.warm_temp		= 40,
	.temp_check_period	= 1,
	.max_bat_chg_current	= 1100,
	.cool_bat_chg_current	= 350,
	.warm_bat_chg_current	= 350,
	.cool_bat_voltage	= 4100,
	.warm_bat_voltage	= 4100,
	.thermal_mitigation	= pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(pm8921_therm_mitigation),
};

static struct pm8xxx_misc_platform_data pm8xxx_misc_pdata = {
	.priority		= 0,
};

static struct pm8921_bms_platform_data pm8921_bms_pdata __devinitdata = {
	.r_sense		= 10,
	.i_test			= 2500,
	.v_failure		= 3000,
	.calib_delay_ms		= 600000,
};

#define	PM8921_LC_LED_MAX_CURRENT	4	/* I = 4mA */

#define PM8XXX_LED_PWM_PERIOD		1000
#define PM8XXX_LED_PWM_DUTY_MS		20
/**
 * PM8XXX_PWM_CHANNEL_NONE shall be used when LED shall not be
 * driven using PWM feature.
 */
#define PM8XXX_PWM_CHANNEL_NONE		-1

static struct led_info pm8921_led_info[] = {
	[0] = {
		.name			= "led:battery_charging",
		.default_trigger	= "battery-charging",
	},
	[1] = {
		.name			= "led:battery_full",
		.default_trigger	= "battery-full",
	},
};

static struct led_platform_data pm8921_led_core_pdata = {
	.num_leds = ARRAY_SIZE(pm8921_led_info),
	.leds = pm8921_led_info,
};

static int pm8921_led0_pwm_duty_pcts[56] = {
		1, 4, 8, 12, 16, 20, 24, 28, 32, 36,
		40, 44, 46, 52, 56, 60, 64, 68, 72, 76,
		80, 84, 88, 92, 96, 100, 100, 100, 98, 95,
		92, 88, 84, 82, 78, 74, 70, 66, 62, 58,
		58, 54, 50, 48, 42, 38, 34, 30, 26, 22,
		14, 10, 6, 4, 1
};

static struct pm8xxx_pwm_duty_cycles pm8921_led0_pwm_duty_cycles = {
	.duty_pcts = (int *)&pm8921_led0_pwm_duty_pcts,
	.num_duty_pcts = ARRAY_SIZE(pm8921_led0_pwm_duty_pcts),
	.duty_ms = PM8XXX_LED_PWM_DUTY_MS,
	.start_idx = 0,
};


static struct pm8xxx_led_config pm8921_led_configs[] = {
	[0] = {
		.id = PM8XXX_ID_LED_0,
		.mode = PM8XXX_LED_MODE_PWM2,
		.max_current = PM8921_LC_LED_MAX_CURRENT,
		.pwm_channel = 5,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
		.pwm_duty_cycles = &pm8921_led0_pwm_duty_cycles,
	},
	[1] = {
		.id = PM8XXX_ID_LED_1,
		.mode = PM8XXX_LED_MODE_PWM1,
		.max_current = PM8921_LC_LED_MAX_CURRENT,
		.pwm_channel = 4,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
	},
};

static struct pm8xxx_led_platform_data pm8xxx_leds_pdata = {
		.led_core = &pm8921_led_core_pdata,
		.configs = pm8921_led_configs,
		.num_configs = ARRAY_SIZE(pm8921_led_configs),
};

static struct pm8921_platform_data pm8921_platform_data __devinitdata = {
	.irq_pdata		= &pm8xxx_irq_pdata,
	.gpio_pdata		= &pm8xxx_gpio_pdata,
	.mpp_pdata		= &pm8xxx_mpp_pdata,
	.rtc_pdata              = &pm8xxx_rtc_pdata,
	.pwrkey_pdata		= &pm8xxx_pwrkey_pdata,
	.keypad_pdata		= &keypad_data,
	.misc_pdata		= &pm8xxx_misc_pdata,
	.regulator_pdatas	= msm_pm8921_regulator_pdata,
	.charger_pdata		= &pm8921_chg_pdata,
	.bms_pdata		= &pm8921_bms_pdata,
	.adc_pdata		= &pm8921_adc_pdata,
	.leds_pdata		= &pm8xxx_leds_pdata,
};

static struct msm_ssbi_platform_data msm8960_ssbi_pm8921_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8921-core",
		.platform_data		= &pm8921_platform_data,
	},
};

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static int fpga_init(void)
{
	int ret;

	ret = gpio_request(FPGA_CS_GPIO, "fpga_cs");
	if (ret) {
		pr_err("FPGA CS gpio_request failed: %d\n", ret);
		goto fail;
	}

	gpio_direction_output(FPGA_CS_GPIO, 1);

	return 0;
fail:
	return ret;
}
#else
static int fpga_init(void)
{
	return 0;
}
#endif

static struct msm_cpuidle_state msm_cstates[] __initdata = {
	{0, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{0, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},

	{0, 2, "C2", "POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE},

	{1, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{1, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},
};

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 0,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 0,
		.suspend_enabled = 0,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 0,
		.idle_enabled = 1,
		.suspend_enabled = 0,
	},
};

static struct msm_rpmrs_level msm_rpmrs_levels[] __initdata = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		100, 8000, 100000, 1,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		2000, 6000, 60100000, 3000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, GDHS, MAX, ACTIVE),
		false,
		4200, 5000, 60350000, 3500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, MAX, ACTIVE),
		false,
		6300, 4500, 65350000, 4800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, GDHS, MAX, ACTIVE),
		false,
		11700, 2500, 67850000, 5500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		13800, 2000, 71850000, 6800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		29700, 500, 75850000, 8800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		29700, 0, 76350000, 9800,
	},
};

#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)
#define I2C_LIQUID (1 << 5)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#ifdef CONFIG_MSM_CAMERA
static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
#ifdef CONFIG_IMX074
	{
	I2C_BOARD_INFO("imx074", 0x1A),
	},
#endif
#ifdef CONFIG_OV2720
	{
	I2C_BOARD_INFO("ov2720", 0x6C),
	},
#endif
#ifdef CONFIG_ISX012
	{
	I2C_BOARD_INFO("isx012", 0x3D),
	},
#endif
#ifdef CONFIG_S5K6AA
	{
	I2C_BOARD_INFO("s5k6aa", 0x78>>1),
	},
#endif
#ifdef CONFIG_S5K8AAY
	{
	I2C_BOARD_INFO("s5k8aay", 0x3C),
	},
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH_SC628A
	{
	I2C_BOARD_INFO("sc628a", 0x6E),
	},
#endif
};
#endif

/* Sensors DSPS platform data */
#ifdef CONFIG_MSM_DSPS
#define DSPS_PIL_GENERIC_NAME		"dsps"
#endif /* CONFIG_MSM_DSPS */

static void __init msm8960_init_dsps(void)
{
#ifdef CONFIG_MSM_DSPS
	struct msm_dsps_platform_data *pdata =
		msm_dsps_device.dev.platform_data;
	pdata->pil_name = DSPS_PIL_GENERIC_NAME;
	pdata->gpios = NULL;
	pdata->gpios_num = 0;

	platform_device_register(&msm_dsps_device);
#endif /* CONFIG_MSM_DSPS */
}

#ifdef CONFIG_ISL9519_CHARGER
static struct isl_platform_data isl_data __initdata = {
	.valid_n_gpio		= 0,	/* Not required when notify-by-pmic */
	.chg_detection_config	= NULL,	/* Not required when notify-by-pmic */
	.max_system_voltage	= 4200,
	.min_system_voltage	= 3200,
	.chgcurrent		= 1000, /* 1900, */
	.term_current		= 400,	/* Need fine tuning */
	.input_current		= 2048,
};

static struct i2c_board_info isl_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("isl9519q", 0x9),
		.irq		= 0,	/* Not required when notify-by-pmic */
		.platform_data	= &isl_data,
	},
};
#endif /* CONFIG_ISL9519_CHARGER */

static struct i2c_registry msm8960_i2c_devices[] __initdata = {
#ifdef CONFIG_MSM_CAMERA
	{
		I2C_SURF | I2C_FFA | I2C_FLUID | I2C_LIQUID | I2C_RUMI,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
		msm_camera_boardinfo,
		ARRAY_SIZE(msm_camera_boardinfo),
	},
#endif
#ifdef CONFIG_ISL9519_CHARGER
	{
		I2C_LIQUID,
		MSM_8960_GSBI10_QUP_I2C_BUS_ID,
		isl_charger_i2c_info,
		ARRAY_SIZE(isl_charger_i2c_info),
	},
#endif /* CONFIG_ISL9519_CHARGER */
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_MHL_I2C_BUS_ID,
		mhl_i2c_board_info,
		ARRAY_SIZE(mhl_i2c_board_info),
	},
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_FSA9485_I2C_BUS_ID,
		micro_usb_i2c_devices_info,
		ARRAY_SIZE(micro_usb_i2c_devices_info),
	},
#endif
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		mxt224_info,
		ARRAY_SIZE(mxt224_info),
	},
#ifdef CONFIG_KEYBOARD_CYPRESS_TOUCH
{
	I2C_SURF | I2C_FFA | I2C_FLUID,
	MSM_TOUCHKEY_I2C_BUS_ID,
	touchkey_i2c_devices_info,
	ARRAY_SIZE(touchkey_i2c_devices_info),
},
#endif
#ifdef CONFIG_BATTERY_MAX17040
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_FUELGAUGE_I2C_BUS_ID,
		fuelgauge_i2c_board_info,
		ARRAY_SIZE(fuelgauge_i2c_board_info),
	},
#endif
#ifdef CONFIG_NFC_PN544
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8960_GSBI7_QUP_I2C_BUS_ID,
		pn544_info,
		ARRAY_SIZE(pn544_info),
	},
#endif /* CONFIG_NFC_PN544	*/
	{
		I2C_LIQUID,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		mxt_device_info,
		ARRAY_SIZE(mxt_device_info),
	},
#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || defined(CONFIG_INPUT_BMP180)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_SNS_I2C_BUS_ID,
		sns_i2c_borad_info,
		ARRAY_SIZE(sns_i2c_borad_info),
	},
#endif
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_OPT_I2C_BUS_ID,
		opt_i2c_borad_info,
		ARRAY_SIZE(opt_i2c_borad_info),
	},
#endif
};
#endif /* CONFIG_I2C */

static void __init register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	u8 mach_mask = 0;
	int i;

	/* Build the matching 'supported_machs' bitmask */
	if (machine_is_msm8960_cdp())
		mach_mask = I2C_SURF;
	else if (machine_is_msm8960_rumi3())
		mach_mask = I2C_RUMI;
	else if (machine_is_msm8960_sim())
		mach_mask = I2C_SIM;
	else if (machine_is_msm8960_fluid())
		mach_mask = I2C_FLUID;
	else if (machine_is_msm8960_liquid())
		mach_mask = I2C_LIQUID;
	else if (machine_is_msm8960_mtp())
		mach_mask = I2C_FFA;
	else
		pr_err("unmatched machine ID in register_i2c_devices\n");

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) {
		if (msm8960_i2c_devices[i].machs & mach_mask)
			i2c_register_board_info(msm8960_i2c_devices[i].bus,
						msm8960_i2c_devices[i].info,
						msm8960_i2c_devices[i].len);
	}
#endif
}

static void __init gpio_rev_init(void)
{
	gpio_keys_button[0].gpio = gpio_rev(VOLUME_UP);
	gpio_keys_button[1].gpio = gpio_rev(VOLUME_DOWN);
	cypress_touchkey_i2c_gpio_data.sda_pin = gpio_rev(TOUCH_KEY_I2C_SDA);
	cypress_touchkey_i2c_gpio_data.scl_pin = gpio_rev(TOUCH_KEY_I2C_SCL);
}
static void __init msm8960_sim_init(void)
{
	BUG_ON(msm_rpm_init(&msm_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(msm_rpmrs_levels,
				ARRAY_SIZE(msm_rpmrs_levels)));
	regulator_suppress_info_printing();
	platform_device_register(&msm8960_device_rpm_regulator);
	msm_clock_init(&msm8960_clock_init_data);
	msm8960_device_ssbi_pm8921.dev.platform_data =
				&msm8960_ssbi_pm8921_pdata;
	pm8921_platform_data.num_regulators = msm_pm8921_regulator_pdata_len;

	/* Simulator supports a QWERTY keypad */
	pm8921_platform_data.keypad_pdata = &keypad_data_sim;

	msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
	msm8960_device_gadget_peripheral.dev.parent = &msm8960_device_otg.dev;
	msm_device_hsusb_host.dev.parent = &msm8960_device_otg.dev;
	gpiomux_init();
	msm8960_i2c_init();
	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	msm8960_init_buses();
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	pm8921_gpio_mpp_init();
	platform_add_devices(sim_devices, ARRAY_SIZE(sim_devices));
	acpuclk_init(&acpuclk_8960_soc_data);

	msm8960_device_qup_spi_gsbi1.dev.platform_data =
				&msm8960_qup_spi_gsbi1_pdata;
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));

	msm8960_init_mmc();
	msm_fb_add_devices();
	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_pm_set_rpm_wakeup_irq(RPM_APCC_CPU0_WAKE_UP_IRQ);
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);
}

static void __init msm8960_rumi3_init(void)
{
	BUG_ON(msm_rpm_init(&msm_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(msm_rpmrs_levels,
				ARRAY_SIZE(msm_rpmrs_levels)));
	regulator_suppress_info_printing();
	platform_device_register(&msm8960_device_rpm_regulator);
	msm_clock_init(&msm8960_dummy_clock_init_data);
	gpiomux_init();
	msm8960_device_ssbi_pm8921.dev.platform_data =
				&msm8960_ssbi_pm8921_pdata;
	pm8921_platform_data.num_regulators = msm_pm8921_regulator_pdata_len;
	msm8960_device_qup_spi_gsbi1.dev.platform_data =
				&msm8960_qup_spi_gsbi1_pdata;
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	msm8960_i2c_init();
	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	pm8921_gpio_mpp_init();
	platform_add_devices(rumi3_devices, ARRAY_SIZE(rumi3_devices));
	msm8960_init_mmc();
	register_i2c_devices();
	msm_fb_add_devices();
	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_pm_set_rpm_wakeup_irq(RPM_APCC_CPU0_WAKE_UP_IRQ);
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);
}

static void __init msm8960_cdp_init(void)
{
	printk(KERN_ERR "%s\n", __func__);
	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	BUG_ON(msm_rpm_init(&msm_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(msm_rpmrs_levels,
				ARRAY_SIZE(msm_rpmrs_levels)));

	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;
	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");
	platform_device_register(&msm8960_device_rpm_regulator);
	msm_clock_init(&msm8960_clock_init_data);
	msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
	msm8960_device_gadget_peripheral.dev.parent = &msm8960_device_otg.dev;
	msm_device_hsusb_host.dev.parent = &msm8960_device_otg.dev;
	gpiomux_init();
	if (machine_is_msm8960_cdp())
		fpga_init();
	if (machine_is_msm8960_liquid())
		pm8921_platform_data.keypad_pdata = &keypad_data_liquid;
	msm8960_device_qup_spi_gsbi1.dev.platform_data =
				&msm8960_qup_spi_gsbi1_pdata;
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	msm8960_device_ssbi_pm8921.dev.platform_data =
				&msm8960_ssbi_pm8921_pdata;
	pm8921_platform_data.num_regulators = msm_pm8921_regulator_pdata_len;
	msm8960_i2c_init();
	msm8960_gfx_init();
	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	msm8960_init_buses();
	platform_add_devices(msm_footswitch_devices,
		msm_num_footswitch_devices);
	if (machine_is_msm8960_liquid())
		platform_device_register(&msm8960_device_ext_3p3v_vreg);
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	pm8921_gpio_mpp_init();
	platform_add_devices(cdp_devices, ARRAY_SIZE(cdp_devices));
	msm8960_init_cam();
	msm8960_init_mmc();
	acpuclk_init(&acpuclk_8960_soc_data);
	if (machine_is_msm8960_liquid())
		mxt_init_hw_liquid();
	mxt224_init();
	register_i2c_devices();
	msm_fb_add_devices();
	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));
	msm8960_init_dsps();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_pm_set_rpm_wakeup_irq(RPM_APCC_CPU0_WAKE_UP_IRQ);
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);
	change_memory_power = &msm8960_change_memory_power;
}

static void __init samsung_t_project_init(void)
{
#ifdef CONFIG_SEC_DEBUG
	sec_debug_init();
#endif
	printk(KERN_ERR "%s\n", __func__);
	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");

	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	BUG_ON(msm_rpm_init(&msm_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(msm_rpmrs_levels,
		ARRAY_SIZE(msm_rpmrs_levels)));

	gpio_rev_init();
	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;
	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");
		platform_device_register(&msm8960_device_rpm_regulator);
		msm_clock_init(&msm8960_clock_init_data);
		msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
		msm8960_device_gadget_peripheral.dev.parent =
					&msm8960_device_otg.dev;
		msm_device_hsusb_host.dev.parent = &msm8960_device_otg.dev;
	gpiomux_init();
	if (machine_is_msm8960_liquid())
		pm8921_platform_data.keypad_pdata = &keypad_data_liquid;
/*	msm8960_device_qup_spi_gsbi1.dev.platform_data =
				&msm8960_qup_spi_gsbi1_pdata; */
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	msm8960_device_ssbi_pm8921.dev.platform_data =
			&msm8960_ssbi_pm8921_pdata;
	pm8921_platform_data.num_regulators = msm_pm8921_regulator_pdata_len;
	msm8960_i2c_init();
	msm8960_gfx_init();
	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	msm8960_init_buses();
	platform_add_devices(msm_footswitch_devices,
		msm_num_footswitch_devices);
	if (machine_is_msm8960_liquid())
		platform_device_register(&msm8960_device_ext_3p3v_vreg);
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	pm8921_gpio_mpp_init();
	platform_add_devices(t_project_devices, ARRAY_SIZE(t_project_devices));
	msm8960_init_cam();
	msm8960_init_mmc();
	acpuclk_init(&acpuclk_8960_soc_data);
	if (machine_is_msm8960_liquid())
		mxt_init_hw_liquid();
	samsung_sys_class_init();
	mxt224_init();
#if defined(CONFIG_SENSORS_AK8975) || \
	defined(CONFIG_MPU_SENSORS_MPU6050B1) || defined(CONFIG_INPUT_BMP180)
	sensor_device_init();
#endif
#ifdef CONFIG_MPU_SENSORS_MPU6050B1
	mpl_init();
#endif
#if defined(CONFIG_OPTICAL_GP2A) || defined(CONFIG_OPTICAL_GP2AP020A00F)
	opt_init();
#endif
#if defined(CONFIG_NFC_PN544)
	pn544_init();
#endif
	msm8960_mhl_gpio_init();
	register_i2c_devices();
	msm_fb_add_devices();
	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));
	msm8960_init_dsps();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_pm_set_rpm_wakeup_irq(RPM_APCC_CPU0_WAKE_UP_IRQ);
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);
	change_memory_power = &msm8960_change_memory_power;
    BUG_ON(msm_pm_boot_init(MSM_PM_BOOT_CONFIG_TZ, NULL));
}

MACHINE_START(MSM8960_SIM, "QCT MSM8960 SIMULATOR")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = msm8960_sim_init,
	.init_early = msm8960_allocate_memory_regions,
MACHINE_END

MACHINE_START(MSM8960_RUMI3, "QCT MSM8960 RUMI3")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = msm8960_rumi3_init,
	.init_early = msm8960_allocate_memory_regions,
MACHINE_END

MACHINE_START(MSM8960_CDP, "QCT MSM8960 CDP")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = msm8960_cdp_init,
	.init_early = msm8960_allocate_memory_regions,
MACHINE_END

MACHINE_START(JAGUAR, "SAMSUNG T RROJECT")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = samsung_t_project_init,
	.init_early = msm8960_allocate_memory_regions,
	.init_very_early = msm8960_early_memory,
MACHINE_END
MACHINE_START(MSM8960_MTP, "QCT MSM8960 MTP")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = msm8960_cdp_init,
	.init_early = msm8960_allocate_memory_regions,
MACHINE_END

MACHINE_START(MSM8960_FLUID, "QCT MSM8960 FLUID")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = msm8960_cdp_init,
	.init_early = msm8960_allocate_memory_regions,
MACHINE_END

MACHINE_START(MSM8960_LIQUID, "QCT MSM8960 LIQUID")
	.map_io = msm8960_map_io,
	.reserve = msm8960_reserve,
	.init_irq = msm8960_init_irq,
	.timer = &msm_timer,
	.init_machine = msm8960_cdp_init,
	.init_early = msm8960_allocate_memory_regions,
MACHINE_END
#endif
