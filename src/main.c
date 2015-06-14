#include <pebble.h>

enum {
  SCREEN_WIDTH = 144,
  SCREEN_HEIGHT = 168,
  
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,

  CIRCLE_LINE_THICKNESS = 2,
  CIRCLE_PADDING = 2,
  SIDE_PADDING = 12,

  HOURS_MAX_COLS = 5,
  MINUTES_MAX_COLS = 6,
  HOURS_CIRCLE_RADIUS = 10,
  MINUTES_CIRCLE_RADIUS = 8,
  HOURS_ROW_START = (HOURS_CIRCLE_RADIUS),
  MINUTES_ROW_START = (4 * HOURS_ROW_START) 
};

static Window *s_main_window;
static Layer *s_display_layer;
static TextLayer *s_date_layer, *s_weather_layer, *s_battery_layer;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100% charged";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%% charged", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void draw_cell(GContext *ctx, GPoint center, short radius, bool filled) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, radius);

  if (!filled) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, center, radius - CIRCLE_LINE_THICKNESS);
  }
}

static GPoint get_cell_centre(short x, short y, short radius) {
  short cell_size = (2 * (radius + CIRCLE_PADDING));

  return GPoint(SCREEN_WIDTH - (12 + (cell_size / 2) + (cell_size * x)), (cell_size / 2) + y);
}

static void draw_cell_row_for_digit(GContext *ctx, short digit, short max_cols, short cell_row, short radius) {
  for (int i = 0; i < max_cols; i++) {
    draw_cell(ctx, get_cell_centre(i, cell_row, radius), radius, (digit >> i) & 0x1);
  }
}

static short get_display_hour(short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }

  short display_hour = hour % 12;
  return display_hour ? display_hour : 12;
}

static void display_layer_update_callback(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  short display_hour = get_display_hour(t->tm_hour);

  draw_cell_row_for_digit(ctx, display_hour, HOURS_MAX_COLS, HOURS_ROW_START, HOURS_CIRCLE_RADIUS);
  draw_cell_row_for_digit(ctx, t->tm_min, MINUTES_MAX_COLS, MINUTES_ROW_START, MINUTES_CIRCLE_RADIUS);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_display_layer = layer_create(bounds);
  layer_set_update_proc(s_display_layer, display_layer_update_callback);
  layer_add_child(window_layer, s_display_layer);

  s_date_layer = text_layer_create(GRect(0, 80, SCREEN_WIDTH, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  s_weather_layer = text_layer_create(GRect(0, 110, SCREEN_WIDTH, 25));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

  s_battery_layer = text_layer_create(GRect(0, 140, SCREEN_WIDTH, 20));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
}

static void main_window_unload(Window *window) {
  layer_destroy(s_display_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_date_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  static char date_layer_buffer[32];

  layer_mark_dirty(s_display_layer);

  strftime(date_layer_buffer, sizeof(date_layer_buffer), "%B %e", tick_time);
  text_layer_set_text(s_date_layer, date_layer_buffer);

  // Update weather every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, 0, 0);
    app_message_outbox_send();
  }

  handle_battery(battery_state_service_peek());
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];

  Tuple *t = dict_read_first(iterator);
  while(t) {
    switch(t->key) {
      case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
        break;
      case KEY_CONDITIONS:
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }
    t = dict_read_next(iterator);
  }

  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(handle_battery);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}