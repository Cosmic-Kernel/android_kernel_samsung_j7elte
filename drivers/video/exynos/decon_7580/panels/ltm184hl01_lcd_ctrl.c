/*
 * drivers/video/decon_7580/panels/ltm184hl01_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/lcd.h>
#include <linux/backlight.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <video/mipi_display.h>
#include "../dsim.h"
#include "dsim_panel.h"

#include "ltm184hl01_param.h"

int	backlight_on;
int board_rev;
int lvds_disp_on_comp;

struct i2c_client *lvds_client;
struct i2c_client *backlight1_client;
struct i2c_client *backlight2_client;

#define V5D3BX_VEESTRENGHT		0x00001f07
#define V5D3BX_VEEDEFAULTVAL		0
#define V5D3BX_DEFAULT_STRENGHT		5
#define V5D3BX_DEFAULT_LOW_STRENGHT	8
#define V5D3BX_DEFAULT_HIGH_STRENGHT	10
#define V5D3BX_MAX_STRENGHT		15

#define V5D3BX_CABCBRIGHTNESSRATIO	815
#define V5D3BX_180HZ_DEFAULT_RATIO	180957 /*0x2C2DD:100% duty*/
#define AUTOBRIGHTNESS_LIMIT_VALUE	207

#define MAX_BRIGHTNESS_LEVEL		255
#define MID_BRIGHTNESS_LEVEL		112
#define DIM_BRIGHTNESS_LEVEL		12
#define LOW_BRIGHTNESS_LEVEL		3
#define MIN_BRIGHTNESS				3

#define V5D3BX_MAX_BRIGHTNESS_LEVEL		255
#define V5D3BX_MID_BRIGHTNESS_LEVEL		112
#define V5D3BX_DIM_BRIGHTNESS_LEVEL		12
#define V5D3BX_LOW_BRIGHTNESS_LEVEL		3
#define V5D3BX_MIN_BRIGHTNESS			3

#define BL_DEFAULT_BRIGHTNESS		MID_BRIGHTNESS_LEVEL

/*#define CONFIG_BLIC_TUNING*/
#if defined(CONFIG_BLIC_TUNING)
struct dsim_device *dsim_t;
#endif

static struct LP8558_rom_data LP8558_BOOSTUP[] = {
	{0xA7,	0xF3},
	{0x00,	0xE3},/*Apply BRT setting for outdoor 22.5mA*//*480cd*/
};

static struct LP8558_rom_data LP8558_NORMAL[] = {
	{0xA7,	0xF3},
	{0x00,	0xCB},/*Apply BRT setting for normal for 21.5mA*//*420cd*/
};

struct lcd_info {
	struct lcd_device *ld;
	struct backlight_device *bd;
	unsigned char id[3];
	unsigned char code[5];
	unsigned char tset[8];
	unsigned char elvss[4];

	int	temperature;
	unsigned int coordinate[2];
	unsigned char date[7];
	unsigned int state;
	unsigned int brightness;
	unsigned int br_index;
	unsigned int acl_enable;
	unsigned int current_acl;
	unsigned int current_hbm;
	unsigned int siop_enable;

	unsigned char **hbm_tbl;
	unsigned char **acl_cutoff_tbl;
	unsigned char **acl_opr_tbl;
	struct mutex lock;
	struct dsim_device *dsim;
};


static void ltm184hl01_lvds_pwm_set(struct lcd_info *lcd);
static int dsim_panel_set_brightness(struct lcd_info *lcd, int force)
{
	int ret = 0;

	ltm184hl01_lvds_pwm_set(lcd);

	return ret;
}

static int panel_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int panel_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	struct lcd_info *lcd = bl_get_data(bd);

	ret = dsim_panel_set_brightness(lcd, 0);
	if (ret) {
		dev_err(&lcd->ld->dev, "%s: failed to set brightness\n", __func__);
		goto exit_set;
	}
exit_set:
	return ret;

}

static const struct backlight_ops panel_backlight_ops = {
	.get_brightness = panel_get_brightness,
	.update_status = panel_set_brightness,
};


static int ltm184hl01_exit(struct lcd_info *lcd)
{
	unsigned int reg, value;
	int ret;
	struct panel_private *panel = &dsim_t->priv;

	ret = sscanf(buf, "%x %x", &reg, &value);

	LP8558_eprom_drv_arr_normal[0].addr = reg;
	LP8558_eprom_drv_arr_normal[0].val = value;

	lvds_disp_on_comp = 0;
	dev_info(&lcd->ld->dev, "[lcd] : %s was called\n", __func__);

	return size;
}

static DEVICE_ATTR(bl_ic_tune, 0664, backlight_i2c_show , backlight_i2c_store);

#endif

static int lp8558_1_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
#if defined(CONFIG_BLIC_TUNING)
	struct class *backlight_class;
	struct device *backlight_dev;
#endif

	dsim_info("[backlight] : %s was called\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[lp8558_1] : need I2C_FUNC_I2C.\n");
		ret = -ENODEV;
		return ret;
	}

	backlight1_client = client;
	backlight_on = of_get_gpio(client->dev.of_node, 0);

	of_property_read_u32(client->dev.of_node, "rev", &board_rev);
	dsim_info("[backlight] : board rev (%d)\n", board_rev);

#if defined(CONFIG_BLIC_TUNING)
	backlight_class = class_create(THIS_MODULE, "bl-dbg");
	backlight_dev = device_create(backlight_class, NULL, 0, NULL,  "ic-tuning");
	if (IS_ERR(backlight_dev))
		dsim_info("[backlight] :Failed to create device(backlight_dev)!\n");
	else {
		if (device_create_file(backlight_dev, &dev_attr_bl_ic_tune) < 0)
			dsim_info("[backlight] :Failed to create device file(%s)!\n", dev_attr_bl_ic_tune.attr.name);
	}
#endif

	return ret;
}

static struct i2c_device_id lp8558_1_id[] = {
	{"lp8558_1", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, lp8558_1_id);

static struct of_device_id lp8558_1_i2c_dt_ids[] = {
	{ .compatible = "lp8558_1" },
	{ }
};

MODULE_DEVICE_TABLE(of, lp8558_1_i2c_dt_ids);

static struct i2c_driver lp8558_1_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "bl_lp8558_1",
		.of_match_table	= of_match_ptr(lp8558_1_i2c_dt_ids),
	},
	.id_table = lp8558_1_id,
	.probe = lp8558_1_probe,
};

static int lp8558_2_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;

	dsim_info("[backlight] : %s was called\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[lp8558_2] : need I2C_FUNC_I2C.\n");
		ret = -ENODEV;
		return ret;
	}

	backlight2_client = client;

	return ret;
}

static int Is_lvds_sysclk_16M(void)
{
	/*
		system_rev 0 ,9 : 19.2Mhz
		system_rev 1~8 ,10~:16.9344Mhz
	*/
	if (board_rev == 0 || board_rev == 9)
		return 0;
	else
		return 1;
}

MODULE_DEVICE_TABLE(i2c, lp8558_2_id);

static struct of_device_id lp8558_2_i2c_dt_ids[] = {
	{ .compatible = "lp8558_2" },
	{ }
};

MODULE_DEVICE_TABLE(of, lp8558_2_i2c_dt_ids);

static struct i2c_driver lp8558_2_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "bl_lp8558_2",
		.of_match_table	= of_match_ptr(lp8558_2_i2c_dt_ids),
	},
	.id_table = lp8558_2_id,
	.probe = lp8558_2_probe,
};

static int ltm184hl01_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	dsim_info("[lcd] : %s was called new 0x%x %d\n", __func__ ,  lcdtype, board_rev);

	panel->dim_data = (void *)NULL;
	panel->lcdConnected = PANEL_CONNECTED;
	panel->panel_type = 0;

	if (lcdtype == 0x0)
		panel->lcdConnected = PANEL_DISCONNEDTED;

	if (panel->lcdConnected == PANEL_DISCONNEDTED) {
		dsim_err("[lcd] : %s lcd was not connected\n", __func__);

		gpio_request_one(backlight_on, GPIOF_OUT_INIT_LOW, "BLIC_ON");
		gpio_free(backlight_on);
		usleep_range(10000, 11000);

		goto probe_exit;
	}

	dsim->priv.id[0] = lcdtype;
	lvds_disp_on_comp = 1; /* in the first boot, lvds was initialized on bootloader already.*/

probe_exit:
	return ret;
}

static int ltm184hl01_lvds_init(struct lcd_info *lcd)
{
	int ret = 0;

	dsi_hs_clk = lcd->dsim->lcd_info.hs_clk;
	/* dsi_hs_clk
	lte :	843M
	wifi : 822M
	*/

	dev_info(&lcd->ld->dev, "[lcd] : %s hs_clk(%d) was called\n", __func__, dsi_hs_clk);

	ql_lvds_WriteReg(0x700, 0x18900040);
	if (Is_lvds_sysclk_16M()) {
		if (dsi_hs_clk == 843)
			ql_lvds_WriteReg(0x704, 0x10238);
		else
			ql_lvds_WriteReg(0x704, 0x1022A);
	} else
		ql_lvds_WriteReg(0x704, 0x101DA);
	ql_lvds_WriteReg(0x70C, 0x00004604);
	ql_lvds_WriteReg(0x710, 0x545100B);
	ql_lvds_WriteReg(0x714, 0x20);
	ql_lvds_WriteReg(0x718, 0x00000102);
	ql_lvds_WriteReg(0x71C, 0xA8002F);
	ql_lvds_WriteReg(0x720, 0x1800);
	ql_lvds_WriteReg(0x154, 0x00000000);
	usleep_range(1000, 1100);
	ql_lvds_WriteReg(0x154, 0x80000000);
	usleep_range(1000, 1100);
	ql_lvds_WriteReg(0x700, 0x18900840);
	ql_lvds_WriteReg(0x70C, 0x5E46);
	ql_lvds_WriteReg(0x718, 0x00000202);

	if (!backlight2_client) {
		ret = i2c_add_driver(&lp8558_2_i2c_driver);
		if (ret)
			pr_err("[backlight ]lp8558_2_i2c_init registration failed. ret= %d\n", ret);
	}

	return ret;
}


struct dsim_panel_ops ltm184hl01_panel_ops = {
	.early_probe = ltm184hl01_early_probe,
	.probe		= ltm184hl01_probe,
	.displayon	= ltm184hl01_displayon,
	.exit		= ltm184hl01_exit,
	.init		= NULL,
	.lvds_init	= ltm184hl01_lvds_init,
	.lvds_pwm_set	= ltm184hl01_lvds_pwm_set,
};

	if (Is_lvds_RGB666_backlight_RT8561())
		ql_lvds_WriteReg(0x608, 0x20F0A);/*20 : delay for skew*/
	else
		ql_lvds_WriteReg(0x608, 0x20F80);/*20 : delay for skew*/

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "SDC_%02X%02X%02X\n", priv->id[0], priv->id[1], priv->id[2]);

	if (Is_lvds_RGB666_backlight_RT8561())
		ql_lvds_WriteReg(0x604, 0x3FFFFD08);
	else
		ql_lvds_WriteReg(0x604, 0x3FFFFC00);
	msleep(200);/*after init -3*/

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%x %x %x\n", priv->id[0], priv->id[1], priv->id[2]);

	return strlen(buf);
}

static ssize_t brightness_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *panel = dev_get_drvdata(dev);

#ifdef __RGB_OUT__
	dev_info(&lcd->ld->dev, "[lcd-test]<delay for RGB out> !!! ================================\n");
	msleep(500);
	msleep(500);
	pr_info("[lcd-test]<making RGB out> !!! ===================================\n");
	ql_lvds_WriteReg(0x70C, 0x5E76);
	ql_lvds_WriteReg(0x710, 0x54D004F);
	ql_lvds_WriteReg(0x134, 0x05);
	mdelay(1);
	ql_lvds_WriteReg(0x154, 0x00000000);
	mdelay(1);
	ql_lvds_WriteReg(0x154, 0x80000000);
	mdelay(1);
	dev_info(&lcd->ld->dev, "[lcd-test]<ending RGB out> !!! ===================================\n");
#endif

	dev_info(&lcd->ld->dev, "[lcd] : %s was called--\n", __func__);
	return ret;

	return strlen(buf);
}

static int ltm184hl01_displayon(struct lcd_info *lcd)
{
	int ret = 0;
	struct panel_private *priv = &lcd->dsim->priv;

	dev_info(&lcd->ld->dev, "[lcd] : %s was called\n", __func__);

	if (!priv->lcdConnected)
		return ret;

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (priv->auto_brightness != value) {
			dev_info(dev, "%s: %d, %d\n", __func__, priv->auto_brightness, value);
			mutex_lock(&priv->lock);
			priv->auto_brightness = value;
			mutex_unlock(&priv->lock);
			dsim_panel_set_brightness(dsim, 0);
		}
	}
	return size;
}

static ssize_t siop_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", priv->siop_enable);

	dev_info(&lcd->ld->dev, "[lcd] : %s was called--\n", __func__);
	return ret;
}

static ssize_t siop_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (priv->siop_enable != value) {
			dev_info(dev, "%s: %d, %d\n", __func__, priv->siop_enable, value);
			mutex_lock(&priv->lock);
			priv->siop_enable = value;
			mutex_unlock(&priv->lock);

			dsim_panel_set_brightness(dsim, 1);
		}
	}
	return size;
}

static ssize_t power_reduce_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", priv->acl_enable);

	return strlen(buf);
}

static void ltm184hl01_lvds_pwm_set(struct lcd_info *lcd)
{

	dsim = container_of(priv, struct dsim_device, priv);
	rc = kstrtoul(buf, (unsigned int)0, (unsigned long *)&value);

	if (!lvds_disp_on_comp) {
		dev_info(&lcd->ld->dev, "[lcd] %s lvds_disp_on_comp needed\n", __func__);
		return;
	}
	return size;
}

	dev_info(&lcd->ld->dev, "[lcd] : %s was called- bl level(%d)\n", __func__, lcd->bd->props.brightness);

	level = lcd->bd->props.brightness;

	if (LEVEL_IS_HBM(level)) {
		lp8558_array_write(backlight1_client, LP8558_BOOSTUP, ARRAY_SIZE(LP8558_BOOSTUP));
		lp8558_array_write(backlight2_client, LP8558_BOOSTUP, ARRAY_SIZE(LP8558_BOOSTUP));
	} else {
		lp8558_array_write(backlight1_client, LP8558_NORMAL, ARRAY_SIZE(LP8558_NORMAL));
		lp8558_array_write(backlight2_client, LP8558_NORMAL, ARRAY_SIZE(LP8558_NORMAL));
	}

static ssize_t temperature_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;

	dsim = container_of(priv, struct dsim_device, priv);

	bl_reg = (vx5b3d_level*V5D3BX_180HZ_DEFAULT_RATIO)/MAX_BRIGHTNESS_LEVEL;
	ql_lvds_WriteReg(0x164, bl_reg);

	dev_info(&lcd->ld->dev, "[lcd]: brightness level(%d) vx5b3d_level(%d) bl_reg(%d)\n", level, vx5b3d_level, bl_reg);
}

		dsim_panel_set_brightness(dsim, 1);
		dev_info(dev, "%s: %d, %d\n", __func__, value, priv->temperature);
	}

	return size;
}
#ifdef _BRIGHTNESS_BOOSTUP_
static ssize_t weakness_hbm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%d\n", priv->weakness_hbm_comp);

	return strlen(buf);
}

static ssize_t weakness_hbm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int reg, value;
	int ret;
	struct panel_private *priv = &dsim_t->priv;

	ret = device_create_file(&dsim->priv.bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&dsim->priv.bd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_siop_enable);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ltm184hl01_lvds_pwm_set(lcd_t);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_temperature);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

#ifdef _BRIGHTNESS_BOOSTUP_
	ret = device_create_file(&dsim->priv.bd->dev, &dev_attr_weakness_hbm_comp);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif
}



static int dsim_panel_early_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	panel->ops = dsim_panel_get_priv_ops(dsim);

	if (panel->ops->early_probe)
		ret = panel->ops->early_probe(dsim);
	return ret;
}

static int dsim_panel_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	dsim->lcd = lcd_device_register("panel", dsim->dev, &dsim->priv, NULL);
	if (IS_ERR(dsim->lcd)) {
		dsim_err("%s : faield to register lcd device\n", __func__);
		ret = PTR_ERR(dsim->lcd);
		goto probe_err;
	}
	ret = dsim_backlight_probe(dsim);
	if (ret) {
		dsim_err("%s : failed to prbe backlight driver\n", __func__);
		goto probe_err;
	}

	panel->lcdConnected = PANEL_CONNECTED;
	panel->state = PANEL_STATE_RESUMED;
	panel->temperature = NORMAL_TEMPERATURE;
	panel->acl_enable = 0;
	panel->current_acl = 0;
	panel->auto_brightness = 0;
	panel->siop_enable = 0;
	panel->current_hbm = 0;
	panel->current_vint = 0;
	mutex_init(&panel->lock);

	if (panel->ops->probe) {
		ret = panel->ops->probe(dsim);
		if (ret) {
			dsim_err("%s : failed to probe panel\n", __func__);
			goto probe_err;
		}
	}

#if defined(CONFIG_EXYNOS_DECON_LCD_SYSFS)
	lcd_init_sysfs(dsim);
#endif
	dsim_info("%s probe done\n", __FILE__);
probe_err:
	return ret;
}

static int dsim_panel_lvds_init(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	dsim_info("%s : lvds init, panel state (%d)\n", __func__, panel->state);

	if (panel->state == PANEL_STATE_SUSPENED) {
		panel->state = PANEL_STATE_RESUMED;
		if (panel->ops->lvds_init) {
			ret = panel->ops->lvds_init(dsim);
			if (ret) {
				dsim_err("%s : failed to lvds init\n", __func__);
				panel->state = PANEL_STATE_SUSPENED;
				goto lvds_init_err;
			}
		}
	}

lvds_init_err:

	return ret;
}

static int dsim_panel_displayon(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *priv = &dsim->priv;
	struct lcd_info *lcd = dsim->priv.par;

	dev_info(&lcd->ld->dev, "[lcd] : %s was called new 0x%x %d\n", __func__ ,  lcdtype, board_rev);

	priv->lcdConnected = 1;
	lcd->bd->props.max_brightness = EXTEND_BRIGHTNESS;
	lcd->bd->props.brightness = UI_DEFAULT_BRIGHTNESS;
	lcd->dsim = dsim;
	lcd->state = PANEL_STATE_RESUMED;
	lcd->acl_enable = 0;
	lcd->current_acl = 0;
	lcd->siop_enable = 0;
	lcd->current_hbm = 0;

	if (lcdtype == 0x0)
		priv->lcdConnected = 0;

	if (!priv->lcdConnected) {
		dev_err(&lcd->ld->dev, "[lcd] : %s lcd was not connected\n", __func__);

	dsim_panel_set_brightness(dsim, 1);

	if (panel->ops->displayon) {
		ret = panel->ops->displayon(dsim);
		if (ret) {
			dsim_err("%s : failed to panel display on\n", __func__);
			goto displayon_err;
		}
	}

	lcd->id[0] = lcdtype;
	lvds_disp_on_comp = 1; /* in the first boot, lvds was initialized on bootloader already.*/

probe_exit:
	return ret;
}

static int dsim_panel_suspend(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	if (panel->state == PANEL_STATE_SUSPENED)
		goto suspend_err;

	panel->state = PANEL_STATE_SUSPENDING;

	if (panel->ops->exit) {
		ret = panel->ops->exit(dsim);
		if (ret) {
			dsim_err("%s : failed to panel exit\n", __func__);
			goto suspend_err;
		}
	}
	panel->state = PANEL_STATE_SUSPENED;

suspend_err:
	return ret;
}

static int dsim_panel_resume(struct dsim_device *dsim)
{
	int ret = 0;
	return ret;
}


static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	sprintf(buf, "SDC_%02X%02X%02X\n", lcd->id[0], lcd->id[1], lcd->id[2]);

	return strlen(buf);
}

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	sprintf(buf, "%x %x %x\n", lcd->id[0], lcd->id[1], lcd->id[2]);

	return strlen(buf);
}

static ssize_t siop_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", lcd->siop_enable);

	return strlen(buf);
}

static ssize_t siop_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = kstrtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->siop_enable != value) {
			dev_info(dev, "%s: %d, %d\n", __func__, lcd->siop_enable, value);
			mutex_lock(&lcd->lock);
			lcd->siop_enable = value;
			mutex_unlock(&lcd->lock);

			dsim_panel_set_brightness(lcd, 1);
		}
	}
	return size;
}

static ssize_t power_reduce_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", lcd->acl_enable);

	return strlen(buf);
}

static ssize_t power_reduce_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = kstrtoul(buf, (unsigned int)0, (unsigned long *)&value);

	if (rc < 0)
		return rc;
	else {
		if (lcd->acl_enable != value) {
			dev_info(dev, "%s: %d, %d\n", __func__, lcd->acl_enable, value);
			mutex_lock(&lcd->lock);
			lcd->acl_enable = value;
			mutex_unlock(&lcd->lock);
			dsim_panel_set_brightness(lcd, 1);
		}
	}
	return size;
}


static ssize_t temperature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[] = "-20, -19, 0, 1\n";

	strcat(buf, temp);
	return strlen(buf);
}

static ssize_t temperature_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	rc = kstrtoint(buf, 10, &value);

	if (rc < 0)
		return rc;
	else {
		mutex_lock(&lcd->lock);
		lcd->temperature = value;
		mutex_unlock(&lcd->lock);

		dsim_panel_set_brightness(lcd, 1);
		dev_info(dev, "%s: %d, %d\n", __func__, value, lcd->temperature);
	}

	return size;
}

static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);
static DEVICE_ATTR(window_type, 0444, window_type_show, NULL);
static DEVICE_ATTR(siop_enable, 0664, siop_enable_show, siop_enable_store);
static DEVICE_ATTR(power_reduce, 0664, power_reduce_show, power_reduce_store);
static DEVICE_ATTR(temperature, 0664, temperature_show, temperature_store);

static struct attribute *lcd_sysfs_attributes[] = {
	&dev_attr_lcd_type.attr,
	&dev_attr_window_type.attr,
	&dev_attr_siop_enable.attr,
	&dev_attr_power_reduce.attr,
	&dev_attr_temperature.attr,
	NULL,
};

static const struct attribute_group lcd_sysfs_attr_group = {
	.attrs = lcd_sysfs_attributes,
};

static void lcd_init_sysfs(struct lcd_info *lcd)
{
	int ret = 0;

	ret = sysfs_create_group(&lcd->ld->dev.kobj, &lcd_sysfs_attr_group);
	if (ret < 0)
		dev_err(&lcd->ld->dev, "failed to add lcd sysfs\n");
}


static int dsim_panel_early_probe(struct dsim_device *dsim)
{
	int ret = 0;

	ret = ltm184hl01_early_probe(dsim);

	return ret;
}

static int dsim_panel_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct lcd_info *lcd;

	dsim->priv.par = lcd = kzalloc(sizeof(struct lcd_info), GFP_KERNEL);
	if (!lcd) {
		pr_err("%s: failed to allocate for lcd\n", __func__);
		ret = -ENOMEM;
		goto probe_err;
	}

	dsim->lcd = lcd->ld = lcd_device_register("panel", dsim->dev, lcd, NULL);
	if (IS_ERR(lcd->ld)) {
		pr_err("%s: failed to register lcd device\n", __func__);
		ret = PTR_ERR(lcd->ld);
		goto probe_err;
	}

	lcd->bd = backlight_device_register("panel", dsim->dev, lcd, &panel_backlight_ops, NULL);
	if (IS_ERR(lcd->bd)) {
		pr_err("%s: failed to register backlight device\n", __func__);
		ret = PTR_ERR(lcd->bd);
		goto probe_err;
	}

	mutex_init(&lcd->lock);

	ret = ltm184hl01_probe(dsim);
	if (ret) {
		dev_err(&lcd->ld->dev, "%s: failed to probe panel\n", __func__);
		goto probe_err;
	}

#if defined(CONFIG_EXYNOS_DECON_LCD_SYSFS)
	lcd_init_sysfs(lcd);
#endif
	dev_info(&lcd->ld->dev, "%s: %s: done\n", kbasename(__FILE__), __func__);
probe_err:
	return ret;
}

static int dsim_panel_lvds_init(struct dsim_device *dsim)
{
	struct lcd_info *lcd = dsim->priv.par;
	int ret = 0;

	dev_info(&lcd->ld->dev, "%s: lvds init, panel state (%d)\n", __func__, lcd->state);

	if (lcd->state == PANEL_STATE_SUSPENED) {
		lcd->state = PANEL_STATE_RESUMED;
		ret = ltm184hl01_lvds_init(lcd);
		if (ret) {
			dev_err(&lcd->ld->dev, "%s: failed to lvds init\n", __func__);
			lcd->state = PANEL_STATE_SUSPENED;
			goto lvds_init_err;
		}
	}

lvds_init_err:

	return ret;
}

static int dsim_panel_displayon(struct dsim_device *dsim)
{
	struct lcd_info *lcd = dsim->priv.par;
	int ret = 0;

	if (lcd->state == PANEL_STATE_SUSPENED)
		lcd->state = PANEL_STATE_RESUMED;

	dsim_panel_set_brightness(lcd, 1);

	ret = ltm184hl01_displayon(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "%s: failed to panel display on\n", __func__);
		goto displayon_err;
	}

displayon_err:
	return ret;
}

static int dsim_panel_suspend(struct dsim_device *dsim)
{
	struct lcd_info *lcd = dsim->priv.par;
	int ret = 0;

	if (lcd->state == PANEL_STATE_SUSPENED)
		goto suspend_err;

	lcd->state = PANEL_STATE_SUSPENDING;

	ret = ltm184hl01_exit(lcd);
	if (ret) {
		dev_err(&lcd->ld->dev, "%s: failed to panel exit\n", __func__);
		goto suspend_err;
	}

	lcd->state = PANEL_STATE_SUSPENED;

suspend_err:
	return ret;
}

struct mipi_dsim_lcd_driver ltm184hl01_mipi_lcd_driver = {
	.early_probe	= dsim_panel_early_probe,
	.probe		= dsim_panel_probe,
	.displayon	= dsim_panel_displayon,
	.suspend	= dsim_panel_suspend,
	.display_lvds_init = dsim_panel_lvds_init,
};
