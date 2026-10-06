# comelit-esphome
Comelit Simplebus Interface for Home Assistant

It works with simplebus 1, simplebus 1 color, simplebus 2.

![render](/images/render_fronte.png) ![render2](/images/render_retro.png)

**Side project:** *There is also a side-project that shares the same hardware: [comelit-esp8266](https://github.com/mansellrace/comelit-esp8266). it is a simplified and unrelated version of the home assistant world, created to allow decoding and interfacing the same protocol, with the same hardware, to interface the comelit bus even different home automation systems, to build intercom call repeaters, etc.*

## Introduction to the project
Initially, I wanted to modify my Comelit mini hands-free intercom 6721W to interface it with Home Assistant, so that I can receive a notification when someone intercoms me, and to be able to open the two doors controlled by the intercom conveniently remotely.

![Comelit mini](/images/comelit_mini.jpg)

The intercom works on Comelit's proprietary Simplebus2 2-wire bus.

I wanted to connect directly to the printed circuit board of the indoor station, which, however, uses the same speaker for ringing and voice calling, has touch buttons, the situation was getting complicated. I discarded the idea of using a Ring Intercom, because although it works very well and supports the Simplebus2 protocol, it does not allow you to control the opening of the second door, is bulky, works only in the cloud, and has the problem of battery power.

I then discovered the wonderful work of **[plusvic](https://github.com/plusvic/simplebus2-intercom)** who analyzed and decoded the simplebus protocol, and made a ring repeater based on a PIC used to decode the protocol, a wireless transmission chip and ESP8266. 
**[aoihaugen](https://github.com/aoihaugen/simplebus2-intercom)** created a fork, and adapted the code to decode the signal on arduino. I want to give a huge thanks to both of you, without your work I would never have reached my goal, I took abundant cues from both of you for hardware and software.

In my implementation I used a Wemos d1 mini with Esphome-based firmware for easy integration on Home Assistant. Can also interface with [Homey Pro](homey.md)

![PCB2](/images/pcb2.jpg) ![PCB](/images/pcb.jpg)

The project makes it possible to receive and send commands that pass through the comelit bus, such as calling an internal intercom, opening a door, etc.

It is not possible to receive and send audio and video stream to home assistant.

You can receive a notification when someone sends a call to your internal intercom, and you can open the external door via home assistant.

You can send any command that the indoor intercom or the outdoor intercom generates.

The project supports switching of comelit expansion relays, allows control of multiple gates, separate triggers for call from external intercom or for out-of-door call, etc.

The pcb is powered directly from the bus, and is to be connected directly in parallel with the indoor intercom.

## Purchase materials and pcb

I can supply the printed circuit board, components, or even the entire hardware already soldered and tested.

If you are interested, please contact me at mansellrace@gmail.com

## [Protocol explanation](protocol.md)

## [Hardware and schematics](hardware.md)

## [External component docs](components/README.md)

## First set-up
Prerequisite: Home Assistant.

Boards supplied by me arrive already flashed. If you assembled your own, install the firmware from the [project page](https://mansellrace.github.io/comelit-esphome/) straight from your browser over USB.

- Connect the pcb to the bus. A wifi network called comelit-default will appear. Connect and open the browser, a page will pop up that allows you to set up your wifi network. If it does not open go [here](http://192.168.4.1)
- Home Assistant will discover the device automatically. Add it from the ESPHome integration. Among its entities you will find an **Intercom address** number, an **Incoming call** binary sensor and an **Open Door** button.
- Find your address: go [here](http://comelit-default.local/), where you will find a log of the commands received from the bus. Press a button on your intercom and note the address it generated.
- Set the **Intercom address** number to that address. The **Incoming call** sensor now fires whenever someone calls your intercom. The address is read at runtime, so there is nothing to recompile.
- The **Open Door** button sends command 16, which in most cases already opens the main gate.
- Firmware updates show up on their own as an update entity in Home Assistant. Press "Install" when one appears.
- Have fun!

Besides the binary sensor, every command received on the bus is also fired as a Home Assistant event, which you can use to trigger automations without adding any entity. [More information here](components/README.md#event)

## Doorbell notification on your phone

[@matzZz](https://github.com/matzZz) built a Home Assistant blueprint on top of this project: when someone rings, the phones you choose get a push notification with an **Open door** button.

[![Open your Home Assistant instance and show the blueprint import dialog with a specific blueprint pre-filled.](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fgithub.com%2Fmansellrace%2Fcomelit-esphome%2Fblob%2Fmain%2Fblueprints%2Fautomation%2Fcomelit_doorbell_door_opener.yaml)

- The door cannot be opened by accident: the button only shows up on a long press, a locked phone asks for Face ID or to be unlocked first, and the button works only for a limited time after the ring (2 minutes by default). Every ring gets its own button, so an old notification opens nothing.
- As soon as someone opens the door, or the time runs out, the notification disappears from every phone. Optionally, everyone gets a message saying who opened.
- It can notify only the phones that are at home.

It only needs two entities the device already has: choose **Incoming call** as the ring sensor and **Open Door** as the door opener.

Requirements: Home Assistant 2025.4 or later, and the Home Assistant Companion app on the phones. To notify only the phones that are at home, location tracking must be enabled in the app.

You can also install it by hand: copy [comelit_doorbell_door_opener.yaml](blueprints/automation/comelit_doorbell_door_opener.yaml) into the `blueprints/automation/` folder of your Home Assistant configuration.

## Customising your device

The stock firmware covers one intercom address and two doors. If you need more than that (several binary sensors, extra buttons, expansion relays, a different sensitivity), you can **adopt** the device:

- Install the "ESPHome Device Builder" add-on. It will show your device with an **ADOPT** button.
- Adopting copies the whole stock configuration into your dashboard, so every entity is there in front of you, ready to be renamed, removed or added to.
- Full list of options: [external component docs](components/README.md).

> [!IMPORTANT]
> Once you adopt the device, its configuration is yours and the automatic update entity goes away. From that point on you update it by recompiling from the ESPHome Device Builder.

## Commands description

An explanation of the commands that can be found on the bus can be found [here](protocol.md#list-of-commands)

## Updates:
- **2023, June**: First hardware revision and first available software
- **2023, August**: Hw version 2.0, first tests with onboard switching regulator, added pads to easily connect to some external pins
- **2023, September**: Hw version 2.2, added protection circuit for power section. Added dynamic resistor in series with power supply.
- **2023, November**: Hw version 2.4, minor adjustments to power section, added possibility of simultaneous bus and usb connection.
- **2023, November**: Thanks to [@monxas](https://github.com/monxas) for creating a box that can be 3d printed :)
- **2024, January**: Hw version 2.5, added ability to adjust reception sensitivity to two levels
- **2024, February**: Release of the new software. Now the project is based on an external component of esphome and configuration is much easier.
- **2024, August**: Hw version 2.6, added compatibility with simplebus 1 and simplebus 1 color. There is also a hardware revision dedicated to intercom kit systems with 2 wires for bus wires and 2 wires for power.
- **2025, April**: Hw version 2.7 specific for Simplebus 1
- **2025, August**: Software release 2025.08. Home Assistant events now require `homeassistant_services: true` inside the `api` configuration.
- **2026, August**: The address of a binary sensor can be a lambda, so it can be driven from a Home Assistant `number` entity and changed at runtime without recompiling anything.
- **2026, August**: Two new entities report what travels on the bus: a `text_sensor` showing the last command as `C50_A10`, and an `event` entity tied to one address that fires with the command number.
- **2026, August**: Boards now ship pre-flashed and keep themselves up to date. Each release is built and published automatically, a **Firmware** update entity shows the new version in Home Assistant, and the device can be adopted in the ESPHome Device Builder to get the whole configuration for editing. Esphome compiler is no longer required for most installations, and the setup is quicker. Blank boards can be flashed from the browser at the [project page](https://mansellrace.github.io/comelit-esphome/).
- **2026, September**: Thanks to [@matzZz](https://github.com/matzZz) for a Home Assistant blueprint that sends a push notification with an "Open door" button when someone rings. [Details here](#doorbell-notification-on-your-phone)
- **2026, October**: Thanks to Eladio for testing the board with Homey Pro. [Homey Pro quick guide](homey.md)
