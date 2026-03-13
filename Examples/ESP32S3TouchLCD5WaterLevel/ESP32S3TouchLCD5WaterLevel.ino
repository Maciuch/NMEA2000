#include <Arduino.h>
#ifndef ESP32_CAN_TX_PIN
#define ESP32_CAN_TX_PIN GPIO_NUM_15
#endif
#ifndef ESP32_CAN_RX_PIN
#define ESP32_CAN_RX_PIN GPIO_NUM_16
#endif

#ifndef ARDUINO_WAVESHARE_ESP32S3_TOUCH_LCD_5
#define ARDUINO_WAVESHARE_ESP32S3_TOUCH_LCD_5
#endif
#include <NMEA2000_CAN.h>
#include <N2kMessages.h>

// This example is for the Waveshare ESP32-S3-Touch-LCD-5.
// CAN TX is GPIO15 and CAN RX is GPIO16.
//
// You must have ESP32_Display_Panel and lvgl installed.

#include <ESP_Panel_Library.h>
#include <lvgl.h>

ESP_Panel *panel = NULL;
lv_obj_t *water_level_label;
lv_obj_t *water_level_bar;

// LVGL Display flushing callback
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    ESP_Panel *p = (ESP_Panel *)disp_drv->user_data;
    p->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, (void *)color_p);
    lv_disp_flush_ready(disp_drv);
}

void setup_lvgl() {
    lv_init();

    // Allocate draw buffers
    uint32_t buf_size = 800 * 480 / 10;
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size);

    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = panel;
    lv_disp_drv_register(&disp_drv);
}

void setup_lvgl_ui() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

  // Title
  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Fresh Water Level");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  // Bar to visually show level
  water_level_bar = lv_bar_create(scr);
  lv_obj_set_size(water_level_bar, 400, 60);
  lv_obj_align(water_level_bar, LV_ALIGN_CENTER, 0, -20);
  lv_bar_set_range(water_level_bar, 0, 100);
  lv_bar_set_value(water_level_bar, 0, LV_ANIM_OFF);

  // Text label for percentage
  water_level_label = lv_label_create(scr);
  lv_label_set_text(water_level_label, "-- %");
  lv_obj_set_style_text_color(water_level_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(water_level_label, LV_ALIGN_CENTER, 0, 40);
}

void FluidLevel(const tN2kMsg &N2kMsg);

typedef struct {
  unsigned long PGN;
  void (*Handler)(const tN2kMsg &N2kMsg);
} tNMEA2000Handler;

tNMEA2000Handler NMEA2000Handlers[]={
  {127505L,&FluidLevel},
  {0,0}
};

void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  int iHandler;
  for (iHandler=0; NMEA2000Handlers[iHandler].PGN!=0 && !(N2kMsg.PGN==NMEA2000Handlers[iHandler].PGN); iHandler++);
  if (NMEA2000Handlers[iHandler].PGN!=0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg);
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize display
  panel = new ESP_Panel();
  panel->init();
  panel->begin();

  setup_lvgl();
  setup_lvgl_ui();

  // Set Product information
  NMEA2000.SetProductInformation("00000003", // Manufacturer's Model serial code
                                 102, // Manufacturer's product code
                                 "ESP32-S3 LCD Water Level Display",  // Manufacturer's Model ID
                                 "1.0.0.0 (2024-03-13)",  // Manufacturer's Software version code
                                 "1.0.0.0 (2024-03-13)" // Manufacturer's Model version
                                 );

  // Set device information
  NMEA2000.SetDeviceInformation(112255, // Unique number. Use e.g. Serial number.
                                130, // Device function=Display
                                120, // Device class=Display
                                2040 // Just chosen free from code list
                               );

  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);
  NMEA2000.SetForwardStream(&Serial);
  NMEA2000.EnableForward(false);
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg);
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 24);
  NMEA2000.Open();
}

void loop() {
  NMEA2000.ParseMessages();
  lv_timer_handler();
  delay(5);
}

void FluidLevel(const tN2kMsg &N2kMsg) {
    unsigned char Instance;
    tN2kFluidType FluidType;
    double Level=0;
    double Capacity=0;

    if (ParseN2kFluidLevel(N2kMsg,Instance,FluidType,Level,Capacity) ) {
      if (FluidType == N2kft_Water) {
        Serial.print("Fresh Water Level: ");
        Serial.print(Level);
        Serial.println("%");

        // Update LVGL UI
        lv_bar_set_value(water_level_bar, (int32_t)Level, LV_ANIM_ON);
        lv_label_set_text_fmt(water_level_label, "%.1f %%", Level);
      }
    }
}
