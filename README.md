# morseduno
morseduno is a project centered around an Arduino Nano / Uno sending out morse messages over a LED or PWM.

It can be programmed over serial, for example with the included python file.

## TODO
- add hardware repeat switch

## Encoding to save space
In Order to save space in the RAM and EEPROM the data is stored in a space efficient way.

### Morsecodes.h
The header-file `Morsecodes.h` contains the morse-codes for the supported chars and symbols. The entries are mapped to the ASCII-range from 33 (_exclamation mark_) to 95 (_underscore_).

Incoming lowercase chars can be detected by a simple if-Statement and corrected to lowercase by subtracting 32.
```C++
if (c >= 97) {
	c -= 32;
}

c -= 33
```