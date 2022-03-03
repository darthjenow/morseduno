#include <EEPROM.h>
#include <math.h>

#include "Morsecodes.h"

#define DOT_LENGTH 300 // length of a dot in milli seconds
/**
 * a dash ist 3 dots long
 * between two chars are 3 dots of silence
 * between two words are 7 dots of silence
 */

#define MESSAGE_REPEAT_DELAY 13 // how many dots to be waited before the message starts over again (maximum of 4.194 seconds (= full timer run))

#define MORSE_PIN_LED LED_BUILTIN // pin for visual morsing
#define MORSE_PIN_BEEP 11 // pin for PWM (audio) morsing

#define BAUD_RATE 9600 // baud-rate for the serial communication

char message[1022];
unsigned int message_i = 0;
unsigned int message_length = 0;

byte morse_millis[14];
byte morse_millis_i = 0;

bool serial_string_in_progress = false;

// timer interrupt
ISR(TIMER1_COMPA_vect) {
	TCNT1 = 0; // Reset the timer

	// get the time of the text morse-char
	OCR1A = 16 * DOT_LENGTH * get_next_morse_millis();

	do_morse(!(morse_millis_i % 2));
}

byte get_next_morse_millis() {
	do {
		morse_millis_i++;

		// if morse_millis reached its maximum, load a new char and reset the counter
		if (morse_millis_i == 14) {

			load_next_morse_char();

			morse_millis_i = 0;
		}
	} while (morse_millis[morse_millis_i] == 0);

	return morse_millis[morse_millis_i];
}

/**
 * @brief load the next morse-char into the morse_millis-array
 * 
 */
void load_next_morse_char() {
	// reset the array
	for (int i = 0; i< 14; i++) {
		morse_millis[i] = 0;
	}

	// if message_i reached the length, send a long pause and start over again
	if (message_i == message_length) {
		message_i = 0;

		morse_millis[1] = MESSAGE_REPEAT_DELAY;
	} else {
		// if the next char is a space, handle it by itself
		if (message[message_i] == ' ') {
			// set the first "off"-slot in the array to the word-distance-delay
			morse_millis[1] = 7;
		} else {
			// get the next morse-code from the header-file
			unsigned int morse_code = get_morse_char(message[message_i]);

			// length of the morse-char
			byte len = ceil(log(morse_code) / M_LN2);
			
			for (byte i = 0; i < len; i += 2) {
				morse_millis[i] = (1 & (morse_code >> (len - i - 2)) )* 2 + 1; // * 2 + 1 converts from 0/1 value to 1/3 value
				morse_millis[i + 1] = 1; // create a dot-long pause after the morse
			}

			morse_millis[len - 1] = 3; // set the pause between individual chars
		}

		message_i++;
	}
}

// retrieves the mrose_code from the header-file identified by the char
int get_morse_char(char c) {
	// if the letter is lower-case, make it uppercase
	if (c >= 97) {
		c -= 32;
	}

	return morse_codes[c - 33];
}

// save a char to the message array
void add_char_to_message(char c) {
	message[message_length] = c;

	message_length++;
}

// controls the GPIO-output for morsing signals
void do_morse(bool state) {
	// toggle the LED
	digitalWrite(MORSE_PIN_LED, state);

	// toggle the PWM-signal with 50% pulse width
	analogWrite(MORSE_PIN_BEEP, state * 127);
}

// save the length of the message to the EEPROM
void save_to_eeprom() {
	EEPROM.put(0, message_length);
	EEPROM.put(2, message);
}

// load the last morse-string from EEPROM
void load_from_EEPROM() {
	EEPROM.get(0, message_length);
	EEPROM.get(2, message);
}

// the setup function runs once when you press reset or power the board
void setup() {
	// Start the serial communication with the specified baudrate
	Serial.begin(BAUD_RATE);

	// initialize digital pin LED_BUILTIN as an output.
	pinMode(MORSE_PIN_LED, OUTPUT);
	pinMode(MORSE_PIN_BEEP, OUTPUT);

	delay(2000);

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
				message_i = 0;

				// set the timer one value before the comparater value so it gets triggered with the next clock cycle
				TCNT1 = OCR1A - 1;

				serial_string_in_progress = false;

				save_to_eeprom();

				break;
			default:
				// if it is the first char, reset the length
				if (!serial_string_in_progress) {
					serial_string_in_progress = true;

					message_length = 0;
				}
				
				add_char_to_message (c);

				break;
		}
	}
}
