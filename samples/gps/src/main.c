/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

/* TODO: This needs to be fixed. OK for now. */
#include "../../../../ext/lib/gps_nmea_parser/include/gps/gps.h"

extern void SIM80x_Get_GPS(gps_t *gps);

void main(void)
{
	gps_t gps;

    while (1)
    {
    	/* Get GPS. */
		SIM80x_Get_GPS(&gps);
		printk("Latitude: %d", (int)gps.latitude);
		printk("Longitude: %d", (int)gps.longitude);

		/* sleep.. */
		k_sleep(100);
    }
}
