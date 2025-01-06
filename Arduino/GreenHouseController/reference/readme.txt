LCD sketch and hardware works with power to LCD connected to VIN (presumably +5V from USB).

OneWire temperature works with power tri-state buffer connected to 3.3V.

DHT sketch works with 10KOhm pullup and 3.3V with AM2302.

Not enough IOs on the NodeMCU to control AC relays.  Use the Extra GPIOs from the LCD board.