
#include "demos/lv_demos.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES 320
#define TFT_VER_RES 240
#define TFT_ROTATION LV_DISPLAY_ROTATION_90

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

typedef struct
{
  TFT_eSPI *tft;
} lv_tft_espi_t;

/* Tick source, tell LVGL how much time (milliseconds) has passed */
static uint32_t my_tick(void)
{
  return millis();
}

lv_display_t *disp;

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  lv_tft_espi_t *dsc = (lv_tft_espi_t *)lv_display_get_driver_data(disp);

  uint16_t touchX, touchY;
  bool touched = dsc->tft->getTouch(&touchY, &touchX);

  if (touched)
  {
    data->state = LV_INDEV_STATE_PRESSED;
    // Swap or invert coordinates if needed based on your display rotation
    data->point.x = touchX;
    data->point.y = touchY;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup(void)
{
  /* Initialize LVGL */
  lv_init();
  /* Set the tick callback */
  lv_tick_set_cb(my_tick);

  disp = lv_tft_espi_create(TFT_VER_RES, TFT_HOR_RES, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, TFT_ROTATION);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
  lv_indev_set_read_cb(indev, my_touchpad_read);

  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "你好");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);           /* let this time pass */
}
