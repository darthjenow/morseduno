# morseduno
morseduno is a project centered around an Arduino Nano / Uno sending out morse messages over a LED or PWM.

It can be programmed over serial, for example with the included python file.

## Encoding to save space
In order to save space the data is stored in a space efficient way.

### Morsecodes.h
The header-file `Morsecodes.h` contains the morse-codes for the supported chars and symbols. The entries are mapped to the ASCII-range from 33 (_exclamation mark_) to 95 (_underscore_).

Incoming lowercase chars can be detected by a simple if-Statement and corrected to lowercase by subtracting 32.
```C++
if (c >= 97) {
	c -= 32;
}

c -= 33
```

## Storing Messages
Messages can be sent to the *Morseduno* over serial (Baudrate = 9600).
You can use any serial terminal, for exaple the one in the Arduino IDE or alternatively you can use the included *send.py* utility.
The requirements are:
- pyserial
- console-menu
- configparser

### Building
You can also build it into a universal executable with `pyinstaller`:
```
pyintaller send.py
```