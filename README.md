# morseduno
morseduno is a project centered around an Arduino Nano / Uno sending out morse messages over a LED or PWM.

It can be programmed over serial, for example with the included python file.

## TODO
- save to EEPROM
- optimize the size of the EEPROM-Storage and the RAM-Storage
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

c += 33
```

### morse_millis[]
During the runtime the morse-code is stored in the morse_millis-array. It contains (again encoded) the multiples of dot-lengths to be output.

Every even entry is a signal and every odd entry is silence.

The length are mapped to binary values as follows:

| binary value | integer value | dot-length-facotr |
| :----------: | :-----------: | :---------------: |
| 0b00         | 0             | 0                 |
| 0b01         | 1             | 1                 |
| 0b10         | 2             | 3                 |
| 0b11         | 3             | 7                 |

Every entry of the morse_millis-array stores 4 of those value with the first one beeing the ones to the right and the fourth beeing the ones to the right.