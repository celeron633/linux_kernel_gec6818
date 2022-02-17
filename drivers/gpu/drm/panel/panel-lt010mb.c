/*
 * MIPI-DSI based lt101mb  LCD panel driver.
 *
 * Copyright (c) 2015 Nexell Co., Ltd
 *
 * Youngbok Park <ybpark@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>
#include <linux/backlight.h>

#define LT101MB_WIDTH_MM		217
#define LT101MB_HEIGHT_MM		136

struct lt101mb {
	struct device *dev;
	struct drm_panel panel;

	struct regulator_bulk_data supplies[2];
	int reset_gpio;
	u32 power_on_delay;
	u32 reset_delay;
	u32 init_delay;
	bool flip_horizontal;
	bool flip_vertical;
	struct videomode vm;
	u32 width_mm;
	u32 height_mm;
	bool is_power_on;

	struct backlight_device *bl_dev;
	u8 id[3];
	/* This field is tested by functions directly accessing DSI bus before
	 * transfer, transfer is skipped if it is set. In case of transfer
	 * failure or unexpected response the field is set to error value.
	 * Such construct allows to eliminate many checks in higher level
	 * functions.
	 */
	int error;
};

static inline struct lt101mb *panel_to_lt101mb(struct drm_panel *panel)
{
	return container_of(panel, struct lt101mb, panel);
}

static int lt101mb_clear_error(struct lt101mb *ctx)
{
	int ret = ctx->error;

	ctx->error = 0;
	return ret;
}

static void lt101mb_dcs_write(struct lt101mb *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return;

	ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %zd writing dcs seq: %*ph\n", ret,
			(int)len, data);
		ctx->error = ret;
	}
}

#define lt101mb_dcs_write_seq(ctx, seq...) \
({\
	const u8 d[] = { seq };\
	BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64, "DCS sequence too big for stack");\
	lt101mb_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

#define lt101mb_dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	lt101mb_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static void lt101mb_set_sequence(struct lt101mb *ctx)
{
	if (ctx->error != 0)
		return;

	usleep_range(17000, 18000);
/*	
    //   printf("Start of DSC Mipi Commands to Panel\n");
	lt101mb_dcs_write_seq_static(ctx, 0xFF, 0x98, 0x81, 0x03); 
	// printf("Finished sending first DSC Command to Panel\n");
		mdelay(11);
     //   printf("Finished first command 11ms delay\n");
	 lt101mb_dcs_write_seq_static(ctx, 0x01, 0x00);  
		mdelay(11);	
	
	 lt101mb_dcs_write_seq_static(ctx, 0x02, 0x00);  
		mdelay(11);	
	lt101mb_dcs_write_seq_static(ctx, 0x03, 0x72);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x04, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x05, 0x00);  
		mdelay(11);	
	
	 lt101mb_dcs_write_seq_static(ctx, 0x06, 0x09);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x07, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x08, 0x00);  
		mdelay(11);	
	
	
	 lt101mb_dcs_write_seq_static(ctx, 0x09, 0x01);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x0a, 0x00);  
		mdelay(11);
	// printf("Finished sending Tenth DSC Command to Panel\n");
	 lt101mb_dcs_write_seq_static(ctx, 0x0b, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x0c, 0x01);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x0d, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x0e, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x0f, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x10, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x11, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x12, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x13, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x14, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x15, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x16, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x17, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x18, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x19, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x1a, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x1b, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x1c, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x1d, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x1e, 0x40);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x1f, 0x80);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x20, 0x05);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x21, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x22, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x23, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x24, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x25, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x26, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x27, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x28, 0x33);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x29, 0x02);  
		mdelay(11);	
	
	 lt101mb_dcs_write_seq_static(ctx, 0x2a, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x2b, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x2c, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x2d, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x2e, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x2f, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x30, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x31, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x32, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x33, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x34, 0x04);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x35, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x36, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x37, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x38, 0x3c);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x39, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x3a, 0x40);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x3b, 0x40);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x3c, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x3d, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x3e, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x3f, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x40, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x41, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x42, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x43, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x44, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x50, 0x01);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x51, 0x23);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x52, 0x45);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x53, 0x67);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x54, 0x89);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x55, 0xab);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x56, 0x01);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x57, 0x23);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x58, 0x45);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x59, 0x67);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x5a, 0x89);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x5b, 0xab);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x5c, 0xcd);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x5d, 0xef);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x5e, 0x11);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x5f, 0x01);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x60, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x61, 0x15);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x62, 0x14);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x63, 0x0e);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x64, 0x0f);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x65, 0x0c);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x66, 0x0d);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x67, 0x06);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x68, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x69, 0x07);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6a, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6b, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6c, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6d, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6e, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6f, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x70, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x71, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x72, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x73, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x74, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x75, 0x01);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x76, 0x00);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x77, 0x14);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x78, 0x15);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x79, 0x0e);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x7a, 0x0f);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x7b, 0x0c);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x7c, 0x0d);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x7d, 0x06);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x7e, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x7f, 0x07);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x80, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x81, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x82, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x83, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x84, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x85, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x86, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x87, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x88, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x89, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x8a, 0x02);  
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0xFF, 0x98, 0x81, 0x04);  // Set to Page 4 Command
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6c, 0x15);  // Set VCORE Setting to 0x15
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6e, 0x2a);  // Set Power Control2 to 0x2A
		mdelay(11);	
	 lt101mb_dcs_write_seq_static(ctx, 0x6f, 0x34);  // Set Power Control3 to 0x34
		mdelay(11);            
	
 	 lt101mb_dcs_write_seq_static(ctx, 0x3a, 0x94);   // Command 0x3A not in 0.92 spec version
		mdelay(11);      
  	 lt101mb_dcs_write_seq_static(ctx, 0x8d, 0x14);    // Set Power Control4 to 0x14
		mdelay(11);   
  	 lt101mb_dcs_write_seq_static(ctx, 0x87, 0xba);    // Command 0x87 not in 0.92 spec version
		mdelay(11); 	
  	 lt101mb_dcs_write_seq_static(ctx, 0x26, 0x76);    // Command 0x26 not in 0.92 spec version
		mdelay(11);       
  	 lt101mb_dcs_write_seq_static(ctx, 0xb2, 0xd1);    // Command 0xB2 not in 0.92 spec version
		mdelay(11);        
  	 lt101mb_dcs_write_seq_static(ctx, 0xb5, 0x06);    // Command 0xB5 not in 0.92 spec version
		mdelay(11);    
  	 lt101mb_dcs_write_seq_static(ctx, 0xFF, 0x98, 0x81, 0x01);  // Set to Page 4 command
		mdelay(11);        
  	 lt101mb_dcs_write_seq_static(ctx, 0x22, 0x09);    // Set Panel Operation Mode to 0x0B
		mdelay(11); 
  	 lt101mb_dcs_write_seq_static(ctx, 0x31, 0x00);    // Set Display Inversion to 0x00
		mdelay(11); 
  	 lt101mb_dcs_write_seq_static(ctx, 0x53, 0x90);    // Set VCOM Control1-2 to 0x90
		mdelay(11); 	
   	 lt101mb_dcs_write_seq_static(ctx, 0x55, 0xa2);    // Set VCOM Control1-4 to 0xA2
		mdelay(11);        
   	 lt101mb_dcs_write_seq_static(ctx, 0x50, 0xb7);    // Set Power Control1-1 to 0xB7
		mdelay(11);
   	 lt101mb_dcs_write_seq_static(ctx, 0x51, 0xb7);    // Set Power Control1-2 to 0xB7
		mdelay(11);	
   	 lt101mb_dcs_write_seq_static(ctx, 0x60, 0x22);    // Set Source Timing Adjust-1 to 0x22
		mdelay(11);	
    	 lt101mb_dcs_write_seq_static(ctx, 0x61, 0x00);    // Set Source Timing Adjust-2 to 0x00
		mdelay(11);    
    	 lt101mb_dcs_write_seq_static(ctx, 0x62, 0x19);    // Set Source Timing Adjust-3 to 0x19
		mdelay(11);       
    	 lt101mb_dcs_write_seq_static(ctx, 0x63, 0x10);    // Set Source Timing Adjust-4 to 0x10
		mdelay(11);     
      	 lt101mb_dcs_write_seq_static(ctx, 0xa0, 0x08);    // Set Positive Gamma Correction-1 to 0x08
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xa1, 0x1a);    // Set Positive Gamma Correction-2 to 0x1A
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xa2, 0x27);    // Set Positive Gamma Correction-3 to 0x27
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xa3, 0x15);    // Set Positive Gamma Correction-4 to 0x15
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xa4, 0x17);     // Set Positive Gamma Correction-5 to 0x17
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xa5, 0x2a);      // Set Positive Gamma Correction-6 to 0x2A
		mdelay(11); 	
      	 lt101mb_dcs_write_seq_static(ctx, 0xa6, 0x1e);    // Set Positive Gamma Correction-7 to 0x1E
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xa7, 0x1f);     // Set Positive Gamma Correction-8 to 0x1F
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xa8, 0x8b);    // Set Positive Gamma Correction-9 to 0x8B
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xa9, 0x1b);    // Set Positive Gamma Correction-10 to 0x1B
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xaa, 0x27);      // Set Positive Gamma Correction-11 to 0x27
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xab, 0x78);     // Set Positive Gamma Correction-12 to 0x78
		mdelay(11); 	
       	 lt101mb_dcs_write_seq_static(ctx, 0xac, 0x18);     // Set Positive Gamma Correction-13 to 0xac
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xad, 0x18);      // Set Positive Gamma Correction-14 to 0x2A
		mdelay(11); 	
      	 lt101mb_dcs_write_seq_static(ctx, 0xae, 0x4c);    // Set Positive Gamma Correction-15 to 0x08
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xaf, 0x21);     // Set Positive Gamma Correction-16 to 0x1a
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xb0, 0x27);    // Set Positive Gamma Correction-17 to 0x27
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xb1, 0x54);    // Set Positive Gamma Correction-18 to 0x15
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xb2, 0x67);      // Set Positive Gamma Correction-19 to 0x17
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xb3, 0x39);     // Set Positive Gamma Correction-20 to 0x2a
		mdelay(11);	
    
  
      	 lt101mb_dcs_write_seq_static(ctx, 0xc0, 0x08);    // Set Negative Gamma Correction-1 to 0x08
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xc1, 0x1a);    // Set Negative Gamma Correction-2 to 0x1A
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xc2, 0x27);    // Set Negative Gamma Correction-3 to 0x27
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xc3, 0x15);    // Set Negative Gamma Correction-4 to 0x15
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xc4, 0x17);     // Set Negative Gamma Correction-5 to 0x17
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xc5, 0x2a);      // Set Negative Gamma Correction-6 to 0x2A
		mdelay(11); 	
      	 lt101mb_dcs_write_seq_static(ctx, 0xc6, 0x1e);    // Set Negative Gamma Correction-7 to 0x1E
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xc7, 0x1f);     // Set Negative Gamma Correction-8 to 0x1F
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xc8, 0x8b);    // Set Negative Gamma Correction-9 to 0x8B
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xc9, 0x1b);    // Set Negative Gamma Correction-10 to 0x1B
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xca, 0x27);      // Set Negative Gamma Correction-11 to 0x27
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xcb, 0x78);     // Set Negative Gamma Correction-12 to 0x78
		mdelay(11); 	
       	 lt101mb_dcs_write_seq_static(ctx, 0xcc, 0x18);     // Set Negative Gamma Correction-13 to 0x18
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xcd, 0x18);      // Set Negative Gamma Correction-14 to 0x18
		mdelay(11); 	
      	 lt101mb_dcs_write_seq_static(ctx, 0xce, 0x4c);    // Set Negative Gamma Correction-15 to 0x4c
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xcf, 0x21);     // Set Negative Gamma Correction-16 to 0x21
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xd0, 0x27);    // Set Negative Gamma Correction-17 to 0x27
		mdelay(11);     
       	 lt101mb_dcs_write_seq_static(ctx, 0xd1, 0x54);    // Set Negative Gamma Correction-18 to 0x54
		mdelay(11);      
       	 lt101mb_dcs_write_seq_static(ctx, 0xd2, 0x67);      // Set Negative Gamma Correction-19 to 0x67
		mdelay(11);    
       	 lt101mb_dcs_write_seq_static(ctx, 0xd3, 0x39);     // Set Negative Gamma Correction-20 to 0x39
		mdelay(11);		
  	 lt101mb_dcs_write_seq_static(ctx, 0xFF, 0x98, 0x81, 0x00);  // Set to Page 0 command
		mdelay(11);  
       	 lt101mb_dcs_write_seq_static(ctx, 0x36, 0x03);     
		mdelay(11);
       
  	lt101mb_dcs_write_seq_static(ctx, 0x35);	 
	mdelay(11);       
		    
	lt101mb_dcs_write_seq_static(ctx, 0x11);	 // Set Sleep Out
	// printf("Finished sending sleep out 0x11 command\n");
	mdelay(120);
	// printf("Finished delay after sending sleep out command \n");
	lt101mb_dcs_write_seq_static(ctx, 0x29);	// Set Display On
	mdelay(120);	
	// printf("Finished sending Display On Command 0x29 command\n");
	*/
}

static int lt101mb_power_on(struct lt101mb *ctx)
{
	int ret;

	if (ctx->is_power_on)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	msleep(ctx->power_on_delay);

	gpio_direction_output(ctx->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpio_set_value(ctx->reset_gpio, 0);
	msleep(100);
	gpio_set_value(ctx->reset_gpio, 1);
	msleep(10);
	msleep(ctx->reset_delay);

	ctx->is_power_on = true;

	return 0;
}

static int lt101mb_power_off(struct lt101mb *ctx)
{
	if (!ctx->is_power_on)
		return 0;

	gpio_set_value(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);

	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	ctx->is_power_on = false;

	return 0;
}

static int lt101mb_disable(struct drm_panel *panel)
{
	struct lt101mb *ctx = panel_to_lt101mb(panel);

	lt101mb_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_OFF);
	if (ctx->error != 0)
		return ctx->error;

	msleep(35);

	lt101mb_dcs_write_seq_static(ctx, MIPI_DCS_ENTER_SLEEP_MODE);
	if (ctx->error != 0)
		return ctx->error;

	msleep(125);
	if (ctx->bl_dev) {
		ctx->bl_dev->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->bl_dev);
	}

	return 0;
}

static int lt101mb_unprepare(struct drm_panel *panel)
{
	struct lt101mb *ctx = panel_to_lt101mb(panel);
	int ret;

	ret = lt101mb_power_off(ctx);
	if (ret)
		return ret;

	lt101mb_clear_error(ctx);

	return 0;
}

static int lt101mb_prepare(struct drm_panel *panel)
{
	struct lt101mb *ctx = panel_to_lt101mb(panel);
	int ret;

	ret = lt101mb_power_on(ctx);
	if (ret < 0)
		return ret;

	lt101mb_set_sequence(ctx);
	ret = ctx->error;

	if (ret < 0)
		lt101mb_unprepare(panel);

	return ret;
}

static int lt101mb_enable(struct drm_panel *panel)
{
	struct lt101mb *ctx = panel_to_lt101mb(panel);

	if (ctx->bl_dev) {
		ctx->bl_dev->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->bl_dev);
	}

	lt101mb_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_ON);
	if (ctx->error != 0)
		return ctx->error;

	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock = 1535985,
	.hdisplay = 1920,
	.hsync_start = 1920 + 466,
	.hsync_end = 1920 + 466 + 11,
	.htotal = 1920 + 466 + 11 + 11,
	.vdisplay = 1200,
	.vsync_start = 1200 + 10,
	.vsync_end = 1200 + 10 + 5,
	.vtotal = 1200 + 10 + 5 + 5,
	.vrefresh = 0,
};

static int lt101mb_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct lt101mb *ctx = panel_to_lt101mb(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode\n");
		return 0;
	}

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	drm_mode_set_name(mode);
	mode->width_mm = ctx->width_mm;
	mode->height_mm = ctx->height_mm;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs lt101mb_drm_funcs = {
	.disable = lt101mb_disable,
	.unprepare = lt101mb_unprepare,
	.prepare = lt101mb_prepare,
	.enable = lt101mb_enable,
	.get_modes = lt101mb_get_modes,
};

static int lt101mb_parse_dt(struct lt101mb *ctx)
{
	struct device *dev = ctx->dev;
	struct device_node *np = dev->of_node;

	of_property_read_u32(np, "power-on-delay", &ctx->power_on_delay);
	of_property_read_u32(np, "reset-delay", &ctx->reset_delay);
	of_property_read_u32(np, "init-delay", &ctx->init_delay);

	ctx->flip_horizontal = of_property_read_bool(np, "flip-horizontal");
	ctx->flip_vertical = of_property_read_bool(np, "flip-vertical");

	return 0;
}

static int lt101mb_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *backlight;
	struct lt101mb *ctx;
	int ret;

	if (!drm_panel_connected("lt101mb"))
		return -ENODEV;

	ctx = devm_kzalloc(dev, sizeof(struct lt101mb), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	ctx->is_power_on = false;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO
		| MIPI_DSI_MODE_VIDEO_HFP | MIPI_DSI_MODE_VIDEO_HBP
		| MIPI_DSI_MODE_VIDEO_HSA | MIPI_DSI_MODE_VSYNC_FLUSH;

	ret = lt101mb_parse_dt(ctx);
	if (ret < 0)
		return ret;

	ctx->supplies[0].supply = "vdd3";
	ctx->supplies[1].supply = "vci";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		dev_warn(dev, "failed to get regulators: %d\n", ret);

	ctx->reset_gpio = of_get_named_gpio(dev->of_node, "reset-gpio", 0);
	if (ctx->reset_gpio < 0) {
		dev_err(dev, "cannot get reset-gpios %d\n",
			ctx->reset_gpio);
		return ctx->reset_gpio;
	}

	ret = devm_gpio_request(dev, ctx->reset_gpio, "reset-gpio");
	if (ret) {
		dev_err(dev, "failed to request reset-gpio\n");
		return ret;
	}

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->bl_dev = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->bl_dev)
			return -EPROBE_DEFER;
	}

	ctx->width_mm = LT101MB_WIDTH_MM;
	ctx->height_mm = LT101MB_HEIGHT_MM;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &lt101mb_drm_funcs;

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0) {
		backlight_device_unregister(ctx->bl_dev);
		return ret;
	}

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		backlight_device_unregister(ctx->bl_dev);
		drm_panel_remove(&ctx->panel);
	}

	return ret;
}

static int lt101mb_remove(struct mipi_dsi_device *dsi)
{
	struct lt101mb *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
	backlight_device_unregister(ctx->bl_dev);
	lt101mb_power_off(ctx);

	return 0;
}

static void lt101mb_shutdown(struct mipi_dsi_device *dsi)
{
	struct lt101mb *ctx = mipi_dsi_get_drvdata(dsi);

	lt101mb_power_off(ctx);
}

static const struct of_device_id lt101mb_of_match[] = {
	{ .compatible = "lt101mb02000" },
	{ }
};
MODULE_DEVICE_TABLE(of, lt101mb_of_match);

static struct mipi_dsi_driver lt101mb_driver = {
	.probe = lt101mb_probe,
	.remove = lt101mb_remove,
	.shutdown = lt101mb_shutdown,
	.driver = {
		.name = "panel-lt101mb02000",
		.of_match_table = lt101mb_of_match,
	},
};
module_mipi_dsi_driver(lt101mb_driver);

MODULE_AUTHOR("Youngbok Park <ybpark@nexell.co.kr>");
MODULE_DESCRIPTION("MIPI-DSI based lt101mb LCD Panel Driver");
MODULE_LICENSE("GPL v2");
