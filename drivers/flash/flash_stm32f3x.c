/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN flash_stm32f3
#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>

#include "flash_stm32.h"


/* offset and len must be aligned on 2 for write
 * , positive and not beyond end of flash */
bool flash_stm32_valid_range(struct device *dev, off_t offset, u32_t len,
			     bool write)
{
	return (!write || (offset % 2 == 0 && len % 2 == 0U)) &&
		flash_stm32_range_exists(dev, offset, len);
}

static unsigned int get_page(off_t offset)
{
	return offset / FLASH_PAGE_SIZE;
}

static int erase_page(struct device *dev, unsigned int page)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	u32_t page_address = CONFIG_FLASH_BASE_ADDRESS;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	page_address += page * FLASH_PAGE_SIZE;

	/* Set the PER bit and select the page you wish to erase */
	regs->CR |= FLASH_CR_PER;
	/* Set page address */
	regs->AR = page_address;
	/* Set the STRT bit */
	regs->CR |= FLASH_CR_STRT;

	/* Wait for the BSY bit */
	rc = flash_stm32_wait_flash_idle(dev);

	regs->CR &= ~FLASH_CR_PER;

	return rc;
}

int flash_stm32_block_erase_loop(struct device *dev, unsigned int offset,
				 unsigned int len)
{
	int i, rc = 0;

	i = get_page(offset);
	for (; i <= get_page(offset + len - 1) ; ++i) {
		rc = erase_page(dev, i);
		if (rc < 0) {
			break;
		}
	}

	return rc;
}


static int write_hword(struct device *dev, off_t offset, u16_t val)
{
	volatile u16_t *flash = (u16_t *)(offset + CONFIG_FLASH_BASE_ADDRESS);
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	u32_t tmp;
	int rc;

	/* if the control register is locked, do not fail silently */
	if (regs->CR & FLASH_CR_LOCK) {
		return -EIO;
	}

	/* Check that no Flash main memory operation is ongoing */
	rc = flash_stm32_wait_flash_idle(dev);
	if (rc < 0) {
		return rc;
	}

	/* Check if this half word is erased */
	if (*flash != 0xFFFF) {
		return -EIO;
	}

	/* Set the PG bit */
	regs->CR |= FLASH_CR_PG;

	/* Flush the register write */
	tmp = regs->CR;

	/* Perform the data write operation at the desired memory address */
	*flash = val;

	/* Wait until the BSY bit is cleared */
	rc = flash_stm32_wait_flash_idle(dev);

	/* Clear the PG bit */
	regs->CR &= (~FLASH_CR_PG);

	return rc;
}


int flash_stm32_write_range(struct device *dev, unsigned int offset,
			    const void *data, unsigned int len)
{
	int i, rc = 0;

	for (i = 0; i < len; i += 2, offset += 2U) {
		rc = write_hword(dev, offset, ((const u16_t *) data)[i>>1]);
		if (rc < 0) {
			return rc;
		}
	}

	return rc;
}

void flash_stm32_page_layout(struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size)
{
	static struct flash_pages_layout stm32f3_flash_layout = {
		.pages_count = 0,
		.pages_size = 0,
	};

	ARG_UNUSED(dev);

	if (stm32f3_flash_layout.pages_count == 0) {
		stm32f3_flash_layout.pages_count =
			DT_REG_SIZE(DT_INST(0, soc_nv_flash)) / FLASH_PAGE_SIZE;
		stm32f3_flash_layout.pages_size = FLASH_PAGE_SIZE;
	}

	*layout = &stm32f3_flash_layout;
	*layout_size = 1;
}
