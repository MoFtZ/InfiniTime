# Picture watch face — a personalised image beside the clock

**Branch:** working branch `develop`, cut from upstream `v1.16`.

This adds a new watch face that shows a picture the wearer chooses — a cartoon or game character, or any image — above a large digital clock on a plain black screen. It is aimed at personalising the watch for children: each child can flip through a small gallery of images and settle on their favourite, while the time stays big and legible. The picture content lives on the watch's external flash and can be changed without rebuilding the firmware.

## Goal

Give a young wearer a digital watch face that feels like theirs: a favourite character shown prominently, with the time and date still easy to read at a glance. Switching between images should be effortless and playful on the watch itself, and the chosen image should persist across sleep and restarts. Adding new images later should be a matter of putting image files on the watch, not editing or reflashing firmware. The face should behave sensibly even before any images are supplied, and it must not drag in unrelated changes that would complicate keeping the fork close to upstream.

## Approach

The face follows the existing InfiniTime watch-face pattern exactly. It is a self-contained screen registered alongside the other faces and built from the same data the Digital face already uses — time, date, and the battery, Bluetooth and notification status indicators. Everything new is additive: no existing face or shared screen is modified, so the change stays localised and easy to carry alongside upstream merges.

The distinguishing behaviour is that the face displays one image from a gallery held on the external flash, and lets the wearer swipe sideways to move through that gallery. Rather than compiling a fixed list of images into the firmware, the face discovers whatever images are present in a dedicated folder each time it opens. This turns "add or remove a picture" into a content task done from a phone, and keeps the firmware unchanged as the gallery grows.

## Layout and legibility

The screen is black. The chosen image occupies roughly the top sixty percent, and a large, bold digital clock with the date below it sits in the lower portion. The familiar status indicators — battery, Bluetooth connection, and a new-notification marker — sit at the top edge, matching the Digital face.

Legibility, which is the obvious risk when text shares a screen with artwork, is solved structurally rather than with effects: because the background is black and the clock lives in its own region below the image rather than on top of it, the white time and date always sit on plain black and stay crisp. No darkening gradient, outline, or panel behind the text is needed. This is a deliberate simplification made possible by the earlier decision to keep the background black and to give the clock its own space. Note that the PineTime uses a backlit LCD rather than an OLED, so black is chosen for its clean, high-contrast look and easy readability, not for any battery saving.

## Switching images

The wearer changes image by swiping left or right, moving to the previous or next picture in the gallery and wrapping around at the ends — the feel of flipping through a photo album. This gesture was chosen over a tap or an on-screen control set for three reasons: it maps naturally to "next/previous", it is very unlikely to be triggered by accident by a child brushing the screen, and left and right swipes are currently unused on the watch face, so consuming them does not disturb any existing navigation. The reserved gestures the face must leave alone are the upward and downward swipes that open the app launcher, the double-tap that sleeps the watch, and the physical button; the face handles only the sideways swipes and passes everything else through.

The current selection persists so the same picture is shown after the watch sleeps or restarts. The selection is remembered by the picture's file name rather than by a bare position in the list, so that adding, removing, or reordering images does not silently switch a child to a different picture. On opening, the face looks for the remembered file among those present; if it is still there, it is shown, and if it has gone, the face falls back to the first available image. The persistence is handled by the face itself through a small marker it owns, deliberately avoiding any change to the shared settings component and its stored-settings format. This keeps the feature self-contained and the upstream settings untouched; the existing Pride Flag face persists its choice as a fixed enumerated setting, but that approach does not fit a gallery whose members are variable files chosen at runtime.

## Where the images live

Images are read from a single dedicated folder on the external flash — proposed as `/images/watchface/`. The folder name is intentionally generic rather than "characters", because the content is not limited to characters; it is simply the set of pictures this face rotates through, whatever they happen to be. It must be its own folder and not the shared images root, because the root already holds system artwork such as the Pine logo and the navigation icons, which the face must not pick up and try to display.

Each time the face opens, it lists that folder, collects the image files, and orders them by file name. File naming is therefore how the order is controlled — a numeric prefix gives a predictable sequence. Only files in the watch face's own image format are treated as pictures, so the marker used to remember the selection, and any stray non-image file, are ignored by the scan.

If the folder is missing or contains no images, the face still works: it shows the clock, date, and status indicators on black, with no picture. This means the face is usable before any images have been uploaded, and it can never end up in a broken state because of missing content. For the same reason the face always loads rather than being disabled until content exists — which matters especially because it is the watch's default face.

## Image format and capacity

The watch cannot display ordinary image files such as PNGs directly; it shows images in LVGL's compact binary sprite format. Every picture therefore passes through a one-time conversion on a computer, from the supplied image to a sprite sized for the layout, using InfiniTime's existing image-conversion tooling. The sprites are stored full-colour with transparency, so a file's size is set by its dimensions rather than by how flat or detailed the artwork is; the cartoon and blocky game styles intended here simply read well when shrunk to sprite size on the black screen.

Capacity is not a concern. The filesystem region of the external flash is a little over three megabytes, most of it free, and a layout-sized sprite runs to roughly a hundred kilobytes. That comfortably allows dozens of pictures. Only one image is ever active, and the graphics layer streams image data from flash as it draws rather than holding a whole picture in the small working memory, so a large sprite poses no memory problem and matches how InfiniTime's existing flash-stored images already work.

## Getting images onto the watch

The wearer's family supplies the artwork; the images here are produced with an AI image generator. Generated pictures work best when they are a single subject, centred, framed as a bust or full body, on a plain or transparent background, with bold shapes and little fine detail — which both suits the black screen and survives being shrunk to sprite size. Those pictures are converted to sprites and packaged by InfiniTime's existing resource build: each image is added to the resource manifest, and the standard build emits the resources package with the sprites placed in the watch-face folder. No bespoke conversion tool is added: the source images are committed to the repository alongside the firmware's other resources, and their manifest entries sit beside the existing ones.

That resources package is then uploaded to the watch over Bluetooth using the wearer's companion app — InfiniLink on iOS. Whether a companion app can also push individual files to a folder without repackaging varies by app; the resource-package route is the dependable path and is assumed here. Because the face reads whatever is in the folder, the workflow to add a picture is only "convert and upload", never a firmware change.

## Copyright

The pictures the family intends to use depict popular, trademarked characters. Displaying them on a family member's own watch is a personal-use matter and is the wearer's decision. The images are committed to this fork as ordinary resources and are converted and packaged by the same resource build that ships the fonts and icons, so a checkout reproduces the exact gallery and continuous integration can emit a ready-to-flash resources package. They therefore live in the repository's history and release assets like any other resource, which is accepted here given the fork's personal, family-use purpose. The firmware code itself stays content-agnostic: it references no specific image and shows whatever sprites are present.

## Structure and registration

The face is a new screen class in the same place and style as the other watch faces, with the matching trait specialisation, an entry in the watch-face enumeration, inclusion in the user-facing watch-face list, and addition to the build's list of watch-face sources — the same short set of touch-points every watch face already uses, and nothing beyond them. It draws its clock, date, status indicators and image using the standard widgets, refreshes on the usual periodic task, and updates each element only when its underlying value has actually changed, so it stays as light on power and redraws as the existing faces. It reads the image folder through the existing filesystem component, which already provides directory listing.

## Default face

Because this fork's stripped-down firmware removes the on-watch watch-face chooser, there is no menu for switching faces. Picture is therefore set as the firmware's default watch face: a freshly provisioned watch comes up on it, and with no chooser it is effectively the only face the wearer sees. This suits the kids' watch — one fixed, personalised face — and reinforces why the face must always load even with no content, and why any on-watch choice happens within the image gallery rather than through a face menu.

The default is the initial value of the stored watch-face setting, so it applies only when a watch loads its defaults rather than a saved settings file. Because an existing watch keeps its saved settings across a firmware update, the stored settings version is bumped so that flashing this firmware discards the old saved settings and boots on the current defaults, bringing up the Picture face. This resets the other settings to their defaults once, which on this watch are the intended stripped values anyway.

## Constraints

The build must stay green and the change must remain confined to the new face, its registration, the default-face value, and the settings-version bump that makes that default apply, leaving existing faces and other unrelated code untouched, to keep the fork mergeable with upstream. The face must respect the watch's reserved gestures and only consume the sideways swipes. It must never fail because content is missing or malformed: a missing folder, an empty folder, or an unreadable file must degrade gracefully rather than crash or hang.

## Out of scope

The artwork is supplied by the family and is not produced as part of the firmware. There is no on-watch gallery manager, image editor, or uploader — pictures are managed with the existing resource-upload flow. There is no animation, transition effect, or per-image configuration; each picture is a still image and all pictures share the one layout. The choice of clock format follows the watch's existing twelve- or twenty-four-hour setting and is not a new option. No new user-facing setting is added.

## Risks and notes

The main risks are around content the firmware does not control. A file that is not a valid sprite, or is the wrong size, should be skipped or shown as-is without destabilising the face; the design's reliance on graceful degradation is what guards against a bad upload bricking the face. Remembering the selection by file name rather than position is a deliberate choice to avoid silently reassigning a child's picture when the gallery changes, at the cost of a little more care on load to resolve the name to a present file. The uncertainty over whether the iOS companion app can push single files means the smooth "drop one file" experience is not guaranteed on that platform; the resource-package upload is the assured route, and the design does not depend on per-file upload existing. Finally, keeping selection persistence inside the face rather than in shared settings trades a small amount of duplicated bookkeeping for a fully self-contained, upstream-neutral change, which is the right trade given the goal of minimal divergence.
