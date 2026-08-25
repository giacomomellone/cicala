# Patching Zephyr

The west workspace under `deps/` is gitignored and recreated from pinned
revisions. Local Zephyr changes therefore live in `firmware/patches/` and are
listed in `firmware/patches.yml`.

## Current patch

`zephyr/ssd16xx-preserve-image-on-init.patch` adds two SSD16xx options:

| Option                                   | Behaviour                                                    |
| ---------------------------------------- | ------------------------------------------------------------ |
| `CONFIG_SSD16XX_PRESERVE_IMAGE_ON_INIT`  | Keep controller RAM and skip the initialization refresh      |
| `CONFIG_SSD16XX_PRESERVE_IMAGE_HW_RESET` | Pulse RESET while keeping the RAM clear and refresh disabled |

An ESP32-S3 deep-sleep wake restarts Zephyr while the panel controller remains
powered. Preserving controller RAM lets the first application refresh remain
partial and keeps the RTC refresh counter aligned with the image on the glass.
The driver clears rows 120 and 121 outside its reported 120-row geometry. The
physical panel has 122 rows.

`CONFIG_CICALA_PANEL_DEEP_SLEEP` selects the hardware-reset option because an
SSD1680 in deep sleep ignores SPI until RESET. That combination is disabled
until panel RAM retention across the reset has been measured.

`app/src/sleep.c` requires the preserve-image option at compile time. This turns
a missing patch into a build failure; Kconfig otherwise reports an unknown
symbol as a warning.

## Apply or remove patches

```sh
just fw-patch      # apply firmware/patches.yml
just fw-unpatch    # restore the pinned Zephyr tree
```

`just fw-init` applies patches after `west update`. Run `just fw-patch` again
after later updates.

`fw-unpatch` runs west's clean operation inside `deps/zephyr`. It discards every
local change and untracked file in that checkout. Files in the Cicala repo
are unaffected.

## Add a patch

1. Test the change in `deps/zephyr/`.
2. Save the diff and calculate its checksum:

   ```sh
   git -C deps/zephyr diff -- <paths> > firmware/patches/zephyr/<name>.patch
   shasum -a 256 firmware/patches/zephyr/<name>.patch
   ```

3. Add the patch to `firmware/patches.yml`. `module` is the workspace path,
   such as `deps/zephyr`.
4. Verify a clean application:

   ```sh
   just fw-unpatch
   just fw-patch
   ```

Mark a patch `upstreamable: true` when it should be proposed to Zephyr. Record
the upstream pull request and merge commit in `patches.yml`, then remove the
local patch after the pinned Zephyr revision contains it.

## Refresh timing

`ssd16xx_update_display()` starts a refresh and returns. The driver waits for
BUSY before the next command, so application timing includes a partial refresh
when another RAM write follows but does not include the final full refresh.
Measure full-refresh duration from the BUSY pin. The 15-second application
timeout remains above either refresh mode.
