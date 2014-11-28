/*
Copyright (C) 2014 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "pebble.h"

static AppSync sync;
static uint8_t sync_buffer[200];

enum WeatherKey {
  BLUETOOTHVIBE_KEY = 0x0,
  HOURLYVIBE_KEY = 0x1,
  FLIP_KEY = 0x2,
  MASK_KEY = 0x3,
  INVERT_COLOR_KEY = 0x4
};

static int bluetoothvibe;
static int hourlyvibe;
static int flip;
static int mask;
static int invert;

GColor background_color = GColorBlack;

static uint8_t batteryPercent;

static Window *window;
static Layer *window_layer;

static bool appStarted = false;

BitmapLayer *layer_conn_img;
BitmapLayer *layer_conn_img2;
GBitmap *img_bt_connect;
GBitmap *img_bt_disconnect;
GBitmap *img_bt_connect2;
GBitmap *img_bt_disconnect2;

static GBitmap *mask_image;
static BitmapLayer *mask_layer;
static GBitmap *mask_image2;
static BitmapLayer *mask_layer2;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

static GBitmap *day_name_image2;
static BitmapLayer *day_name_layer2;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_MTIME_DIGITS 4
static GBitmap *mtime_digits_images[TOTAL_MTIME_DIGITS];
static BitmapLayer *mtime_digits_layers[TOTAL_MTIME_DIGITS];

#define TOTAL_FLIP_DIGITS 4
static GBitmap *ftime_digits_images[TOTAL_FLIP_DIGITS];
static BitmapLayer *ftime_digits_layers[TOTAL_FLIP_DIGITS];

const int FLIP_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_M0,
  RESOURCE_ID_IMAGE_NUM_M1,
  RESOURCE_ID_IMAGE_NUM_M2,
  RESOURCE_ID_IMAGE_NUM_M3,
  RESOURCE_ID_IMAGE_NUM_M4,
  RESOURCE_ID_IMAGE_NUM_M5,
  RESOURCE_ID_IMAGE_NUM_M6,
  RESOURCE_ID_IMAGE_NUM_M7,
  RESOURCE_ID_IMAGE_NUM_M8,
  RESOURCE_ID_IMAGE_NUM_M9
};

#define TOTAL_DATE_DIGITS 2	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

static GBitmap *date_digits_images2[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers2[TOTAL_DATE_DIGITS];

const int DATE_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};

#define TOTAL_SECONDS_DIGITS 2
static GBitmap *seconds_digits_images[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers[TOTAL_SECONDS_DIGITS];

static GBitmap *seconds_digits_images2[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers2[TOTAL_SECONDS_DIGITS];

static GBitmap *seconds_digits_images24[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers24[TOTAL_SECONDS_DIGITS];

const int SEC_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_S0,
  RESOURCE_ID_IMAGE_NUM_S1,
  RESOURCE_ID_IMAGE_NUM_S2,
  RESOURCE_ID_IMAGE_NUM_S3,
  RESOURCE_ID_IMAGE_NUM_S4,
  RESOURCE_ID_IMAGE_NUM_S5,
  RESOURCE_ID_IMAGE_NUM_S6,
  RESOURCE_ID_IMAGE_NUM_S7,
  RESOURCE_ID_IMAGE_NUM_S8,
  RESOURCE_ID_IMAGE_NUM_S9,
  RESOURCE_ID_IMAGE_TINY_PERCENT
};

#define TOTAL_BATTERY_PERCENT_DIGITS 3
static GBitmap *battery_percent_image[TOTAL_BATTERY_PERCENT_DIGITS];
static BitmapLayer *battery_percent_layers[TOTAL_BATTERY_PERCENT_DIGITS];

static GBitmap *battery_image;
static BitmapLayer *battery_image_layer;
static BitmapLayer *battery_layer;

int charge_percent = 0;
InverterLayer *inverter_layer = NULL;
Layer *line_layer;

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}


static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;
  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
  if (old_image != NULL) {
	gbitmap_destroy(old_image);
	old_image = NULL;
  }
}

void set_invert_color(bool invert) {
  if (invert && inverter_layer == NULL) {
    // Add inverter layer
    Layer *window_layer = window_get_root_layer(window);

    inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
  } else if (!invert && inverter_layer != NULL) {
    // Remove Inverter layer
    layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    inverter_layer_destroy(inverter_layer);
    inverter_layer = NULL;
  }
  // No action required
}

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple* new_tuple,
                                        const Tuple* old_tuple,
                                        void* context) {

  // App Sync keeps new_tuple in sync_buffer, so we may use it directly
  switch (key) {
	  
    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break;      
	  
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break;	    

	case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
	  persist_write_bool(INVERT_COLOR_KEY, invert);
      set_invert_color(invert);
      break;
	 
	case FLIP_KEY:
      flip = new_tuple->value->uint8 != 0;
	  persist_write_bool(FLIP_KEY, flip);
	  
	  if (flip) {
		  
		//layer_set_hidden( bitmap_layer_get_layer(ftime_digits_layers[0]), false);			  
		layer_set_hidden( bitmap_layer_get_layer(ftime_digits_layers[1]), false);
        layer_set_hidden( bitmap_layer_get_layer(ftime_digits_layers[2]), false);
        layer_set_hidden( bitmap_layer_get_layer(ftime_digits_layers[3]), false);
		
		for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(time_digits_layers[i]), true);
			}
		  
		layer_set_hidden( bitmap_layer_get_layer(layer_conn_img), true);
		layer_set_hidden( bitmap_layer_get_layer(layer_conn_img2), false);

	    layer_set_hidden( bitmap_layer_get_layer(day_name_layer), true);
	    layer_set_hidden( bitmap_layer_get_layer(day_name_layer2), false);

        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers2[0]), false);
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers2[1]), false);
		layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[1]), true);
		  
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[0]), false);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[1]), false);
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[1]), true);
		  
	    if (clock_is_24h_style()) {
	  		   for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
     		   layer_set_hidden( bitmap_layer_get_layer(time_digits_layers[i]), true);
			   }
			   for (int i = 0; i < TOTAL_FLIP_DIGITS; ++i) {
         	   layer_set_hidden( bitmap_layer_get_layer(ftime_digits_layers[i]), true);
			   }	
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[1]), true);
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[1]), true);
		  }
	  } else {
		//layer_set_hidden( bitmap_layer_get_layer(time_digits_layers[0]), true);			  
        layer_set_hidden( bitmap_layer_get_layer(time_digits_layers[1]), false);
        layer_set_hidden( bitmap_layer_get_layer(time_digits_layers[2]), false);
        layer_set_hidden( bitmap_layer_get_layer(time_digits_layers[3]), false);
	  	
		for (int i = 0; i < TOTAL_FLIP_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(ftime_digits_layers[i]), true);
			}

		layer_set_hidden( bitmap_layer_get_layer(layer_conn_img), false);
		layer_set_hidden( bitmap_layer_get_layer(layer_conn_img2), true);

		layer_set_hidden( bitmap_layer_get_layer(day_name_layer2), true);
		layer_set_hidden( bitmap_layer_get_layer(day_name_layer), false);

        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers2[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers2[1]), true);		  
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[0]), false);
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[1]), false);
		  
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[1]), true);		  
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[0]), false);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[1]), false);
		  
		    if (clock_is_24h_style()) {
	  		   for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
     		   layer_set_hidden( bitmap_layer_get_layer(time_digits_layers[i]), true);
			   }
			   for (int i = 0; i < TOTAL_FLIP_DIGITS; ++i) {
        	   layer_set_hidden( bitmap_layer_get_layer(ftime_digits_layers[i]), true);
			   }
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers2[1]), true);
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers[1]), true);
		  }	
	  }
      break;
	
	case MASK_KEY:
      mask = new_tuple->value->uint8 !=0;
	  persist_write_bool(MASK_KEY, mask);	
	  if (flip) {
		  layer_set_hidden(bitmap_layer_get_layer(mask_layer2), !mask);
		  layer_set_hidden(bitmap_layer_get_layer(mask_layer), true);
	  } else {
		  layer_set_hidden(bitmap_layer_get_layer(mask_layer), !mask);
		  layer_set_hidden(bitmap_layer_get_layer(mask_layer2), true);
			  }
	  		    if (clock_is_24h_style()) {
				  layer_set_hidden(bitmap_layer_get_layer(mask_layer2), true);
				  layer_set_hidden(bitmap_layer_get_layer(mask_layer), true);

				}
	  
	  if(!mask) {
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
		  
      }
      else {
        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
		layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers24[0]), true);
        layer_set_hidden( bitmap_layer_get_layer(seconds_digits_layers24[1]), true);
      }
	  break;
  }
}

void update_battery(BatteryChargeState charge_state) {

  batteryPercent = charge_state.charge_percent;

  if(batteryPercent>=98) {
	layer_set_hidden(bitmap_layer_get_layer(battery_image_layer), false);
    for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
      layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), true);
    }  
    return;
  }

 for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
	layer_set_hidden(bitmap_layer_get_layer(battery_image_layer), true);
    layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), false);
  }  
  set_container_image(&battery_percent_image[0], battery_percent_layers[0], SEC_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint(112, 133));
  set_container_image(&battery_percent_image[1], battery_percent_layers[1], SEC_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint(119, 133));
  set_container_image(&battery_percent_image[2], battery_percent_layers[2], SEC_IMAGE_RESOURCE_IDS[10], GPoint(126, 133));

//	  if (clock_is_24h_style()) {
 //		  set_container_image(&battery_percent_image[0], battery_percent_layers[0], SEC_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint(112, 133));
	//	  set_container_image(&battery_percent_image[1], battery_percent_layers[1], SEC_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint(119, 133));
  	//	  set_container_image(&battery_percent_image[2], battery_percent_layers[2], SEC_IMAGE_RESOURCE_IDS[10], GPoint(126, 156));
	  //}
}

static void toggle_bluetooth_icon(bool connected) {
  if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
        bitmap_layer_set_bitmap(layer_conn_img2, img_bt_connect2);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
        bitmap_layer_set_bitmap(layer_conn_img2, img_bt_disconnect2);
  }
  if (appStarted && bluetoothvibe) {
      
        vibes_double_pulse();
	}
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon(connected);
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_days(struct tm *tick_time) {
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint(20, 132));
  set_container_image(&day_name_image2, day_name_layer2, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint(20, 119));
	
  set_container_image(&date_digits_images[0], date_digits_layers[0], DATE_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(80, 130));
  set_container_image(&date_digits_images[1], date_digits_layers[1], DATE_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(95, 130));
  set_container_image(&date_digits_images2[0], date_digits_layers2[0], DATE_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(80, 117));
  set_container_image(&date_digits_images2[1], date_digits_layers2[1], DATE_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(95, 117));	
}

static void update_hours(struct tm *tick_time) {

	if (appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
	}
	
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(3, -35));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(3, 16));

  set_container_image(&ftime_digits_images[0], ftime_digits_layers[0], FLIP_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(3, 157));
  set_container_image(&ftime_digits_images[1], ftime_digits_layers[1], FLIP_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(3, 107));
	
	if (display_hour/10 == 0) {
    layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
	if (flip) {
		 	layer_set_hidden(bitmap_layer_get_layer(ftime_digits_layers[0]), true); 
	  }
    } else {
    layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
		if (flip) {
		   layer_set_hidden(bitmap_layer_get_layer(ftime_digits_layers[0]), false);
		}
    }
	
	if (clock_is_24h_style()) {
  
	    set_container_image(&mtime_digits_images[0], mtime_digits_layers[0], DATE_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(20, 31));
        set_container_image(&mtime_digits_images[1], mtime_digits_layers[1], DATE_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(35, 31));

		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[1]), true);
		
		layer_set_hidden(bitmap_layer_get_layer(ftime_digits_layers[0]), true);
		layer_set_hidden(bitmap_layer_get_layer(ftime_digits_layers[1]), true);
	  }
}

static void update_minutes(struct tm *tick_time) {
  set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(3, 70));
  set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(3, 120));
			
  set_container_image(&ftime_digits_images[2], ftime_digits_layers[2], FLIP_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(3, 53));
  set_container_image(&ftime_digits_images[3], ftime_digits_layers[3], FLIP_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(3, 3));

		if (clock_is_24h_style()) {

  		set_container_image(&mtime_digits_images[2], mtime_digits_layers[2], DATE_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(55, 31));
  		set_container_image(&mtime_digits_images[3], mtime_digits_layers[3], DATE_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(70, 31));
		
		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[2]), true);
		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[3]), true);
			
		layer_set_hidden(bitmap_layer_get_layer(ftime_digits_layers[2]), true);
		layer_set_hidden(bitmap_layer_get_layer(ftime_digits_layers[3]), true);
		}
}

static void update_seconds(struct tm *tick_time) {
  set_container_image(&seconds_digits_images[0], seconds_digits_layers[0], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(112, 145));
  set_container_image(&seconds_digits_images[1], seconds_digits_layers[1], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(119, 145));
	
  set_container_image(&seconds_digits_images2[0], seconds_digits_layers2[0], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(112, 120));
  set_container_image(&seconds_digits_images2[1], seconds_digits_layers2[1], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(119, 120));
	
	if (clock_is_24h_style()) {

  		set_container_image(&seconds_digits_images24[0], seconds_digits_layers24[0], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(90, 47));
  		set_container_image(&seconds_digits_images24[1], seconds_digits_layers24[1], SEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(97, 47));

		}
}
	
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	
  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
   update_hours(tick_time);
   update_minutes(tick_time);
  }
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }	
}

void force_update(void) {
    update_battery(battery_state_service_peek());
    toggle_bluetooth_icon(bluetooth_connection_service_peek());
}

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));
  memset(&ftime_digits_layers, 0, sizeof(ftime_digits_layers));
  memset(&ftime_digits_images, 0, sizeof(ftime_digits_images));
  memset(&mtime_digits_layers, 0, sizeof(mtime_digits_layers));
  memset(&mtime_digits_images, 0, sizeof(mtime_digits_images));
	
  memset(&battery_percent_layers, 0, sizeof(battery_percent_layers));
  memset(&battery_percent_image, 0, sizeof(battery_percent_image));
	
  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));
  memset(&date_digits_layers2, 0, sizeof(date_digits_layers2));
  memset(&date_digits_images2, 0, sizeof(date_digits_images2));
	
  memset(&seconds_digits_layers, 0, sizeof(seconds_digits_layers));
  memset(&seconds_digits_images, 0, sizeof(seconds_digits_images));
  memset(&seconds_digits_layers2, 0, sizeof(seconds_digits_layers2));
  memset(&seconds_digits_images2, 0, sizeof(seconds_digits_images2));
	
  // Setup messaging
  const int inbound_size = 200;
  const int outbound_size = 200;
  app_message_open(inbound_size, outbound_size);	
	
  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);
  
  background_color  = GColorBlack;
  window_set_background_color(window, background_color);	
	
     // resources
	img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHON);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHOFF);
	img_bt_connect2     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHON);
    img_bt_disconnect2  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHOFF);
	
    layer_conn_img  = bitmap_layer_create(GRect(0, 62, 144, 9));
    bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);

	layer_conn_img2  = bitmap_layer_create(GRect(0, 98, 144, 9));
    bitmap_layer_set_bitmap(layer_conn_img2, img_bt_connect2);
	
  // time, date, battery layers
  GRect dummy_frame = { {0, 0}, {0, 0} };
	
  for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
	
	for (int i = 0; i < TOTAL_FLIP_DIGITS; ++i) {
    ftime_digits_layers[i] = bitmap_layer_create(dummy_frame);  
    layer_add_child(window_layer, bitmap_layer_get_layer(ftime_digits_layers[i]));
  }
	
	day_name_layer = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));
	
	day_name_layer2 = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer2));

	
    for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers2[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers2[i]));
  }
	

	for (int i = 0; i < TOTAL_MTIME_DIGITS; ++i) {
    mtime_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(mtime_digits_layers[i]));
  }
	
	for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    battery_percent_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(battery_percent_layers[i]));
  }		

	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers[i]));
  }
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers2[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers2[i]));
  }
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers24[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers24[i]));
  }
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img)); 
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img2)); 

  battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  GRect frame4 = (GRect) {
    .origin = { .x = 114, .y = 133 },
    .size = battery_image->bounds.size
  };
  battery_layer = bitmap_layer_create(frame4);
  battery_image_layer = bitmap_layer_create(frame4);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
	
 // seconds mask
   mask_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK);
   GRect framemask = (GRect) {
    .origin = { .x = 112, .y = 145 },
    .size = { .w = 20, .h = 10 }
  }; 

  mask_layer = bitmap_layer_create(framemask);
  layer_add_child(window_layer, bitmap_layer_get_layer(mask_layer));
  bitmap_layer_set_bitmap(mask_layer, mask_image);
  layer_set_hidden(bitmap_layer_get_layer(mask_layer), !mask);
  	
  mask_image2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK);
	 GRect framemask2 = (GRect) {
    .origin = { .x = 112, .y = 121 },
    .size = { .w = 20, .h = 10 }
  };
  mask_layer2 = bitmap_layer_create(framemask2);
  layer_add_child(window_layer, bitmap_layer_get_layer(mask_layer2));
  bitmap_layer_set_bitmap(mask_layer2, mask_image2);
  layer_set_hidden(bitmap_layer_get_layer(mask_layer2), !mask);
  	
  // 24hr line
  if (clock_is_24h_style()) {
  	GRect line_frame = GRect(0, 59, 144, 2);
  	line_layer = layer_create(line_frame);
  	layer_set_update_proc(line_layer, line_layer_update_callback);
  	layer_add_child(window_layer, line_layer);
	}
	
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, DAY_UNIT + MINUTE_UNIT + SECOND_UNIT);

Tuplet initial_values[] = {
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
	TupletInteger(FLIP_KEY, persist_read_bool(FLIP_KEY)),
	TupletInteger(MASK_KEY, persist_read_bool(MASK_KEY)),
    TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                NULL, NULL);

  appStarted = true;
 
	 // handlers
    battery_state_service_subscribe(&update_battery);
    bluetooth_connection_service_subscribe(&bluetooth_connection_callback);
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

	 // draw first frame
    force_update();
}

static void deinit(void) {
  app_sync_deinit(&sync);

  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();

  layer_remove_from_parent(bitmap_layer_get_layer(mask_layer));
  bitmap_layer_destroy(mask_layer);
  gbitmap_destroy(mask_image);
  mask_image = NULL;

  layer_remove_from_parent(bitmap_layer_get_layer(mask_layer2));
  bitmap_layer_destroy(mask_layer2);
  gbitmap_destroy(mask_image2);
  mask_image2 = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_conn_img));
  bitmap_layer_destroy(layer_conn_img);
  gbitmap_destroy(img_bt_connect);
  gbitmap_destroy(img_bt_disconnect);
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_conn_img2));
  bitmap_layer_destroy(layer_conn_img2);
  gbitmap_destroy(img_bt_connect2);
  gbitmap_destroy(img_bt_disconnect2);
	
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
  day_name_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer2));
  bitmap_layer_destroy(day_name_layer2);
  gbitmap_destroy(day_name_image2);
  day_name_image2 = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
  bitmap_layer_destroy(battery_layer);
  gbitmap_destroy(battery_image);
  
  layer_remove_from_parent(bitmap_layer_get_layer(battery_image_layer));
  bitmap_layer_destroy(battery_image_layer);

	for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    time_digits_images[i] = NULL;
    bitmap_layer_destroy(time_digits_layers[i]);
	time_digits_layers[i] = NULL;
  }

	for (int i = 0; i < TOTAL_FLIP_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(ftime_digits_layers[i]));
    gbitmap_destroy(ftime_digits_images[i]);
    ftime_digits_images[i] = NULL;
    bitmap_layer_destroy(ftime_digits_layers[i]);
	ftime_digits_layers[i] = NULL;
  }

	for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers[i]));
    gbitmap_destroy(seconds_digits_images[i]);
    seconds_digits_images[i] = NULL;
    bitmap_layer_destroy(seconds_digits_layers[i]);
	seconds_digits_layers[i] = NULL;  
  }
	
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers2[i]));
    gbitmap_destroy(seconds_digits_images2[i]);
    seconds_digits_images2[i] = NULL;
    bitmap_layer_destroy(seconds_digits_layers2[i]);
	seconds_digits_layers2[i] = NULL;  
  }

	for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(battery_percent_layers[i]));
    gbitmap_destroy(battery_percent_image[i]);
    battery_percent_image[i] = NULL;
    bitmap_layer_destroy(battery_percent_layers[i]);
	battery_percent_layers[i] = NULL;
  }
		
	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    date_digits_images[i] = NULL;
    bitmap_layer_destroy(date_digits_layers[i]);
	date_digits_layers[i] = NULL;
  }
	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers2[i]));
    gbitmap_destroy(date_digits_images2[i]);
    date_digits_images2[i] = NULL;
    bitmap_layer_destroy(date_digits_layers2[i]);
	date_digits_layers2[i] = NULL;
  }
		for (int i = 0; i < TOTAL_MTIME_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(mtime_digits_layers[i]));
    gbitmap_destroy(mtime_digits_images[i]);
    mtime_digits_images[i] = NULL;
    bitmap_layer_destroy(mtime_digits_layers[i]);
	mtime_digits_layers[i] = NULL;
  }
	
  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);
	
  //window_destroy(window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}