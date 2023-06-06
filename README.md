# PicoLED

This project enables you to utilize a Raspberry Pi Pico W along with a WS2812 LED strip to create various LED lighting effects.
To begin, flash your Raspberry Pi Pico W with the led.uf2 file available in the releases section.

Next, connect the Raspberry Pi Pico W and the LED strip.
Depending on the power requirements of the LED strip, you have two options. If the power draw is within the Pi Pico's capabilities, directly connect the LED strip as follows: connect 5V(LED) to VBUS(Pi Pico), GND(LED) to GND(Pi Pico), and DIN(LED) to GPIO2(Pi Pico). Power the Pi Pico using a micro USB cable.
Alternatively, if an external power supply is needed, connect 5V(LED) to both VSYS(Pi Pico) and 5V(Power supply), GND(LED) to both GND(Pi Pico) and GND(Power supply), and DIN(LED) to GPIO2(Pi Pico). Then, turn on the power supply.

Once the Pi Pico powers on, you can connect to the access point it creates using your phone. The default credentials are "PicoLED" for the network name and "password" for the password. After connecting, a captcha portal should open, allowing you to configure and apply various lighting effects.
