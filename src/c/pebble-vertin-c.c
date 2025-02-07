#include <pebble.h>

static Window *s_window;

// Array and resource handling were easier because I found this example:
//
// https://github.com/pebble/pebble-sdk-examples/blob/master/watchfaces/ninety_one_dub/src/ninety_one_dub.c

static const int BLACK_BITMAP_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_0_B,
  RESOURCE_ID_IMAGE_1_B,
  RESOURCE_ID_IMAGE_2_B,
  RESOURCE_ID_IMAGE_3_B,
  RESOURCE_ID_IMAGE_4_B,
  RESOURCE_ID_IMAGE_5_B,
  RESOURCE_ID_IMAGE_6_B,
  RESOURCE_ID_IMAGE_7_B,
  RESOURCE_ID_IMAGE_8_B,
  RESOURCE_ID_IMAGE_9_B
};

static const int WHITE_BITMAP_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_0_W,
  RESOURCE_ID_IMAGE_1_W,
  RESOURCE_ID_IMAGE_2_W,
  RESOURCE_ID_IMAGE_3_W,
  RESOURCE_ID_IMAGE_4_W,
  RESOURCE_ID_IMAGE_5_W,
  RESOURCE_ID_IMAGE_6_W,
  RESOURCE_ID_IMAGE_7_W,
  RESOURCE_ID_IMAGE_8_W,
  RESOURCE_ID_IMAGE_9_W
};

static GBitmap *black_bitmaps[10];
static GBitmap *white_bitmaps[10];

static Layer *cutpie_layer;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(cutpie_layer);
}

// https://developer.rebble.io/developer.pebble.com/guides/graphics-and-animations/drawing-primitives-images-and-text/index.html
static void update_pie(Layer *layer, GContext *ctx) {
  GRect layer_bounds = layer_get_bounds(layer);
  GRect pie_bounds = GRect(-100, -100, layer_bounds.size.w + 200, layer_bounds.size.h + 200);

  time_t now = time(NULL);
  // https://cplusplus.com/reference/ctime/tm/
  struct tm *t = localtime(&now);
  int seconds = t->tm_sec;

  int hour_ones_digit = (t->tm_hour) % 10;
  int hour_tens_digit = ((t->tm_hour) - hour_ones_digit) / 10;
  int minute_ones_digit = (t->tm_min) % 10;
  int minute_tens_digit = ((t->tm_min) - minute_ones_digit) / 10;

  GColor background_color = t->tm_min % 2 ? GColorBlack : GColorWhite;
  GColor pie_color = t->tm_min % 2 ? GColorWhite : GColorBlack;
  
  static GBitmap **positive_bitmaps;
  static GBitmap **negative_bitmaps;

  positive_bitmaps = t->tm_min % 2 ? white_bitmaps : black_bitmaps;
  negative_bitmaps = t->tm_min % 2 ? black_bitmaps : white_bitmaps;

  // q1: 0-15, q2: 15-30, q3: 30-45, q4: 45-60
  GRect q1_bounds = GRect(layer_bounds.size.w/2, 0, layer_bounds.size.w/2, layer_bounds.size.h/2);
  GRect q2_bounds = GRect(layer_bounds.size.w/2, layer_bounds.size.h/2, layer_bounds.size.w/2, layer_bounds.size.h/2);
  GRect q3_bounds = GRect(0, layer_bounds.size.h/2, layer_bounds.size.w/2, layer_bounds.size.h/2);
  GRect q4_bounds = GRect(0, 0, layer_bounds.size.w/2, layer_bounds.size.h/2);

  graphics_context_set_fill_color(ctx, background_color);
  graphics_fill_rect(ctx, layer_bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, pie_color);

  int32_t angle_start = DEG_TO_TRIGANGLE(0);

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "hour, tens: %d", hour_tens_digit);
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "hour, ones: %d", hour_ones_digit);
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "minute, tens: %d", minute_tens_digit);
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "minute, ones: %d", minute_ones_digit);

  // Draw everything with GCompOpSet, which at least supports transparency.
  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  graphics_draw_bitmap_in_rect(ctx, positive_bitmaps[hour_ones_digit], q1_bounds);
  graphics_draw_bitmap_in_rect(ctx, positive_bitmaps[minute_ones_digit], q2_bounds);
  graphics_draw_bitmap_in_rect(ctx, positive_bitmaps[minute_tens_digit], q3_bounds);
  graphics_draw_bitmap_in_rect(ctx, positive_bitmaps[hour_tens_digit], q4_bounds);

  if (seconds) {
    // Thanks to: https://github.com/pebble/pebble-sdk-examples/blob/master/watchfaces/simple_analog/src/simple_analog.c
    //   int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
    int32_t angle_end = DEG_TO_TRIGANGLE(360 * seconds / 60);

    // void graphics_fill_radial(GContext * ctx, GRect rect, GOvalScaleMode scale_mode, uint16_t inset_thickness, int32_t angle_start, int32_t angle_end)
    graphics_fill_radial(ctx, pie_bounds, GOvalScaleModeFitCircle, pie_bounds.size.w / 2, angle_start, angle_end);
  }

  // TODO: See if there's some way to use GCompOpAssign to help with inverted text.
  // graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (seconds > 7) {
    graphics_draw_bitmap_in_rect(ctx, negative_bitmaps[hour_ones_digit], q1_bounds);
  }
  
  if (seconds > 22) {
    graphics_draw_bitmap_in_rect(ctx, negative_bitmaps[minute_ones_digit], q2_bounds);
  }

  if (seconds > 37) {
    graphics_draw_bitmap_in_rect(ctx, negative_bitmaps[minute_tens_digit], q3_bounds);
  }

  if (seconds > 52) {
    graphics_draw_bitmap_in_rect(ctx, negative_bitmaps[hour_tens_digit], q4_bounds);
  }
}

static void main_window_load(Window *window) {
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  cutpie_layer = layer_create(bounds);

  layer_set_update_proc(cutpie_layer, update_pie);
  layer_add_child(window_layer, cutpie_layer);

  layer_mark_dirty(cutpie_layer);

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void main_window_unload(Window *window) {
  layer_destroy(cutpie_layer);
}

static void init(void) {
  memset(&black_bitmaps, 0, sizeof(black_bitmaps));
  memset(&white_bitmaps, 0, sizeof(white_bitmaps));

  for (int a = 0; a < 10; a++) {
    black_bitmaps[a] = gbitmap_create_with_resource(BLACK_BITMAP_RESOURCE_IDS[a]);
    white_bitmaps[a] = gbitmap_create_with_resource(WHITE_BITMAP_RESOURCE_IDS[a]);
  }
 
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void deinit(void) {
  window_destroy(s_window);

  for (int a=0; a<10; a++) {
    gbitmap_destroy(black_bitmaps[a]);
    gbitmap_destroy(white_bitmaps[a]);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
