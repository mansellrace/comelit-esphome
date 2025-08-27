# comelit-esphome
Comelit Simplebus Interface for Home Assistant

It works with simplebus 1, simplebus 1 color, simplebus 2.

![render](/images/render_fronte.png) ![render2](/images/render_retro.png)

> [!IMPORTANT]
> ### **Please note**:
>
> for new features introduced in version 2025.08, you need to add the line "homeassistant_services: true"
>
> inside the api configuration, like this
>
>      api:
>        homeassistant_services: true
  

**Big news:**  The hardware design has evolved, and has reached hw revision 2.6. The main changes are in the power supply section, now using a wide-range switching IC directly soldered to the pcb. Now compatible with simplebus 1 and simplebus 1 color. There is also a hardware revision dedicated to intercom kit systems with 2 wires for bus wires and 2 wires for power. More details in the [updates section](#updates)

**Big big news**: Now the project is much easier to set up, the software is based on an external component

**Side project:** *There is also a side-project that shares the same hardware: [comelit-esp8266](https://github.com/mansellrace/comelit-esp8266). it is a simplified and unrelated version of the home assistant world, created to allow decoding and interfacing the same protocol, with the same hardware, to interface the comelit bus even different home automation systems, to build intercom call repeaters, etc.*

## Introduction to the project
Initially, I wanted to modify my Comelit mini hands-free intercom 6721W to interface it with Home Assistant, so that I can receive a notification when someone intercoms me, and to be able to open the two doors controlled by the intercom conveniently remotely.

![Comelit mini](/images/comelit_mini.jpg)

The intercom works on Comelit's proprietary Simplebus2 2-wire bus.

I wanted to connect directly to the printed circuit board of the indoor station, which, however, uses the same speaker for ringing and voice calling, has touch buttons, the situation was getting complicated. I discarded the idea of using a Ring Intercom, because although it works very well and supports the Simplebus2 protocol, it does not allow you to control the opening of the second door, is bulky, works only in the cloud, and has the problem of battery power.

I then discovered the wonderful work of **[plusvic](https://github.com/plusvic/simplebus2-intercom)** who analyzed and decoded the simplebus protocol, and made a ring repeater based on a PIC used to decode the protocol, a wireless transmission chip and ESP8266. 
**[aoihaugen](https://github.com/aoihaugen/simplebus2-intercom)** created a fork, and adapted the code to decode the signal on arduino. I want to give a huge thanks to both of you, without your work I would never have reached my goal, I took abundant cues from both of you for hardware and software.

In my implementation I used a Wemos d1 mini with Esphome-based firmware for easy integration on Home Assistant. Can also interface with Homey Pro

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
Prerequisite: Home Assistant and "ESPHome Device Compiler" add-on installed
- Connect the pcb to the bus. A wifi network called comelit-default will appear. Connect and open the browser, a page will pop up that allows you to set up your wifi network. If it does not open go [here](http://192.168.4.1)
- After configuring wifi go [here](http://comelit-default.local/). There you will find a log where you can view the commands received from the bus and you will find a button that launches command 16 with address 1, in most cases it will already be able to open the main gate.
Try pressing a button on your intercom, on the log you will get what command and what address it generated. Make a note of the address, you will need it later.
- Open your ESPHome Device Compiler page. Create a new device, press skip, select esp8266, press skip, press edit on the newly created device.
Add the following lines to the configuration file at the bottom:

      external_components:
        - source: github://mansellrace/comelit-esphome
  
      comelit_intercom:
- Add the line "homeassistant_services: true" inside the api configuration, like this

      api:
        homeassistant_services: true

- There are two ways to receive commands, by event or by binary sensor.
  - For the standard configuration, a home assistant event is generated for each command received. If you want to use events you don't have to add anything now. You can intercept this event directly on home assistant to trigger an automation.  [More information here](components/README.md#event)
  - You can create a binary sensor that goes to "on" whenever a particular combination of command and address is received. The address is what you discovered in the previous step [More information here](components/README.md#binary-sensor)

      Example of binary sensor config:

        binary_sensor:
          - platform: comelit_intercom
            address: 10  <- Insert your address here

- Transmission of commands can be via home assistant service or via button entity. [More information here](components/README.md#transmit-a-command)

  Example of a button that opens the main door.

      button:
        - platform: template
          name: Open Door
          on_press:
            - comelit_intercom.send:
                command: 16
                address: 1

- Press “Install,” then “Manual Download.” A .bin file will be generated; the process may take a few minutes.
- Go [here](http://comelit-default.local/) and upload the bin file as an ota update.
- Home Assistant will find the new device you created automatically.
If you need to change anything in the code, you can now install the new firmware directly from ESPHome Device Compiler by pressing install -> Wirelessly
- Have fun!

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
