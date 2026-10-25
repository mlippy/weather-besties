# Ideas queue

Things we'd like to add later, roughly in order. Each has a sketch of how it would work so it's quick to pick up.

## This fork's features on the 2.0 screens

The history is now Boris's commits with this fork's on top, and both designs build (`components/wb_ui/ui_select.h`); the family's board runs 2.0. It already has the WPA2 setup, OTA updates with an **Update** pill in Settings, the real-clock fix, this fork's settings, the thermometer next to his "33° now", his My Day page fed by this fork's calendar reader (`cal_publish` in `cal.c`) with countdowns in its header, and dress-up days. Still classic-only:

- **Special events and calendar reminders:** feed `ui_set_events()` (today a no-op in `ui_v2_extras.c`) into his `outfit.c` sentence and chips ("Soccer today: bring rain gear!"); his bike-bus rule is the same idea for one event.
- **Weather alerts:** a calm strip above his sentence, using `alerts.c`'s kid titles; the official name in his Settings page.
- **Mini thermometers on his forecast cards:** no free room on those cards today (mini Pippa, temperatures and the garment list fill them); maybe as a thin bar under the high.
- **Sun and moon:** next to the weather in his top right.
- **Size:** 2.0 is about 4.6 MB; offer Boris the numbers (his plan estimated 2.6 MB before the Pippa art).

Keep the changes to his `ui.c` as small hooks, so rebases stay easy. The generic pieces (calendar reader, calm alerts, thermometer, WPA2 hotspot, OTA, GT911 build fix, the LVGL format-warning fix) are worth offering him as pull requests.

### Pulling in Boris's updates

```
git fetch upstream            # git remote add upstream https://github.com/boriskozak/weather-besties.git, once
git rebase upstream/main      # this fork's commits replay on his latest
git push --force-with-lease origin main
```

Then build both designs (`ui_select.h`), run `make UI=classic shots`, `make UI=v2 shots` and `make test` in `sim/`, and check the conflicts were only in the shared files listed in the README.

## Make it obvious what a kid can tap

Done in 1.7.8: one raised look for everything tappable (soft edge, sinks when pressed, bigger tap area), flat status pills, and a pink glow on the next unchecked tile or Routine step after 20 idle seconds (`v2_raise`, `v2_hint_target` in `ui_v2_extras.c`). Still to try if it's not enough:

- **Hand hint:** after the glow, a small hand icon taps the next item once.
- **A one-time "how to" screen:** the pet points at the tiles, the tabs and the Routine checklist, with a big "Got it!" button.
- **A tap sound or a little bounce** on the pet when something is checked.

## The dog

- **Her own heart pose** on 2.0 (today she gets the floating heart Pippa's other outfits get), and maybe a happy tail-wag frame.
- **Her on the classic screens beyond startup and setup:** e.g. next to the speech bubble.

## Next up

### Indoor temperature ("Your room: 68°")

A small sensor on the board's unused sensor header shows the temperature in the kid's room, so they can compare inside and outside ("It's 68° in here and 41° out there: coat!").

- **Hardware:** an I2C temperature + humidity sensor, such as an SHT40, AHT20 or BME280 breakout (a few dollars). It shares the I2C bus the touch chip already uses (GPIO 8/9), so it needs only 4 wires: 3.3 V, ground, SDA, SCL. Check the header pinout against Waveshare's schematic before wiring.
- **Firmware:** read it every minute in `main/board.c`, pass the value to the UI. Keep the reading away from the board itself: the screen and CPU warm it a few degrees, so the sensor should hang off a short cable, not sit on the board.
- **Screen:** one short line, no new layout: e.g. the footer status ("Your room 68°") or next to "Feels like".
- **Advice (optional):** "Warm inside, cold outside: grab your coat on the way out."

### The kid's own touches

- **Favorite color:** a theme choice on the setup page (the theme table is at the top of `components/wb_ui/ui.c`; the pink accent `0xE0457B` is used for headlines and buttons).
- **Favorite animal:** swap the two cats for her favorite animal. The artwork is SVG drawn in `tools/icons.py`; run `python3 tools/make_assets.py` to rebuild the C files.
- **Fun weather fact of the day:** a list of short, kid-friendly facts baked into the firmware ("Snowflakes always have 6 sides!"), one per day, shown on the next-days page title row.
- **Greeting variety:** a few different greetings ("Rise and shine, Kid!") so it doesn't feel the same every morning.

## Also discussed

- **"Remember to bring" reminders:** library book on Tuesday, gym shoes on PE days. Mostly works today as indoor special events or calendar events ("Kid: library book"); could get its own backpack icon.
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

- **On the real board:** the family's board ran the classic screens (1.6.2) and has run 2.0 since 1.7.3. The 2.0 additions since then (dress-up days, the countdown pill) were checked in the PC renderer first. See "On the real board" in the README and the checklist in [deploy.md](deploy.md).
- **Old weather while offline:** the advice follows the clock, but the big temperature and the four parts of the day still show the last download, with only the red "Offline" footer to say so. Boris's 2.0 handles this properly (a "Weather from" chip, and no outfit after 12 hours); on 2.0 Boris's chip does this already (this gap is the classic screens').
- **Browser emulator** (`sim/web/`) is Boris's, for the 2.0 screens only (rebuilding needs wasi-sdk, `make web`). The classic screens have the PC renderer.
- **Calendar:** repeating events with BYSETPOS, BYWEEKNO, BYYEARDAY or hourly repeats aren't understood (Google rarely makes them). Times with a time zone are taken as local to the weather location; UTC times use today's UTC offset, so on the two daylight-saving switch days an event can be an hour off.
