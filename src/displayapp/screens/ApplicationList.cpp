#include "displayapp/screens/ApplicationList.h"
#include "displayapp/screens/Tile.h"
#include "displayapp/DisplayApp.h"
#include <lvgl/lvgl.h>
#include <functional>
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

auto ApplicationList::CreateScreenList() const {
  std::array<std::function<std::unique_ptr<Screen>()>, nScreens> screens;
  for (size_t i = 0; i < screens.size(); i++) {
    screens[i] = [this, i]() -> std::unique_ptr<Screen> {
      return CreateScreen(i);
    };
  }
  return screens;
}

ApplicationList::ApplicationList(DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 const Pinetime::Controllers::Battery& batteryController,
                                 const Pinetime::Controllers::Ble& bleController,
                                 const Pinetime::Controllers::AlarmController& alarmController,
                                 Controllers::DateTime& dateTimeController,
                                 Pinetime::Controllers::BrightnessController& brightnessController,
                                 std::array<Tile::Applications, nItems>&& applications)
  : app {app},
    settingsController {settingsController},
    batteryController {batteryController},
    bleController {bleController},
    alarmController {alarmController},
    dateTimeController {dateTimeController},
    brightnessController {brightnessController},
    applications {std::move(applications)},
    screens {app, settingsController.GetAppMenu(), CreateScreenList(), Screens::ScreenListModes::UpDown} {
}

ApplicationList::~ApplicationList() {
  lv_obj_clean(lv_scr_act());
}

bool ApplicationList::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // Bidirectional ring: swiping off either end of the list returns to the watchface.
  if (event == TouchEvents::SwipeUp && screens.IsLast()) {
    app->StartApp(Apps::Clock, DisplayApp::FullRefreshDirections::Up);
    return true;
  }
  if (event == TouchEvents::SwipeDown && screens.IsFirst()) {
    app->StartApp(Apps::Clock, DisplayApp::FullRefreshDirections::Down);
    return true;
  }
  return screens.OnTouchEvent(event);
}

std::unique_ptr<Screen> ApplicationList::CreateScreen(unsigned int screenNum) const {
  return std::make_unique<Screens::Tile>(screenNum,
                                         nScreens,
                                         app,
                                         settingsController,
                                         batteryController,
                                         bleController,
                                         alarmController,
                                         dateTimeController,
                                         brightnessController,
                                         applications[screenNum]);
}
