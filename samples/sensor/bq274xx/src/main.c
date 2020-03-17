/*
 * Copyright (c) 2019 Linumiz 
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

static void do_main(struct device *dev)
{
	struct sensor_value voltage, current, state_of_charge,
 		 full_charge_capacity, remaining_charge_capacity, avg_power,
		 int_temp, current_standby, current_max_load,
		 state_of_health;

	int status = 0;

	while (1) {
		status = sensor_sample_fetch(dev);
		if (status < 0) {
			printk("Unable to fetch the samples \n");
		}
		
		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE, 
				&voltage);
		if (status < 0) {
			printk("Unable to get the voltage value \n");
		}
	
		printk("Voltage: %d.%06dV \n", voltage.val1, 
				voltage.val2);	

		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_CURRENT,
			       	&current);
		if (status < 0) {
			printk("Unable to get the current value \n");
		}
		
		if ((current.val2 < 0) && (current.val1 >= 0)) {
			current.val2 = -(current.val2);
			printk("Avg Current: -%d.%06dA \n",current.val1,
			     		current.val2);
		} else if ((current.val2 > 0) && (current.val1 < 0)) {
			printk("Avg Current: %d.%06dA \n", current.val1,
					current.val2);	
		} else if ((current.val2 < 0) && (current.val1 < 0)) {
			current.val2 = -(current.val2);
			printk("Avg Current: %d.%06dA \n", current.val1,
					current.val2);
		} else {
			printk("Avg Current: %d.%06dA \n", current.val1,
					current.val2);
		}

		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_STDBY_CURRENT,
			       	&current_standby);
		if (status < 0) {
			printk("Unable to get the current value \n");
		}
		
		if ((current_standby.val2 < 0) && (current_standby.val1 >= 0)) {
			current_standby.val2 = -(current_standby.val2);
			printk("Standby Current: -%d.%06dA \n",current_standby.val1,
			     		current_standby.val2);
		} else if ((current_standby.val2 > 0) && (current_standby.val1 < 0)) {
			printk("Standby Current: %d.%06dA \n", current_standby.val1,
					current_standby.val2);	
		} else if ((current_standby.val2 < 0) && (current_standby.val1 < 0)) {
			current_standby.val2 = -(current_standby.val2);
			printk("Standby Current: %d.%06dA \n", current_standby.val1,
					current_standby.val2);
		} else {
			printk("Standby Current: %d.%06dA \n", current_standby.val1,
					current_standby.val2);
		}

		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT, 
					&current_max_load);
		if (status < 0) {
			printk("Unable to get the current value \n");
		}
		
		if ((current_max_load.val2 < 0) && (current_max_load.val1 >= 0)) {
			current_max_load.val2 = -(current_max_load.val2);
			printk("Max Load Current: -%d.%06dA \n",current_max_load.val1,
			     		current_max_load.val2);
		} else if ((current_max_load.val2 > 0) && (current_max_load.val1 < 0)) {
			printk("Max Load Current: %d.%06dA \n", current_max_load.val1,
					current_max_load.val2);	
		} else if ((current_max_load.val2 < 0) && (current_max_load.val1 < 0)) {
			current_max_load.val2 = -(current_max_load.val2);
			printk("Max Load Current: %d.%06dA \n", current_max_load.val1,
					current.val2);
		} else {
			printk("Max Load Current: %d.%06dA \n", current_max_load.val1,
					current_max_load.val2);
		}

		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, 
				&state_of_charge);
		if (status < 0) {
			printk("Unable to get state of charge \n");
		}

		printk("State of charge: %d%% \n", state_of_charge.val1);

		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_STATE_OF_HEALTH, 
				&state_of_health);
		if (status < 0) {
			printk("Unable to get state of charge \n");
		}

		printk("State of health: %d%% \n", state_of_health.val1);

		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_POWER, 
				&avg_power);
		if (status < 0) {
			printk("Unable to get avg power \n");
		}

                if ((avg_power.val2 < 0) && (avg_power.val1 >= 0)) {
                        avg_power.val2 = -(avg_power.val2);
                        printk("Avg Power: -%d.%06dW \n",avg_power.val1,
                                        avg_power.val2);
                } else if ((avg_power.val2 > 0) && (avg_power.val1 < 0)) {
                        printk("Avg Power: %d.%06dW \n", avg_power.val1,
                                        avg_power.val2);
                } else if ((avg_power.val2 < 0) && (avg_power.val1 < 0)) {
                        avg_power.val2 = -(avg_power.val2);
			printk("Avg Power: %d.%06dW \n", avg_power.val1,
                                        avg_power.val2);
                } else {
                        printk("Avg Power: %d.%06dW \n", avg_power.val1,
                                        avg_power.val2);
                }
		
		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY, 
				&full_charge_capacity);
		if (status < 0) {
			printk("Unable to get full charge capacity \n");
		}

		printk("Full charge capacity: %d.%06dAh \n", 
				full_charge_capacity.val1, full_charge_capacity.val2);


		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY,
			      	&remaining_charge_capacity);
		if (status < 0) {
			printk("Unable to get remaining charge capacity \n");
		}

		printk("Remaining charge capacity: %d.%06dAh \n", 
			remaining_charge_capacity.val1, remaining_charge_capacity.val2);

		status = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP,
						&int_temp);
		if (status < 0) {
			printk("Unable to read internal temperature \n");
		}

		printk("Gauge Temperature: %d.%06d C \n",
				int_temp.val1, int_temp.val2);

		printk("\n\n");

		k_sleep(K_MSEC(5000));
	}

}

void main(void)
{
	struct device *dev;

	dev = device_get_binding(DT_INST_0_TI_BQ274XX_LABEL);
	if (!dev) {
		printk("Failed to get device binding");
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->config->name);

	do_main(dev);
}
