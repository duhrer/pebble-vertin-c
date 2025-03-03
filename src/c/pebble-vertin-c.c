#include <pebble.h>

static Window *s_window;

// Array and resource handling were easier because I found this example:
//
// https://github.com/pebble/pebble-sdk-examples/blob/master/watchfaces/ninety_one_dub/src/ninety_one_dub.c

static const int BITMAP_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_0,
  RESOURCE_ID_IMAGE_1,
  RESOURCE_ID_IMAGE_2,
  RESOURCE_ID_IMAGE_3,
  RESOURCE_ID_IMAGE_4,
  RESOURCE_ID_IMAGE_5,
  RESOURCE_ID_IMAGE_6,
  RESOURCE_ID_IMAGE_7,
  RESOURCE_ID_IMAGE_8,
  RESOURCE_ID_IMAGE_9
};

static GBitmap *bitmaps[10];

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor PrimaryColour;
  GColor SecondaryColour;
  bool EnableNightMode;
  GColor PrimaryNightColour;
  GColor SecondaryNightColour;
  int NightStartHour;
  int NightStartMinute;
  int NightEndHour;
  int NightEndMinute;
} ClaySettings;

static ClaySettings settings;

typedef struct ColourScheme {
  GColor PrimaryColour;
  GColor SecondaryColour;
} ColourScheme;

static ColourScheme get_current_colour_scheme(tm *t) {
  struct ColourScheme CurrentColourScheme;

  // Day mode
  if (
    !settings.EnableNightMode ||
    (t->tm_hour == settings.NightEndHour && (t->tm_min >= settings.NightEndMinute)) ||
    (t->tm_hour > settings.NightEndHour && (t->tm_hour < settings.NightStartHour)) ||
    (t->tm_hour == settings.NightStartHour && (t->tm_min < settings.NightStartMinute))
  ) {
    CurrentColourScheme.PrimaryColour = t->tm_min % 2 ? settings.SecondaryColour : settings.PrimaryColour;
    CurrentColourScheme.SecondaryColour = t->tm_min % 2 ? settings.PrimaryColour : settings.SecondaryColour;
  }
  // Night Mode
  else {
    CurrentColourScheme.PrimaryColour = t->tm_min % 2 ? settings.SecondaryNightColour : settings.PrimaryNightColour;
    CurrentColourScheme.SecondaryColour = t->tm_min % 2 ? settings.PrimaryNightColour : settings.SecondaryNightColour;
  }

  return CurrentColourScheme;
}

static Layer *cutpie_layer;
static Layer *text_layer;

static void text_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(cutpie_layer);
}

static void cutpie_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(cutpie_layer);
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

    // TODO: Check available height and recentre to available-(full/2).

    for (int y = 0; y < bounds.size.h; y++) {
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);

      // The centre of the diagram is at bounds.size.w/2 by bounds.size.h/2, so
      // we have to adjust the y before multiplying, and the x after.
      int xIntersection = (int) ((y - (bounds.size.h/2)) * cotangent_by_seconds[seconds]) + (bounds.size.w/2);

      ColourScheme CurrentColourScheme = get_current_colour_scheme(t);

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
            GColor pixel_colour = (GColor){ .argb = info.data[x]};
            GColor new_colour = gcolor_equal(pixel_colour, CurrentColourScheme.PrimaryColour) ? CurrentColourScheme.SecondaryColour : CurrentColourScheme.PrimaryColour;

            memset(&info.data[x], new_colour.argb, 1);
          #elif defined(PBL_BW)
            // For black and white devices, the colour space is 1 bit per pixel, and white is 1.
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

static void update_text(Layer *layer, GContext *ctx) {
  GRect layer_bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  // https://cplusplus.com/reference/ctime/tm/
  struct tm *t = localtime(&now);

  int hour_ones_digit = (t->tm_hour) % 10;
  int hour_tens_digit = ((t->tm_hour) - hour_ones_digit) / 10;
  int minute_ones_digit = (t->tm_min) % 10;
  int minute_tens_digit = ((t->tm_min) - minute_ones_digit) / 10;

  ColourScheme CurrentColourScheme = get_current_colour_scheme(t);

  graphics_context_set_fill_color(ctx, CurrentColourScheme.SecondaryColour);
  graphics_fill_rect(ctx, layer_bounds, 0, GCornerNone);

  // TODO: Check available height and recentre to available-(full/2).

  // q1: 0-15, q2: 15-30, q3: 30-45, q4: 45-60
  GRect q1_bounds = GRect(layer_bounds.size.w/2, 0, layer_bounds.size.w/2, layer_bounds.size.h/2);
  GRect q2_bounds = GRect(layer_bounds.size.w/2, layer_bounds.size.h/2, layer_bounds.size.w/2, layer_bounds.size.h/2);
  GRect q3_bounds = GRect(0, layer_bounds.size.h/2, layer_bounds.size.w/2, layer_bounds.size.h/2);
  GRect q4_bounds = GRect(0, 0, layer_bounds.size.w/2, layer_bounds.size.h/2);

  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  GBitmap *q1_bitmap = bitmaps[hour_ones_digit];
  GColor *palette = gbitmap_get_palette(q1_bitmap);
  palette[0] = CurrentColourScheme.PrimaryColour;
  palette[1] = CurrentColourScheme.SecondaryColour;

  graphics_draw_bitmap_in_rect(ctx, q1_bitmap, q1_bounds);

  GBitmap *q2_bitmap = bitmaps[minute_ones_digit];
  gbitmap_set_palette(q2_bitmap, palette, false);
  graphics_draw_bitmap_in_rect(ctx, q2_bitmap, q2_bounds);

  GBitmap *q3_bitmap = bitmaps[minute_tens_digit];
  gbitmap_set_palette(q3_bitmap, palette, false);
  graphics_draw_bitmap_in_rect(ctx, q3_bitmap, q3_bounds);

  GBitmap *q4_bitmap = bitmaps[hour_tens_digit];
  gbitmap_set_palette(q4_bitmap, palette, false);
  graphics_draw_bitmap_in_rect(ctx, q4_bitmap, q4_bounds);
}

// TODO: Find button handling example and add controls for the colour mode.

static void main_window_load(Window *window) {
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);

  // https://developer.rebble.io/developer.pebble.com/guides/best-practices/building-for-every-pebble/index.html
  GRect bounds = layer_get_unobstructed_bounds(window_layer);
  
  text_layer = layer_create(bounds);
  layer_set_update_proc(text_layer, update_text);
  layer_add_child(window_layer, text_layer);

  cutpie_layer = layer_create(bounds);

  layer_set_update_proc(cutpie_layer, update_pie);
  layer_add_child(window_layer, cutpie_layer);

  layer_mark_dirty(text_layer);
  layer_mark_dirty(cutpie_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, text_tick_handler);
  tick_timer_service_subscribe(SECOND_UNIT, cutpie_tick_handler);
}

static void main_window_unload(Window *window) {
  layer_destroy(cutpie_layer);
  layer_destroy(text_layer);
}

// Adapted from Pebble Clay example project:
// https://github.com/pebble-examples/clay-example/

// Initialize the default settings
static void default_settings() {
  settings.PrimaryColour = GColorBlue;
  settings.SecondaryColour = GColorWhite;

  settings.EnableNightMode = false;
  settings.PrimaryNightColour = GColorDarkGray;
  settings.SecondaryNightColour = GColorBlack;

  settings.NightStartHour = 22;
  settings.NightStartMinute = 0;
  settings.NightEndHour = 6;
  settings.NightEndMinute = 0;
}

// Read settings from persistent storage
static void load_settings() {
  // Load the default settings
  default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save the settings to persistent storage
static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));

  // Update the time displayed based on the settings.
  layer_mark_dirty(text_layer);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *primary_colour_t = dict_find(iter, MESSAGE_KEY_PrimaryColour);
  if(primary_colour_t) {
    settings.PrimaryColour = GColorFromHEX(primary_colour_t->value->int32);
  }

  Tuple *secondary_colour_t = dict_find(iter, MESSAGE_KEY_SecondaryColour);
  if(secondary_colour_t) {
    settings.SecondaryColour = GColorFromHEX(secondary_colour_t->value->int32);
  }

  Tuple *enable_night_mode_t = dict_find(iter, MESSAGE_KEY_EnableNightMode);
  if (enable_night_mode_t) {
    settings.EnableNightMode = enable_night_mode_t->value->int32 == 1;
  }

  Tuple *primary_night_colour_t = dict_find(iter, MESSAGE_KEY_PrimaryNightColour);
  if(primary_night_colour_t) {
    settings.PrimaryNightColour = GColorFromHEX(primary_night_colour_t->value->int32);
  }

  Tuple *secondary_night_colour_t = dict_find(iter, MESSAGE_KEY_SecondaryNightColour);
  if(secondary_night_colour_t) {
    settings.SecondaryNightColour = GColorFromHEX(secondary_night_colour_t->value->int32);
  }

  Tuple *night_start_t = dict_find(iter, MESSAGE_KEY_NightStart);
  if (night_start_t) {
    char night_start_hour_string[3];
    memcpy(night_start_hour_string, &night_start_t->value->cstring[0], 2);
    night_start_hour_string[2] = '\0';

    settings.NightStartHour = atoi(night_start_hour_string);

    char night_start_minute_string[3];
    memcpy(night_start_minute_string, &night_start_t->value->cstring[3], 2);
    night_start_minute_string[2] = '\0';

    settings.NightStartMinute = atoi(night_start_minute_string);

    APP_LOG(APP_LOG_LEVEL_INFO, "Night Start Time: %02d:%02d", settings.NightStartHour, settings.NightStartMinute);
  }

  Tuple *night_end_t = dict_find(iter, MESSAGE_KEY_NightEnd);
  if (night_end_t) {
    char night_end_hour_string[3];
    memcpy(night_end_hour_string, &night_end_t->value->cstring[0], 2);
    night_end_hour_string[2] = '\0';

    settings.NightEndHour = atoi(night_end_hour_string);

    char night_end_minute_string[3];
    memcpy(night_end_minute_string, &night_end_t->value->cstring[3], 2);
    night_end_minute_string[2] = '\0';

    settings.NightEndMinute = atoi(night_end_minute_string);

    APP_LOG(APP_LOG_LEVEL_INFO, "Night End Time: %02d:%02d", settings.NightEndHour, settings.NightEndMinute);
  }

  save_settings();
}

static void init(void) {
  load_settings();

  // Open AppMessage connection
  app_message_register_inbox_received(inbox_received_handler);

  // TODO: Find out and document why this value is how it is.
  app_message_open(128, 128);

  memset(&bitmaps, 0, sizeof(bitmaps));

  for (int a = 0; a < 10; a++) {
    bitmaps[a] = gbitmap_create_with_resource(BITMAP_RESOURCE_IDS[a]);
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
    gbitmap_destroy(bitmaps[a]);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
