#include <EEPROM.h>
#include <math.h>

#include "Morsecodes.h"

#define DOT_LENGTH 300 // length of a dot in milli seconds
/**
 * a dash ist 3 dots long
 * between two chars are 3 dots of silence
 * between two words are 7 dots of silence
 */

#define MESSAGE_REPEAT_DELAY 1 // how many times a full timer run (4.194 seconds) should be waited before the message starts over again

#define MORSE_PIN_LED LED_BUILTIN // pin for visual morsing
#define MORSE_PIN_BEEP 11 // pin for PWM (audio) morsing

#define BAUD_RATE 9600 // baud-rate for the serial communication

byte morse_millis[1022]; // array to hold all the timings, stored in multiples of the dot length to save space
unsigned int morse_millis_i = 0;
unsigned int morse_millis_length = 0;

bool serial_string_in_progress = false;

// timer interrupt
ISR(TIMER1_COMPA_vect) {
	TCNT1 = 0; // Reset the timer

	// if the morse_millis counter has reached the end of the array, reset it to the beginning
	if (morse_millis_i < morse_millis_length) {
		// if the current entry is zero, increase the counter
		if (get_morse_millis_factor(morse_millis_i) == 0) {
			morse_millis_i++;
		}

		OCR1A = 16 * DOT_LENGTH * get_morse_millis_factor(morse_millis_i);

		do_morse(!(morse_millis_i % 2));
	} else {
		OCR1A = 65535;

		if (morse_millis_i == morse_millis_length + MESSAGE_REPEAT_DELAY - 1) {
			morse_millis_i = -1;
		} 
	}

	// increase the counter of the morse_millis-array
	morse_millis_i++;
}

/**
 * @brief convert a char into a storage efficient morse-char
 * 
 * @param c the char to encode
 */
void save_char_as_morse(char c) {
	// if it is a space, handle it directly
	if (c == ' ') {
		// save a word-end-char
		save_morse_millis(0, morse_millis_length);
		save_morse_millis(3, morse_millis_length + 1);
		morse_millis_length += 2;
	} else {
		// if the letter is lower-case, make it uppercase
		if (c >= 97) {
			c -= 32;
		}

		int morse_code_binary = morse_codes[c - 33];
		byte len;

		while (morse_code_binary > 0) {
			len = ceil(log(morse_code_binary + 1) / M_LN2);

			// if the right-boolean of the active pair is 1, it is a dash
			if (morse_code_binary >> (len - 2) & 1) {
				save_morse_millis(2, morse_millis_length);
			} else { // else store a dot
				save_morse_millis(1, morse_millis_length);
			}
			// store the pause between two chars
			save_morse_millis(1, morse_millis_length + 1);
			morse_millis_length += 2;

			morse_code_binary -= morse_code_binary & (0b11 << (len - 2));
		}
		// save a between chars char
		save_morse_millis(0, morse_millis_length);
		save_morse_millis(2, morse_millis_length + 1);
		morse_millis_length += 2;
	}
}

/**
 * @brief store a lenght factor of a morse char in the morse_millis-array
 * 
 * @param length the length-factor of the morse-char (encoded as: 0b00 --> 0, 0b01 --> dot, 0b10 --> 3 dots, 0b11 --> 7 dots)
 * @param pos the position in the array
 */
void save_morse_millis(byte length, int pos) {
	morse_millis[pos / 4] |= length << (pos % 4) * 2;
}

byte get_morse_millis_factor(int pos) {
	switch ((morse_millis[pos / 4] >> (pos % 4) * 2) & 0b11) {
		case 0:
			return 0;
			break;
		case 1:
			return 1;
			break;
		case 2:
			return 3;
			break;
		case 3:
			return 7;
			break;
	}
}

// controls the GPIO-output for morsing signals
void do_morse(bool state) {
	// toggle the LED
	digitalWrite(MORSE_PIN_LED, state);

	// toggle the PWM-signal with 50% pulse width
	analogWrite(MORSE_PIN_BEEP, state * 127);
}

// save the current morse-string to EEPROM
void save_to_EEPROM() {
	EEPROM.put(0, morse_millis_length);

	for (int i = 0; i < morse_millis_length; i++) {
		EEPROM.put(i + 2, morse_millis[i]);
	}
}

// load the last morse-string from EEPROM
void load_from_EEPROM() {
	EEPROM.get(0, morse_millis_length);

	for (int i = 0; i < morse_millis_length; i++) {
		EEPROM.get(i + 2, morse_millis[i]);
	}
}

// the setup function runs once when you press reset or power the board
void setup() {
	// Start the serial communication with the specified baudrate
	Serial.begin(BAUD_RATE);

	// initialize digital pin LED_BUILTIN as an output.
	pinMode(MORSE_PIN_LED, OUTPUT);
	pinMode(MORSE_PIN_BEEP, OUTPUT);

	// retrieve the data from the EEPROM
	load_from_EEPROM();

	// setup the timer interrupts
	// disable the interrupts
	cli();

	TCCR1A = 0; // Reset entire TC1A register
	TCCR1B = B00000101; // Reset entire TCCR1B register and set Prescalar = 1024

	TIMSK1 |= B00000010; // Set OCIE1A to 1 so we enable compare match A

	OCR1A = 10; // set compare register A to 10 so it gets triggered almost instantly
	TCNT1 = 0; // Reset Timer 1 value to 0

	// re-enable the interrupts
	sei();
}

// the loop function runs over and over again forever
void loop() {
	delay(50);
}

void serialEvent() {
	while (Serial.available()) {
		char c = Serial.read();

		switch (c) {
			// line-ending char
			case 10:
			case 13:
				// reset the running counter to the beginning
				morse_millis_i = 0;

				// set the timer one value before the comparater value so it gets triggered with the next clock cycle
				TCNT1 = OCR1A - 1;
				
				save_to_EEPROM();

				serial_string_in_progress = false;

				break;
			default:
				// if it is the first char, reset the length
				if (!serial_string_in_progress) {
					serial_string_in_progress = true;

					morse_millis_length = 0;
				}
				save_char_as_morse (c);

				break;
		}
	}
}
