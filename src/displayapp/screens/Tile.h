#pragma once

#include <lvgl/lvgl.h>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "components/battery/BatteryController.h"
#include "components/brightness/BrightnessController.h"
#include "displayapp/widgets/PageIndicator.h"
#include "displayapp/widgets/StatusIcons.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class Tile : public Screen {
      public:
        // What tapping the card does: launch the given app, or cycle the backlight brightness in place.
        enum class Action : uint8_t { LaunchApp, CycleBrightness };

        struct Applications {
          const char* icon;
          const char* name;
          Pinetime::Applications::Apps application;
          Action action;
        };

        explicit Tile(uint8_t screenID,
                      uint8_t numScreens,
                      DisplayApp* app,
                      Controllers::Settings& settingsController,
                      const Controllers::Battery& batteryController,
                      const Controllers::Ble& bleController,
                      const Controllers::AlarmController& alarmController,
                      Controllers::DateTime& dateTimeController,
                      Controllers::BrightnessController& brightnessController,
                      const Applications& application);

        ~Tile() override;

        void UpdateScreen();
        void OnButtonEvent(lv_obj_t* obj, lv_event_t event);

      private:
        DisplayApp* app;
        Controllers::Settings& settingsController;
        Controllers::DateTime& dateTimeController;
        Controllers::BrightnessController& brightnessController;

        lv_task_t* taskUpdate;

        lv_obj_t* label_time;
        lv_obj_t* button;
        lv_obj_t* icon;
        lv_obj_t* label;

        Widgets::PageIndicator pageIndicator;
        Widgets::StatusIcons statusIcons;

        Pinetime::Applications::Apps launchApp;
        Action action;
      };
    }
  }
}
