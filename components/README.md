Comelit Intercom component
===================

> [!IMPORTANT]
> ### **Please note**:
>
> for new features introduced in version 2025.08, you need to add the line "homeassistant_services: true"
>
> inside the api configuration, like this
>
>      api:
>        homeassistant_services: true

The ``comelit_intercom`` allows you to easily interface the esphome-comelit project on esphome and home assistant

Basic esphome configuration entry:

    external_components:
      - source: github://mansellrace/comelit-esphome

    comelit_intercom:

 
Example of a configuration entry, which generates a binary_sensor entity that goes to on when receiving a call to internal intercom 10, and a button entity that when pressed sends the command to open external door

    external_components:
      - source: github://mansellrace/comelit-esphome
      
    comelit_intercom:
    
	binary_sensor:
	  - platform: comelit_intercom
	    address: 10
	button:
	  - platform: template
	    name: Open Door
	    on_press:
	      - comelit_intercom.send:
	          command: 16
	          address: 10

More configuration examples are available at this link: (under construction)


Configuration variables:
------------------------

- **hw_version** (*Optional*, string): The hardware version of the pcb. One of  ``older`` or ``2.5``. Defaults to ``2.5``.
- **sensitivity** (*Optional*, string): The level of receiving sensitivity.
  - For hardware version 2.5, the allowed values are ``high``  and  ``low`` .
  - For older hardware version, it is not possible to change the sensitivity.
- **rx_pin** (*Optional*, pin): The pin used to receive commands. Defaults to ``D6``.
- **tx_pin** (*Optional*, pin): The pin used to transmitt commands. Defaults to ``D1``.
- **filter** (_Optional_,  [Time](https://esphome.io/guides/configuration-types#config-time)): Filter any pulses that are shorter than this. Useful for removing glitches from noisy signals. Defaults to `500us`. 
-   **idle**  (_Optional_,  [Time](https://esphome.io/guides/configuration-types#config-time)): The amount of time that a signal should remain stable (i.e. not change) for it to be considered complete. Defaults to  `10ms`.
-   **buffer_size**  (_Optional_, int): The size of the internal buffer for storing the remote codes. Defaults to  `400b`.
-  **dump** (_Optional_, bool): If set to on, the timing of received signals is printed on the log. Useful for debugging. If you also set the logger to "VERBOSE" you will also see the raw data received. Defaults to  `false`.
- <a id="eventlist">**event**</a>  (_Optional_, string): The name of the event that will be generated on home assistant when receiving a command from the bus. For example, if  set to `comelit`, the event generated will be "esphome.comelit".
Read more about how to use it in the [event section](#event)
Default to `comelit`.
If this parameter is set to `none` no event will be generated.


Binary sensor
===================

You can configure binary sensors that go on when a particular combination of command and address is received from the bus.

You can also set only the address, in this case the default command is 50, which occurs when a call is made from the outside intercom to the inside intercom.


Configuration examples:

	binary_sensor:
	  - platform: comelit_intercom
	    address: 16
	  - platform: comelit_intercom
	    command: 29
	    address: 1
	    name: Internal Door opened
	    auto_off: 60s

- **address** (**Required**, int): The address that when received sets the sensor to on .
- **command** (*Optional*, int): The command that when received sets the sensor to on . Defaults to  `50`.
- **auto_off** (*Optional*,  [Time](https://esphome.io/guides/configuration-types#config-time)):  The time after which the sensor returns to off. If set to `0s` the sensor once it goes on, it stays there until it is turned off by an automation. Defaults to  `30s`.
- **icon** (*Optional*, icon): Manually set the icon to use for the sensor in the frontend. Default to `mdi:doorbell`.
- **id** (*Optional*, string): Manually specify the ID for code generation.
- **name** (*Optional*, string): The name for the sensor. Default to `Incoming call`.

    Note:
    If you have friendly_name set for your device and you want 
    the sensor to use that name, you can set `name: None`.



Event
========
If the [event](#eventlist) parameter is not set to `none`, an event will be generated each time a command is received.

You can intercept events on home assistant on the page "developer tools -> event"

Each time a command is received, an event like this will be generated:

	event_type: esphome.comelit
	data:
	  device_id: xxxxxxxxxxxxxxxxxxxxxxxxx
	  address: "13"
	  command: "50"
	origin: LOCAL
	time_fired: "2024-01-01T00:00:00.000000+00:00"
	context:
	  id: xxxxxxxxxxxxxxxxxxxxxxxx
	  parent_id: null
	  user_id: null

To intercept this event to trigger an home assistant automation, you can use a trigger of type "event."

The trigger configuration will look like this:

	platform: event
	event_type: esphome.comelit
	event_data:
	  command: "50"
	  address: "13"
You have to change the address and the name of the event you have chosen, if you have set a different one.

Transmit a command
==================
To send commands to the bus, the following action is available:

	- comelit_intercom.send:
	    command: 16
	    address: 1

- **command** (**Required**, int)
- **address** (**Required**, int)

### Button:
The action can be easily inserted into a button type entity:

	button:
	  - platform: template
	    name: Open Door
	    on_press:
	      - comelit_intercom.send:
	          command: 16
	          address: 5


### Service:
You can create a home assistant service, which can easily be invoked by an automation or script:

	api:
	  encryption:
	    key: "xxxxxxxxxxxxxxxxxxx"
	  services:
	    - service: open_door
	      then:
	        - comelit_intercom.send:
	            command: 16
	            address: 5

### Sending multiple commands:
There are some special configurations that require sending 2 or more commands consecutively on the bus.
In this case, a delay of at least 200ms must be inserted between the commands (one command takes about 180ms to be sent)

	- comelit_intercom.send:
	    command: 29
	    address: 1
	- delay: 200ms
	- comelit_intercom.send:
	    command: 16
	    address: 1
