# comelit-esphome
Comelit Simplebus Interface for Home Assistant

![render](/images/render_fronte.png) ![render2](/images/render_retro.png)

**Big news:**  The hardware design has evolved, and has reached hw revision 2.5. The main changes are in the power supply section, now using a wide-range switching IC directly soldered to the pcb. More details in the [updates section](#updates)

**Big big news**: Now the project is much easier to set up, the software is based on an external component

**New features:** *There is also a side-project that shares the same hardware: [comelit-esp8266](https://github.com/mansellrace/comelit-esp8266). it is a simplified and unrelated version of the home assistant world, created to allow decoding and interfacing the same protocol, with the same hardware, to interface the comelit bus even different home automation systems, to build intercom call repeaters, etc.*

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

## [Protocol explanation](protocol.md)

## [Hardware and schematics](hardware.md)

## [External component docs](components/README.md)

## First set-up
Prerequisite: Home assistant and esphome add-on installed
- Connect the wemos to the pc and add it to your esphome configuration.  Add the following lines to the configuration file at the bottom and then reinstall the firmware:
- Connect the pcb to the bus and observe the esphome log. You should see the commands coming through the bus. Try commanding the outside door to open, you will find out what the address of your intercom is.
- There are two ways to receive commands, by event or by binary sensor.
  - For the standard configuration, a home assistant event is generated for each command received. You can intercept this event to trigger an automation.  [More information here](components/README.md#event)
  - You can create a binary sensor that goes to on whenever a particular combination of command and address is received. [More information here](components/README.md#binary-sensor)
- Transmission of commands can be via home assistant service or via button entity. [More information here](components/README.md#transmit-a-command)

## Commands description

An explanation of the commands that can be found on the bus can be found [here](protocol.md#list-of-commands)

## Purchase materials and pcb

I can supply the printed circuit board, components, or even the entire hardware already soldered and tested.

If you are interested, please contact me at mansellrace@gmail.com

## Updates:
- **2023, June**: First hardware revision and first available software
- **2023, August**: Hw version 2.0, first tests with onboard switching regulator, added pads to easily connect to some external pins
- **2023, September**: Hw version 2.2, added protection circuit for power section. Added dynamic resistor in series with power supply.
- **2023, November**: Hw version 2.4, minor adjustments to power section, added possibility of simultaneous bus and usb connection
- **2024, January**: Hw version 2.5, added ability to adjust reception sensitivity to two levels
- **2024, February**: Release of the new software. Now the project is based on an external component of esphome and configuration is much easier.
