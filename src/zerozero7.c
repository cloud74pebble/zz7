#include <pebble.h>
#define BOX_SIZE 30
#define BOX_END 45

#ifdef PBL_ROUND
  #define BOX_MAX 180
#elif PBL_RECT
  #define BOX_MAX 225
#else
  #define BOX_MAX 168
#endif

#define BGSIZE 225

#define ANIM_DURATION 300
#define ANIM_FIRST_DELAY 1000
#define ANIM_DELAY 0
#define ANI_STEPS 7

#define XKEY_VIBE_BT 0
#define XKEY_VIS_DATE 1
#define XKEY_VIS_DOW 2
#define XKEY_VIS_BAT 3

#define BRANDING true

//Windows und Layer
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_bat_layer;
static Layer *s_bg_layer;
static Layer *s_box_layer;
static Layer *s_box2_layer;
static BitmapLayer *s_bgbox_layer;
static BitmapLayer *s_logo_layer;

//Resourcen
static GBitmap *s_background_bitmap;
static GBitmap *s_bgani_bitmap;
static GBitmap *s_logo_bitmap;
static GFont s_time_font;
static GFont s_date_font;

//Animation
static PropertyAnimation *s_box_animation;
static int s_current_stage = 0;

// Function prototype 
static void next_animation();
static void PerformConfig(DictionaryIterator *iter, uint32_t iKEY);

//Funktionen die in MainWindow genutzt werden
static void update_hour() {
  layer_mark_dirty(s_bg_layer);
}
static void update_time() {
  
  bool bDow = persist_exists(XKEY_VIS_DOW) ? persist_read_bool(XKEY_VIS_DOW) : true;
  bool bDate = persist_exists(XKEY_VIS_DATE) ? persist_read_bool(XKEY_VIS_DATE) : true;

  char cDate[7] = "";
  if (bDate) {strcat(cDate, "%d. %B");}
  char cDow[3] = "";
  if (bDow) {strcat(cDow, "%a");} 
  
  char cDateFormat[13] = "";
  
  strcat(cDateFormat, cDate);
  strcat(cDateFormat, "%n");
  strcat(cDateFormat, cDow);
  
  
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  if ((tick_time->tm_min)==0) {
    update_hour();
  };
  
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  static char s_bufferd[30];
  strftime(s_bufferd, sizeof(s_bufferd), cDateFormat, tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  // Display this date on the TextLayer
  text_layer_set_text(s_date_layer, s_bufferd);
}

//Funktionen MainWindow
static void draw_background(Layer *layer, GContext *ctx) {

  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  int hour = tick_time->tm_hour;
  int i_corr = 0;
  
  if (hour > 12){
    hour = hour -12;
  }
  
  if ((hour == 3) || (hour == 9)) {
    i_corr = 0;
  }
 
  // Get image center
  GRect img_bounds = gbitmap_get_bounds(s_background_bitmap);
  GPoint src_ic = grect_center_point(&img_bounds);

  // Get context center
  GRect ctx_bounds = layer_get_bounds(layer);
  GPoint ctx_ic = grect_center_point(&ctx_bounds);
  
  // Angle of rotation
  int angle = ((hour * TRIG_MAX_ANGLE) / 12) + i_corr ;

  // Draw!
  graphics_draw_rotated_bitmap(ctx, s_background_bitmap, src_ic, angle, ctx_ic);
}

//Animation
static void anim_stopped_handler(Animation *animation, bool finished, void *context) {
#ifdef PBL_SDK_2
  // Free the animation only on SDK 2.x
  property_animation_destroy(s_box_animation);
#endif

  // Schedule the next one, unless the app is exiting
  if (finished) {
    next_animation();
  }
}
static void anim_update_proc(Layer *layer, GContext *ctx) {
  int x = layer_get_bounds(layer).size.w / 2 ;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(x,x) ,x -1);
  //layer_mark_dirty(s_box2_layer);
}
static void anim_update2_proc(Layer *layer, GContext *ctx) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Paint BOX2");
  int x = layer_get_bounds(layer).size.w / 2 ;
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_circle(ctx, GPoint(x,x) ,x -1);
}

static void next_animation() {
  // Determine start and finish positions
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_frame(window_layer);
  
  GRect start, finish;
  
  int i_Kreis = layer_get_frame(window_layer).size.w / 5;
  APP_LOG(APP_LOG_LEVEL_INFO, "i_Kreis: %d", i_Kreis);
  int i_X = 0;
  int i_Y = 0;
  int i_X_alt = 0;
  int i_Y_alt = 0;
  
  int i_delay = ANIM_DELAY;
  int i_duration = ANIM_DURATION;
  
  switch (s_current_stage) {
    case 0:
      
      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: %d", 0);
      i_delay = ANIM_FIRST_DELAY;
      
      //Layer positionieren
      layer_set_frame(bitmap_layer_get_layer(s_bgbox_layer), GRect(bounds.size.w+1, 0, bounds.size.w, bounds.size.h));
      
      //Layer sichtbarkeit anpassen
      layer_set_hidden(s_box2_layer, true);
      layer_set_hidden(s_bg_layer, true);
      layer_set_hidden(bitmap_layer_get_layer(s_bgbox_layer), true);
      layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), true);
      layer_set_hidden(text_layer_get_layer(s_date_layer), true);
      layer_set_hidden(text_layer_get_layer(s_time_layer), true);

      //"Alt"-Koordinaten ermitteln
      i_X_alt = - BOX_SIZE;  
      i_Y_alt = (bounds.size.h - BOX_SIZE)/2;
      
      //Neue Koordinaten
      i_X = 0;
      i_Y = i_Y_alt;
        
      //Animationskoordinaten setzen
      start = GRect(i_X_alt, i_Y_alt, BOX_SIZE, BOX_SIZE);
      finish = GRect(i_X, i_Y, BOX_SIZE, BOX_SIZE);
      
      //Animationsebene sichtbar machen
      layer_set_hidden(s_box_layer, false);
      break;

    case 1:

      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: 1");

      //"Alt"-Koordinaten ermitteln
      i_X_alt = 0;  
      i_Y_alt = (bounds.size.h - BOX_SIZE)/2;
      
      //Neue Koordinaten
      i_X = i_Kreis;
      i_Y = i_Y_alt;
        
      //Animationskoordinaten setzen
      start = GRect(i_X_alt, i_Y_alt, BOX_SIZE, BOX_SIZE);
      finish = GRect(i_X, i_Y, BOX_SIZE, BOX_SIZE);

      //Kreiskopie positionieren
      layer_set_frame(s_box2_layer, start);

      //Kreiskopie sichtbar machen
      layer_set_hidden(s_box2_layer, false);
      break;

    case 2:

      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: 2");

      //"Alt"-Koordinaten ermitteln
      i_X_alt = i_Kreis;  
      i_Y_alt = (bounds.size.h - BOX_SIZE)/2;
      
      //Neue Koordinaten
      i_X = i_Kreis*2;
      i_Y = i_Y_alt;
        
      //Animationskoordinaten setzen
      start = GRect(i_X_alt, i_Y_alt, BOX_SIZE, BOX_SIZE);
      finish = GRect(i_X, i_Y, BOX_SIZE, BOX_SIZE);

      //Kreiskopie positionieren
      layer_set_frame(s_box2_layer, start);

      break;

    case 3:

      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: 3");

      //Layer sichtbarkeit anpassen
      layer_set_hidden(s_box2_layer, true);

      //"Alt"-Koordinaten ermitteln
      i_X_alt = i_Kreis*2;  
      i_Y_alt = (bounds.size.h - BOX_SIZE)/2;
      
      //Neue Koordinaten
      i_X = i_Kreis*3;
      i_Y = i_Y_alt;
        
      //Animationskoordinaten setzen
      start = GRect(i_X_alt, i_Y_alt, BOX_SIZE, BOX_SIZE);
      finish = GRect(i_X, i_Y, BOX_SIZE, BOX_SIZE);

      //Kreiskopie positionieren
      layer_set_frame(s_box2_layer, start);

      //Kreiskopie sichtbar machen
      layer_set_hidden(s_box2_layer, false);
      break;

    case 4:

      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: 4");

      //Layer sichtbarkeit anpassen
      layer_set_hidden(s_box2_layer, true);

      //"Alt"-Koordinaten ermitteln
      i_X_alt = i_Kreis*3;  
      i_Y_alt = (bounds.size.h - BOX_SIZE)/2;
      
      //Neue Koordinaten
      i_X = i_Kreis*4;
      i_Y = i_Y_alt;
        
      //Animationskoordinaten setzen
      start = GRect(i_X_alt, i_Y_alt, BOX_SIZE, BOX_SIZE);
      finish = GRect(i_X, i_Y, BOX_SIZE, BOX_SIZE);

      //Kreiskopie positionieren
      layer_set_frame(s_box2_layer, start);

      //Kreiskopie sichtbar machen
      layer_set_hidden(s_box2_layer, false);
      break;

    case 5:

      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: 5");

      //Layer sichtbarkeit anpassen
      layer_set_hidden(s_box2_layer, true);

      //"Alt"-Koordinaten ermitteln
      i_X_alt = i_Kreis*4;  
      i_Y_alt = (bounds.size.h - BOX_SIZE)/2;
      
      //Neue Koordinaten
      i_X = bounds.size.h;
      i_Y = i_Y_alt;
        
      //Animationskoordinaten setzen
      start = GRect(i_X_alt, i_Y_alt, BOX_SIZE, BOX_SIZE);
      finish = GRect(i_X, i_Y, BOX_SIZE, BOX_SIZE);

      //Kreiskopie positionieren
      layer_set_frame(s_box2_layer, start);

      //Kreiskopie sichtbar machen
      layer_set_hidden(s_box2_layer, false);
      break;

    
    case 6:
      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: 6");
  
      i_duration = ANIM_DURATION * 2;
    
      //Kreise ausblenden
      layer_set_hidden(s_box_layer, true);
      layer_set_hidden(s_box2_layer, true);
      layer_set_frame(s_box_layer, GRect(-BOX_SIZE,(bounds.size.h - BOX_SIZE)/2, BOX_SIZE, BOX_SIZE));
      layer_set_frame(s_box2_layer, GRect(-BOX_SIZE,(bounds.size.h - BOX_SIZE)/2, BOX_SIZE, BOX_SIZE));
    
      //Animations_Layer einbelnden
      layer_set_hidden(bitmap_layer_get_layer(s_bgbox_layer), false);

      //"Alt"-Koordinaten ermitteln
      i_X_alt = bounds.size.w+1;  
      i_Y_alt = 0;
      
      //Neue Koordinaten
      i_X = -1;
      i_Y = 0;
       
      //Animationskoordinaten setzen
      start = GRect(i_X_alt, i_Y_alt, bounds.size.w, bounds.size.h);
      finish = GRect(i_X, i_Y, bounds.size.w, bounds.size.h);
      break;
    
    case 7:
      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: 7");

      layer_set_hidden(bitmap_layer_get_layer(s_bgbox_layer), true);
      layer_set_hidden(text_layer_get_layer(s_date_layer), false);
      layer_set_hidden(text_layer_get_layer(s_time_layer), false);
      psleep(200);
      layer_set_hidden(s_bg_layer, false);
      layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), false);
      break;

    default:
      APP_LOG(APP_LOG_LEVEL_INFO, "Animation Schritt: default");

      start = GRect(-BOX_SIZE, (bounds.size.h - BOX_SIZE)/2 , BOX_SIZE, BOX_SIZE);
      finish = GRect(-BOX_SIZE, (bounds.size.h - BOX_SIZE)/2 , BOX_SIZE, BOX_SIZE);
      break;
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Animation von X=%d nach X=%d", i_X_alt, i_X);

  if (s_current_stage < ANI_STEPS){
    // Schedule the next animation
    switch (s_current_stage) {
      case 0-5:
        s_box_animation = property_animation_create_layer_frame(s_box_layer, &start, &finish);
        animation_set_curve((Animation*)s_box_animation, AnimationCurveLinear);
        break;
      
      case 6:
        s_box_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(s_bgbox_layer), &start, &finish); 
        animation_set_curve((Animation*)s_box_animation, AnimationCurveEaseOut);
      break;
      
      default:
        s_box_animation = property_animation_create_layer_frame(s_box_layer, &start, &finish);
        break;
    }
    
    animation_set_duration((Animation*)s_box_animation, i_duration);
    animation_set_delay((Animation*)s_box_animation, i_delay);
    animation_set_handlers((Animation*)s_box_animation, (AnimationHandlers) {
      .stopped = anim_stopped_handler
    }, NULL);
    
    //Animation starten
    animation_schedule((Animation*)s_box_animation); 
    
    //Step erhöhen
    s_current_stage = (s_current_stage + 1);

  } else {
    s_current_stage = 0;
  };
  layer_mark_dirty(window_layer);

 
}

//Settings verarbeiten
static void PerformConfig(DictionaryIterator *iter, uint32_t iKEY) {
  Tuple *x_t = dict_find(iter, iKEY);
  if(x_t && x_t->value->int32 > 0) {
    persist_write_bool(iKEY, true);
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY %d ist TRUE", (int)iKEY);
  } else {
    persist_write_bool(iKEY, false);
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY %d ist FALSE", (int)iKEY);
  }
}
static void message_dropped(AppMessageResult reason, void *context){
   APP_LOG(APP_LOG_LEVEL_INFO, "DROPPED! Code <%d>", reason);
}
static void config_handler(DictionaryIterator *iter, void *context) {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Verarbeite Config!");
  
  //Settings verarbeiten und speichern
  PerformConfig(iter, XKEY_VIBE_BT);
  PerformConfig(iter, XKEY_VIS_DATE);
  PerformConfig(iter, XKEY_VIS_DOW);
  PerformConfig(iter, XKEY_VIS_BAT);
  
  update_time();

}

//Main Window
static void main_window_load(Window *window) {

  // Schriftarten initialisieren
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BOND_28));
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Hintergrung erzeugen
  s_bg_layer = layer_create(bounds);  
  layer_set_update_proc(s_bg_layer, draw_background);

  s_logo_layer = bitmap_layer_create(GRect(0,0,BGSIZE,BGSIZE));
  bitmap_layer_set_bitmap(s_logo_layer, s_logo_bitmap);
  bitmap_layer_set_alignment(s_logo_layer,GAlignCenter);
  layer_set_bounds(bitmap_layer_get_layer(s_logo_layer), GRect((bounds.size.w-BGSIZE)/2,(bounds.size.h-BGSIZE)/2,bounds.size.w,bounds.size.h));
  bitmap_layer_set_compositing_mode(s_logo_layer, GCompOpSet);
  s_bgbox_layer = bitmap_layer_create(GRect(0,0,BGSIZE,BGSIZE));
  bitmap_layer_set_bitmap(s_bgbox_layer, s_bgani_bitmap);
  bitmap_layer_set_alignment(s_bgbox_layer,GAlignCenter);
  layer_set_bounds(bitmap_layer_get_layer(s_bgbox_layer), GRect((bounds.size.w-BGSIZE)/2,(bounds.size.h-BGSIZE)/2,bounds.size.w,bounds.size.h));
  // Create the TextLayer with specific bounds
  s_bat_layer = text_layer_create(
      GRect(0, ((bounds.size.h-30)/2)+12 , bounds.size.w, 12));
  s_time_layer = text_layer_create(
      GRect(0, ((bounds.size.h-30)/2)-5 , bounds.size.w, 30));
  s_date_layer = text_layer_create(
      GRect(0, ((bounds.size.h)/2)+5 , bounds.size.w, 30));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_bat_layer, GColorClear);
  text_layer_set_text_color(s_bat_layer, GColorBlack);
  text_layer_set_text_alignment(s_bat_layer, GTextAlignmentCenter);
  //text_layer_set_font(s_bat_layer, s_bat_font);
  text_layer_set_text(s_bat_layer, "4");
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorRed);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, s_time_font);
  
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorRed);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, s_date_font);

  // Create Animating Layer
  s_box_layer = layer_create(GRect(0, 0 , BOX_SIZE, BOX_SIZE));
  layer_set_frame(s_box_layer, GRect(-BOX_SIZE, (bounds.size.h-BOX_SIZE)/2,0,0));
  layer_set_update_proc(s_box_layer, anim_update_proc);
  s_box2_layer = layer_create(GRect(0,0 , BOX_SIZE, BOX_SIZE));
  layer_set_update_proc(s_box2_layer, anim_update2_proc);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, s_bg_layer);
  if (BRANDING) {
    layer_add_child(window_layer, bitmap_layer_get_layer(s_logo_layer));
  }
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bgbox_layer));

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  //layer_add_child(window_layer, text_layer_get_layer(s_bat_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, s_box2_layer);
  layer_add_child(window_layer, s_box_layer);
  layer_set_hidden(s_box2_layer, true);
  
  //Uhrzeit aktualisieren
  update_time();
  update_hour();
 
}
static void main_window_unload(Window *window) {
  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  
  layer_destroy(s_bg_layer);
  layer_destroy(s_box_layer);
  layer_destroy(s_box2_layer);

  bitmap_layer_destroy(s_bgbox_layer);
  bitmap_layer_destroy(s_logo_layer);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  
}

//Events
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void focus_handler(bool inFocus) {

  layer_mark_dirty(s_bg_layer);
  layer_set_hidden(s_bg_layer, true);
  layer_set_hidden(text_layer_get_layer(s_time_layer), true);
  layer_set_hidden(text_layer_get_layer(s_date_layer), true);
 
  if (inFocus) {
    animation_unschedule_all();
    s_current_stage = ANI_STEPS - 2;
    psleep(1000);
    layer_set_hidden(s_bg_layer, false);
    layer_set_hidden(text_layer_get_layer(s_time_layer), false);
    layer_set_hidden(text_layer_get_layer(s_date_layer), false);
    //next_animation();
    
  }
}

static void bt_handler(bool connected) {
  bool bVibe = persist_exists(XKEY_VIBE_BT) ? persist_read_bool(XKEY_VIBE_BT) : true;
  if (connected) {
    text_layer_set_text_color(s_time_layer, GColorRed);
    if (bVibe) {vibes_short_pulse();}
  } else {
    text_layer_set_text_color(s_time_layer, GColorBlack);
    if (bVibe) {vibes_long_pulse();}
  }
  layer_mark_dirty(text_layer_get_layer(s_time_layer));
  
}


//Init, DeInit und main
static void init() {
  
  //localisation
  setlocale(LC_ALL, "");
    
  // Create GBitmaps
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BOND_BG);
  s_bgani_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BOND_ANI);
  s_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
 
  // Hintergrundfarbe setzen
  window_set_background_color(s_main_window, GColorBlack);
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  //Verarbeite Messages
  app_message_register_inbox_dropped(message_dropped);
  app_message_register_inbox_received(config_handler);
  app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);

  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  //Register Bluetooth
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_handler
  });
  
  //App-Focus handler
  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .will_focus = focus_handler,
  });
  

  // Starte animation
  next_animation();
  


}

static void deinit() {
  // Stop any animation in progress
  animation_unschedule_all();
  
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  app_focus_service_unsubscribe();
  app_message_deregister_callbacks();
    
  // Destroy Window
  window_destroy(s_main_window);
     
  // Destroy GBitmap
  gbitmap_destroy(s_logo_bitmap);
  gbitmap_destroy(s_bgani_bitmap);
  gbitmap_destroy(s_background_bitmap);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}


