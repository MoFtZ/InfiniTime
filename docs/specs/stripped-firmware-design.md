# Minimal "kids at school" firmware — strip-down design

**Repo:** fork `MoFtZ/InfiniTime`, branch `develop`.

This is the first of two sub-projects for a minimal watch aimed at children at school. This sub-project strips the firmware down to a small, distraction-free watch. A separate later sub-project adds an automatic Bluetooth schedule (the watch enables Bluetooth only during set windows to sync the time, then turns it off) together with a settings lock-down so the schedule cannot be bypassed.

## Goal

Produce a lean, robust watch with nothing on it to distract a child during the school day: it tells the time, and offers an alarm, a timer, and a stopwatch — nothing else. Stripping the rest also makes the firmware smaller, extends battery life, and reduces the Bluetooth attack surface. The time is set from a phone, never on the watch, so there is no on-watch clock-setting screen for a child to tamper with. Bluetooth itself, time sync, and over-the-air firmware updates are kept, because the later schedule feature and ongoing firmware updates depend on them.

## Approach

The work uses InfiniTime's own supported customization points wherever possible and stays shallow: trim the built-in app and watchface lists, stop registering the unwanted Bluetooth services, disable the heart-rate sensor, and remove the on-watch clock-setting screen. Low-level driver code that is merely unused is left compiled in place rather than ripped out.

This shallow approach is preferred over a deeper one that physically removes drivers and service code, because the apps and watchfaces are the large consumers of program space — removing eleven apps and six watchfaces already reclaims most of the available flash — while the leftover driver code is small. Keeping the cuts shallow also keeps risk low and keeps the fork easy to merge with upstream. Going deeper can be revisited later if flash space ever becomes tight.

## What is kept and what is removed

**Apps.** The watch keeps Alarm, Timer, and Stopwatch. It removes Steps, Heart Rate, Music, Paint, Paddle, Twos, Dice, Metronome, Navigation, Calculator, and Weather. This is done through the supported user-app list, so it is a configuration change rather than code surgery.

**Watchfaces.** Only the Digital face is kept; Analog, PineTimeStyle, Terminal, Infineat, CasioStyleG7710, and PrideFlag are removed through the supported watchface list. The Digital face itself is edited to drop the step-count and heart-rate readouts, so it shows only the essentials (time, date, battery, and Bluetooth status).

**Sensors.** The motion sensor stays on so raise-to-wake keeps working. The heart-rate sensor is removed from the interface entirely (no app, no readout on the face) and is never powered, which is the main battery saving. The heart-rate driver code itself is left compiled but unused.

**Bluetooth services.** Time synchronization, over-the-air firmware update, device information, and battery reporting are kept. The music-control, weather, and navigation services are removed by no longer registering them, which trims flash and Bluetooth attack surface. Phone notifications and incoming-call handling are deliberately *not* removed, because the notification display is shared with the find-my-watch and firmware-update-progress features and is woven through many components; removing it would be invasive and would cost those features. Instead, notifications are turned off by default so nothing is displayed, and locking that down — together with disabling Bluetooth during the school day — is handled by the second sub-project. The net effect for the child is the same: nothing pops up on the watch.

**On-watch clock setting.** The "Date & Time" setting screen is removed, along with its entry in the settings menu, since the time comes from the phone and a child should not be able to change it.

**Other settings.** Brightness, display timeout, wake modes, battery information, and the firmware/About screen are kept. Settings entries that belong to removed features — heart-rate, weather, and the style settings of the removed watchfaces — go away with them. The Bluetooth on/off toggle stays for now; restricting access to it belongs to the later lock-down sub-project.

## Constraints

The build must stay green: every removal has to leave no dangling references, so the firmware still compiles and links. Changes should stay as small and localized as possible to keep the fork mergeable with upstream. The Bluetooth core, time sync, and firmware-update path must remain intact, because the schedule feature and future updates rely on them.

## Out of scope

The automatic Bluetooth schedule and the settings lock-down are a separate sub-project. Deep removal of driver and service source code is not done here. No version-string or branding changes are part of this work.

## Risks and notes

Trimming the app and watchface lists is low-risk because it uses the intended mechanism. Editing the Digital watchface is a small, contained interface change. The more involved part is removing the Bluetooth services, which touches where services are registered and any places that react to their messages (for example, music or notification events); these references must all be removed together so nothing is left dangling. With the heart-rate app and face readout gone, the heart-rate sensor is simply never activated, which is what delivers the battery saving even though the driver remains compiled.
