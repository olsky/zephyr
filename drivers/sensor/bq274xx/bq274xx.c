/*
 * Copyright (c) 2019 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <string.h>
#include <sys/byteorder.h>

#include "bq274xx.h"

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int bq274xx_channel_get(struct device *dev, 
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct bq274xx_data *bq274xx = dev->driver_data;
	float int_temp;
	switch (chan) {
		case SENSOR_CHAN_GAUGE_VOLTAGE:
			val->val1 = ((bq274xx->voltage / 1000));
			val->val2 = ((bq274xx->voltage % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_AVG_CURRENT:
			val->val1 = ((bq274xx->avg_current / 1000));
			val->val2 = ((bq274xx->avg_current % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_STDBY_CURRENT:
			val->val1 = ((bq274xx->stdby_current / 1000));
			val->val2 = ((bq274xx->stdby_current % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT:
			val->val1 = ((bq274xx->max_load_current / 1000));
			val->val2 = ((bq274xx->max_load_current % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_TEMP:
			int_temp = (bq274xx->internal_temperature * 0.1);		
			int_temp = int_temp - 273.15;
			val->val1 = (s32_t)int_temp;
			val->val2 = (int_temp - (s32_t)int_temp) * 1000000;  
			break;

		case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
			val->val1 = bq274xx->state_of_charge;
			val->val2 = 0;
			break;
	
		case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
			val->val1 = bq274xx->state_of_health;
			val->val2 = 0;
			break;

		case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
			val->val1 = (bq274xx->full_charge_capacity / 1000);
			val->val2 = ((bq274xx->full_charge_capacity % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
			val->val1 = (bq274xx->remaining_charge_capacity / 1000);
			val->val2 = ((bq274xx->remaining_charge_capacity % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
			val->val1 = (bq274xx->nom_avail_capacity / 1000);
			val->val2 = ((bq274xx->nom_avail_capacity % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY:
			val->val1 = (bq274xx->full_avail_capacity / 1000);
			val->val2 = ((bq274xx->full_avail_capacity % 1000) * 1000U);
			break;

		case SENSOR_CHAN_GAUGE_AVG_POWER:
			val->val1 = (bq274xx->avg_power / 1000);
		       	val->val2 = ((bq274xx->avg_power % 1000) * 1000U);
			break;	

		default:
			return -ENOTSUP;
	}	

	return 0;
}


static int bq274xx_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct bq274xx_data *bq274xx = dev->driver_data;
	int status = 0;
	
	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_VOLTAGE, 
			          &bq274xx->voltage);
	if (status < 0) {
		LOG_ERR("Failed to read voltage");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_AVG_CURRENT, 
				  &bq274xx->avg_current);
	if (status < 0) {
		LOG_ERR("Failed to read average current ");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_INT_TEMP, 
			          &bq274xx->internal_temperature);
	if (status < 0) {
		LOG_ERR("Failed to read internal temperature");
		return -EIO;
	}
		
	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_STDBY_CURRENT, 
				  &bq274xx->stdby_current);
	if (status < 0) {
		LOG_ERR("Failed to read standby current");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_MAX_CURRENT, 
				  &bq274xx->max_load_current);
	if (status < 0) {
		LOG_ERR("Failed to read maximum current");
		return -EIO;
	}
	
	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_SOC, 
				  &bq274xx->state_of_charge);
	if (status < 0) {
		LOG_ERR("Failed to read state of charge");
		return -EIO;
	}	

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_FULL_CAPACITY, 
				  &bq274xx->full_charge_capacity);
	if (status < 0) {
		LOG_ERR("Failed to read full charge capacity");
		return -EIO;
	}
	
	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_REM_CAPACITY, 
				  &bq274xx->remaining_charge_capacity);
	if (status < 0) {
		LOG_ERR("Failed to read battery remaining charge capacity");
		return -EIO;
	}
	
	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_NOM_CAPACITY,
				  &bq274xx->nom_avail_capacity);
	if (status < 0) {
		LOG_ERR("Failed to read battery nominal available capacity");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_AVAIL_CAPACITY,
				  &bq274xx->full_avail_capacity);
	if (status < 0) {
		LOG_ERR("Failed to read battery full available capacity");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_AVG_POWER,
				  &bq274xx->avg_power);
	if (status < 0) { 
		LOG_ERR("Failed to read battery average power");
		return -EIO;
	}

	status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_SOH,
				  &bq274xx->state_of_health);

	bq274xx->state_of_health = (bq274xx->state_of_health) & 0x00FF;

	if (status < 0) {
		LOG_ERR("Failed to read state of health");
		return -EIO;
	}


	return 0;
}


/**
 * @brief initialise the fuel guage
 *
 * @return 0 for success
 */
int bq274xx_gauge_init(struct device *dev)
{
	struct bq274xx_data *bq274xx = dev->driver_data;
	int status = 0;
	u8_t tmp_checksum = 0, checksumOld = 0, checksumNew = 0;
	u16_t flags = 0, designEnergy_mWh = 0, taperRate = 0, id;
	u8_t designCap_MSB, designCap_LSB, designEnergy_MSB, designEnergy_LSB,
    	     terminateVolt_MSB, terminateVolt_LSB, taperRate_MSB, taperRate_LSB;	     
	u8_t block[32];

	designEnergy_mWh = (u16_t) 3.7 * CONFIG_BQ274XX_DESIGN_CAPACITY;
	taperRate = (u16_t) CONFIG_BQ274XX_DESIGN_CAPACITY / ( 0.1 * CONFIG_BQ274XX_TAPER_CURRENT);

	bq274xx->i2c = device_get_binding(DT_INST_0_TI_BQ274XX_BUS_NAME);
	if (bq274xx == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			  DT_INST_0_TI_BQ274XX_BUS_NAME);
		return -EINVAL;
	} 

	status = bq274xx_get_device_type(bq274xx, &id);
	if (status < 0) {
		LOG_ERR("Unable to get device ID");
		return -EIO;
	}

	if (id != BQ274XX_DEVICE_ID) {
		LOG_ERR("Invalid Device");
		return -EINVAL;
	}

	/** Unseal the battery control register **/
	status = bq274xx_control_reg_write(bq274xx, BQ274XX_UNSEAL_KEY);
	if (status < 0) {
		LOG_ERR("Unable to unseal the battery");
		return -EIO;
	}

	status = bq274xx_control_reg_write(bq274xx, BQ274XX_UNSEAL_KEY);
	if (status < 0) {
		LOG_ERR("Unable to unseal the battery");
		return -EIO;
	}

	/* Send CFG_UPDATE */
	status = bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_SET_CFGUPDATE);
	if (status < 0) {
		LOG_ERR("Unable to set CFGUpdate");
		return -EIO;
	}

	/** Step to place the Gauge into CONFIG_UPDATE Mode **/
	do {
		status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_FLAGS, &flags);
		if (status < 0) {
			LOG_ERR("Unable to read flags");
			return -EIO;
		}

		if (!(flags & 0x0010)) {
			k_sleep(50);
		}

	}while(!(flags & 0x0010));

	status = bq274xx_command_reg_write(bq274xx, BQ274XX_EXTENDED_DATA_CONTROL, 0x00);	
	if (status < 0) {
		LOG_ERR("Failed to enable block data memory");
		return -EIO;
	}

	/* Access State subclass */
	status = bq274xx_command_reg_write(bq274xx, BQ274XX_EXTENDED_DATA_CLASS, 0x52);
	if (status < 0) {
		LOG_ERR("Failed to update state subclass");
		return -EIO;
	}
	
	/* Write the block offset */
	status = bq274xx_command_reg_write(bq274xx, BQ274XX_EXTENDED_DATA_BLOCK, 0x00);
	if (status < 0) {
		LOG_ERR("Failed to update block offset");
		return -EIO;
	}
	
	for (u8_t i = 0; i < 32; i++) {
		block[i] = 0;
	}
	
	status = bq274xx_read_data_block(bq274xx, 0x00, block, 32);
	if (status < 0) {
		LOG_ERR("Unable to read block data");
		return -EIO;
	}

	tmp_checksum = 0;
	for (u8_t i = 0; i < 32; i++) {
		tmp_checksum += block[i];
	}

	tmp_checksum = 255 - tmp_checksum;
	
	/* Read the block checksum */
	status = i2c_reg_read_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, 
				BQ274XX_EXTENDED_CHECKSUM, &checksumOld);	
	if (status < 0) {
		LOG_ERR("Unable to read block checksum \n");
		return -EIO;
	}

	designCap_MSB = CONFIG_BQ274XX_DESIGN_CAPACITY >> 8;
        designCap_LSB = CONFIG_BQ274XX_DESIGN_CAPACITY & 0x00FF;
	designEnergy_MSB = designEnergy_mWh >> 8;
	designEnergy_LSB = designEnergy_mWh & 0x00FF;
	terminateVolt_MSB = CONFIG_BQ274XX_TERMINATE_VOLTAGE >> 8;
	terminateVolt_LSB = CONFIG_BQ274XX_TERMINATE_VOLTAGE & 0x00FF;
	taperRate_MSB = taperRate >> 8;
	taperRate_LSB = taperRate & 0x00FF;

	status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, 
			BQ274XX_EXTENDED_BLOCKDATA_DESIGN_CAP_HIGH, designCap_MSB);
        if (status < 0) {
                LOG_ERR("Failed to write designCAP MSB");
                return -EIO;
        }

	status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, 
			BQ274XX_EXTENDED_BLOCKDATA_DESIGN_CAP_LOW, designCap_LSB);
	if (status < 0) {
		LOG_ERR("Failed to erite designCAP LSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS,
			BQ274XX_EXTENDED_BLOCKDATA_DESIGN_ENR_HIGH, designEnergy_MSB);
        if (status < 0) {
                LOG_ERR("Failed to write designEnergy MSB");
                return -EIO;
        }

        status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS,
		       BQ274XX_EXTENDED_BLOCKDATA_DESIGN_ENR_LOW, designEnergy_LSB);
        if (status < 0) {
              	LOG_ERR("Failed to erite designEnergy LSB");
                return -EIO;
        }

        status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, 
		BQ274XX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_HIGH, terminateVolt_MSB);
        if (status < 0) {
                LOG_ERR("Failed to write terminateVolt MSB");
                return -EIO;
        }

        status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, 
		BQ274XX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_LOW, terminateVolt_LSB);
        if (status < 0) {
                LOG_ERR("Failed to write terminateVolt LSB");
                return -EIO;
        }

        status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS, 
		BQ274XX_EXTENDED_BLOCKDATA_TAPERRATE_HIGH, taperRate_MSB);
        if (status < 0) {
                LOG_ERR("Failed to write taperRate MSB");
                return -EIO;
        }

        status = i2c_reg_write_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS,
		BQ274XX_EXTENDED_BLOCKDATA_TAPERRATE_LOW, taperRate_LSB);
        if (status < 0) {
                LOG_ERR("Failed to erite taperRate LSB");
                return -EIO;
        }

        for (u8_t i = 0; i < 32; i++) {
                block[i] = 0;
        }

        status = bq274xx_read_data_block(bq274xx, 0x00, block, 32);
        if (status < 0) {
                LOG_ERR("Unable to read block data");
                return -EIO;
        }

        checksumNew = 0;
        for (u8_t i = 0; i < 32; i++) {
                checksumNew += block[i];
        }

	checksumNew = 255 - checksumNew;

        status = bq274xx_command_reg_write(bq274xx, BQ274XX_EXTENDED_CHECKSUM, checksumNew);
        if (status < 0) {
                LOG_ERR("Failed to update new checksum");
                return -EIO;
        }
	
	tmp_checksum = 0;
	status = i2c_reg_read_byte(bq274xx->i2c, BQ274XX_I2C_ADDRESS,
			BQ274XX_EXTENDED_CHECKSUM, &tmp_checksum);
        if (status < 0) {
                LOG_ERR("Failed to read checksum");
                return -EIO;
        }	

	status = bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_BAT_INSERT);
	if (status < 0) {
		LOG_ERR("Unable to configure BAT Detect");
		return -EIO;
	}

        status = bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_SOFT_RESET);
        if (status < 0) {
                LOG_ERR("Failed to soft reset the gauge");
                return -EIO;
        }

	flags = 0;
        /* Poll Flags   */
        do {
                status = bq274xx_command_reg_read(bq274xx, BQ274XX_COMMAND_FLAGS, &flags);
                if (status < 0) {
                        LOG_ERR("Unable to read flags");
                        return -EIO;
                }

                if (!(flags & 0x0010)) {
                        k_sleep(50);
                }
        } while((flags & 0x0010));

        /* Seal the gauge */
        status = bq274xx_control_reg_write(bq274xx, BQ274XX_CONTROL_SEALED);
        if (status < 0) {
                LOG_ERR("Failed to seal the gauge \n");
                return -EIO;
        }
	
	return 0;	
}

static const struct sensor_driver_api bq274xx_battery_driver_api = {
	.sample_fetch = bq274xx_sample_fetch,
	.channel_get = bq274xx_channel_get,
};

static struct bq274xx_data bq274xx_driver;

DEVICE_AND_API_INIT(bq274xx, DT_INST_0_TI_BQ274XX_LABEL,
	       	    bq274xx_gauge_init, &bq274xx_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &bq274xx_battery_driver_api);
