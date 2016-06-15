/*
 * LED Kernel Temprerature Trigger
 *
 * Copyright 2005-2006 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/regulator/consumer.h>
#include <linux/rockchip/common.h>
#include <linux/rockchip/dvfs.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include "../leds.h"

#define TRIG_TEMP   80
#define LOOP_SECS   30 * HZ
#define NODELAY_LOOP_SECS 0

struct temp_trig_data {
     struct led_classdev *led;
     struct delayed_work work;
     struct regulator *regulator;
     int trig_temp;
};
static void temp_trig_work(struct work_struct *_work)
{

        int temp = INVALID_TEMP;
        int volt;
        struct temp_trig_data *temp_data = container_of(_work,struct temp_trig_data, work.work);

        volt = regulator_get_voltage(temp_data->regulator);
        temp = rockchip_tsadc_get_temp(0, volt);
      //  printk("__Temp: temp[%d]\n", temp);
        if (temp > temp_data->trig_temp && temp != INVALID_TEMP)
	    __led_set_brightness(temp_data->led, LED_FULL);
        else
	    __led_set_brightness(temp_data->led, LED_OFF);
        schedule_delayed_work(&temp_data->work, LOOP_SECS);
}

static void temp_trig_activate(struct led_classdev *led_cdev)
{
        struct temp_trig_data *temp_data;
        temp_data = kzalloc(sizeof(*temp_data), GFP_KERNEL);
        if (!temp_data)
                return;
        temp_data->led=led_cdev;
		led_cdev->trigger_data = temp_data;
        temp_data->trig_temp=TRIG_TEMP;
        temp_data->regulator = regulator_get(NULL, "vdd_arm");
        INIT_DELAYED_WORK(&temp_data->work, temp_trig_work);
        led_cdev->activated = true;
		schedule_delayed_work(&temp_data->work, NODELAY_LOOP_SECS);
        return;
}

static void temp_trig_deactivate(struct led_classdev *led_cdev)
{
	struct temp_trig_data *temp_data = led_cdev->trigger_data;
	if (led_cdev->activated) {
		cancel_delayed_work(&temp_data->work);
		kfree(temp_data);
		led_cdev->activated = false;
	}
	led_set_brightness(led_cdev, LED_OFF);
}

static struct led_trigger temp_led_trigger = {
	.name     = "temp",
	.activate = temp_trig_activate,
	.deactivate = temp_trig_deactivate,
};

static int __init temp_trig_init(void)
{
	return led_trigger_register(&temp_led_trigger);
}

static void __exit temp_trig_exit(void)
{
	led_trigger_unregister(&temp_led_trigger);
}

module_init(temp_trig_init);
module_exit(temp_trig_exit);

MODULE_AUTHOR("Terry <terry@sewesion.com>");
MODULE_DESCRIPTION("Temprerature LED trigger");
MODULE_LICENSE("GPL");
