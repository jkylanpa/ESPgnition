# ESPgnition
ESPgnition — ESP32-S3 powered USB gamepad &amp; SimHub telemetry display for your sim racing rig

sim-ignition-hid
A DIY USB ignition switch / button box for sim racing, built on the LILYGO T-Display-S3 (ESP32-S3). It shows up in Windows as a native USB HID gamepad with 4 buttons, and doubles as a live telemetry display (RPM, speed, gear, lap time, delta) fed by SimHub over the same USB cable — no extra drivers, no PC-side background app required.
![status](https://img.shields.io/badge/status-working-brightgreen)
![platform](https://img.shields.io/badge/platform-ESP32--S3-blue)
Features
Native USB composite device: HID gamepad (4 buttons) + USB CDC serial, both over one USB-C cable
Built-in 1.9" ST7789 display shows live SimHub telemetry: RPM, speed, gear, lap time, delta to session best
Status LED lights up while SimHub is actively sending data
Debounced button inputs
3D-printable enclosure (OpenSCAD, parametric) with an integrated 20x20 V-slot T-slot mount for attaching directly to a sim rig frame
Hardware
LILYGO T-Display-S3 (ESP32-S3-WROOM-1, 1.9" 170x320 ST7789)
4x momentary push buttons (12mm panel mount recommended)
1x 5mm LED + 220–330Ω resistor
M5 bolt + roll-in T-nut (for the 20x20 V-slot mount)
Wiring
Function	GPIO
Button 1	1
Button 2	2
Button 3	11
Button 4	12
Status LED	13
Buttons are wired between their GPIO and GND (internal pull-ups are used in firmware — no external resistors needed for the buttons themselves).
Firmware setup
Arduino IDE configuration
Install the esp32 board package (Boards Manager) and select Board: LilyGo T-Display-S3.
Install the TFT_eSPI library (Library Manager).
In `Arduino/libraries/TFT_eSPI/User_Setup_Select.h`:
Comment out `#include <User_Setup.h>`
Add `#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>`
Board settings (Tools menu):
USB CDC On Boot: Enabled
USB Mode: USB-OTG (TinyUSB)
PSRAM: OPI PSRAM
Flashing
Open `firmware/sim_ignition_hid.ino`, select the correct COM port, and upload as usual. On first boot the device should enumerate in Windows as both:
a USB game controller (visible in `joy.cpl`), and
a COM port
If it doesn't enumerate at all, hold BOOT while plugging in the USB cable to force download mode and confirm the port/cable are working, then re-flash.
SimHub setup
The display is driven entirely by SimHub → Additional Plugins → Custom Serial Devices — no other software needed on the PC side.
Enable the plugin: `Settings → Plugins → Custom serial devices`
Add a new device on the COM port that appeared after flashing, baud rate 115200
Add an Update message with this NCalc formula:
```
   'R:' + format([DataCorePlugin.GameData.NewData.Rpms],'0') + ';S:' + format([DataCorePlugin.GameData.NewData.SpeedKmh],'0') + ';G:' + [DataCorePlugin.GameData.NewData.Gear] + ';L:' + [DataCorePlugin.GameData.NewData.CurrentLapTime] + ';D:' + format(isnull([DataCorePlugin.GameData.NewData.DeltaToSessionBest],0),'0.000') + '\n'
   ```
Make sure the message is Enabled, and set an update rate around 15–20 Hz (higher isn't necessary and can cause flicker).
The firmware parses this as a simple `KEY:value;KEY:value` line terminated with `\n`. Property paths vary slightly between games (iRacing, ACC, ETS2, etc.) — check SimHub's property browser if a field stays blank.
Note: `DeltaToSessionBest` stays null until you've completed a valid clean lap in the session — that field won't update until then.
Repo structure
```
firmware/
  sim_ignition_hid.ino
README.md
```
License
MIT — do whatever you want with it.
