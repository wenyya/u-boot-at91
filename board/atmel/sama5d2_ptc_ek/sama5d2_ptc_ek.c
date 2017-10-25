/*
 * Copyright (C) 2017 Microchip Corporation
 *		      Wenyou Yang <wenyou.yang@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <i2c.h>
#include <nand.h>
#include <version.h>
#include <asm/io.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/atmel_pio4.h>
#include <asm/arch/atmel_mpddrc.h>
#include <asm/arch/atmel_sdhci.h>
#include <asm/arch/clk.h>
#include <asm/arch/gpio.h>
#include <asm/arch/sama5d2.h>
#include <asm/arch/sama5d2_smc.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_NAND_ATMEL
static void board_nand_hw_init(void)
{
	struct at91_smc *smc = (struct at91_smc *)ATMEL_BASE_SMC;

	at91_periph_clk_enable(ATMEL_ID_HSMC);

	/* Configure SMC CS3 for NAND */
	writel(AT91_SMC_SETUP_NWE(1) | AT91_SMC_SETUP_NCS_WR(1) |
	       AT91_SMC_SETUP_NRD(1) | AT91_SMC_SETUP_NCS_RD(1),
	       &smc->cs[3].setup);
	writel(AT91_SMC_PULSE_NWE(2) | AT91_SMC_PULSE_NCS_WR(3) |
	       AT91_SMC_PULSE_NRD(2) | AT91_SMC_PULSE_NCS_RD(3),
	       &smc->cs[3].pulse);
	writel(AT91_SMC_CYCLE_NWE(5) | AT91_SMC_CYCLE_NRD(5),
	       &smc->cs[3].cycle);
	writel(AT91_SMC_TIMINGS_TCLR(2) | AT91_SMC_TIMINGS_TADL(7) |
	       AT91_SMC_TIMINGS_TAR(2)  | AT91_SMC_TIMINGS_TRR(3)   |
	       AT91_SMC_TIMINGS_TWB(7)  | AT91_SMC_TIMINGS_RBNSEL(3)|
	       AT91_SMC_TIMINGS_NFSEL(1), &smc->cs[3].timings);
	writel(AT91_SMC_MODE_RM_NRD | AT91_SMC_MODE_WM_NWE |
	       AT91_SMC_MODE_EXNW_DISABLE |
	       AT91_SMC_MODE_DBW_8 |
	       AT91_SMC_MODE_TDF_CYCLE(3),
	       &smc->cs[3].mode);

	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 22, 0);	/* D0 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 23, 0);	/* D1 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 24, 0);	/* D2 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 25, 0);	/* D3 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 26, 0);	/* D4 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 27, 0);	/* D5 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 28, 0);	/* D6 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 29, 0);	/* D7 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 2, 0);	/* RE */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 30, 0);	/* WE */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 31, 1);	/* NCS */
	atmel_pio4_set_b_periph(AT91_PIO_PORTC, 8, 1);	/* RDY */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 0, 1);	/* ALE */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 1, 1);	/* CLE */
}
#endif

static void board_usb_hw_init(void)
{
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 12, 1);
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
#ifdef CONFIG_DEBUG_UART
	debug_uart_init();
#endif

	return 0;
}
#endif

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

#ifdef CONFIG_NAND_ATMEL
	board_nand_hw_init();
#endif
#ifdef CONFIG_CMD_USB
	board_usb_hw_init();
#endif

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				    CONFIG_SYS_SDRAM_SIZE);
	return 0;
}

#ifdef CONFIG_CMD_I2C
static int set_ethaddr_from_eeprom(void)
{
	const int ETH_ADDR_LEN = 6;
	unsigned char ethaddr[ETH_ADDR_LEN];
	const char *ETHADDR_NAME = "ethaddr";
	struct udevice *bus, *dev;

	if (getenv(ETHADDR_NAME))
		return 0;

	if (uclass_get_device_by_seq(UCLASS_I2C, 1, &bus)) {
		printf("Cannot find I2C bus 1\n");
		return -1;
	}

	if (dm_i2c_probe(bus, AT24MAC_ADDR, 0, &dev)) {
		printf("Failed to probe I2C chip\n");
		return -1;
	}

	if (dm_i2c_read(dev, AT24MAC_REG, ethaddr, ETH_ADDR_LEN)) {
		printf("Failed to read ethernet address from EEPROM\n");
		return -1;
	}

	if (!is_valid_ethaddr(ethaddr)) {
		printf("The ethernet address read from EEPROM is not valid!\n");
		return -1;
	}

	return eth_setenv_enetaddr(ETHADDR_NAME, ethaddr);
}
#else
static int set_ethaddr_from_eeprom(void)
{
	return 0;
}
#endif

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	set_ethaddr_from_eeprom();

	return 0;
}
#endif

