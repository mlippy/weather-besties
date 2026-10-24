# How it's built

A 4.3-inch touchscreen weather station that tells Piper what to wear. It is one C program on a small microcontroller board: no Linux, no app store, just firmware that fetches a forecast (and any weather alerts) every 15 minutes, reads the family calendar every hour, and draws it.

## The board

It's a Waveshare ESP32-S3-Touch-LCD-4.3: an Espressif ESP32-S3 module soldered behind a 4.3-inch touchscreen, with the support chips a smart display needs. WiFi, a CPU, memory and a screen on one card, powered over USB-C.

| Part | What | Notes |
|---|---|---|
| Chip | ESP32-S3 | 2 cores at 240 MHz, 2.4 GHz WiFi and Bluetooth LE |
| Memory | 512 KB fast RAM | plus 8 MB slower RAM (PSRAM) and 16 MB flash |
| Screen | 800 x 480 IPS | 16-bit color (RGB565) over a parallel RGB bus |
| Touch | GT911 controller | 5-point capacitive, talks over I2C |

```
                    Home WiFi (2.4 GHz) -> Open-Meteo, weather.gov, Google Calendar, NTP
                              |
 +----------------------------+-----------------------------+
 | ESP32-S3 module                                          |
 | 2 cores @ 240 MHz, 512 KB RAM, 8 MB PSRAM, 16 MB flash   |
 | runs everything: screens, WiFi, forecast, setup hotspot  |
 +----+----------------------+------------------+-----------+
      | 16 data + 4 sync     | I2C (GPIO 8/9)   | INT (GPIO 4)
      | 16 MHz pixel clock   |                  |
 +----+-------------+   +----+-------------+  +-+-----------+
 | 4.3" IPS panel   |   | CH422G expander  |--| GT911 touch |
 | 800x480, RGB565  |   | backlight, resets|  | 5-point     |
 +------------------+   | USB / CAN switch |  +-------------+
                        +------------------+   (reset line)

 USB-C "UART": CH343P serial chip (flashing, logs)
 USB-C "USB":  native USB, through a USB switch
```

Everything hangs off one ESP32-S3. The panel has no memory of its own, so the chip streams raw pixels at it nonstop. Touch and the IO expander share one I2C bus, and the expander is how the firmware turns on the backlight and resets the touch chip at boot.

On the board but unused so far: microSD slot, RS485, CAN, LiPo charger, sensor header.

## Hardware and software split

It's all one C program, built with Espressif's ESP-IDF framework. The screens and the clothing logic never touch hardware directly. They only talk to LVGL (a C graphics library) and to plain data structs. The board-specific code is a thin layer of about five files.

| Layer | Files / contents | Builds for |
|---|---|---|
| Board glue | `main.c`, `board.c`, `net.c`, `portal.c`, `settings.c` | firmware only |
| Screens + logic | `ui.c`, `advice.c` (clothes, special events), `wx.c` (forecast), `alerts.c` (NWS alerts), `cal.c` (iCal reader) | board, PC, browser |
| Artwork + fonts | 49 images, 6 sizes of Fredoka, as C arrays | board, PC, browser |
| Libraries | LVGL 9.5 (graphics), cJSON (parsing) | board, PC, browser |
| Board libraries | GT911 touch driver, DNS server for setup | firmware only |
| ESP-IDF 5.5 | FreeRTOS, WiFi + lwIP, mbedTLS (HTTPS), SNTP clock, HTTP client + server, NVS settings, LCD / I2C / GPIO drivers | firmware only |

Swap the board glue for a different file and the same screens run on a laptop or in a web page, pixel for pixel.

## What runs while it's on

FreeRTOS gives the program tasks, timers and locks, and nothing else. Two tasks do the real work, one per CPU core, and a lock keeps them from touching the screen at the same time.

**Net task (core 0)**

1. Wakes every 15 minutes, or on a Refresh tap.
2. HTTPS GET to `api.open-meteo.com`, and parses about 6 KB of JSON (`wx.c`): today, yesterday, and the next 3 days.
3. HTTPS GET to `api.weather.gov/alerts/active?point=lat,lon` for US National Weather Service alerts (`alerts.c`). It keeps the alerts that matter to a kid, most serious first, and swaps each official name for a calm kid title and line ("Big snow on the way"). The official name only shows in Settings, for grown-ups. If this call fails, the last alerts stay up for about an hour, then clear.
4. Once an hour (or on Refresh, or when the day changes), streams the family calendar's secret iCal address through `cal.c` in 4 KB chunks, so even a calendar with years of history never has to fit in memory. It keeps Piper's events for today and tomorrow, plus countdown events up to 60 days out, working out repeats (weekly soccer, yearly birthdays), skipped and moved instances, and all-day events. If a read fails, the last good list stays, since its events carry their own dates.
5. Picks clothes (`advice.c`), including reminders for special events (from the setup page and the calendar) using the forecast for the hour they start, then takes the lock to hand everything to the UI.

**Graphics task (core 1)**

1. Reads GT911 touch every 20 ms (swipe, hold).
2. Runs the widgets (`ui.c` on LVGL).
3. Draws only what changed, in 2 strips of 20 rows in fast RAM, then copies them into the 768 KB framebuffer in PSRAM.

The screen hardware reads the framebuffer over and over by DMA, through 2 small bounce buffers (16 MHz / (820 x 500) clocks per frame, about 39 frames a second), so the CPU only redraws the parts that change.

The clock comes from internet time (SNTP) plus the time-zone offset the forecast reports, which handles daylight saving for free.

## What's on the screen

| Where | What |
|---|---|
| Top bar | "Good morning, Piper!", the date and clock |
| Today page, left | Weather picture, temperature, conditions, high/low, feels like; under the picture, a small moon (tonight's phase, drawn as two circles, no artwork) with sunrise and sunset |
| Today page, right | The speech bubble: an optional alert bar (calm purple or blue), headline, "than yesterday" line (or the alert's kid line), the tip with any event reminders, and up to 3 clothes |
| Today page, bottom | The next four parts of the day |
| Next-days page (swipe left) | Three day cards with a short tip and the change from the day before |
| Footer | A countdown ("11 more sleeps until Piper's birthday!") or else the town; page dots; last update |
| Settings (long-press) | WiFi, town, update time, address, version, data sources, official alert name; Change settings, Refresh, Close |

Long text never pushes the layout around: the bubble steps its fonts down until the advice fits.

## The setup hotspot

The board has no keyboard, so setup happens on a phone. On first boot (or from Settings) the board stops being a WiFi client and becomes a small password-protected (WPA2) WiFi network called `WeatherBesties-XXXX`, the last 4 characters coming from its radio's hardware address. The password is new for every setup and shown on the screen. Three small servers on the board (hotspot + DHCP, DNS, web), all at `192.168.4.1`, then get the phone to open the setup page automatically, the same way hotel and airport WiFi do.

| Step | Phone | Board |
|---|---|---|
| 1 | Scans the QR code on screen: `WIFI:T:WPA;S:WeatherBesties-3F2A;P:kite7mango;;` | |
| 2 | Joins the hotspot with that password | DHCP: "you're 192.168.4.2, setup page at .4.1" |
| 3 | "Am I online?" Looks up `captive.apple.com` | DNS: `192.168.4.1`, for every name asked |
| 4 | `GET /hotspot-detect.html` | `302` redirect to `http://192.168.4.1/` |
| 5 | Pops a sign-in sheet, `GET /` | Form with nearby networks |
| 6 | `POST /save` | Saves to flash, restarts in 1.5 s |
| 7 | | Reboots as a normal WiFi client, joins home WiFi, finds the town, fetches the forecast |

The DNS and redirect answers are the whole trick. A DNS server that answers every lookup with the board's own address, plus a web server that redirects any unknown page to the form, makes the phone's connectivity check fail in exactly the way that pops a sign-in sheet. The DHCP reply also carries the setup address directly (the newer "captive portal" DHCP option), for phones that look for it.

- **Scanning while hosting.** The radio runs as hotspot and client at once, so the client side can scan for nearby networks to list on the form.
- **Special events** are edited on the same form: a name, the weekdays it repeats (or one date), the start hour, whether it's outside, and whether to count down to it. Up to 8, saved to flash with the other settings.
- **The calendar link** is treated like the WiFi password: saved to flash, never sent back to the page. The form shows only whether one is saved and how the last read went.
- **The QR code** is the standard WiFi QR format phone cameras already understand, so there's no network name or password to type.
- **Security, plainly.** The hotspot uses WPA2 with a random 10-character password made fresh for each setup, so the home WiFi password and calendar link are encrypted on their way from the phone. Once saved, they sit in the board's flash without encryption: anyone holding the board with a USB cable could read them. That's normal for hobby boards; putting the board on a guest network limits what a leak would expose. Encrypting the stored settings is in the [queue](queue.md), deferred.

## Where it lives in flash

The whole program is a single 3.6 MB image, flashed at `0x0` (or just the app, 3.6 MB at `0x10000`, for updates that keep settings). Two thirds of it is artwork, fonts and other constant data, not code.

```
16 MB flash
|64K| App slot, 7 MB                                  | Free, 8.9 MB          |
|   | code 1.1 MB | artwork + data 2.4 MB | spare 3.5 MB|                       |

first 64 KB: bootloader (21 KB), partition table, settings (24 KB), radio calibration
```

See `partitions.csv` for the exact offsets. The full image covers the settings area with blank bytes, which is why flashing it at `0x0` erases saved settings and the app-only image doesn't. At boot the code and constants get copied into the 8 MB PSRAM and run from there, so saving settings to flash never makes the screen glitch.

## One screen codebase, three places it runs

Because the screens never touch hardware, the same files (`ui.c`, `advice.c`, `wx.c`, `alerts.c`, `cal.c` + LVGL + artwork + fonts) compile three ways. Only the small glue file changes, and that's what made the screenshots and the browser emulator possible without the board on the desk.

| Target | Toolchain + glue | Output |
|---|---|---|
| Board firmware | Xtensa GCC + `board.c` | 3.6 MB image, flashed at `0x0` |
| Screenshot renderer | GCC + `sim_main.c` | PC program, writes PNGs |
| Browser emulator | clang wasm32 + `emu_main.c` | WebAssembly + canvas (the checked-in build is older; `make web` refreshes it) |

The calendar reader has its own tests (`sim/test_cal.c`, `make test`), run against a sample family calendar and checked against Google's real US-holidays feed.

The icons start as SVG drawings in a Python script (`tools/icons.py`) and get rendered and packed into RGB565 pixel arrays with an alpha channel. The Fredoka font gets rasterized into bitmap tables at six sizes. Both end up as C files (`tools/make_assets.py`), so the board never needs a filesystem or an image decoder.

---

Built on ESP-IDF 5.5.6, LVGL 9.5.0, cJSON and Espressif's GT911 driver. Pins and panel timing from Waveshare's demo code for this board. Weather from Open-Meteo, alerts from the US National Weather Service, events from the family Google Calendar.
