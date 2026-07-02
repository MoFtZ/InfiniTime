# Alarm recurrence — per-day repeat with a day picker

**Branch:** working branch `develop`, cut from upstream `v1.16`.

This redesigns how an alarm's recurrence is chosen. Today each alarm carries one of three fixed recurrence modes — Once, Daily, or Weekdays — cycled through by tapping a single button. This replaces that with a picker that lets the user choose any combination of the seven weekdays, while still surfacing the common patterns (once, every day, weekdays, weekend) as friendly named presets.

## Goal

Let a user repeat an alarm on exactly the days they want — any single day, any mix of days, the weekend, or the existing common patterns — instead of being limited to the three preset modes. The change should feel native to the existing alarm app: the same launcher list, the same per-alarm configuration screen, and the same habit of tapping the recurrence control, now leading to a small day-selection screen rather than cycling a label. Common choices must stay fast and readable, and existing saved alarms must survive the upgrade.

## Approach

Recurrence stops being a fixed enumeration and becomes a set of weekdays. Internally each alarm remembers which of the seven days it should fire on. "Once" is expressed as the empty set — no days selected — which preserves exactly today's one-shot behaviour: the alarm fires at the next occurrence of its time and then disables itself. Selecting any day or days turns the alarm into a repeating alarm on those days.

The recurrence control on the configuration screen no longer cycles through modes. Instead it opens a new day-picker screen with seven day toggles and a live summary. This mirrors how mainstream phones and fitness watches handle alarm repetition, where an empty day selection is understood to mean a single, non-repeating alarm.

This approach is preferred over simply adding more entries to the existing cycle (adding "weekend" and each individual day as extra stops). Cycling through eleven or more states to reach a single day is tedious, and a picker expresses arbitrary combinations that a preset cycle never could. It is also preferred over inline day toggles crammed onto the already-busy configuration screen, which has no comfortable room for seven more controls on a 240-by-240 display; a dedicated screen keeps each view uncluttered.

## The recurrence model

Each alarm stores its recurrence as a set of weekdays, held as a single byte with one bit per day, using the C library's weekday numbering so that scheduling can test a day directly against the current weekday. An empty set means the alarm is non-repeating ("Once"); a full set of all seven days means it repeats every day. This single value replaces the former three-way recurrence mode and is the only piece of per-alarm recurrence state.

## Labelling

The chosen days are summarised as a short human-readable string. The same string is produced by one shared routine and shown in all three places recurrence appears — the launcher list entry, the recurrence button on the configuration screen, and the summary on the day-picker screen — so the three can never disagree. Presets are recognised first, and only selections that match no preset fall through to a listed form:

- No days selected reads as **Once**.
- All seven days reads as **Daily**.
- Exactly Monday through Friday reads as **Weekdays**.
- Exactly Saturday and Sunday reads as **Weekend**.
- One or two other days are shown as three-letter day names, such as "Tue" or "Tue Thu".
- Three or more other days are shown as a fixed seven-slot strip described below.

Presets take precedence deliberately: "Weekdays" and "Weekend" read far more clearly than any letter form, so the named version always wins when the selection matches one exactly. Only genuinely arbitrary mixes, which have no natural name, use the abbreviated or strip forms.

The named presets are shown in title case everywhere. This makes the configuration screen's recurrence button, which today displays its mode in upper case, adopt the same title-case wording as the rest of the app — a small, deliberate consistency change within the code being touched.

## The seven-slot strip

For arbitrary selections of three or more days, a compact form is needed that fits the narrow launcher entry and never becomes ambiguous. Plain concatenated single-letter initials fail on both counts: the standard weekday initials repeat (two days begin with "T", two with "S"), so a lone letter cannot be read reliably, and a spaced list of longer abbreviations can grow wide enough to overflow the launcher entry.

The strip solves this by always showing all seven weekday positions in a fixed Monday-to-Sunday order. Each selected day shows its single-letter initial in its own position; each unselected day shows an underscore placeholder. Because every letter sits in a fixed weekday slot, the repeated initials are no longer ambiguous — position identifies the day — and the string is always exactly seven characters wide, so it can never overflow. For example, Monday-Wednesday-Friday reads as "M_W_F__", and Monday-Wednesday-Saturday is distinct from Monday-Wednesday-Sunday because the set day lands in a different slot.

The underscore is used for the blank slot because it is already part of the compiled font; no font change is required. The whole strip is drawn in the same subdued style the launcher already uses for the recurrence subtitle.

## User interface

**Launcher list.** Unchanged in structure. Each alarm entry continues to show its time, a recurrence subtitle, and an enable switch; the subtitle now shows the summary string described above.

**Configuration screen.** Unchanged except for the recurrence control. The control shows the current summary string, and tapping it opens the day-picker screen instead of cycling to the next mode.

**Day-picker screen.** A new screen with seven day toggles laid out for easy tapping, plus the live summary shown prominently so the current choice — including "Once" when nothing is selected — is always clear. A confirm/back control returns to the configuration screen. Editing the day selection follows the same convention the app already uses when the time is changed: the alarm is disabled until the user re-arms it with the enable switch, so a half-made change never leaves an alarm armed unexpectedly.

## Scheduling

Choosing the next moment an alarm should fire now works uniformly from the day set. The candidate time is the alarm's time today, rolled to the next day if that time has already passed, exactly as today. When the day set is non-empty, the candidate is then advanced day by day until it lands on a selected weekday. When the day set is empty, the alarm keeps today's one-shot behaviour and disables itself after firing.

This removes the special-case handling that currently exists only for the Weekdays mode, where the scheduler nudges the target off Saturday and Sunday. Both places that compute an alarm's next fire time — scheduling the timer and finding the soonest alarm among several — need the same day-set logic, so that shared calculation is factored into a single helper they both use, rather than duplicating it as the current code does.

## Storage and migration

Alarms are saved to the watch's flash with a format version. Because the stored shape of an alarm changes — a day set in place of the former recurrence mode — the format version is incremented, and loading recognises the previous version and converts it: the old Once maps to the empty set, Daily maps to all seven days, and Weekdays maps to the Monday-to-Friday set. Existing saved alarms therefore keep their behaviour across the upgrade. The migration path already present for the older single-alarm format is left intact.

## Constraints

The build must stay green and the change should remain localised to the alarm feature — its controller, its saved-data format, and its screen — leaving unrelated code untouched to keep the fork mergeable with upstream. The one-shot behaviour of a non-repeating alarm must be preserved exactly, including disabling itself after it fires. The summary string must fit the launcher entry in every possible selection.

## Out of scope

No new recurrence concepts beyond a weekly day set: there is no "every other week", no specific-date alarms, and no per-day different times. The number of alarms, the alarm time entry, the alerting and snooze behaviour, and the alarm sound are all unchanged. The choice of first day of week is not made configurable here; the picker and strip use a fixed Monday-first order.

## Risks and notes

The main risks are in the saved-data migration and the scheduling change, since both affect existing behaviour. The migration must map every old value correctly so upgraded watches keep their alarms; the unified scheduling must reproduce the current Once and Weekdays behaviour precisely while adding the new cases. The seven-slot strip's guaranteed fit rests on it always being seven characters in the launcher's font; the widest realistic selections were checked against the launcher entry, and the fixed width removes any per-combination overflow risk. Keeping the label logic in one shared routine is what prevents the launcher, configuration button, and picker summary from drifting apart as the feature evolves.
