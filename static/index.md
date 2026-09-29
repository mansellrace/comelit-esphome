# About

An ESPHome firmware that connects a Comelit Simplebus 2 intercom to Home Assistant.
The board is powered from the 2-wire bus itself and taps it directly, so no extra wiring is needed.

Boards are shipped pre-flashed with this firmware. You only need the button below if you
are flashing a blank board yourself.

# Installation

You can use the button below to install the pre-built firmware directly to your device via USB from the browser.

<esp-web-install-button manifest="firmware/comelit-default.manifest.json"></esp-web-install-button>

<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>

# After installing

Connect to the `comelit-default` Wi-Fi network the device creates, enter your Wi-Fi
credentials in the captive portal, and Home Assistant will discover the device
automatically. Firmware updates then arrive on their own as an update entity.
Change the intercom address entity to match the address of your intercom, and the
binary sensor will go on when someone calls your intercom.

To get a notification on your phone with a button that opens the door, see the
[Home Assistant blueprint](https://github.com/mansellrace/comelit-esphome#doorbell-notification-on-your-phone).
