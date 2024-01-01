# PicoLED

The PicoLED project allows you to use a Raspberry Pi Pico W in conjunction with a WS2812 LED strip to generate different LED lighting effects.

## Getting Started

To get started with this project, follow the steps below:

### Flashing the Raspberry Pi Pico W

1. Download the `led.uf2` file from the [Releases](https://github.com/DarkIceXD/PicoLED/releases) section.
2. Connect your Raspberry Pi Pico W to your computer via a micro USB cable while holding the Bootsel button.
3. Flash the led.uf2 file onto the Raspberry Pi Pico W by dragging and dropping it.

### Connecting the Raspberry Pi Pico W and LED Strip

Depending on the power requirements of the LED strip, choose one of the following methods:

#### Option 1: Powering over USB (if within Pi Pico's capabilities)

Connect the LED strip as follows:
- Connect `5V(LED)` to `VBUS(Pi Pico)`
- Connect `GND(LED)` to `GND(Pi Pico)`
- Connect `DIN(LED)` to `GPIO2(Pi Pico)`

Power the Pi Pico using a micro USB cable.

#### Option 2: External Power Supply (recommended)

If an external power supply is required:
- Connect `5V(Power supply)` to both`5V(LED)` and `VSYS(Pi Pico)` 
- Connect `GND(Power supply)` to both `GND(LED)` and `GND(Pi Pico)` 
- Connect `DIN(LED)` to `GPIO2(Pi Pico)`

Then, turn on the power supply.

### Connecting to the Pi Pico

1. Once the Pi Pico powers on, search for available Wi-Fi networks on your phone.
2. Connect to the Wi-Fi network named "PicoLED" (default network name).
3. Use "password" as the password when prompted.

A captcha portal should open, enabling you to configure and apply various lighting effects.

## Contributing

Pull requests and contributions are welcome. For major changes, please open an issue first to discuss the proposed changes.
