#include <EEPROM.h>
#include <math.h>
#include <Bounce2.h>

#include "Morsecodes.h"

// uncomment this define to enable debugging over the serial port
// #define SERIAL_DEBUG

#ifdef SERIAL_DEBUG
	#define DEBUG_PRINT(x) Serial.print(x)
	#define DEBUG_PRINTLN(x) Serial.println(x)
#else
	#define DEBUG_PRINT(x) do {} while (0)
	#define DEBUG_PRINTLN(x) do {} while (0)
#endif

#define DOT_LENGTH_MIN 100 // minimal length of a dot in milli seconds
#define DOT_LENGTH_MAX 900 // maximal length of a dot in milli seconds
/**
 * a dash ist 3 dots long
 * between two chars are 3 dots of silence
 * between two words are 7 dots of silence
 */

#define MESSAGE_REPEAT_DELAY 13 // how many dots to be waited before the message starts over again (maximum of 4.194 seconds (= full timer run))

#define MORSE_PIN_LED 8 // pin for visual morsing
#define MORSE_PIN_BEEP 3 // pin for PWM (audio) morsing
#define MORSE_ENABLE 2 // enables or disables the morsing
#define DOT_LENGTH_READ A7

#define BAUD_RATE 9600 // baud-rate for the serial communication

char message[1022];
unsigned int message_i = 0;
unsigned int message_length = 0;

byte morse_millis[14];
byte morse_millis_i = 0;

bool serial_string_in_progress = false;

Bounce morse_enable_bounce = Bounce();

// timer interrupt
ISR(TIMER1_COMPA_vect) {
	TCNT1 = 0; // Reset the timer

	// get the time of the text morse-char
	OCR1A = 16 * (DOT_LENGTH_MIN + (DOT_LENGTH_MAX - DOT_LENGTH_MIN) / 1023.0 * analogRead(DOT_LENGTH_READ)) * get_next_morse_millis();

	do_morse(!(morse_millis_i % 2));
}

// returns the millis for the next morse char
byte get_next_morse_millis() {
	do {
		morse_millis_i++;

		// if morse_millis reached its maximum, load a new char and reset the counter
		if (morse_millis_i == 14) {
			load_next_morse_char();

			morse_millis_i = 0;
		}
	} while (morse_millis[morse_millis_i] == 0);
	
	if (morse_millis_i % 2 == 0) {
		switch(morse_millis[morse_millis_i]) {
			case 1:
				DEBUG_PRINT('.');
				break;
			case 3:
				DEBUG_PRINT('-');
				break;
			default:
				break;
		}
	}

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

		DEBUG_PRINTLN();
	} else {
		// if the next char is a space, handle it by itself
		if (message[message_i] == ' ') {
			// set the first "off"-slot in the array to the word-distance-delay
			morse_millis[1] = 7;

			DEBUG_PRINT(" / ");
		} else {
			// get the next morse-code from the header-file
			unsigned int morse_code = get_morse_char(message[message_i]);

			// length of the morse-char
			byte len = 0;
			if (morse_code == 0b0) {
				len = 2;
			} else {
				len = ceil(log(morse_code) / M_LN2 + 0.005);
			}

			for (byte i = 0; i < len; i += 2) {
				morse_millis[i] = (1 & (morse_code >> (len - i - 2)) )* 2 + 1; // * 2 + 1 converts from 0/1 value to 1/3 value
				morse_millis[i + 1] = 1; // create a dot-long pause after the morse
			}

			morse_millis[len - 1] = 3; // set the pause between individual chars

			DEBUG_PRINT(' ');
		}

		message_i++;
	}
}

/**
 * @brief Retrieve the morse-code for a char from the header-file
 * 
 * @param c char of which the code should be retrieved
 * @return int morse representation of the char
 */
int get_morse_char(char c) {
	// if the letter is lower-case, make it uppercase
	if (c >= 97) {
		c -= 32;
	}

	return morse_codes[c - 33];
}

/**
 * @brief save a char to the message array
 * 
 * @param c char to save
 */
void add_char_to_message(char c) {
	message[message_length] = c;

	message_length++;
}

/**
 * @brief controls the GPIO-output for morsing signals
 * 
 * @param state wether the morse-code is ON or OFF
 */
void do_morse(bool state) {
	// toggle the LED
	digitalWrite(MORSE_PIN_LED, state);

	// toggle the PWM-signal with 50% pulse width
	digitalWrite(MORSE_PIN_BEEP, state);
}

/**
 * @brief save the current message to EEPROM
 * 
 */
void save_to_eeprom() {
	EEPROM.put(0, message_length);
	EEPROM.put(2, message);

	serial_print_message();
}

/**
 * @brief load the morse-string from EEPROM
 * 
 */
void load_from_EEPROM() {
	EEPROM.get(0, message_length);
	EEPROM.get(2, message);

	serial_print_message();
}

/**
 * @brief output the current message to the serial console
 * 
 */
void serial_print_message() {
	for (int i = 0; i < message_length; i++) {
		DEBUG_PRINT(message[i]);
	}
	DEBUG_PRINTLN();
}

void morse_enable(bool state) {
	if (state) {
		message_i = 0;
		morse_millis_i = -1;

		load_next_morse_char();

		// restart the timer
		TCCR1B |= B00000101;

	} else {
		// stop the timer
		TCCR1B &= B11111000;

		//immidiately turn all the output off
		do_morse(false);
	}
}

/**
 * @brief Initialize the pins, serial, timer and message from EEPROM
 * 
 */
void setup() {
	// Start the serial communication with the specified baudrate
	Serial.begin(BAUD_RATE);

	// initialize digital pin LED_BUILTIN as an output.
	pinMode(MORSE_PIN_LED, OUTPUT);
	pinMode(MORSE_PIN_BEEP, OUTPUT);

	// initialize the toggle-switch
	morse_enable_bounce.attach(MORSE_ENABLE, INPUT);
	morse_enable_bounce.interval(5);

	delay(1000);

	// retrieve the data from the EEPROM
	load_from_EEPROM();

	// setup the timer interrupts
	// disable the interrupts
	cli();

	TCCR1A = 0; // Reset entire TC1A register
	// morse_enable(morse_enable_bounce.read()); // enable the timer

	morse_enable_bounce.update();

	if (morse_enable_bounce.read()) {
		TCCR1B = B00000101; // enable the timer
	} else {
		TCCR1B = B00000000; // disable the timer
	}

	TIMSK1 |= B00000010; // Set OCIE1A to 1 so we enable compare match A

	OCR1A = 10; // set compare register A to 10 so it gets triggered almost instantly
	TCNT1 = 0; // Reset Timer 1 value to 0

	// re-enable the interrupts
	sei();

	if (!morse_enable_bounce.read()) {
		morse_enable(false);
	}
}

void loop() {
	morse_enable_bounce.update();

	if (morse_enable_bounce.changed()) {
		morse_enable(morse_enable_bounce.read());
	}

	delay(50);
}

/**
 * @brief Respons to incoming serial messages by saving by concating them into one message
 * 
 */
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
