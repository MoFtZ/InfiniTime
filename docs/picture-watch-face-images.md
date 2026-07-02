# Adding images to the Picture watch face

The **Picture** watch face shows an image above the clock and lets you swipe
left/right to flip between images. It displays whatever image sprites it finds
in the `/images/watchface/` folder on the watch's flash, in filename order, and
falls back to just the clock if that folder is empty.

This guide converts your PNGs into watch sprites and uploads them using
InfiniTime's **built-in resource build** — the same mechanism that ships the
firmware's fonts and icons. The images are committed to the repo alongside
those resources, so a build reproduces the whole gallery.

## 1. Prepare your PNGs

- One subject, centred, bold shapes, minimal fine detail. It sits on a black
  screen and occupies roughly the top 60% of the 240×240 display, so aim for
  about 200×150 px.
- Export as **PNG with alpha (RGBA)** — a transparent or black background both
  work. This fork's image converter uses the full-colour-with-transparency
  format (`CF_TRUE_COLOR_ALPHA`); the 256-colour indexed format is not
  available, so expect roughly ~80–90 KB per image. Dozens still fit easily in
  the ~3.3 MB of flash.
- Put the PNGs in `src/resources/images/watchface/` and commit them there,
  alongside the firmware's other image resources.
- Name the files in the order you want to cycle through them — the watch sorts
  by filename. For example: `01_first.png`, `02_second.png`, `03_third.png`.

## 2. Register each image in the resource list

The resource build reads `src/resources/images.json`. Add one entry per image
and commit it alongside the PNG.

Each entry mirrors the existing `pine_small` entry but targets the watchface
folder. The entry's key becomes the sprite's filename on the watch
(`<key>.bin`), so reuse your ordering prefix in the key:

```json
"01_first": {
   "sources": "images/watchface/01_first.png",
   "color_format": "CF_TRUE_COLOR_ALPHA",
   "output_format": "bin",
   "binary_format": "ARGB8565_RBSWAP",
   "target_path": "/images/watchface/"
}
```

`target_path` places the file at `/images/watchface/01_first.bin` on the watch,
which is exactly where the Picture face looks.

## 3. Build the resource package

Build in the standard `infinitime/infinitime-build` Docker image, which
regenerates the resource package (this is the same `all` build CI runs):

```
/opt/build.sh all
```

The package is written to
`build/output/infinitime-resources-<version>.zip`, now containing your
`/images/watchface/*.bin` sprites alongside the usual fonts and icons.

## 4. Upload to the watch

Flash that `infinitime-resources-<version>.zip` with InfiniLink, the same way
you update the watch's resources. It writes your sprites to
`/images/watchface/`. Open the **Picture** watch face and swipe left/right to
flip between them; your choice is remembered across restarts.

The `/images/watchface/` folder is created automatically the first time you
open the Picture watch face. If your companion app does not create missing
parent folders when uploading, open the Picture face once before flashing so
the folder exists.

**Upload one image first to confirm the flow end-to-end** before generating the
whole set.

## Notes

- To add or remove images later, change the files in
  `src/resources/images/watchface/` and their `images.json` entries, rebuild,
  and re-flash the package. No firmware change or reflash is needed — the face
  discovers whatever is present.
- The face loads up to 32 images. If you add more than that, only 32 will
  appear (which 32 is not guaranteed to follow filename order), so keep the
  folder at or below 32 images.
- Keep any third-party character artwork for personal use only. It is committed
  here as a resource on a personal, family-use fork; the firmware code itself
  references no specific image and shows whatever sprites are present.
