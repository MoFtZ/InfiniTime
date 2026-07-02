# Alarm snooze — postpone a ringing alarm

**Branch:** working branch `develop`, cut from upstream `v1.16`.

This adds a snooze capability to the alarm. Today a ringing alarm offers only a single choice — stop it — and an alarm left untouched silences itself after a short timeout. This lets the user postpone a ringing alarm for a short interval so it rings again shortly after, the familiar behaviour of bedside and phone alarms, while keeping an unattended alarm from ringing indefinitely.

## Goal

Give the user a way, while an alarm is ringing, to silence it now but have it ring again a few minutes later, without cancelling the alarm or disturbing its recurring schedule. Snoozing should feel native to the existing alarm app: the same ringing screen the user already sees, with a snooze control sitting alongside the existing stop control. An alarm that is simply ignored should also be postponed rather than abandoned, so a user who does not reach the watch in time still gets woken again — but only up to a sensible limit, after which the alarm gives up so a forgotten watch does not ring forever and drain its battery.

## Approach

Snooze is modelled as a short, one-shot rescheduling of the alarm that is currently ringing. When the user snoozes, the alarm stops sounding immediately and is set to ring again after a fixed interval. This reuses the alarm's existing firing path exactly: the postponed alarm fires through the same mechanism as a scheduled alarm, so it re-opens the ringing screen and sounds the same way, with no separate alerting path to maintain.

All snooze state and timing lives in the alarm controller, not in the alarm screen. This is deliberate: the ringing screen can be dismissed and destroyed when the user navigates away after snoozing, so a snooze owned by the screen would be lost the moment the user left the app. Because the controller already owns alarm scheduling, the alerting state, and the single timer that drives alarms, placing the snooze there keeps the feature cohesive and lets the screen remain a thin view that simply asks the controller to snooze.

Two behaviours that already exist are extended rather than replaced. The control the user taps to stop a ringing alarm gains a companion snooze control on the same screen. The existing timeout that silences an untouched ringing alarm is changed so that, instead of dismissing the alarm, it snoozes it — subject to the same limit as a manual snooze.

## The snooze interval

A snoozed alarm rings again after a fixed interval of nine minutes. The value is a single tunable constant with no user-facing setting; nine minutes is the traditional bedside-clock snooze and is chosen only as a familiar default. Making the interval configurable per alarm or globally is intentionally excluded to avoid a settings change; the constant is the seam to revisit if a setting is ever wanted.

## The snooze limit

Snoozing is bounded by a count. Each alarm ringing session — beginning when a scheduled alarm first fires — allows a fixed number of snoozes, after which the alarm can no longer be postponed and will dismiss itself. The limit is three snoozes, again a single tunable constant. Both a manual snooze and an ignored-timeout snooze count against this limit equally, so an unattended alarm rings its initial time plus three postponements and then stops on its own. When the limit is reached, the ringing screen no longer offers the snooze control and the ignore-timeout dismisses rather than snoozes, so the final ring behaves like today's alarm. The snooze count resets whenever a fresh scheduled alarm fires, so each morning's alarm starts with its full allowance.

## User interface

The ringing screen is unchanged except that, while snoozing is still allowed, it presents a snooze control beside the existing stop control. Tapping snooze silences the alarm, records the snooze in the controller, and leaves the ringing screen the same way stopping does — returning the user to the watch face — because the pending snooze now lives in the controller and does not need the screen to stay open. On the final ring, once the snooze limit has been reached, only the stop control is shown, so the user is never offered a snooze that would not be honoured. Stopping an alarm behaves exactly as it does today, whether the alarm reached the screen by its schedule or by a snooze.

## Interaction with recurrence and scheduling

A snooze is a transient, one-shot postponement layered on top of the alarm's normal schedule; it never alters the alarm's recurrence or its enabled state. In particular, a snoozed alarm is not treated as "fired" for the purpose of the rule that disables a non-repeating alarm after it goes off — that disabling happens only when the alarm is finally stopped, not when it is postponed. Once the user stops a snoozed alarm, the ordinary end-of-alarm behaviour applies: a non-repeating alarm disables itself and a repeating alarm is rescheduled to its next selected day, exactly as today.

Because the controller drives all alarms from a single timer, a pending snooze and the regular schedule share that timer. Any event that recomputes the schedule — such as the user enabling or editing another alarm while a snooze is pending — cancels the pending snooze cleanly and the regular schedule resumes. Stopping the alarm likewise clears any snooze state. The intent is that snooze state is always consistent with whatever the controller last decided to do, and never lingers as a stale second timer.

## Constraints

The build must stay green and the change should remain localised to the alarm feature — its controller and its screen — leaving unrelated code untouched to keep the fork mergeable with upstream. The existing stop behaviour, including how a non-repeating alarm disables itself and how a repeating alarm reschedules, must be preserved exactly. The snooze must reuse the existing firing and alerting path rather than introducing a parallel one.

## Out of scope

The snooze interval and the snooze limit are fixed constants, not user settings. There is no per-alarm snooze configuration and no snooze entry in the settings app. Snooze state is held only in memory and is not persisted to flash, so a snooze does not survive a reboot; a watch that restarts mid-snooze simply returns to its normal schedule. The alarm sound, the alarm time entry, the number of alarms, and the recurrence feature are all unchanged.

## Risks and notes

The main risks are in the interaction between snooze and the existing scheduling. A snoozed re-fire must not be swallowed by the logic that disables a non-repeating alarm after it fires, or the second ring would never happen; the design keeps disabling tied to stopping, not to firing, precisely to avoid this. The single shared timer means snooze and the regular schedule must not fight over it — the design resolves this by letting any reschedule cancel the pending snooze rather than running two timers. The not-persisted-across-reboot behaviour is a deliberate simplification and is called out so it is not mistaken for a defect. Finally, because the ignore-timeout now postpones rather than dismisses, the snooze limit is what guarantees an unattended alarm eventually stops; the limit and the timeout behaviour are therefore two halves of one decision and should be kept in sync if either is changed.
