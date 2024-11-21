#include <SPI.h>

// Global variables to store the command received via SPI and its status
volatile char command = '\0';
volatile bool newCommand = false;

// Button pins for each color and each player
const int player1ButtonRedPin = 4;
const int player1ButtonGreenPin = A0;
const int player1ButtonBluePin = 2;

const int player2ButtonRedPin = 5;
const int player2ButtonGreenPin = 6;
const int player2ButtonBluePin = 7;

// RGB LED pins for each color and each player
const int player1RedLEDPin = 8;
const int player1GreenLEDPin = 9;
const int player1BlueLEDPin = A2;

const int player2RedLEDPin = A5;
const int player2GreenLEDPin = A4;
const int player2BlueLEDPin = A3;

// Timing variables for press duration
unsigned long pressStartTime = 0;
bool waitingForPress = false;
bool player1Turn = true;

void setup() {
    // Initialize Serial communication for debugging
    Serial.begin(115200);

    // Enable SPI in slave mode
    SPCR |= bit(SPE);

    // Set MISO as output for SPI communication
    pinMode(MISO, OUTPUT);

    // Enable SPI interrupt
    SPI.attachInterrupt();

    // Initialize button pins as input with pull-up resistors
    pinMode(player1ButtonRedPin, INPUT_PULLUP);
    pinMode(player1ButtonGreenPin, INPUT_PULLUP);
    pinMode(player1ButtonBluePin, INPUT_PULLUP);
    pinMode(player2ButtonRedPin, INPUT_PULLUP);
    pinMode(player2ButtonGreenPin, INPUT_PULLUP);
    pinMode(player2ButtonBluePin, INPUT_PULLUP);

    // Initialize LED pins as outputs
    pinMode(player1RedLEDPin, OUTPUT);
    pinMode(player1GreenLEDPin, OUTPUT);
    pinMode(player1BlueLEDPin, OUTPUT);
    pinMode(player2RedLEDPin, OUTPUT);
    pinMode(player2GreenLEDPin, OUTPUT);
    pinMode(player2BlueLEDPin, OUTPUT);
}

// SPI interrupt service routine
ISR(SPI_STC_vect) {
    // Read the received SPI data
    char receivedChar = SPDR;

    // Placeholder command to be ignored
    if (receivedChar != '#') {
        command = receivedChar;
        newCommand = true;
    }
}

void loop() {
    // Check if a new command is received
    if (newCommand) {
        newCommand = false;

        // If the command is a valid color, start a new challenge
        if (command == 'r' || command == 'g' || command == 'b') {
            startButtonChallenge(command);
        } else {
            // Send a default response for unrecognized commands
            SPDR = '$';
        }
    }

    // If waiting for a player's button press, check the response
    if (waitingForPress) {
        checkButtonResponse();
    }
}

// Start a new button press challenge for the current color
void startButtonChallenge(char color) {
    Serial.print("Received: ");
    Serial.print((char)SPDR);
    Serial.println(player1Turn ? " for p1" : " for p2");

    pressStartTime = millis();  // Start timing
    waitingForPress = true;     // Enable response waiting

    // Activate the LED corresponding to the current color
    activateLED(color);
}

// Check the player's button press response
void checkButtonResponse() {
    unsigned long elapsedTime = millis() - pressStartTime;

    // Determine which button corresponds to the current color and player
    int buttonPin;
    if (player1Turn) {
        buttonPin = (command == 'r') ? player1ButtonRedPin :
                    (command == 'g') ? player1ButtonGreenPin :
                    player1ButtonBluePin;
    } else {
        buttonPin = (command == 'r') ? player2ButtonRedPin :
                    (command == 'g') ? player2ButtonGreenPin :
                    player2ButtonBluePin;
    }

    // Check if the player pressed the correct button
    if (digitalRead(buttonPin) == HIGH) { 
        waitingForPress = false;

        // Determine score based on response time
        char score;
        if (elapsedTime <= 300) {
            score = 'a';  // Fast response
        } else if (elapsedTime <= 600) {
            score = 'b';  // Moderate response
        } else if (elapsedTime <= 1000) {
            score = 'c';  // Slow response
        } else {
            score = 'i';  // Missed timing
        }

        // Send the score back via SPI
        SPDR = score;

        resetLEDs();          // Turn off all LEDs
        player1Turn = !player1Turn;  // Switch player turns
    } else if (elapsedTime > 900) {  // Timeout condition
        waitingForPress = false;

        // Send a timeout indicator
        SPDR = 'i';

        resetLEDs();          // Turn off all LEDs
        player1Turn = !player1Turn;  // Switch player turns
    }
}

// Activate the LED corresponding to the current color and player
void activateLED(char color) {
    resetLEDs();  // Ensure all LEDs are off before activating a new one

    // Determine which LED to activate based on the player and color
    int ledPin;
    if (player1Turn) {
        ledPin = (color == 'r') ? player1RedLEDPin :
                 (color == 'g') ? player1GreenLEDPin :
                 player1BlueLEDPin;
    } else {
        ledPin = (color == 'r') ? player2RedLEDPin :
                 (color == 'g') ? player2GreenLEDPin :
                 player2BlueLEDPin;
    }

    digitalWrite(ledPin, HIGH);  // Turn on the appropriate LED
}

// Turn off all LEDs
void resetLEDs() {
    digitalWrite(player1RedLEDPin, LOW);
    digitalWrite(player1GreenLEDPin, LOW);
    digitalWrite(player1BlueLEDPin, LOW);
    digitalWrite(player2RedLEDPin, LOW);
    digitalWrite(player2GreenLEDPin, LOW);
    digitalWrite(player2BlueLEDPin, LOW);
}
