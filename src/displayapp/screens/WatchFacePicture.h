#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/BleController.h"
#include "displayapp/widgets/StatusIcons.h"
#include "utility/DirtyValue.h"
#include "displayapp/apps/Apps.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class AlarmController;
    class NotificationManager;
    class FS;
  }

  namespace Applications {
    namespace Screens {

      class WatchFacePicture : public Screen {
      public:
        WatchFacePicture(Controllers::DateTime& dateTimeController,
                         const Controllers::Battery& batteryController,
                         const Controllers::Ble& bleController,
                         const Controllers::AlarmController& alarmController,
                         Controllers::NotificationManager& notificationManager,
                         Controllers::Settings& settingsController,
                         Controllers::FS& filesystem);
        ~WatchFacePicture() override;

        void Refresh() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        static constexpr const char* PicturesDir = "/images/watchface";
        static constexpr const char* PictureDrivePrefix = "F:/images/watchface/";
        static constexpr std::size_t MaxPictures = 32;
        static constexpr std::size_t MaxNameLength = 40; // must stay < littlefs name_max (50)

        char pictureNames[MaxPictures][MaxNameLength] = {};
        char currentPath[64] = {};
        std::size_t pictureCount = 0;
        std::size_t currentPicture = 0;

        void LoadPictureList();
        void ShowCurrentPicture();

        static constexpr const char* SelectionFile = "/images/watchface.sel";
        void SaveSelection();
        void RestoreSelection();

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime {};
        Utility::DirtyValue<bool> notificationState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;

        lv_obj_t* image;
        lv_obj_t* labelTime;
        lv_obj_t* labelTimeAmPm;
        lv_obj_t* labelDate;
        lv_obj_t* notificationIcon;

        Controllers::DateTime& dateTimeController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::FS& filesystem;

        lv_task_t* taskRefresh;
        Widgets::StatusIcons statusIcons;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Picture> {
      static constexpr WatchFace watchFace = WatchFace::Picture;
      static constexpr const char* name = "Picture";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFacePicture(controllers.dateTimeController,
                                             controllers.batteryController,
                                             controllers.bleController,
                                             controllers.alarmController,
                                             controllers.notificationManager,
                                             controllers.settingsController,
                                             controllers.filesystem);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
