# Deploying Weather Besties

A checklist for getting the board running in Piper's room. About 20 minutes.

## 1. Before you start

- [ ] The board (Waveshare ESP32-S3-Touch-LCD-4.3) and a USB-C cable that carries **data**, not just power
- [ ] A computer with **Chrome or Edge** (for flashing), and a phone (for setup)
- [ ] The home **2.4 GHz** WiFi name and password (the board can't use 5 GHz-only networks)
- [ ] Your town or ZIP code
- [ ] A USB power adapter (5 V, 1 A or more) and a spot near where Piper gets dressed, within WiFi range

## 2. Get the family calendar link

1. On a computer, open [Google Calendar](https://calendar.google.com), click the **gear**, then **Settings**.
2. On the left under **Settings for my calendars**, click the family calendar.
3. Scroll to **Integrate calendar** and copy **Secret address in iCal format** (it ends in `basic.ics`).
   - Don't see it? Only an owner of the calendar can see it, and some Google Workspace (work or school) accounts turn it off.
4. Keep it somewhere you can paste on your phone during setup (a note to yourself works).

Then, in the calendar:

- [ ] Make sure events meant for Piper have **Piper** in the title ("Piper soccer", "Piper: dentist"). Other events are ignored. You can choose other words during setup (like "Piper, Family").
- [ ] Add **(outside)** or ☀️ to titles that happen outdoors, so the screen reminds her about gloves, rain gear or water.
- [ ] Add **(countdown)**, 🎂 or 🎉 to the big ones ("Piper's birthday 🎂", "Trip to Grandma's 🎉"). These show even without "Piper" in the title.

The secret address is a password for that calendar: anyone with it can read the events. If you ever need to, use **Reset** next to it in Google Calendar, then paste the new one on the board's setup page.

## 3. Flash the firmware

1. Plug the board into the computer using the USB-C port labeled **UART**.
2. Open the [esptool-js web flasher](https://espressif.github.io/esptool-js/), click **Connect**, pick the port.
3. Set **Flash Address** to `0x0`, choose `firmware/weather-besties-v1.3.0.bin`, click **Program**. About a minute.
4. Press **RESET** on the board (or unplug and plug it back in).

If it won't connect: hold **BOOT**, tap **RESET**, let go of BOOT, and try again. If no serial port shows up, try the other USB-C port.

## 4. Set it up from your phone

1. The screen shows a QR code and a name like `WeatherBesties-3F2A`. Scan the QR code with the phone camera (or join that WiFi by hand).
2. A setup page opens (if not, browse to `http://192.168.4.1`). Fill in:
   - [ ] Home WiFi and password
   - [ ] Town or ZIP
   - [ ] Name on the screen: **Piper**
   - [ ] °F or °C, and night hours if you want the screen off at night
   - [ ] **Show weather alerts** (on by default)
   - [ ] Any special events that aren't on the calendar
   - [ ] **Google Calendar:** paste the secret address; **Only events with these words:** `Piper` (or `Piper, Family`)
3. Tap **Save**. The board restarts and joins WiFi. Switch the phone back to home WiFi.

## 5. Check that it works

- [ ] The weather shows, and the town at the bottom left is right
- [ ] The clock shows the right time (it may take a minute after the first weather update)
- [ ] Swipe left shows the next three days; swipe right comes back. **Touch in the wrong place?** See the README, "Not yet checked on the real board".
- [ ] Hold a finger on the greeting for 1.5 s: Settings opens and shows the WiFi, town and version **1.3.0**
- [ ] **Calendar:** put a test event in the family calendar for tomorrow, like "Piper test (outside)". On the board, open Settings and tap the refresh button. It should show in the bubble after 7 PM, or in the morning. To see what the board read, open Settings, tap **Change settings**, and look under the calendar link: "Read at 7:15 PM: 240 events in the calendar, 3 coming up." Then tap Save (or just restart the board) to leave setup.
- [ ] **Countdown:** with a (countdown) event in the next 60 days, the bottom left shows "N more sleeps until ...!"
- [ ] Delete the test event

Google's iCal feed can take a little while to show changes, and the board reads it once an hour. **Refresh** in Settings reads it right away.

## 6. Day to day

- **Screen off at night:** set night hours in setup. A tap wakes it for a minute.
- **Changing things:** hold the greeting for 1.5 s, then **Change settings**. The board makes its setup hotspot again; the calendar link stays saved unless you paste a new one or tick "Stop using the calendar".
- **Weather alerts** show as a calm heads-up for Piper; the official alert name is in Settings. They come every 15 minutes, so they're a heads-up, not a safety alarm.

## 7. Updating the firmware later

There are two images in `firmware/`:

| File | Flash at | What happens to settings |
|---|---|---|
| `weather-besties-vX.Y.Z.bin` | `0x0` | **Erased** (WiFi, town, events, calendar link). Use for a first install or a fresh start. |
| `weather-besties-vX.Y.Z-app.bin` | `0x10000` | **Kept.** Use this for updates. |

Command line: `python -m esptool --chip esp32s3 write_flash 0x10000 weather-besties-v1.3.0-app.bin`

## If something's wrong

| What you see | Try |
|---|---|
| "Can't reach WiFi" | Check the password and that the network is 2.4 GHz. Tap **Set up WiFi** to fix it. |
| "Couldn't find your town" | Use "Town, ST" instead of a ZIP. |
| Calendar status says the link doesn't work | Copy the secret address again (it changes if someone reset it). |
| Calendar events don't show | The title needs a filter word (like Piper) or a tag; it only shows today's and tomorrow's events (countdowns up to 60 days). |
| Touch lands in the wrong place, flickering, dark screen | See "Not yet checked on the real board" in the README. Serial logs: 115200 baud on the UART port. |
