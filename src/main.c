#include <pebble.h>
  
#define KEY_TIME 0
#define KEY_TICKS 1
  
#define POST_UPDATE_SEC 10;
#define READ_UPDATE_SEC 10;
  
enum {
	STATUS_KEY = 2,	
	MESSAGE_KEY = 3
};
  
static Window *s_main_window;
static Window *s_field_window;
static Window *s_timeout_window;

static TextLayer *s_time_layer;
static TextLayer *s_field_layer;

static GFont s_time_font;
static GFont s_field_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

int choosing_flag = 0;

// we don't have a race because we assume the handler takes less than a minute, and we perform the update before we parse
int pull_time; // stores last time pulled (number of minutes TODO -- what do we do at midnight?)
int num_seconds; // number of seconds since pull
int time_guess = 0;
int last_guess = 12 * 60 * 60;
int time_update = 2 * 60;
// TODO -- deal with midnight

// Write message to buffer & send
void send_message(int data){

	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_uint32(iter, STATUS_KEY, data);
	
	dict_write_end(iter);
  app_message_outbox_send();
}

static void update_time(int time_cur) {

  // Create a long-lived buffer
  static char buffer[] = "00:00:00";
  int hour = (time_cur/(60*60));
  int minute = (time_cur%(60*60)/60);
  int second = (time_cur%(60*60))%60;
  if (minute < 10) {
    snprintf(buffer, sizeof(buffer), "%d:0%d:%d", hour, minute, second);
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
 
}
static void main_select_raw_up_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_remove(s_main_window, false);
  window_stack_push(s_field_window, true);
  
}

static void field_up_click_handler(ClickRecognizerRef recognizer, void *context) {
    time_guess+=60;     
    update_time(time_guess);
}

static void field_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (time_guess >= 60) {
    time_guess-=60;     
    update_time(time_guess);
  }
}

static void field_select_raw_down_handler(ClickRecognizerRef recognizer, void *context) {
    
}

static void field_select_raw_up_handler(ClickRecognizerRef recognizer, void *context) {
  send_message(time_guess);
  last_guess = time_guess;
  choosing_flag = 0;
  window_stack_remove(s_field_window, true);
  window_stack_push(s_main_window, true);
}

static void timeout_select_raw_up_handler(ClickRecognizerRef recognizer, void *context) {
  send_message(time_guess);
  choosing_flag = 0;
  last_guess = time_guess;
  window_stack_remove(s_timeout_window, true);
  window_stack_push(s_main_window, true);
}


static void field_back_handler(ClickRecognizerRef recognizer, void *context) {
      choosing_flag = 0;
      window_stack_remove(s_field_window, false);
      window_stack_push(s_main_window, true);
}


static void main_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, main_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, main_down_click_handler);
  window_raw_click_subscribe(BUTTON_ID_SELECT, main_select_raw_down_handler, main_select_raw_up_handler, NULL);

}

static void field_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, field_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, field_down_click_handler);
  window_raw_click_subscribe(BUTTON_ID_SELECT, field_select_raw_down_handler, field_select_raw_up_handler, NULL); 
  window_single_click_subscribe(BUTTON_ID_BACK, field_back_handler);
}

static void timeout_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, field_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, field_down_click_handler);
  window_raw_click_subscribe(BUTTON_ID_SELECT, field_select_raw_down_handler, timeout_select_raw_up_handler, NULL); 
  
}

static void field_window_load(Window *window) {
  
  choosing_flag = 1;
  // Here we load the bitmap assets
  // resource_init_current_app must be called before all asset loading
  window_set_fullscreen(s_field_window, true);
  time_guess = pull_time + num_seconds;

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 60, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorClear);
  text_layer_set_text(s_time_layer, "Loading");
  
   // Create time TextLayer
  s_field_layer = text_layer_create(GRect(5, 10, 139, 50));
  text_layer_set_background_color(s_field_layer, GColorClear);
  text_layer_set_text_color(s_field_layer, GColorBlack);
  text_layer_set_text(s_field_layer, "Enter Your Time Approximation");
  
  //Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PACIFICO_32));
  s_field_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PACIFICO_16));

  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  text_layer_set_font(s_field_layer, s_field_font);
  text_layer_set_text_alignment(s_field_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
   // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_field_layer));
  update_time(time_guess);
}

static void timeout_window_load(Window *window) {
  
  choosing_flag = 1;
  // Here we load the bitmap assets
  // resource_init_current_app must be called before all asset loading
  window_set_fullscreen(s_timeout_window, true);
  time_guess = last_guess;

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 60, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorClear);
  text_layer_set_text(s_time_layer, "Loading");
  
   // Create time TextLayer
  s_field_layer = text_layer_create(GRect(5, 10, 139, 50));
  text_layer_set_background_color(s_field_layer, GColorClear);
  text_layer_set_text_color(s_field_layer, GColorBlack);
  text_layer_set_text(s_field_layer, "Enter Your Time Approximation");
  
  //Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PACIFICO_32));
  s_field_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PACIFICO_16));

  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  text_layer_set_font(s_field_layer, s_field_font);
  text_layer_set_text_alignment(s_field_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
   // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_field_layer));
  
  update_time(time_guess);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // again, we assume our handler works in such a 
  // way that we'll service this interrupt before the next one, and there will be no race condition
  // We're not exactly writing robust code here
  APP_LOG(APP_LOG_LEVEL_INFO, "asking again after %d %d seconds", time_update, pull_time + num_seconds);
  num_seconds++;
  if (time_update <= 0 && !choosing_flag) {   
      time_update = ((rand() + 5)%10) * 60;
      vibes_short_pulse();
      window_stack_remove(s_main_window, false);
      window_stack_push(s_timeout_window, true);
  }
  if (!((pull_time + num_seconds)%1) && !choosing_flag) {
      update_time(pull_time + num_seconds);
  }
  if (!choosing_flag) {
    time_update--;
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

static void main_window_load(Window *window) {
  choosing_flag = 0;
 
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 60, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "Loading");
  
  //Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PACIFICO_32));

  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  
  // Make sure the time is displayed from the start
  update_time(pull_time + num_seconds);
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



static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  
  // Read first item
  
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TICKS:
      pull_time = (int)t->value->uint32;
      num_seconds = 0;
      if (last_guess == 0) {
        srand(pull_time); 
      }
      last_guess = pull_time;
      
      
       
      update_time(pull_time);
      break;
    default:
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  
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
  s_timeout_window = window_create();
  window_set_fullscreen(s_main_window, true);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_timeout_window, (WindowHandlers) {
    .load = timeout_window_load,
    .unload = field_window_unload
  });
  
  window_set_click_config_provider(s_main_window, main_config_provider);
  window_set_click_config_provider(s_field_window, field_config_provider);
  window_set_click_config_provider(s_timeout_window, timeout_config_provider);
  
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
