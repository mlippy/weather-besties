# Deploying Weather Besties

A checklist for getting the board running in the kid's room. About 20 minutes.

## 1. Before you start

- [ ] The board (Waveshare ESP32-S3-Touch-LCD-4.3 or 4.3B) and a USB-C cable that carries **data**, not just power
- [ ] A computer with **Chrome or Edge** (for flashing), and a phone (for setup)
- [ ] The home **2.4 GHz** WiFi name and password (the board can't use 5 GHz-only networks). **Use your guest or IoT network if your router has one**, see below.
- [ ] Your town or ZIP code
- [ ] A USB power adapter (5 V, 1 A or more) and a spot near where your kid gets dressed, within WiFi range

### Why a guest network

The board saves the WiFi password on its flash without encryption, so anyone who takes the board and plugs in a USB cable could read it. On a guest or IoT network that only gets them onto the guest network, not your computers and phones. Most routers (and mesh systems like eero, Google/Nest Wifi, Orbi, Deco) have a guest network option in their app; turn it on, make sure it's on 2.4 GHz (or "2.4 and 5 GHz"), and give the board that name and password. The board only needs the internet, so the guest network's isolation doesn't get in its way.

## 2. Get the family calendar link

1. On a computer, open [Google Calendar](https://calendar.google.com), click the **gear**, then **Settings**.
2. On the left under **Settings for my calendars**, click the family calendar.
3. Scroll to **Integrate calendar** and copy **Secret address in iCal format** (it ends in `basic.ics`).
   - Don't see it? Only an owner of the calendar can see it, and some Google Workspace (work or school) accounts turn it off.
4. Keep it somewhere you can paste on your phone during setup (a note to yourself works).

Then, in the calendar:

- [ ] Make sure events meant for your kid have their name in the title ("Kid soccer", "Kid: dentist"). Other events are ignored once you set filter words during setup (like "Kid, Family").
- [ ] Add **(outside)** or ☀️ to titles that happen outdoors, so the screen reminds her about gloves, rain gear or water.
- [ ] Add **(dress)** or 👗 to dress-up days ("Picture day (dress)"): on the 2.0 screens that day starts in the dress, when it's mild enough for one. Without it, a **Dress up?** pill on the screen does the same with a tap.
- [ ] Add **(countdown)**, 🎂 or 🎉 to the big ones (on 2.0 they show in the My Day header) ("Kid's birthday 🎂", "Trip to Grandma's 🎉"). These show even without the name in the title.

The secret address is a password for that calendar: anyone with it can read the events. If you ever need to, use **Reset** next to it in Google Calendar, then paste the new one on the board's setup page.

## 3. Flash the firmware

1. Plug the board into the computer: on the 4.3, the USB-C port labeled **UART**; on the 4.3B, its only USB-C port.
2. Open the [esptool-js web flasher](https://espressif.github.io/esptool-js/), click **Connect**, pick the port.
3. Set **Flash Address** to `0x0`, choose `firmware/weather-besties-v1.8.0.bin`, click **Program**. About a minute.
4. Press **RESET** on the board (or unplug and plug it back in).

If it won't connect: hold **BOOT**, tap **RESET**, let go of BOOT, and try again. If no serial port shows up, try the other USB-C port.

## 4. Set it up from your phone

1. The screen shows a QR code, a name like `WeatherBesties-3F2A`, and a password. Scan the QR code with the phone camera; it joins with the password for you (or join that WiFi by hand and type the password). The password is new every time setup starts.
2. A setup page opens (if not, browse to `http://192.168.4.1`). Fill in:
   - [ ] Home WiFi and password (the guest network, if you have one)
   - [ ] Town or ZIP
   - [ ] Name on the screen: your kid's first name (optional; it's stored only on the board)
   - [ ] The dog's name, if you picked the dog (optional)
   - [ ] °F or °C, and night hours if you want the screen off at night
   - [ ] **Show weather alerts** (on by default)
   - [ ] **Routines:** the time to leave for school (8:30 AM to start) and bedtime (8:00 PM)
   - [ ] **Jackets and coats** (2.0): the feels-like cut-offs (45 / 57 / 64 °F to start) and "just a light jacket if the afternoon warms up to" (60 °F)
   - [ ] Any special events that aren't on the calendar
   - [ ] **Google Calendar:** paste the secret address; **Only events with these words:** leave blank to show every event, or your kid's name (or `Name, Family`) to show only theirs
3. Tap **Save**. The board restarts and joins WiFi. Switch the phone back to home WiFi.

## 5. Check that it works

- [ ] The weather shows (2.0: with the thermometer beside the weather icon; classic: the town at the bottom left is right)
- [ ] The clock shows the right time (it may take a minute after the first weather update)
- [ ] 2.0: the tabs at the bottom switch pages (Today, Tomorrow, My Day, Routine). Classic: swipe left for the next three days, right to come back. **Touch in the wrong place?** See the README, "On the real board".
- [ ] Hold a finger still for 1.5 s on the greeting or page title: Settings opens and shows the WiFi, town and version **1.8.0**
- [ ] In Settings, tap **Update**: it should say "Up to date! (version 1.8.0)", then go back to the weather
- [ ] **Calendar:** put a test event in the family calendar for tomorrow, like "Kid test (outside)" (with your kid's name). On the board, open Settings and tap the refresh button. 2.0: it shows on the My Day tab. Classic: it shows in the bubble after 7 PM, or in the morning. To see what the board read, open Settings, tap **Setup**, and look under the calendar link: "Read at 7:15 PM: 240 events in the calendar, 3 coming up." Then tap Save (or just restart the board) to leave setup.
- [ ] **Countdown:** add an all-day event a week or two out titled "Trip to Grandma's 🎉". 2.0: the My Day header shows "N more sleeps until Trip to Grandma's!"; classic: the bottom left does.
- [ ] **Routine (2.0):** the Routine tab shows the morning list before noon and the bedtime list after, with a bar counting down to the times from setup. Tap a step: it ticks green.
- [ ] **Dress-up day (2.0):** add an all-day event for tomorrow titled "Picture day (dress)". If tomorrow is mild enough for a light jacket, the Tomorrow page shows the dress with the pill on **✓ Dress-up**; in cold, rain or heat the weather's outfit wins and there's no pill.
- [ ] Delete the test events

Google's iCal feed can take a little while to show changes, and the board reads it once an hour. **Refresh** in Settings reads it right away.

## 6. Day to day

- **Screen off at night:** set night hours in setup. A tap wakes it for a minute.
- **Dress-up days (2.0):** on a mild day, tap **Dress up?** next to the day to switch that day to the dress; tap again to switch back. Tag calendar events "(dress)" to plan ahead.
- **Changing things:** hold the greeting or page title for 1.5 s, then **Change WiFi or town** (2.0) or **Setup** (classic). The board makes its setup hotspot again; the calendar link stays saved unless you paste a new one or tick "Stop using the calendar".
- **Weather alerts** show as a calm heads-up for your kid; the official alert name is in Settings. They come every 15 minutes, so they're a heads-up, not a safety alarm.

## 7. Updating the firmware later

**Nothing to do: it updates itself over WiFi.** Around 3 AM (or when you tap **Update** in Settings) it checks `firmware/latest.json` in the repo and installs anything newer, keeping all settings. A new version that can't get the weather within 10 minutes is rolled back automatically. Turn "Install updates automatically" off on the setup page if you'd rather only update by tapping **Update**.

USB is only for a fresh start or a rescue:

| Files | Flash at | Settings |
|---|---|---|
| `weather-besties-vX.Y.Z.bin` | `0x0` | **Erased** (WiFi, town, events, calendar link). First install or a fresh start. |
| The four files in `firmware/usb/README.txt` | their addresses | **Kept.** Also the way to move a board from 1.4.x (one app slot) to 1.8.0 without setting it up again. |

## If something's wrong

| What you see | Try |
|---|---|
| Phone won't join the setup hotspot | Type the password shown on the screen; some phones ignore the QR password the first time. |
| "Can't reach WiFi" | Check the password and that the network is 2.4 GHz. Tap **Set up WiFi** to fix it. |
| "Couldn't find your town" | Use "Town, ST" instead of a ZIP. |
| Calendar status says the link doesn't work | Copy the secret address again (it changes if someone reset it). |
| Calendar events don't show | The title needs a filter word (like your kid's name) or a tag; it only shows today's and tomorrow's events (countdowns up to 60 days). |
| Touch lands in the wrong place, flickering, dark screen | See "On the real board" in the README. Serial logs: 115200 baud on the UART port (4.3) or the only USB-C port (4.3B). |
| Update says it didn't finish | It tries again the next night. If a new version keeps rolling back, the log says why (`ota:` lines). |
