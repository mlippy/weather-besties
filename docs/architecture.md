# How it's built

A 4.3-inch touchscreen weather station that tells a kid what to wear. It is one C program on a small microcontroller board: no Linux, no app store, just firmware that fetches a forecast (and any weather alerts) every 15 minutes, reads the family calendar every hour, and draws it.

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

 4.3:  USB-C "UART": CH343P serial chip (flashing, logs)
       USB-C "USB":  native USB, through a USB switch
 4.3B: one USB-C, the chip's native USB (flashing, logs); also 7-36 V DC in,
       battery charger, RTC, isolated I/O (unused). Same pins for everything above.
```

Everything hangs off one ESP32-S3. The panel has no memory of its own, so the chip streams raw pixels at it nonstop. Touch and the IO expander share one I2C bus, and the expander is how the firmware turns on the backlight and resets the touch chip at boot.

On the board but unused so far: microSD slot, RS485, CAN, LiPo charger, sensor header.

## Hardware and software split

It's all one C program, built with Espressif's ESP-IDF framework. The screens and the clothing logic never touch hardware directly. They only talk to LVGL (a C graphics library) and to plain data structs. The board-specific code is a thin layer of about five files.

| Layer | Files / contents | Builds for |
|---|---|---|
| Board glue | `main.c`, `board.c`, `net.c`, `portal.c`, `settings.c` | firmware only |
| Screens (one of two, `ui_select.h`) | classic: `classic/ui.c`, `advice.c` (clothes, special events), `thermo.c` (thermometer); 2.0: Boris's `ui.c`, `outfit.c`, `cat.c` | board, PC, browser |
| Shared logic | `wx.c` (forecast), `alerts.c` (NWS alerts), `cal.c` (iCal reader), `events.c`, `update.c` (OTA manifest), `wb_request.c` | board, PC, browser |
| Artwork + fonts | 49 images, 6 sizes of Fredoka, as C arrays | board, PC, browser |
| Libraries | LVGL 9.5 (graphics), cJSON (parsing) | board, PC, browser |
| Board libraries | GT911 touch driver, DNS server for setup | firmware only |
| ESP-IDF 5.5 | FreeRTOS, WiFi + lwIP, mbedTLS (HTTPS), SNTP clock, HTTP client + server, NVS settings, LCD / I2C / GPIO drivers | firmware only |

Swap the board glue for a different file and the same screens run on a laptop or in a web page, pixel for pixel.

## What runs while it's on

FreeRTOS gives the program tasks, timers and locks, and nothing else. Two tasks do the real work, one per CPU core, and a lock keeps them from touching the screen at the same time.

**Net task (core 0)**

1. Wakes every 15 minutes, or on a Refresh or Update tap. After a failed download it retries in 10 seconds (twice), then every minute, then every 5 minutes. WiFi power saving is off: the board is wall powered, and modem sleep made weak-signal downloads time out.
2. HTTPS GET to `api.open-meteo.com`, and parses about 6 KB of JSON (`wx.c`): today, yesterday, and the next 3 days.
3. HTTPS GET to `api.weather.gov/alerts/active?point=lat,lon` for US National Weather Service alerts (`alerts.c`). It keeps the alerts that matter to a kid, most serious first, and swaps each official name for a calm kid title and line ("Big snow on the way"). The official name only shows in Settings, for grown-ups. If this call fails, the last alerts stay up for about an hour, then clear.
4. Once an hour (or on Refresh, or when the day changes), streams the family calendar's secret iCal address through `cal.c` in 4 KB chunks, so even a calendar with years of history never has to fit in memory. It keeps the kid's events for today and tomorrow, plus countdown events up to 60 days out, working out repeats (weekly soccer, yearly birthdays), skipped and moved instances, and all-day events. If a read fails, the last good list stays, since its events carry their own dates.
5. Around 3 AM, checks for a firmware update (see below).
6. Takes the lock and hands everything to the UI: the forecast, alerts, special events and calendar items, today's and tomorrow's calendar events for My Day (`cal_publish`), and dress-up days (`cal_dress_days`). The classic screens pick clothes in `advice.c` (with reminders for special events, using the forecast for the hour they start); the 2.0 screens in Boris's `outfit.c`.

**Graphics task (core 1)**

1. Reads GT911 touch every 20 ms (swipe, hold).
2. Runs the widgets (`ui.c` on LVGL). Every object LVGL creates lives in PSRAM (`main/lv_mem_psram.c`): with plain `malloc`, the 2.0 screens' hundreds of small objects landed in internal RAM and left WiFi too little to receive with (1.7.0).
3. Draws only what changed, into a drawing area in fast internal RAM (40 lines to start; it grows to 60-100 lines once the first download shows how much RAM WiFi and HTTPS leave, and shrinks back if memory gets tight), then copies it into the 768 KB framebuffer in PSRAM. LVGL refreshes every 10 ms.

Swipes are detected by comparing where the finger went down with where it came up (80+ px, mostly sideways), which catches the slow swipes LVGL's own gesture detection misses; the page flips without a drag animation.

The screen hardware reads the framebuffer over and over by DMA, through 2 small bounce buffers (16 MHz / (820 x 500) clocks per frame, about 39 frames a second), so the CPU only redraws the parts that change.

The clock comes from internet time (SNTP) plus the time-zone offset the forecast reports, which handles daylight saving for free. The advice goes by that clock, not by the time of the last download, so offline it still switches to tomorrow at 7 PM and moves to the next day after midnight.

Every request the board makes has to fit the HTTP client's send buffer (request line plus headers). ESP-IDF's default is 512 bytes and the forecast request needs about 610, which is what broke Boris's v1.1.0 on the real board. The buffers are 2 KB, and `sim/test_request.c` (`make test`) builds every request with the firmware's own code and checks the fit.

## What's on the screen

Two designs implement the same `ui.h`; `components/wb_ui/ui_select.h` picks one at build time, and the build compiles only that one's screens and artwork. The family's board runs 2.0.

**2.0** (Boris's design, described in [REDESIGN_PLAN.md](REDESIGN_PLAN.md), plus this fork's `ui_v2_extras.c`):

| Where | What |
|---|---|
| Today / Tomorrow | The page title (with the name from setup when it fits), the day chip and its **Dress up?** pill (or the "2 of 4 ready" chip once something is checked), the thermometer and weather, Pippa or the dog in the day's outfit, garment tiles and extras to check off, one sentence about the day |
| My Day | Today's and tomorrow's calendar events as cards (icons, NOW / NEXT / DONE), the countdown pill in the header |
| Next days | Three day cards with the outfit and a mini pet |
| Settings (hold an empty spot) | Pet picker, WiFi, town, version, speed test, Refresh, Change WiFi or town, Update |

**Classic:**

| Where | What |
|---|---|
| Top bar | "Good morning, Kid!" (the name from setup), the date and clock |
| Today page, left | Weather picture, temperature, a thermometer (fixed 0-100 °F scale, color by temperature, a tick at freezing, `thermo.c`), conditions, high/low, feels like; under the picture, a small moon (tonight's phase, drawn as two circles, no artwork) with sunrise and sunset |
| Today page, right | The speech bubble: an optional alert bar (calm purple or blue), headline, "than yesterday" line (or the alert's kid line), the tip with any event reminders, and up to 3 clothes |
| Today page, bottom | The next four parts of the day |
| Next-days page (swipe left) | Three day cards, each with a mini thermometer for the high (same scale), a short tip and the change from the day before |
| Footer | A countdown ("11 more sleeps until Kid's birthday!") or else the town; page dots; last update |
| Settings (long-press) | WiFi, town, update time, address, version, data sources, official alert name; Setup, Refresh, Update, Close |

Long text never pushes the layout around: the bubble steps its fonts down until the advice fits.

## Over-the-air updates

`main/ota.c`, with the manifest parsing in `components/wb_ui/update.c` (tested by `sim/test_update.c`):

1. Around 3 AM (once a day, if automatic updates are on) or on the **Update** button, the net task reads `firmware/latest.json` from the repo's `main` branch on `raw.githubusercontent.com`.
2. If its version is newer than `WB_VERSION`, it streams the app file into the other app slot with `esp_ota_write` (4 KB at a time, erasing as it goes), hashing it with SHA-256 on the way. The screen shows the progress.
3. It checks the size and SHA-256 against `latest.json`, lets `esp_ota_end` check the image itself, and checks the new image's own version matches. Any failure: the slot is abandoned and the old version keeps running.
4. It sets the new slot to boot and restarts. The bootloader (built with rollback enabled) starts it **on trial**.
5. The first good weather download calls `esp_ota_mark_app_valid_cancel_rollback()`. If that hasn't happened in 10 minutes, the app restarts, and the bootloader, seeing a version that never proved itself, goes back to the previous one.

Running from PSRAM helps here too: writing the new image to flash doesn't stall the screen. The build stamps `WB_VERSION` into the image (top-level `CMakeLists.txt`), and `tools/release.py` refuses to package a build whose version doesn't match.

## Memory budget

| | Total | Used | Left |
|---|---|---|---|
| Flash | 16 MB | 128 KB system + two 6 MB app slots | 3.9 MB unused; each slot has 1.4 MB spare (2.0) or 2.4 MB (classic) for the app to grow |
| PSRAM | 8 MB | code + constants copied in at boot (3.4 MB classic, 4.5 MB 2.0), framebuffer (768 KB), LVGL's objects | about 3.8 MB (classic) or 2.7 MB (2.0) for WiFi/HTTPS buffers, JSON (alerts can be a few hundred KB) and downloads |
| Internal RAM | 512 KB | 137 KB static; WiFi, the drawing area (grows only while 64 KB stays free) | Boris measured about 78 KB free after a download on v1.4.0. Since 1.7.1 LVGL stays out of it; "Can't reach the weather service" shows how much is free |

The app is the number to watch: every byte of it costs PSRAM as well as flash. Classic is 3.6 MB; 2.0 is 4.6 MB with Pippa's artwork, My Day's icons and the dog's full wardrobe, even compressed (more than the 2.6 MB his plan estimated before Pippa, and past its 3.5 MB guideline, but it fits). The dog's 2.0 sprites are about 420 KB of that; dropping them from a 2.0 build is one line in `components/wb_ui/CMakeLists.txt`.

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
- **Names** (the greeting, the calendar filter words, the dog's name) are typed here and stored only on the board; none are built into the firmware.
- **The calendar link** is treated like the WiFi password: saved to flash, never sent back to the page. The form shows only whether one is saved and how the last read went.
- **The QR code** is the standard WiFi QR format phone cameras already understand, so there's no network name or password to type.
- **Security, plainly.** The hotspot uses WPA2 with a random 10-character password made fresh for each setup, so the home WiFi password and calendar link are encrypted on their way from the phone. Once saved, they sit in the board's flash without encryption: anyone holding the board with a USB cable could read them. That's normal for hobby boards; putting the board on a guest network limits what a leak would expose. Encrypting the stored settings is in the [queue](queue.md), deferred.

## Where it lives in flash

The app is a 4.6 MB image with the 2.0 screens (3.6 MB with the classic ones). Most of it is artwork, fonts and other constant data, not code. Since 1.5.0 the flash has **two app slots**, so the board can download a new version into one while it runs from the other:

```
16 MB flash
|128K| App slot A (ota_0), 6 MB          | App slot B (ota_1), 6 MB          | Free, 3.9 MB |
|    | app 4.6 MB | spare 1.4 MB         | (the next update lands here)      |              |

first 128 KB: bootloader, partition table (0x8000), settings (0x9000, 24 KB),
              which-slot-to-boot (otadata, 0xf000), radio calibration (0x11000)
```

See `partitions.csv` for the exact offsets. The settings stay at `0x9000`, where 1.4.x had them, so a USB install of the four pieces in `firmware/usb/` keeps them. The full image at `0x0` covers that area with blank bytes, which is why it erases them. At boot the code and constants get copied into the 8 MB PSRAM and run from there, so saving settings to flash never makes the screen glitch.

## One screen codebase, three places it runs

Because the screens never touch hardware, the same files (`ui.c`, `advice.c`, `wx.c`, `alerts.c`, `cal.c` + LVGL + artwork + fonts) compile three ways. Only the small glue file changes, and that's what made the screenshots and the browser emulator possible without the board on the desk.

| Target | Toolchain + glue | Output |
|---|---|---|
| Board firmware | Xtensa GCC + `board.c` | 4.6 MB app (2.0) or 3.6 MB (classic); first install as one image at `0x0` |
| Screenshot renderer | GCC + `sim_main.c` | PC program, writes PNGs |
| Browser emulator | clang wasm32 + `emu_main.c` | WebAssembly + canvas, Boris's 2.0 screens (`make web`) |

The calendar reader has its own tests (`sim/test_cal.c`, `make test`), run against a sample family calendar and checked against Google's real US-holidays feed.

The icons start as SVG drawings in a Python script (`tools/icons.py`) and get rendered and packed into RGB565 pixel arrays with an alpha channel. The Fredoka font gets rasterized into bitmap tables at six sizes. Both end up as C files (`tools/make_assets.py`), so the board never needs a filesystem or an image decoder.

---

Built on ESP-IDF 5.5.6, LVGL 9.5.0, cJSON and Espressif's GT911 driver. Pins and panel timing from Waveshare's demo code for this board. Weather from Open-Meteo, alerts from the US National Weather Service, events from the family Google Calendar.
