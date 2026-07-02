#include "displayapp/screens/WatchFacePicture.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include <cstring>
#include "components/fs/FS.h"

#include "displayapp/screens/NotificationIcon.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

WatchFacePicture::WatchFacePicture(Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   const Controllers::AlarmController& alarmController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::FS& filesystem)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    filesystem {filesystem},
    statusIcons(batteryController, bleController, alarmController) {

  image = lv_img_create(lv_scr_act(), nullptr);
  lv_obj_set_hidden(image, true);

  statusIcons.Create();

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  labelDate = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelDate, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_obj_align(labelDate, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -10);

  labelTime = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelTime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_align(labelTime, labelDate, LV_ALIGN_OUT_TOP_MID, 0, -4);

  labelTimeAmPm = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelTimeAmPm, "");
  lv_obj_align(labelTimeAmPm, labelTime, LV_ALIGN_OUT_RIGHT_TOP, 4, 0);

  filesystem.DirCreate(PicturesDir); // ensure the folder exists (harmless if present)
  LoadPictureList();
  RestoreSelection();
  ShowCurrentPicture();

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFacePicture::~WatchFacePicture() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

bool WatchFacePicture::OnTouchEvent(TouchEvents event) {
  if (pictureCount < 2) {
    return false; // nothing to cycle through
  }
  if (event == TouchEvents::SwipeLeft) {
    currentPicture = (currentPicture + 1) % pictureCount;
  } else if (event == TouchEvents::SwipeRight) {
    currentPicture = (currentPicture + pictureCount - 1) % pictureCount;
  } else {
    return false; // leave up/down swipes and taps to the system
  }
  ShowCurrentPicture();
  SaveSelection();
  return true;
}

void WatchFacePicture::LoadPictureList() {
  pictureCount = 0;

  lfs_dir_t dir;
  if (filesystem.DirOpen(PicturesDir, &dir) != LFS_ERR_OK) {
    return; // folder missing -> no pictures
  }

  lfs_info info;
  while (pictureCount < MaxPictures && filesystem.DirRead(&dir, &info) > 0) {
    if (info.type != LFS_TYPE_REG) {
      continue; // skip "." ".." and subdirectories
    }
    const std::size_t len = std::strlen(info.name);
    // accept only "<name>.bin" that fits our buffer
    if (len < 5 || len >= MaxNameLength || std::strcmp(info.name + len - 4, ".bin") != 0) {
      continue;
    }
    // insertion sort by filename so order is deterministic
    std::size_t pos = pictureCount;
    while (pos > 0 && std::strcmp(pictureNames[pos - 1], info.name) > 0) {
      std::strcpy(pictureNames[pos], pictureNames[pos - 1]);
      pos--;
    }
    std::strcpy(pictureNames[pos], info.name);
    pictureCount++;
  }

  filesystem.DirClose(&dir);
}

void WatchFacePicture::SaveSelection() {
  if (pictureCount == 0) {
    return;
  }
  lfs_file_t file;
  if (filesystem.FileOpen(&file, SelectionFile, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0) {
    return;
  }
  filesystem.FileWrite(&file,
                       reinterpret_cast<const uint8_t*>(pictureNames[currentPicture]),
                       std::strlen(pictureNames[currentPicture]));
  filesystem.FileClose(&file);
}

void WatchFacePicture::RestoreSelection() {
  currentPicture = 0;
  lfs_file_t file;
  if (filesystem.FileOpen(&file, SelectionFile, LFS_O_RDONLY) < 0) {
    return;
  }
  char saved[MaxNameLength] = {};
  const int bytesRead = filesystem.FileRead(&file, reinterpret_cast<uint8_t*>(saved), MaxNameLength - 1);
  filesystem.FileClose(&file);
  if (bytesRead <= 0) {
    return;
  }
  saved[bytesRead] = '\0';
  for (std::size_t i = 0; i < pictureCount; i++) {
    if (std::strcmp(pictureNames[i], saved) == 0) {
      currentPicture = i;
      return;
    }
  }
}

void WatchFacePicture::ShowCurrentPicture() {
  if (pictureCount == 0) {
    lv_obj_set_hidden(image, true);
    return;
  }
  std::snprintf(currentPath, sizeof(currentPath), "%s%s", PictureDrivePrefix, pictureNames[currentPicture]);
  lv_img_set_src(image, currentPath);
  lv_obj_set_hidden(image, false);
  lv_obj_align(image, nullptr, LV_ALIGN_IN_TOP_MID, 0, 8);
}

void WatchFacePicture::Refresh() {
  statusIcons.Update();

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      char ampmChar[3] = "AM";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampmChar[0] = 'P';
      } else if (hour > 12) {
        hour = hour - 12;
        ampmChar[0] = 'P';
      }
      lv_label_set_text(labelTimeAmPm, ampmChar);
      lv_label_set_text_fmt(labelTime, "%2d:%02d", hour, minute);
    } else {
      lv_label_set_text_static(labelTimeAmPm, "");
      lv_label_set_text_fmt(labelTime, "%02d:%02d", hour, minute);
    }
    lv_obj_realign(labelTime);
    lv_obj_realign(labelTimeAmPm);

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(labelDate,
                            "%s %d %s %d",
                            dateTimeController.DayOfWeekShortToString(),
                            day,
                            dateTimeController.MonthShortToString(),
                            year);
      lv_obj_realign(labelDate);
    }
  }
}
