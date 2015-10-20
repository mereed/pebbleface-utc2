#include <pebble.h>

// UI Elements
static Window *s_main_window = NULL;
//static TextLayer *s_localtime_layer = NULL;
static TextLayer *s_localdate_layer = NULL;
static TextLayer *s_timezone_abbr_layer = NULL;
static TextLayer *s_region_layer = NULL;
static TextLayer *s_utctime_layer = NULL;
static TextLayer *s_utctime_layer_nosec = NULL;
static TextLayer *s_utcdate_layer = NULL;
static TextLayer *s_timezone_utc_layer = NULL;
TextLayer *battery_text_layer;
int charge_percent = 0;

TextLayer *time_layer;
TextLayer *time_layer_nosec;
TextLayer *ampm_layer;
TextLayer *ampm_layer2;

static GFont custom_font;
static GFont custom_font2;
static GFont custom_font3;
//static GFont custom_font4;

// Local time, Region and UTC display string buffers
const char *s_timezone_abbr_string = NULL;
static char s_region_string[32] = "";          // Must be 32 characters large for longest option
static char s_localdate_string[16];
static char s_utcdate_string[16];
//static char s_localtime_string[] = "00:00:00   "; // Leave space for 'AM/PM'
static char s_utctime_string[] = "00:00:00";  
static char s_utctime_string_nosec[] = "00:00"; 

// AM-PM or 24 hour clock
static bool s_is_clock_24 = false;

// Days of the week strings
static const char *const s_day_names[7] =
{"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

static GBitmap *background_image;
static BitmapLayer *background_layer;

static uint8_t batteryPercent;

BitmapLayer *layer_conn_img;

static GBitmap *img_bt_connect;
static GBitmap *img_bt_disconnect;

static GBitmap *battery_image;

static BitmapLayer *battery_image_layer;
static BitmapLayer *battery_layer;
	
static bool status_showing = true;
static AppTimer *display_timer;

BitmapLayer *bg_layer;
GBitmap *bg_bitmap;
BitmapLayer *bg_layer1;
GBitmap *bg_bitmap1;
BitmapLayer *bg_layer2;
GBitmap *bg_bitmap2;
BitmapLayer *bg_layer3;
GBitmap *bg_bitmap3;


static void update_battery_state(BatteryChargeState charge_state) {
    static char battery_text[] = "x100%";

  batteryPercent = charge_state.charge_percent;

  if(batteryPercent==100) {
        layer_set_hidden(bitmap_layer_get_layer(battery_layer), false);   
    return;
  }
  layer_set_hidden(bitmap_layer_get_layer(battery_layer), charge_state.is_charging);

    if (charge_state.is_charging) {
        snprintf(battery_text, sizeof(battery_text), "+%d%%", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);       
    } 
    charge_percent = charge_state.charge_percent;   
	text_layer_set_text(battery_text_layer, battery_text);

}    

void handle_bluetooth(bool connected) {
    if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
        vibes_short_pulse();
    }
}

void battery_layer_update_callback(Layer *me, GContext* ctx) {        
  //draw the remaining battery percentage
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 4, ((batteryPercent/100.0)*60.0), 2), 0, GCornerNone);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  time_t current_time = time(NULL);
  struct tm *local_tm = localtime(&current_time);
  struct tm *utc_tm = gmtime(&current_time);
  static char time_buffer[] = "00:00:00";
  static char time_buffer_nosec[] = "00:00";
//  static char ampm_text[] = "xx";
  static char ampm_text2[] = "xx";

  // We don't check units_changed for DAY_UNIT as we are supporting 2 timezones, 
  // and only localtime triggers a DAY_UNIT change.
  
  // Update local date
  snprintf(s_localdate_string, sizeof(s_localdate_string), "%s %d", 
    s_day_names[local_tm->tm_wday], local_tm->tm_mday);
  layer_mark_dirty(text_layer_get_layer(s_localdate_layer));

  // Update local time
if(clock_is_24h_style()){
    strftime(time_buffer, sizeof(time_buffer), "%R:%S", local_tm);
    strftime(time_buffer_nosec, sizeof(time_buffer_nosec), "%R:", local_tm);
  }
  else{
    strftime(time_buffer, sizeof(time_buffer), "%l:%M:%S", local_tm);
    strftime(time_buffer_nosec, sizeof(time_buffer_nosec), "%l:%M", local_tm);
    strftime(ampm_text2, sizeof(ampm_text2), "%p", local_tm);
	text_layer_set_text(ampm_layer2, ampm_text2);
  }
  text_layer_set_text(time_layer, time_buffer);	
  text_layer_set_text(time_layer_nosec, time_buffer_nosec);	
	
  // Update UTC time and timezones only if a timezone has been set
  if (clock_is_timezone_set()) {
    // Show UTC layer if in UTC mode
    layer_set_hidden(text_layer_get_layer(s_timezone_utc_layer), false);
    layer_mark_dirty(text_layer_get_layer(s_timezone_utc_layer));

    // Update UTC date
    snprintf(s_utcdate_string, sizeof(s_utcdate_string), "%s %d", 
        s_day_names[utc_tm->tm_wday], utc_tm->tm_mday);
    layer_mark_dirty(text_layer_get_layer(s_utcdate_layer));

    // Update timezone
#ifdef PBL_PLATFORM_BASALT
    s_timezone_abbr_string = local_tm->tm_zone;
#else
    s_timezone_abbr_string = "NA";
#endif
    text_layer_set_text(s_timezone_abbr_layer, s_timezone_abbr_string);
    layer_mark_dirty(text_layer_get_layer(s_timezone_abbr_layer));

    // Set utc time
	  
	  
   strftime(s_utctime_string_nosec, sizeof(s_utctime_string_nosec), "%R", utc_tm);
   text_layer_set_text(s_utctime_layer_nosec, s_utctime_string_nosec);	
	  
   layer_mark_dirty(text_layer_get_layer(s_utctime_layer_nosec));

	  
   strftime(s_utctime_string, sizeof(s_utctime_string), "%R:%S", utc_tm);
   text_layer_set_text(s_utctime_layer, s_utctime_string);	

   layer_mark_dirty(text_layer_get_layer(s_utctime_layer));
	  
	  
    // Update timezone region
// TODO: This should be a 3.x #ifdef
#ifdef PBL_PLATFORM_BASALT
    clock_get_timezone(s_region_string, TIMEZONE_NAME_LENGTH);
#else
    snprintf(s_region_string, sizeof(s_region_string), "Not supported");
#endif
    layer_set_hidden(text_layer_get_layer(s_region_layer),false);
    layer_mark_dirty(text_layer_get_layer(s_region_layer));
  }
}

/**
 * Convenience function to set up many TextLayers
 */
static void setup_text_layer(TextLayer **text_layer, 
    const char *text_string, char *font, GRect pos, GTextAlignment alignment) {
  *text_layer = text_layer_create(pos);
  text_layer_set_text(*text_layer, text_string);
	text_layer_set_font(*text_layer, fonts_get_system_font(font));
  text_layer_set_text_alignment(*text_layer, alignment);
  text_layer_set_background_color(*text_layer, GColorClear);
}

// Hides seconds
void hide_status() {
	status_showing = false;
	
    layer_set_hidden(bitmap_layer_get_layer(bg_layer),false);	
	layer_set_hidden(bitmap_layer_get_layer(bg_layer1),false);	
    layer_set_hidden(bitmap_layer_get_layer(bg_layer2),true);	
	layer_set_hidden(bitmap_layer_get_layer(bg_layer3),true);	
	
    layer_set_hidden(text_layer_get_layer(s_utctime_layer),true);
    layer_set_hidden(text_layer_get_layer(s_utctime_layer_nosec),false);	
	
    layer_set_hidden(text_layer_get_layer(time_layer),true);
    layer_set_hidden(text_layer_get_layer(time_layer_nosec),false);	

    layer_set_hidden(text_layer_get_layer(ampm_layer2),false);		
	
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

}

// Shows seconds
void show_status() {
	status_showing = true;
	//SHOW SECONDS TIME
	
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
	
    layer_set_hidden(text_layer_get_layer(s_utctime_layer_nosec),true);	
    layer_set_hidden(text_layer_get_layer(s_utctime_layer),false);
		   
	layer_set_hidden(text_layer_get_layer(time_layer_nosec),true);	
    layer_set_hidden(text_layer_get_layer(time_layer),false);
		
	layer_set_hidden(bitmap_layer_get_layer(bg_layer),true);	
	layer_set_hidden(bitmap_layer_get_layer(bg_layer1),true);	
	layer_set_hidden(bitmap_layer_get_layer(bg_layer2),false);	
	layer_set_hidden(bitmap_layer_get_layer(bg_layer3),false);	

	layer_set_hidden(text_layer_get_layer(ampm_layer2),true);	
		
	// 30 Sec timer then call "hide_status". Cancels previous timer if another show_status is called within the 30000ms
	app_timer_cancel(display_timer);
	display_timer = app_timer_register(30000, hide_status, NULL);
}

// Shake/Tap Handler. On shake/tap... call "show_status"
void tap_handler(AccelAxisType axis, int32_t direction) {
	show_status();	
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  background_layer = bitmap_layer_create(layer_get_frame(window_layer));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));	

	
  bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUMBG1);
  bg_layer = bitmap_layer_create(GRect(33, 39, 78, 25));
  bitmap_layer_set_bitmap(bg_layer, bg_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(bg_layer));	
		
	
  bg_bitmap1 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUMBG1);
  bg_layer1 = bitmap_layer_create(GRect(33, 110, 78, 25));
  bitmap_layer_set_bitmap(bg_layer1, bg_bitmap1);
  layer_add_child(window_layer, bitmap_layer_get_layer(bg_layer1));	
	
  bg_bitmap2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUMBG2);
  bg_layer2 = bitmap_layer_create(GRect(12, 110, 118, 25));
  bitmap_layer_set_bitmap(bg_layer2, bg_bitmap2);
  layer_add_child(window_layer, bitmap_layer_get_layer(bg_layer2));	
		
  bg_bitmap3 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUMBG2);
  bg_layer3 = bitmap_layer_create(GRect(12, 39, 118, 25));
  bitmap_layer_set_bitmap(bg_layer3, bg_bitmap3);
  layer_add_child(window_layer, bitmap_layer_get_layer(bg_layer3));	
		
  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_20));
  custom_font2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_14));
  custom_font3 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_12));

   // Time text
  time_layer = text_layer_create(GRect(0, 38, 144, 40));
  text_layer_set_font(time_layer, custom_font);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  time_layer_nosec = text_layer_create(GRect(0, 38, 144, 40));
  text_layer_set_font(time_layer_nosec, custom_font);
  text_layer_set_text_color(time_layer_nosec, GColorBlack);
  text_layer_set_background_color(time_layer_nosec, GColorClear);
  text_layer_set_text_alignment(time_layer_nosec, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer_nosec));
	
  ampm_layer2 = text_layer_create(GRect(113, 49, 50, 40));
  text_layer_set_font(ampm_layer2, custom_font3);
 // text_layer_set_font(ampm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(ampm_layer2, GColorBlack);
  text_layer_set_background_color(ampm_layer2, GColorClear);
  text_layer_set_text_alignment(ampm_layer2, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(ampm_layer2));
	
  // Timezone text
  setup_text_layer(&s_timezone_abbr_layer, 
    s_timezone_abbr_string, "RESOURCE_ID_GOTHIC_14_BOLD", 
    (GRect) {{0, 10}, {144, 28}}, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_timezone_abbr_layer));

	
	// Date text
  setup_text_layer(&s_localdate_layer, 
    s_localdate_string, "RESOURCE_ID_GOTHIC_14", 
    (GRect) {{0, 22}, {144, 18}}, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_localdate_layer));


  // Region text
  setup_text_layer(&s_region_layer, 
    s_region_string, "RESOURCE_ID_GOTHIC_14", 
    (GRect) {{0, 62}, {144, 18 + 4}}, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_region_layer));
  layer_set_hidden(text_layer_get_layer(s_region_layer),true);

  // UTC time text

  s_utctime_layer = text_layer_create(GRect(0, 109, 144, 40));
  text_layer_set_font(s_utctime_layer, custom_font);
  text_layer_set_text_color(s_utctime_layer, GColorBlack);
  text_layer_set_background_color(s_utctime_layer, GColorClear);
  text_layer_set_text_alignment(s_utctime_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_utctime_layer));

  s_utctime_layer_nosec = text_layer_create(GRect(0, 109, 144, 40));
  text_layer_set_font(s_utctime_layer_nosec, custom_font);
  text_layer_set_text_color(s_utctime_layer_nosec, GColorBlack);
  text_layer_set_background_color(s_utctime_layer_nosec, GColorClear);
  text_layer_set_text_alignment(s_utctime_layer_nosec, GTextAlignmentCenter);	
  layer_add_child(window_layer, text_layer_get_layer(s_utctime_layer_nosec));
	
  // UTC text
  setup_text_layer(&s_timezone_utc_layer, 
    "ZULU / UTC", "RESOURCE_ID_GOTHIC_14_BOLD", 
    (GRect) {{0, 81}, {144, 28}}, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_timezone_utc_layer));
  layer_set_hidden(text_layer_get_layer(s_timezone_utc_layer),true);

  // UTC date text
  setup_text_layer(&s_utcdate_layer, 
    s_utcdate_string, "RESOURCE_ID_GOTHIC_14", 
    (GRect) {{0, 93}, {144, 18}}, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_utcdate_layer));


	// resources
    img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECT);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
	
	layer_conn_img  = bitmap_layer_create(GRect(18, 139, 20, 20));
	layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img));
	
		
  battery_text_layer = text_layer_create(GRect(107, 142, 50, 40));
  text_layer_set_text_color(battery_text_layer, GColorBlack);
  text_layer_set_background_color(battery_text_layer, GColorClear);
  text_layer_set_font(battery_text_layer, custom_font3);
  text_layer_set_text_alignment(battery_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));	
	
	
	
 battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_boundsbatt = gbitmap_get_bounds(battery_image);
#else
  GRect bitmap_boundsbatt = battery_image->bounds;
#endif	
  GRect frame4 = GRect(38, 144, bitmap_boundsbatt.size.w, bitmap_boundsbatt.size.h);	
  battery_layer = bitmap_layer_create(frame4);
  battery_image_layer = bitmap_layer_create(frame4);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_set_update_proc(bitmap_layer_get_layer(battery_layer), battery_layer_update_callback);

  layer_add_child(window_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
	
	
  handle_bluetooth(bluetooth_connection_service_peek());
  update_battery_state(battery_state_service_peek());

	

  // Call handler once to populate initial time display
  time_t current_time = time(NULL);
  tick_handler(localtime(&current_time), SECOND_UNIT | MINUTE_UNIT | DAY_UNIT);
}

static void window_unload(Window *window) {

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(bg_layer));
  bitmap_layer_destroy(bg_layer);
  gbitmap_destroy(bg_bitmap);
		
  layer_remove_from_parent(bitmap_layer_get_layer(bg_layer1));
  bitmap_layer_destroy(bg_layer1);
  gbitmap_destroy(bg_bitmap1);
		
  layer_remove_from_parent(bitmap_layer_get_layer(bg_layer2));
  bitmap_layer_destroy(bg_layer2);
  gbitmap_destroy(bg_bitmap2);
		
  layer_remove_from_parent(bitmap_layer_get_layer(bg_layer3));
  bitmap_layer_destroy(bg_layer3);
  gbitmap_destroy(bg_bitmap3);
		
	
  layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
  bitmap_layer_destroy(battery_layer);
  gbitmap_destroy(battery_image);
  
  layer_remove_from_parent(bitmap_layer_get_layer(battery_image_layer));
  bitmap_layer_destroy(battery_image_layer);
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_conn_img));
  bitmap_layer_destroy(layer_conn_img);
  gbitmap_destroy(img_bt_connect);
  gbitmap_destroy(img_bt_disconnect);
	
	
 text_layer_destroy(time_layer);
 text_layer_destroy(time_layer_nosec);
 text_layer_destroy(battery_text_layer);
 text_layer_destroy(s_utctime_layer);
 text_layer_destroy(s_utctime_layer_nosec);
 text_layer_destroy(s_utcdate_layer);
 text_layer_destroy(s_timezone_utc_layer);
 text_layer_destroy(s_region_layer);
 text_layer_destroy(s_localdate_layer);
 text_layer_destroy(s_timezone_abbr_layer);
 text_layer_destroy(ampm_layer2);
	
  fonts_unload_custom_font(custom_font);
  fonts_unload_custom_font(custom_font2);
  fonts_unload_custom_font(custom_font3);
	
}

static void handle_init(void) {
  // Get user's clock preference
  s_is_clock_24 = clock_is_24h_style();

  // Set up main Window
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_stack_push(s_main_window, false);
	
  // Setup tick time handler
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  bluetooth_connection_service_subscribe(handle_bluetooth);
  battery_state_service_subscribe(&update_battery_state);
  accel_tap_service_subscribe(tap_handler);


  //comment this out if you don't want seconds on start
  show_status();
	
}

static void handle_deinit(void) {
	
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
	
  // Destroy main Window
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();

  return 0;
}

