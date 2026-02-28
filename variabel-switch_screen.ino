// UI ada di eez/layar

#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "lv_conf.h"
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include "SensorPCF85063.hpp"
#include "HWCDC.h"
#include <CST816S.h> // Library untuk Touch CST816T

// INCLUDE FILE DARI EEZ STUDIO
#include "ui.h"

HWCDC USBSerial;
#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT / 10];

// 2. Deklarasi Variabel String
const char* name1 = "M Dzakwan";
int counter = 0;           // Ganti name2 jadi int
static char num[16] = "0";
uint32_t lastTick = 0;

SensorPCF85063 rtc;
CST816S touch(IIC_SDA, IIC_SCL, TP_RST, TP_INT); // Inisialisasi Touch
uint32_t lastMillis;

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0, true, LCD_WIDTH, LCD_HEIGHT, 0, 20, 0, 0);

/* --- FUNGSI PEMBACA TOUCHPAD UNTUK LVGL --- */
void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    if (touch.available()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch.data.x;
        data->point.y = touch.data.y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/* --- LOGIKA PERPINDAHAN LAYAR (SWIPE) --- */
void ui_event_gesture_handler(lv_event_t * e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_obj_t * screen = lv_event_get_target(e);

    if (dir == LV_DIR_LEFT) {
        if (screen == objects.main) {
            lv_scr_load_anim(objects.main_2, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        }
    } else if (dir == LV_DIR_RIGHT) {
        if (screen == objects.main_2) {
            lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        }
    }
}

/* Serial debugging */
void my_print(const char *buf) {
  USBSerial.printf(buf);
  USBSerial.flush();
}

/* Flush display */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
  lv_disp_flush_ready(disp);
}

void example_increase_lvgl_tick(void *arg) {
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void setup() {
  USBSerial.begin(115200);

  // 1. Inisialisasi Touch & RTC
  touch.begin(); // Mulai driver CST816T
  Wire.begin(IIC_SDA, IIC_SCL);
  rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);

  // 2. Inisialisasi Layar
  gfx->begin();
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  // 3. LVGL Init
  lv_init();
#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); 
#endif

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LCD_WIDTH * LCD_HEIGHT / 10);

  // Driver Display
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Driver Input (Sangat penting agar bisa swipe!)
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read; 
  lv_indev_drv_register(&indev_drv);

  // Tick Timer
  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  // --- INISIALISASI UI ---
  ui_init(); 

  if (objects.label1) {
    lv_label_set_text(objects.label1, name1);
  }

  if (objects.label2) {
      lv_label_set_text(objects.label2, num);
  }

  // Daftarkan event gesture pada layar
  lv_obj_add_event_cb(objects.main, ui_event_gesture_handler, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(objects.main_2, ui_event_gesture_handler, LV_EVENT_GESTURE, NULL);

  USBSerial.println("CST816T Touch & Swipe Ready");
}

void loop() {
  lv_timer_handler(); 
  
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    counter++; // Angka naik terus
    sprintf(num, "%d", counter); 
    // Update tampilan label2
    if (objects.label2) {
      lv_label_set_text(objects.label2, num);
    }
    
  }

  
  delay(5); 
}
