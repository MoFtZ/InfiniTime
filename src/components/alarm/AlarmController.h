/*  Copyright (C) 2021 mruss77, Florian

    This file is part of InfiniTime.

    InfiniTime is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    InfiniTime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <FreeRTOS.h>
#include <timers.h>
#include <array>
#include <cstdint>
#include "components/datetime/DateTimeController.h"

namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class AlarmController {
    public:
      static constexpr uint8_t MaxAlarms = 4;

      // A ringing alarm can be postponed a fixed number of times, each for a
      // fixed interval, before it dismisses itself. Manual snoozes and the
      // ignore-timeout snooze share this same allowance.
      static constexpr uint8_t MaxSnoozes = 3;
      static constexpr uint8_t SnoozeMinutes = 9;

      // Recurrence is a set of weekdays, one bit per day indexed by tm_wday
      // (Sunday = bit 0 ... Saturday = bit 6). An empty set (0) means the
      // alarm fires once at its next occurrence and then disables itself.
      static constexpr uint8_t DaysNone = 0;
      static constexpr uint8_t DaysDaily = 0x7F;                                                     // all seven days
      static constexpr uint8_t DaysWeekdays = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);  // Mon-Fri (0x3E)
      static constexpr uint8_t DaysWeekend = (1 << 0) | (1 << 6);                                    // Sun + Sat (0x41)

      AlarmController(Controllers::DateTime& dateTimeController, Controllers::FS& fs);

      void Init(System::SystemTask* systemTask);
      void SaveAlarm();
      void SetAlarmTime(uint8_t index, uint8_t alarmHr, uint8_t alarmMin);
      void ScheduleAlarm();
      void DisableAlarm(uint8_t index);
      void SetOffAlarmNow();
      uint32_t SecondsToAlarm() const;
      void StopAlerting();
      void Snooze();

      // True while the current alerting session still has snoozes left.
      bool CanSnooze() const {
        return snoozeCount < MaxSnoozes;
      }

      uint8_t Hours(uint8_t index) const {
        if (index >= MaxAlarms) {
          return 0;
        }
        return alarms[index].hours;
      }

      uint8_t Minutes(uint8_t index) const {
        if (index >= MaxAlarms) {
          return 0;
        }
        return alarms[index].minutes;
      }

      bool IsAlerting() const {
        return isAlerting;
      }

      uint8_t AlertingAlarmIndex() const {
        return alertingAlarmIndex;
      }

      bool IsEnabled(uint8_t index) const {
        if (index >= MaxAlarms) {
          return false;
        }
        return alarms[index].isEnabled;
      }

      bool AnyAlarmEnabled() const {
        for (uint8_t i = 0; i < MaxAlarms; i++) {
          if (alarms[i].isEnabled) {
            return true;
          }
        }
        return false;
      }

      void SetEnabled(uint8_t index, bool enabled);

      uint8_t DaysOfWeek(uint8_t index) const {
        if (index >= MaxAlarms) {
          return DaysNone;
        }
        return alarms[index].daysOfWeek;
      }

      void SetDaysOfWeek(uint8_t index, uint8_t daysOfWeek);

    private:
      // Versions 255 is reserved for now, so the version field can be made
      // bigger, should it ever be needed.
      static constexpr uint8_t alarmFormatVersion = 3;

      struct AlarmSettings {
        uint8_t version = alarmFormatVersion;
        uint8_t hours = 7;
        uint8_t minutes = 0;
        uint8_t daysOfWeek = DaysNone;
        bool isEnabled = false;
      };

      bool isAlerting = false;
      bool isSnoozing = false;
      bool alarmChanged = false;
      uint8_t alertingAlarmIndex = 0;
      uint8_t nextAlarmIndex = 0;
      uint8_t snoozeCount = 0;

      Controllers::DateTime& dateTimeController;
      Controllers::FS& fs;
      System::SystemTask* systemTask = nullptr;
      TimerHandle_t alarmTimer;
      std::array<AlarmSettings, MaxAlarms> alarms;
      std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> alarmTime;

      void LoadSettingsFromFile();
      void SaveSettingsToFile() const;
      uint8_t CalculateNextAlarm() const;
      std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> NextFireTime(uint8_t index) const;
    };
  }
}
