#include "displayapp/screens/Tile.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void lv_update_task(struct _lv_task_t* task) {
    auto* user_data = static_cast<Tile*>(task->user_data);
    user_data->UpdateScreen();
  }

  void event_handler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<Tile*>(obj->user_data);
    screen->OnButtonEvent(obj, event);
  }
}

Tile::Tile(uint8_t screenID,
           uint8_t numScreens,
           DisplayApp* app,
           Controllers::Settings& settingsController,
           const Controllers::Battery& batteryController,
           const Controllers::Ble& bleController,
           const Controllers::AlarmController& alarmController,
           Controllers::DateTime& dateTimeController,
           Controllers::BrightnessController& brightnessController,
           const Applications& application)
  : app {app},
    settingsController {settingsController},
    dateTimeController {dateTimeController},
    brightnessController {brightnessController},
    pageIndicator(screenID, numScreens),
    statusIcons(batteryController, bleController, alarmController),
    launchApp {application.application},
    action {application.action} {

  settingsController.SetAppMenu(screenID);

  statusIcons.Create();
  lv_obj_align(statusIcons.GetObject(), lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -8, 0);

  // Time
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_align(label_time, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  pageIndicator.Create();

  // Full-page card: large icon with the item name beneath it. Tapping anywhere on the card activates it.
  button = lv_btn_create(lv_scr_act(), nullptr);
  button->user_data = this;
  lv_obj_set_event_cb(button, event_handler);
  lv_obj_set_style_local_radius(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 20);
  lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_size(button, LV_HOR_RES - 16, LV_VER_RES - 60);
  lv_obj_align(button, nullptr, LV_ALIGN_CENTER, 0, 10);
  lv_btn_set_layout(button, LV_LAYOUT_OFF);

  icon = lv_label_create(button, nullptr);
  lv_obj_set_style_local_text_font(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_sys_48);
  // The brightness card always reflects the live level, not the icon captured when the list was built.
  if (action == Action::CycleBrightness) {
    lv_label_set_text_static(icon, brightnessController.GetIcon());
  } else {
    lv_label_set_text_static(icon, application.icon);
  }
  lv_obj_align(icon, nullptr, LV_ALIGN_CENTER, 0, -20);

  label = lv_label_create(button, nullptr);
  lv_label_set_text_static(label, application.name);
  lv_obj_align(label, icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  taskUpdate = lv_task_create(lv_update_task, 5000, LV_TASK_PRIO_MID, this);

  UpdateScreen();
}

Tile::~Tile() {
  lv_task_del(taskUpdate);
  lv_obj_clean(lv_scr_act());
}

void Tile::UpdateScreen() {
  lv_label_set_text(label_time, dateTimeController.FormattedTime().c_str());
  statusIcons.Update();
}

void Tile::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED || obj != button) {
    return;
  }

  if (action == Action::CycleBrightness) {
    brightnessController.Step();
    settingsController.SetBrightness(brightnessController.Level());
    lv_label_set_text_static(icon, brightnessController.GetIcon());
    lv_obj_align(icon, nullptr, LV_ALIGN_CENTER, 0, -20);
    return;
  }

  if (launchApp == Apps::Settings) {
    settingsController.SetSettingsMenu(0);
  }
  app->StartApp(launchApp, DisplayApp::FullRefreshDirections::Up);
  running = false;
}
