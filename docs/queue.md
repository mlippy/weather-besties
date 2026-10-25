# Ideas queue

Things we'd like to add later, roughly in order. Each has a sketch of how it would work so it's quick to pick up.

## Piper's features on the 2.0 screens

The history is now Boris's commits with Piper's on top, and both designs build (`components/wb_ui/ui_select.h`). The 2.0 screens already get the WPA2 setup, OTA updates, the real-clock fix and Piper's settings. Still classic-only, to bring over once he's finished 2.0 and run it on his board:

- **Special events and calendar reminders:** feed `ui_set_events()` (today a no-op in `ui_v2_extras.c`) into his `outfit.c` sentence and chips ("Soccer today: bring rain gear!"); his bike-bus rule is the same idea for one event.
- **Weather alerts:** a calm strip above his sentence, using `alerts.c`'s kid titles; the official name in his Settings page.
- **Countdowns:** in his day chip or under the tabs.
- **Thermometer:** `classic/thermo.c` moves to the shared folder, placed next to his "33° now"; minis on his forecast cards.
- **Sun and moon:** next to the weather in his top right.
- **Update button:** in his Settings page (nightly updates already work).
- **Size:** 2.0 is 4.0 MB; offer Boris the numbers (his plan estimated 2.6 MB before the Pippa art).

Keep the changes to his `ui.c` as small hooks, so rebases stay easy. The generic pieces (calendar reader, calm alerts, thermometer, WPA2 hotspot, OTA, GT911 build fix, the LVGL format-warning fix) are worth offering him as pull requests.

### Pulling in Boris's updates

```
git fetch upstream            # git remote add upstream https://github.com/boriskozak/weather-besties.git, once
git rebase upstream/main      # Piper's commits replay on his latest
git push --force-with-lease origin main
```

Then build both designs (`ui_select.h`), run `make UI=classic shots`, `make UI=v2 shots` and `make test` in `sim/`, and check the conflicts were only in the shared files listed in the README.

## Next up

### Indoor temperature ("Your room: 68°")

A small sensor on the board's unused sensor header shows the temperature in Piper's room, so she can compare inside and outside ("It's 68° in here and 41° out there: coat!").

- **Hardware:** an I2C temperature + humidity sensor, such as an SHT40, AHT20 or BME280 breakout (a few dollars). It shares the I2C bus the touch chip already uses (GPIO 8/9), so it needs only 4 wires: 3.3 V, ground, SDA, SCL. Check the header pinout against Waveshare's schematic before wiring.
- **Firmware:** read it every minute in `main/board.c`, pass the value to the UI. Keep the reading away from the board itself: the screen and CPU warm it a few degrees, so the sensor should hang off a short cable, not sit on the board.
- **Screen:** one short line, no new layout: e.g. the footer status ("Your room 68°") or next to "Feels like".
- **Advice (optional):** "Warm inside, cold outside: grab your coat on the way out."

### Piper's own touches

- **Favorite color:** a theme choice on the setup page (the theme table is at the top of `components/wb_ui/ui.c`; the pink accent `0xE0457B` is used for headlines and buttons).
- **Favorite animal:** swap the two cats for her favorite animal. The artwork is SVG drawn in `tools/icons.py`; run `python3 tools/make_assets.py` to rebuild the C files.
- **Fun weather fact of the day:** a list of short, kid-friendly facts baked into the firmware ("Snowflakes always have 6 sides!"), one per day, shown on the next-days page title row.
- **Greeting variety:** a few different greetings ("Rise and shine, Piper!") so it doesn't feel the same every morning.

## Also discussed

- **"Remember to bring" reminders:** library book on Tuesday, gym shoes on PE days. Mostly works today as indoor special events or calendar events ("Piper: library book"); could get its own backpack icon.
- **Rain timing:** "Rain starts around 3 PM" instead of only "rainy afternoon". The hourly forecast is already there.
- **Sunscreen:** a sunscreen icon on high-UV days (UV is already used for sunglasses).
- **Bedtime mode:** after a set hour, dim to a soft night-light showing tomorrow's outfit.

## Deferred indefinitely

### Encrypt stored settings (NVS encryption)

**Status: deferred until a real risk calls for it.** Pick this up only if the threat changes, e.g. the board moves somewhere strangers can handle it, or it needs credentials worth more than a guest-network WiFi password.

- **What it fixes:** the WiFi password and calendar link sit in flash as plain text, so anyone holding the board with a USB cable can read them (`esptool read_flash`). Today that's mitigated by putting the board on a guest network, and by **Reset** on the calendar's secret address if the board is lost.
- **How:** ESP-IDF NVS encryption with the HMAC-based key scheme (`CONFIG_NVS_ENCRYPTION`, `CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC`). The key lives in an eFuse key block the CPU can use but not read out. App-only updates keep working; full flash encryption isn't required.
- **Why it's deferred:** burning the eFuse key is permanent and can't be undone. A mistake (wrong key block, wrong purpose) can leave the board unable to read its settings. The cost of getting it wrong is higher than the risk it removes for a board in a kid's room on a guest network.
- **If we do it:** test on a spare board first, write the exact `espefuse` steps into deploy.md, and keep a "wipe settings" path working.

### Signed firmware updates

**Status: deferred.** Today the board installs whatever `firmware/latest.json` on `main` names (if it's newer and its SHA-256 matches), so anyone who can push to the repo can ship firmware to it. That's only the owner today. If collaborators or automation get push access, add ESP-IDF's "signed app images without secure boot" (`CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT`): updates must be signed with a key kept off GitHub. Unlike secure boot or the settings encryption above, this burns nothing permanent on the chip.

## Known gaps

- **Not run on the real board yet:** Piper's additions, and Boris's 2.0 screens (his v1.4.0 base has run). See "On the real board" in the README and the checklist in [deploy.md](deploy.md).
- **Old weather while offline:** the advice follows the clock, but the big temperature and the four parts of the day still show the last download, with only the red "Offline" footer to say so. Boris's 2.0 handles this properly (a "Weather from" chip, and no outfit after 12 hours); it comes with the 2.0 port.
- **Browser emulator** (`sim/web/`) is Boris's, for the 2.0 screens only (rebuilding needs wasi-sdk, `make web`). The classic screens have the PC renderer.
- **Calendar:** repeating events with BYSETPOS, BYWEEKNO, BYYEARDAY or hourly repeats aren't understood (Google rarely makes them). Times with a time zone are taken as local to the weather location; UTC times use today's UTC offset, so on the two daylight-saving switch days an event can be an hour off.
