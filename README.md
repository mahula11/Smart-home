# SmartHouse

Project in development.

Smart House based on Can Bus.
System is decentralised, devices doesn't need superior elements, communication is realized directly between devices. 
There is one configuration device, which provide support for configuration. 
Another devices first time ask for configuration by request for configuration. 

CanConf
- CanBus device which will provide configuration to other devices on CanBus

CanDevice
- CanBus device is device, which is placed in rooms, in walls, etc. This device connect switch, button, el.socket, light, thermometer etc.
- type of device (socket, switch etc) and his pin number will be obtained over configuration messages from CanConf device
