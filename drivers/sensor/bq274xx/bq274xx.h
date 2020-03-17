/*
 * Copyright(c) 2019 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ274XX_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ274XX_H_


#include <logging/log.h>
LOG_MODULE_REGISTER(bq274xx, CONFIG_SENSOR_LOG_LEVEL);

/*** Default I2C Address ***/
#define BQ274XX_I2C_ADDRESS	0x55

/*** General Constant ***/
#define BQ274XX_UNSEAL_KEY	0x8000 		// Secret code to unseal the BQ27441-G1A
#define BQ274XX_DEVICE_ID	0x0421 		// Default device ID


/*** Standard Commands ***/
#define BQ274XX_COMMAND_CONTROL_LOW		0x00 		// Control() low register
#define BQ274XX_COMMAND_CONTROL_HIGH		0x01		// Control() high register
#define BQ274XX_COMMAND_TEMP			0x02 		// Temperature()
#define BQ274XX_COMMAND_VOLTAGE			0x04 		// Voltage()
#define BQ274XX_COMMAND_FLAGS			0x06 		// Flags()
#define BQ274XX_COMMAND_NOM_CAPACITY		0x08 		// NominalAvailableCapacity()
#define BQ274XX_COMMAND_AVAIL_CAPACITY		0x0A 		// FullAvailableCapacity()
#define BQ274XX_COMMAND_REM_CAPACITY		0x0C 		// RemainingCapacity()
#define BQ274XX_COMMAND_FULL_CAPACITY		0x0E 		// FullChargeCapacity()
#define BQ274XX_COMMAND_AVG_CURRENT		0x10 		// AverageCurrent()
#define BQ274XX_COMMAND_STDBY_CURRENT		0x12 		// StandbyCurrent()
#define BQ274XX_COMMAND_MAX_CURRENT		0x14 		// MaxLoadCurrent()
#define BQ274XX_COMMAND_AVG_POWER		0x18 		// AveragePower()
#define BQ274XX_COMMAND_SOC			0x1C 		// StateOfCharge()
#define BQ274XX_COMMAND_INT_TEMP		0x1E 		// InternalTemperature()
#define BQ274XX_COMMAND_SOH			0x20 		// StateOfHealth()
#define BQ274XX_COMMAND_REM_CAP_UNFL		0x28 		// RemainingCapacityUnfiltered()
#define BQ274XX_COMMAND_REM_CAP_FIL		0x2A 		// RemainingCapacityFiltered()
#define BQ274XX_COMMAND_FULL_CAP_UNFL		0x2C 		// FullChargeCapacityUnfiltered()
#define BQ274XX_COMMAND_FULL_CAP_FIL		0x2E 		// FullChargeCapacityFiltered()
#define BQ274XX_COMMAND_SOC_UNFL		0x30 		// StateOfChargeUnfiltered()


/*** Control Sub-Commands ***/
#define BQ274XX_CONTROL_STATUS			0x0000
#define BQ274XX_CONTROL_DEVICE_TYPE		0x0001
#define BQ274XX_CONTROL_FW_VERSION		0x0002
#define BQ274XX_CONTROL_DM_CODE			0x0004
#define BQ274XX_CONTROL_PREV_MACWRITE		0x0007
#define BQ274XX_CONTROL_CHEM_ID			0x0008
#define BQ274XX_CONTROL_BAT_INSERT		0x000C
#define BQ274XX_CONTROL_BAT_REMOVE		0x000D
#define BQ274XX_CONTROL_SET_HIBERNATE		0x0011
#define BQ274XX_CONTROL_CLEAR_HIBERNATE		0x0012
#define BQ274XX_CONTROL_SET_CFGUPDATE		0x0013
#define BQ274XX_CONTROL_SHUTDOWN_ENABLE		0x001B
#define BQ274XX_CONTROL_SHUTDOWN		0x001C
#define BQ274XX_CONTROL_SEALED			0x0020
#define BQ274XX_CONTROL_PULSE_SOC_INT		0x0023
#define BQ274XX_CONTROL_RESET			0x0041
#define BQ274XX_CONTROL_SOFT_RESET		0x0042
#define BQ274XX_CONTROL_EXIT_CFGUPDATE		0x0043
#define BQ274XX_CONTROL_EXIT_RESIM		0x0044

/*** Extended Data Commands ***/
#define BQ274XX_EXTENDED_OPCONFIG			0x3A 		// OpConfig()
#define BQ274XX_EXTENDED_CAPACITY			0x3C 		// DesignCapacity()
#define BQ274XX_EXTENDED_DATA_CLASS			0x3E 		// DataClass()
#define BQ274XX_EXTENDED_DATA_BLOCK			0x3F 		// DataBlock()
#define BQ274XX_EXTENDED_BLOCKDATA_START		0x40 		// BlockData_start()
#define BQ274XX_EXTENDED_BLOCKDATA_END			0x5F		// BlockData_end()
#define BQ274XX_EXTENDED_CHECKSUM			0x60 		// BlockDataCheckSum()
#define BQ274XX_EXTENDED_DATA_CONTROL			0x61 		// BlockDataControl()
#define BQ274XX_EXTENDED_BLOCKDATA_DESIGN_CAP_HIGH 	0x4A		// BlockData
#define BQ274XX_EXTENDED_BLOCKDATA_DESIGN_CAP_LOW 	0x4B
#define BQ274XX_EXTENDED_BLOCKDATA_DESIGN_ENR_HIGH	0x4C
#define BQ274XX_EXTENDED_BLOCKDATA_DESIGN_ENR_LOW	0x4D
#define BQ274XX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_HIGH	0x50
#define BQ274XX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_LOW	0x51
#define BQ274XX_EXTENDED_BLOCKDATA_TAPERRATE_HIGH	0x5B
#define BQ274XX_EXTENDED_BLOCKDATA_TAPERRATE_LOW	0x5C

#define BQ274XX_DELAY			1000

struct bq274xx_data {
        struct device *i2c;
	uint16_t voltage;
	int16_t avg_current;
	int16_t stdby_current;
	int16_t max_load_current;
	int16_t avg_power;
	uint16_t state_of_charge;
	int16_t state_of_health;
	uint16_t internal_temperature;
	uint16_t full_charge_capacity;
	uint16_t remaining_charge_capacity;
	uint16_t nom_avail_capacity;
	uint16_t full_avail_capacity;
};



int bq274xx_command_reg_read(struct bq274xx_data *bq274xx, u8_t reg_addr,
					int16_t *val)
{
	u8_t i2c_data[2];
	int status;

	status = i2c_burst_read(bq274xx->i2c, BQ274XX_I2C_ADDRESS,
			reg_addr, i2c_data, 2);
	if (status < 0) {
		printk("Unable to read register \n");
		return -EIO;
	}

	*val = (i2c_data[1] << 8) | i2c_data[0];
	
	return 0;
}

int bq274xx_control_reg_write(struct bq274xx_data *bq274xx, u16_t subcommand)
{
	u8_t i2c_data, reg_addr;
	int status = 0;

	reg_addr = BQ274XX_COMMAND_CONTROL_LOW;
	i2c_data = (u8_t)((subcommand) & 0x00FF);
	
	status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, reg_addr, i2c_data);
	if (status < 0) {
		printk("Failed to write into control low register \n");
		return -EIO;
	}

	k_sleep(5);

	reg_addr = BQ274XX_COMMAND_CONTROL_HIGH;
       	i2c_data = (u8_t)((subcommand >> 8) & 0x00FF);

	status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, reg_addr, i2c_data);
	if (status < 0) {
		printk("Failed to write into control high register \n");
		return -EIO;
	}

	return 0;
}

int bq274xx_command_reg_write(struct bq274xx_data *bq274xx, u8_t command, u8_t data)
{
	u8_t i2c_data, reg_addr;
	int status = 0;

        reg_addr = command;
        i2c_data = data;

        status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, reg_addr, i2c_data);
        if (status < 0) {
                printk("Failed to write into control register \n");
                return -EIO;
        }

	return 0;
}

int bq274xx_read_data_block(struct bq274xx_data *bq274xx, u8_t offset, u8_t *data, u8_t bytes)
{
	u8_t i2c_data;
	int status = 0;

	i2c_data = BQ274XX_EXTENDED_BLOCKDATA_START + offset;
       	
	status = i2c_burst_read(bq274xx->i2c, BQ274XX_I2C_ADDRESS, i2c_data, data, bytes);
	if (status < 0) {
		printk("Failed to read block \n");
		return -EIO;
	}

	k_sleep(5);

	return 0;	
}


int bq274xx_get_device_type(struct bq274xx_data *bq274xx, u16_t *val)
{
	int status;

	status = bq274xx_control_reg_write(bq274xx, 
				BQ274XX_CONTROL_DEVICE_TYPE);
	if (status < 0) {
		printk("Unable to write control register \n");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, 
			BQ274XX_COMMAND_CONTROL_LOW, val);

	if (status < 0) {
		printk("Unable to read register \n");
		return -EIO;
	}
	
	return 0;

}
#endif
