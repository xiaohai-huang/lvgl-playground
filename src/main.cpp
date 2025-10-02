#include <Arduino.h>
#include <simfang_16.h>
#include <display.h>
#include <lvgl.h>

// Create subjects for each valve state
static lv_subject_t opening_subject;
static lv_subject_t closing_subject;
static lv_subject_t open_position_subject;
static lv_subject_t close_position_subject;
static lv_subject_t remote_subject;
static lv_subject_t fault_subject;

// Forward declarations for observer callbacks
static void opening_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void closing_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void open_position_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void close_position_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void remote_observer_cb(lv_observer_t *observer, lv_subject_t *subject);
static void fault_observer_cb(lv_observer_t *observer, lv_subject_t *subject);

static void open_valve_event_cb(lv_event_t *e)
{
  // Send command to open the valve
  Serial.println("Open valve");

  // Update subjects to reflect the new state
  lv_subject_set_int(&opening_subject, 1);
  lv_subject_set_int(&closing_subject, 0);
}

static void close_valve_event_cb(lv_event_t *e)
{
  // Send command to close the valve
  Serial.println("Close valve");

  // Update subjects to reflect the new state
  lv_subject_set_int(&opening_subject, 0);
  lv_subject_set_int(&closing_subject, 1);
}

static void stop_valve_event_cb(lv_event_t *e)
{
  // Send command to stop the valve
  Serial.println("Stop valve");

  // Update subjects to reflect the new state
  lv_subject_set_int(&opening_subject, 0);
  lv_subject_set_int(&closing_subject, 0);
}
// remote/on site radio button
static lv_style_t style_radio;
static lv_style_t style_radio_chk;
static int32_t remote_on_site_active_index = 0;

static void radio_event_handler(lv_event_t *e)
{
  int32_t *active_id = (int32_t *)lv_event_get_user_data(e);
  lv_obj_t *cont = (lv_obj_t *)lv_event_get_current_target(e);
  lv_obj_t *act_cb = lv_event_get_target_obj(e);
  lv_obj_t *old_cb = lv_obj_get_child(cont, *active_id);

  /*Do nothing if the container was clicked*/
  if (act_cb == cont)
    return;

  lv_obj_remove_state(old_cb, LV_STATE_CHECKED); /*Uncheck the previous radio button*/
  lv_obj_add_state(act_cb, LV_STATE_CHECKED);    /*Check the current radio button*/

  *active_id = lv_obj_get_index(act_cb);

  if (*active_id == 0) // 远程
  {
    lv_subject_set_int(&remote_subject, 1);
  }
  else
  {
    lv_subject_set_int(&remote_subject, 0);
  }
}

static void create_radio_button(lv_obj_t *parent, const char *txt)
{
  lv_obj_t *obj = lv_checkbox_create(parent);
  lv_checkbox_set_text(obj, txt);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_style(obj, &style_radio, LV_PART_INDICATOR);
  lv_obj_add_style(obj, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);
}

void remote_on_site_buttons()
{
  lv_style_init(&style_radio);
  lv_style_set_radius(&style_radio, LV_RADIUS_CIRCLE);

  lv_style_init(&style_radio_chk);
  lv_style_set_bg_image_src(&style_radio_chk, NULL);

  lv_obj_t *container = lv_obj_create(lv_screen_active());
  lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_size(container, lv_pct(40), lv_pct(80));
  lv_obj_add_event_cb(container, radio_event_handler, LV_EVENT_CLICKED, &remote_on_site_active_index);
  create_radio_button(container, "远程"); // 0
  create_radio_button(container, "就地"); // 1
  lv_obj_set_style_border_width(container, 0, 0);
  lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);

  /*Make the 就地 index 1 checkbox checked*/
  lv_obj_add_state(lv_obj_get_child(container, 1), LV_STATE_CHECKED);
  remote_on_site_active_index = 1;

  lv_obj_align(container, LV_ALIGN_TOP_RIGHT, 0, 0);
}

void main_page()
{
  lv_obj_t *screen = lv_obj_create(NULL);
  lv_scr_load(screen);

  // Initialize subjects with default values (0 = false, 1 = true)
  lv_subject_init_int(&opening_subject, 0);
  lv_subject_init_int(&closing_subject, 0);
  lv_subject_init_int(&open_position_subject, 0);
  lv_subject_init_int(&close_position_subject, 0);
  lv_subject_init_int(&remote_subject, 0);
  lv_subject_init_int(&fault_subject, 0);

  // Create a container for the buttons with grid layout
  lv_obj_t *btnContainer = lv_obj_create(screen);
  lv_obj_set_scrollbar_mode(btnContainer, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_size(btnContainer, LV_PCT(100), 80);
  lv_obj_align(btnContainer, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Define grid columns and rows
  static int32_t column_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

  // Set grid descriptors
  lv_obj_set_grid_dsc_array(btnContainer, column_dsc, row_dsc);
  lv_obj_set_grid_align(btnContainer, LV_GRID_ALIGN_SPACE_BETWEEN, LV_GRID_ALIGN_CENTER);
  lv_obj_set_style_pad_column(btnContainer, 10, 0); // Gap between buttons
  lv_obj_set_style_pad_row(btnContainer, 0, 0);

  // Remove default border and background from container
  lv_obj_set_style_border_width(btnContainer, 0, 0);
  lv_obj_set_style_bg_opa(btnContainer, LV_OPA_TRANSP, 0);

  // Open valve button
  lv_obj_t *btnOpen = lv_btn_create(btnContainer);
  static lv_style_t styleOpen;
  lv_style_init(&styleOpen);
  lv_style_set_bg_color(&styleOpen, lv_color_hex(0x00FF00));
  lv_obj_add_style(btnOpen, &styleOpen, LV_PART_MAIN);
  lv_obj_set_size(btnOpen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_grid_cell(btnOpen, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_add_event_cb(btnOpen, open_valve_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *labelOpen = lv_label_create(btnOpen);
  lv_label_set_text(labelOpen, LV_SYMBOL_PLAY " 开阀");
  lv_obj_set_style_text_color(labelOpen, lv_color_hex(0x000000), 0);
  lv_obj_set_align(labelOpen, LV_ALIGN_CENTER);

  // Stop button
  lv_obj_t *btnStop = lv_btn_create(btnContainer);
  static lv_style_t styleStop;
  lv_style_init(&styleStop);
  lv_style_set_bg_color(&styleStop, lv_color_hex(0xFF0000));
  lv_obj_add_style(btnStop, &styleStop, LV_PART_MAIN);
  lv_obj_set_size(btnStop, LV_PCT(100), LV_PCT(100));
  lv_obj_set_grid_cell(btnStop, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_add_event_cb(btnStop, stop_valve_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *labelStop = lv_label_create(btnStop);
  lv_label_set_text(labelStop, LV_SYMBOL_STOP " 停止");
  lv_obj_set_style_text_color(labelStop, lv_color_hex(0x000000), 0);
  lv_obj_set_align(labelStop, LV_ALIGN_CENTER);

  // Close valve button
  lv_obj_t *btnClose = lv_btn_create(btnContainer);
  static lv_style_t styleClose;
  lv_style_init(&styleClose);
  lv_style_set_bg_color(&styleClose, lv_color_hex(0xFFFF00));
  lv_obj_add_style(btnClose, &styleClose, LV_PART_MAIN);
  lv_obj_set_size(btnClose, LV_PCT(100), LV_PCT(100));
  lv_obj_set_grid_cell(btnClose, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_add_event_cb(btnClose, close_valve_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *labelClose = lv_label_create(btnClose);
  lv_label_set_text(labelClose, LV_SYMBOL_MINUS " 关阀");
  lv_obj_set_style_text_color(labelClose, lv_color_hex(0x000000), 0);
  lv_obj_set_align(labelClose, LV_ALIGN_CENTER);

  // Status indicators
  lv_obj_t *labelOpening = lv_label_create(screen);
  lv_obj_align(labelOpening, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_label_set_text(labelOpening, "开阀中: -");
  lv_obj_set_style_text_color(labelOpening, lv_color_hex(0x00FF00), LV_PART_MAIN);
  // Subscribe to opening_subject as an Observer
  lv_subject_add_observer_obj(&opening_subject, opening_observer_cb, labelOpening, NULL);

  lv_obj_t *labelClosing = lv_label_create(screen);
  lv_obj_align(labelClosing, LV_ALIGN_TOP_LEFT, 10, 30);
  lv_label_set_text(labelClosing, "关阀中: -");
  lv_obj_set_style_text_color(labelClosing, lv_color_hex(0xFF0000), LV_PART_MAIN);
  // Subscribe to closing_subject as an Observer
  lv_subject_add_observer_obj(&closing_subject, closing_observer_cb, labelClosing, NULL);

  lv_obj_t *labelOpenPosition = lv_label_create(screen);
  lv_obj_align(labelOpenPosition, LV_ALIGN_TOP_LEFT, 10, 50);
  lv_label_set_text(labelOpenPosition, "开到位: -");
  lv_obj_set_style_text_color(labelOpenPosition, lv_color_hex(0x00FF00), LV_PART_MAIN);
  // Subscribe to open_position_subject as an Observer
  lv_subject_add_observer_obj(&open_position_subject, open_position_observer_cb, labelOpenPosition, NULL);

  lv_obj_t *labelClosePosition = lv_label_create(screen);
  lv_obj_align(labelClosePosition, LV_ALIGN_TOP_LEFT, 10, 70);
  lv_label_set_text(labelClosePosition, "关到位: -");
  lv_obj_set_style_text_color(labelClosePosition, lv_color_hex(0x00FF00), LV_PART_MAIN);
  // Subscribe to close_position_subject as an Observer
  lv_subject_add_observer_obj(&close_position_subject, close_position_observer_cb, labelClosePosition, NULL);

  lv_obj_t *labelRemote = lv_label_create(screen);
  lv_obj_align(labelRemote, LV_ALIGN_TOP_LEFT, 10, 90);
  lv_label_set_text(labelRemote, "远程: -");
  lv_obj_set_style_text_color(labelRemote, lv_color_hex(0x0000FF), LV_PART_MAIN);
  // Subscribe to remote_subject as an Observer
  lv_subject_add_observer_obj(&remote_subject, remote_observer_cb, labelRemote, NULL);

  lv_obj_t *labelFault = lv_label_create(screen);
  lv_obj_align(labelFault, LV_ALIGN_TOP_LEFT, 10, 110);
  lv_label_set_text(labelFault, "故障: -");
  lv_obj_set_style_text_color(labelFault, lv_color_hex(0xFF0000), LV_PART_MAIN);
  // Subscribe to fault_subject as an Observer
  lv_subject_add_observer_obj(&fault_subject, fault_observer_cb, labelFault, NULL);

  remote_on_site_buttons();
}

// Observer callback functions
static void opening_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
  lv_obj_t *label = lv_observer_get_target_obj(observer);
  int32_t value = lv_subject_get_int(subject);
  lv_label_set_text_fmt(label, "开阀中: %s", value ? "是" : "否");
}

static void closing_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
  lv_obj_t *label = lv_observer_get_target_obj(observer);
  int32_t value = lv_subject_get_int(subject);
  lv_label_set_text_fmt(label, "关阀中: %s", value ? "是" : "否");
}

static void open_position_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
  lv_obj_t *label = lv_observer_get_target_obj(observer);
  int32_t value = lv_subject_get_int(subject);
  lv_label_set_text_fmt(label, "开到位: %s", value ? "是" : "否");
}

static void close_position_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
  lv_obj_t *label = lv_observer_get_target_obj(observer);
  int32_t value = lv_subject_get_int(subject);
  lv_label_set_text_fmt(label, "关到位: %s", value ? "是" : "否");
}

static void remote_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
  lv_obj_t *label = lv_observer_get_target_obj(observer);
  int32_t value = lv_subject_get_int(subject);
  lv_label_set_text_fmt(label, "远程: %s", value ? "是" : "否");
}

static void fault_observer_cb(lv_observer_t *observer, lv_subject_t *subject)
{
  lv_obj_t *label = lv_observer_get_target_obj(observer);
  int32_t value = lv_subject_get_int(subject);
  lv_label_set_text_fmt(label, "故障: %s", value ? "是" : "否");
}

void setup(void)
{
  Serial.begin(115200);
  display_init();
  main_page();
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);           /* let this time pass */
}