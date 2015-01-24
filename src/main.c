#include <pebble.h>
  

#define KEY_TIME 0
#define KEY_TICKS 1
  
static Window *s_main_window;
static Window *s_field_window;
static TextLayer *s_time_layer;

static GFont s_time_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// we don't have a race because we assume the handler takes less than a minute, and we perform the update before we parse
int pull_time; // stores last time pulled (number of minutes TODO -- what do we do at midnight?)
int num_seconds; // number of seconds since pull
// TODO -- deal with midnight


static void update_time() {

  // Create a long-lived buffer
  static char buffer[] = "00:00:00";
  int time_cur = pull_time + num_seconds;
  int hour = (time_cur/(60*60));
  int minute = (time_cur%(60*60)/60);
  int second = (time_cur%(60*60))%60;
  if (minute < 10) {
    snprintf(buffer, sizeof(buffer), "%d:%0d:%d", hour, minute, second);
  } else {
    snprintf(buffer, sizeof(buffer), "%d:%d:%d", hour, minute, second); 
  }
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void main_up_click_handler(ClickRecognizerRef recognizer, void *context) {

}

static void main_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
}

static void main_select_raw_down_handler(ClickRecognizerRef recognizer, void *context) {
  // Show the Window on the watch, with animated=true
  window_stack_push(s_field_window, true);
}
static void main_select_raw_up_handler(ClickRecognizerRef recognizer, void *context) {
  
}

static void main_back_handler(ClickRecognizerRef recognizer, void *context) {
  
}


static void main_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, main_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, main_down_click_handler);
  window_raw_click_subscribe(BUTTON_ID_SELECT, main_select_raw_down_handler, main_select_raw_up_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, main_back_handler);
}
static void field_window_load(Window *window) {
 // Here we load the bitmap assets
  // resource_init_current_app must be called before all asset loading
  window_set_fullscreen(s_field_window, true);

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 75, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "Loading");
  
  //Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_32));

  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  
}

static void main_window_load(Window *window) {
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 75, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "Loading");
  
  //Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_32));

  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  
  //Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  //Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  
}

static void field_window_unload(Window *window) {
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  
  //Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  //Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // again, we assume our handler works in such a 
  // way that we'll service this interrupt before the next one, and there will be no race condition
  // We're not exactly writing robust code here
  num_seconds++;
  if (!((pull_time + num_seconds)%5)) {
      update_time();
  }
  
  // Get time pull update every 10 minutes
  if(tick_time->tm_min % 10 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 1, 16);
    
    // Send the message!
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  
  
  static char time_layer_buffer[32];
  
  // Read first item
  
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {

    case KEY_TIME:
      break;
    case KEY_TICKS:
      pull_time = (int)t->value->uint32;
      num_seconds = 0;
      snprintf(time_layer_buffer, sizeof(time_layer_buffer), "%d", pull_time);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  update_time();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  s_field_window = window_create();
  window_set_fullscreen(s_main_window, true);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_set_click_config_provider(s_main_window, main_config_provider);
  
  window_set_window_handlers(s_field_window, (WindowHandlers) {
    .load = field_window_load,
    .unload = field_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
