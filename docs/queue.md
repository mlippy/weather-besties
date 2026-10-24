# Ideas queue

Things we'd like to add later, roughly in order. Each has a sketch of how it would work so it's quick to pick up.

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

## Known gaps

- **Not yet run on the real board.** See "Not yet checked on the real board" in the README and the checklist in [deploy.md](deploy.md).
- **Browser emulator** (`sim/web/`) is an older build; rebuilding it needs wasi-sdk (`make web`).
- **Calendar:** repeating events with BYSETPOS, BYWEEKNO, BYYEARDAY or hourly repeats aren't understood (Google rarely makes them). Times with a time zone are taken as local to the weather location; UTC times use today's UTC offset, so on the two daylight-saving switch days an event can be an hour off.
