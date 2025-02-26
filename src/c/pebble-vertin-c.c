#include <pebble.h>

static Window *s_window;

GFont deja_vu_sans_font;

static Layer *background_layer;
static TextLayer *hour_layer;
static TextLayer *minute_layer;
static Layer *cutpie_layer;

static GColor positiveColor;
static GColor negativeColor;

// TODO: Add icon image.

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  GColor text_color = tick_time->tm_min % 2 ? positiveColor : negativeColor;

  text_layer_set_text_color(hour_layer, text_color);

  static char hour_buffer[3];
  snprintf(hour_buffer, sizeof(hour_buffer), "%02d", tick_time->tm_hour);
  text_layer_set_text(hour_layer, hour_buffer);

  text_layer_set_text_color(minute_layer, text_color);

  static char minute_buffer[3];
  snprintf(minute_buffer, sizeof(minute_buffer), "%02d", tick_time->tm_min);
  text_layer_set_text(minute_layer, minute_buffer);
}

static void combined_tick_handler(struct tm *tick_time, TimeUnits units_changed) {

  if (tick_time->tm_sec) {
    layer_mark_dirty(cutpie_layer);
    layer_mark_dirty(background_layer);
  }
  else {
    update_time();
  }
}

static int get_quadrant(int x, int y, int width, int height ) {
  if (y <= (height / 2)) {
    if (x <= (width / 2)) {
      return 4;
    }
    else return 1;
  }
  else {
    if (x <= (width / 2)) {
      return 3;
    }
    else return 2;
  }
}

// Canned values for cotangent to avoid having to bring in math.h and perform extra calculations
const double cotangent_by_seconds[60] =
{
  0, // 270 degrees, intentionally zeroed
  -0.10510423526567597,
  -0.2125565616700213,
  -0.3249196962329061,
  -0.4452286853085366,
  -0.5773502691896258,
  -0.7265425280053606,
  -0.9004040442978389,
  -1.110612514829193,
  -1.376381920471173,
  -1.7320508075688754,
  -2.2460367739042164,
  -3.0776835371752513,
  -4.704630109478442,
  -9.51436445422259,
  0, // 0 degrees, intentionally zeroed
  9.514364454222585,
  4.704630109478455,
  3.077683537175254,
  2.2460367739042164,
  1.7320508075688774,
  1.3763819204711738,
  1.110612514829193,
  0.90040404429784,
  0.726542528005361,
  0.577350269189626,
  0.4452286853085361,
  0.3249196962329064,
  0.21255656167002226,
  0.10510423526567644,
  0, // 90 degress, intentionally zeroed
  -0.10510423526567632,
  -0.2125565616700219,
  -0.3249196962329063,
  -0.445228685308536,
  -0.5773502691896254,
  -0.7265425280053608,
  -0.90040404429784,
  -1.1106125148291923,
  -1.3763819204711734,
  -1.7320508075688774,
  -2.2460367739042146,
  -3.0776835371752522,
  -4.704630109478455,
  -9.51436445422256,
  0, // 180 degrees, intentionally zeroed
  9.514364454222623,
  4.70463010947846,
  3.07768353717525,
  2.2460367739042186,
  1.7320508075688767,
  1.376381920471174,
  1.110612514829193,
  0.9004040442978406,
  0.7265425280053611,
  0.5773502691896264,
  0.445228685308536,
  0.3249196962329065,
  0.21255656167002263,
  0.10510423526567635
};

// See: https://developer.rebble.io/developer.pebble.com/guides/graphics-and-animations/framebuffer-graphics/index.html
static bool byte_get_bit(uint8_t *byte, uint8_t bit) {
  return ((*byte) >> bit) & 1;
}

static void byte_set_bit(uint8_t *byte, uint8_t bit, uint8_t value) {
  *byte ^= (-value ^ *byte) & (1 << bit);
}

static void byte_flip_bit(uint8_t *byte, uint8_t bit) {
  byte_set_bit(byte, bit, !byte_get_bit(byte, bit));
}

// https://developer.rebble.io/developer.pebble.com/guides/graphics-and-animations/framebuffer-graphics/index.html
static void update_pie(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  // https://cplusplus.com/reference/ctime/tm/
  struct tm *t = localtime(&now);
  int seconds = t->tm_sec;

  if (seconds) {
    GRect bounds = layer_get_bounds(layer);
    GBitmap *fb = graphics_capture_frame_buffer(ctx);

    for (int y = 0; y < bounds.size.h; y++) {
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);

      // The centre of the diagram is at bounds.size.w/2 by bounds.size.h/2, so
      // we have to adjust the y before multiplying, and the x after.
      int xIntersection = (int) ((y - (bounds.size.h/2)) * cotangent_by_seconds[seconds]) + (bounds.size.w/2);

      for (int x = info.min_x; x <= info.max_x; x++) {
        int quadrant = get_quadrant(x, y, bounds.size.w, bounds.size.h);

        bool invertPixel = false;

        // Anything in Q1 should be inverted
        if (quadrant == 1) {
          if (seconds >= 15) {
            invertPixel = true;
          }
          else if (x > 0 && x < xIntersection) {
            invertPixel = true;
          }
        }
        // Anything in Q2 should be inverted
        else if (quadrant == 2) {
          if (seconds >= 30) {
            invertPixel = true;
          }
          else if (seconds > 15 && x < bounds.size.w && x >= xIntersection) {
            invertPixel = true;
          }
        }
        // Anything in Q3 should be inverted
        else if (quadrant == 3) {
          if (seconds >= 45) {
            invertPixel = true;
          }
          else if (seconds > 30 && x < bounds.size.w && x >= xIntersection) {
            invertPixel = true;
          }
        }
        else if (seconds > 45 && quadrant == 4 && x > 0 && x < xIntersection) {
          invertPixel = true;
        }

        // See this site for a more sophisticated approach:
        //
        // https://github.com/ygalanter/pebble-effect-layer
        if (invertPixel) {
          #if defined(PBL_COLOR)
            // For colour, each pixel has eight bits of colour data (alpha, red, green, blue).
            // Twiddle red Bits
            byte_flip_bit(&info.data[x], 5);
            byte_flip_bit(&info.data[x], 4);
            // Now green bits
            byte_flip_bit(&info.data[x], 3);
            byte_flip_bit(&info.data[x], 2);
            // Now blue bits
            byte_flip_bit(&info.data[x], 1);
            byte_flip_bit(&info.data[x], 0);
          #elif defined(PBL_BW)
            // For black and white devices, the color space is 1 bit per pixel, and white is 1.
            uint8_t byte = x / 8;
            uint8_t bit = x % 8;

            // Just twiddle the one bit.
            byte_flip_bit(&info.data[byte], bit);
          #endif
        }
      }
    }

    graphics_release_frame_buffer(ctx, fb);
  }
}

static void update_background(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  GColor background_color = tick_time->tm_min % 2 ? negativeColor : positiveColor;

  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}


// TODO: Add controls for the colour mode.

static void main_window_load(Window *window) {
  window_set_background_color(window, negativeColor);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  background_layer = layer_create(bounds);

  layer_set_update_proc(background_layer, update_background);
  layer_add_child(window_layer, background_layer);

  hour_layer = text_layer_create(GRect(0, (bounds.size.h/2) - 60, bounds.size.w, bounds.size.h/2));
  text_layer_set_font(hour_layer, deja_vu_sans_font);
  text_layer_set_background_color(hour_layer, GColorClear);
  text_layer_set_text_color(hour_layer, GColorClear);
  text_layer_set_text(hour_layer, "01");
  text_layer_set_text_alignment(hour_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(hour_layer));

  minute_layer = text_layer_create(GRect(0, bounds.size.h/2, bounds.size.w, bounds.size.h/2));
  text_layer_set_font(minute_layer, deja_vu_sans_font);
  text_layer_set_background_color(minute_layer, GColorClear);
  text_layer_set_text_color(minute_layer, GColorClear);
  text_layer_set_text(minute_layer, "23");
  text_layer_set_font(minute_layer, deja_vu_sans_font);
  text_layer_set_text_alignment(minute_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(minute_layer));

  cutpie_layer = layer_create(bounds);

  layer_set_update_proc(cutpie_layer, update_pie);
  layer_add_child(window_layer, cutpie_layer);

  layer_mark_dirty(background_layer);
  update_time();
  layer_mark_dirty(cutpie_layer);

  tick_timer_service_subscribe(SECOND_UNIT, combined_tick_handler);
}

static void main_window_unload(Window *window) {
  layer_destroy(cutpie_layer);
  text_layer_destroy(hour_layer);
  text_layer_destroy(minute_layer);
  layer_destroy(background_layer);
}

static void init(void) {
  // TODO: Load this from settings.  Also need a reload button.
  #if defined(PBL_COLOR)
  positiveColor = GColorBlue;
  negativeColor = GColorYellow;
  #else
  positiveColor = GColorWhite;
  negativeColor = GColorBlack;
  #endif

  deja_vu_sans_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DEJA_VU_SANS_60));

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

  fonts_unload_custom_font(deja_vu_sans_font);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
