#pragma once

#include <array>
#include <memory>
#include "displayapp/apps/Apps.h"
#include "Screen.h"
#include "ScreenList.h"
#include "displayapp/Controllers.h"
#include "components/brightness/BrightnessController.h"
#include "Symbols.h"
#include "Tile.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class ApplicationList : public Screen {
      public:
        // The user apps (Alarm, Timer, Stopwatch) plus the two built-in launcher items: Settings and Brightness.
        static constexpr size_t nItems = UserAppTypes::Count + 2;

        explicit ApplicationList(DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 const Pinetime::Controllers::Battery& batteryController,
                                 const Pinetime::Controllers::Ble& bleController,
                                 const Pinetime::Controllers::AlarmController& alarmController,
                                 Controllers::DateTime& dateTimeController,
                                 Pinetime::Controllers::BrightnessController& brightnessController,
                                 std::array<Tile::Applications, nItems>&& applications);
        ~ApplicationList() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        DisplayApp* app;
        auto CreateScreenList() const;
        std::unique_ptr<Screen> CreateScreen(unsigned int screenNum) const;

        Controllers::Settings& settingsController;
        const Pinetime::Controllers::Battery& batteryController;
        const Pinetime::Controllers::Ble& bleController;
        const Pinetime::Controllers::AlarmController& alarmController;
        Controllers::DateTime& dateTimeController;
        Pinetime::Controllers::BrightnessController& brightnessController;
        std::array<Tile::Applications, nItems> applications;

        // One item per full-page screen.
        static constexpr int nScreens = nItems;

        ScreenList<nScreens> screens;
      };
    }
  }
}
