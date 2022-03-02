#include "Morsecodes.h"

#define DOT_LENGTH 300 // length of a dot in milli seconds
#define DASH_LENGTH_FACTOR 3 // length of a dash (relative to dot-length)
#define CHAR_DISTANCE_FACTOR 3 // distance between to chars (relative to dot-lengh)
#define WORD_DISTANCE_FACTOR 7 // distance between to words (relative to dot-length)

#define MORSE_PIN_LED LED_BUILTIN // pin for visual morsing
#define MORSE_PIN_BEEP 11 // pin for PWM (audio) morsing

#define BAUD_RATE 9600 // baud-rate for the serial communication

String message = "";

// controls the GPIO-output for morsing signals
void do_morse(bool state) {
	// toggle the LED
	digitalWrite(MORSE_PIN_LED, state);

	// toggle the PWM-signal with 50% pulse width
	analogWrite(MORSE_PIN_BEEP, state * 127);
}

// handle the different morse-chars
void morse_blink(char morse_char) {
	switch (morse_char) {
		case '.':
			Serial.print('.');

			do_morse(1);

			delay(DOT_LENGTH);

			do_morse(0);

			delay(DOT_LENGTH);
			break;
		case '-':
			Serial.print('-');

			do_morse(1);

			delay(DOT_LENGTH * DASH_LENGTH_FACTOR);

			do_morse(0);

			delay(DOT_LENGTH);
			break;
		// NULL acts as a char end
		case 0:
			Serial.print(' ');

			delay(DOT_LENGTH * CHAR_DISTANCE_FACTOR);
			break;
		case ' ':
			Serial.print("   ");

			delay(DOT_LENGTH * WORD_DISTANCE_FACTOR);
			break;
		default:
			break;
	}
}

// morse a string
void do_morse_from_string(String data_string) {
	const char* morse_char;

	// go through all the chars
	for (int i = 0; i < data_string.length(); i++) {
		// get the morse-representation of the current char
		morse_char = morse_codes[data_string[i]];

		// if there actually is a representation, process it
		if (strlen(morse_char) > 0) {
			// morse all the individual morse-chars
			for (int j = 0; j < strlen(morse_char); j++) {
				morse_blink(morse_char[j]);
			}
			// morse a letter end
			morse_blink(0);
		}
	}

	delete[] morse_char;
}

// the setup function runs once when you press reset or power the board
void setup() {
	// initialize digital pin LED_BUILTIN as an output.
	pinMode(MORSE_PIN_LED, OUTPUT);
	pinMode(MORSE_PIN_BEEP, OUTPUT);

	// Start the serial communication with the specified baudrate
	Serial.begin(BAUD_RATE);
}

// the loop function runs over and over again forever
void loop() {
	delay(50);
}

void serialEvent() {
	while (Serial.available()) {
		char c = Serial.read();

		switch (c) {
			case 10:
			case 13:
				do_morse_from_string(message);

				Serial.print(message);
				Serial.println();
				// message = "";
				break;
			default:
				message += c;
				break;
		}
	}

	// Serial.println(serial_string.substring());
	// do_morse_from_string(serial_message);
}