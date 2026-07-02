# Clock persistence — keep the time across reboots and OTA updates

**Branch:** working branch `develop`, cut from upstream `v1.16`.

The watch has no battery-backed real-time clock, so the current time lives only in RAM. To keep the time across a restart, the firmware saves the last known time to a small backup in an uninitialised RAM region — the `.noinit` section, which normal startup does not clear — and restores it early in boot. On this fork that restore was silently failing on every reboot: the clock always fell back to the firmware's build date. This makes the backup actually survive a reboot and an over-the-air update, and adds a trust check so a damaged backup is never restored as a bogus time.

## Goal

Two things. First, the displayed time should survive a normal reboot and an OTA firmware update, coming back to what it was rather than resetting to the build date. Second, only a trustworthy backup should be restored: if the backup is missing, corrupt, or implausible, the clock should fall back to the compile-time default instead of showing a wrong time. Both should be achieved with a change confined to the boot-time restore path and the linker layout, leaving the rest of the firmware untouched.

## Why the clock was resetting

The backup lives in the `.noinit` section, which the linker by default places immediately after the application's zero-initialised data. On a watch running the MCUBoot bootloader, MCUBoot runs first on every boot and uses the low part of RAM as its own working memory, zeroing that memory as it goes. The default placement of `.noinit` falls inside the region MCUBoot uses, so the backup — including the magic marker that says the backup is valid — is wiped on every boot and the restore check always fails. This is why a full upstream build does not show the problem: its larger data footprint pushes `.noinit` above the bootloader's region, whereas this stripped-down fork's smaller footprint leaves `.noinit` inside it.

## Pinning the backup above the bootloader's RAM

The fix is to place the backup at a fixed address just above the memory the bootloader uses, rather than letting it trail the application's data. MCUBoot's working memory reaches to roughly `0x20006424`; the backup is pinned a little above that, at `0x20007000`, which leaves margin above the bootloader's ceiling and sits well below the bootloader's stack near the top of RAM — in memory the bootloader never touches. The value therefore survives across a reboot.

This pin is applied only to the linker script for the bootloaded image. The standalone image is left unpinned deliberately: nothing runs before it that could clobber the region, so it has no need of the fix.

Because the address is now fixed instead of following the application's data, the backup also keeps the same location from one firmware build to the next, so it survives an OTA update as well as a plain reboot.

The pin is kept deliberately low — just above the bootloader's ceiling. This fork runs a single heap that fills all free RAM from just above the backup up to the stack, so the backup sits below the heap and any gap left between the bootloader's ceiling and the pin would become dead RAM subtracted from the heap. Placing the pin as low as is safe keeps the heap as large as possible; at `0x20007000` the heap still has roughly 35 kB. If the application's data ever grows past the pin, the link fails rather than silently overlapping, which is the signal to raise the address and re-check heap headroom.

## Validating the backup before restoring it

Surviving the reboot is necessary but not sufficient: an uninitialised or partially written region could still be read as a plausible-looking but wrong time. So the backup is restored only when three independent checks all pass. A magic word marks the region as written by this firmware. A checksum — a small FNV-1a hash of the stored time, written next to the backup whenever the time is saved and recomputed on boot — confirms the stored time was not partially written or corrupted. A range check confirms the time is plausible: at or after the firmware's build-time default and less than about a hundred years beyond it. If any check fails, the clock keeps the compile-time default it already holds. The checksum is a corruption detector, not a security measure.

## Constraints

The change stays close to upstream: it touches only the bootloaded image's linker script, the boot-time restore decision, and the periodic save that writes the backup. Heap headroom must stay healthy, which is why the pin is placed as low as is safe. The behaviour of the clock in every other respect is unchanged.

## Out of scope

This does not add battery-backed real-time-clock hardware, and it does not store the backup in flash — the backup lives only in RAM, so a full power loss still loses the time and the clock returns to the build default. There is no user-facing setting; the addresses, the checksum, and the plausibility window are fixed design values, not options.

## Risks and notes

The two halves of the design are coupled to the bootloader's memory map. The pin must stay above the bootloader's working-memory ceiling but low enough not to waste heap, and both facts should be re-verified if the bootloader or the application's memory footprint changes; the link-time failure on overlap is the built-in guard against the footprint growing past the pin. The validation checks are what guarantee a wrong value is never shown — the magic word alone is not enough, which is why the checksum and range check are layered on top.
