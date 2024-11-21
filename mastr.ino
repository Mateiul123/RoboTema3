#include <SPI.h>

// Button pin for starting the game
const int buttonPin = 2;

// Duration for the entire game in milliseconds
const unsigned long gameDuration = 30000;

// Array of color commands
char colors[] = {'r', 'g', 'b'};

// Timing variables
unsigned long gameStartTime = 0;
unsigned long lastRoundTime = 0;
unsigned long roundInterval = 1000;  // 1 second per round

// Game state variables
bool gameStarted = false;
bool awaitingResponse = false;
char response;

// Player details
String player1, player2;
int player1Score = 0;
int player2Score = 0;
int roundCounter = 0;
bool isPlayer1Turn = true;

void setup() {
  // Initialize Serial communication 
  Serial.begin(115200);

  // Initialize SPI for communication
  SPI.begin();

  // Configure the slave select pin
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);

  // Configure the button pin
  pinMode(buttonPin, INPUT_PULLUP);

  // Display initial prompt
  Serial.println("Press the button to start!");
}

void loop() {
  // Wait for the game to start
  if (!gameStarted && digitalRead(buttonPin) == LOW) {
    delay(200);  // Debounce delay
    gameStarted = true;

    // Get player names
    Serial.println("Enter Player 1 Name:");
    while (Serial.available() == 0) {}  // Wait for input
    player1 = Serial.readStringUntil('\n');

    Serial.println("Enter Player 2 Name:");
    while (Serial.available() == 0) {}  // Wait for input
    player2 = Serial.readStringUntil('\n');

    // Record game start time
    gameStartTime = millis();
  }

  // Main game logic
  if (gameStarted) {
    unsigned long currentMillis = millis();

    // End the game if the duration has elapsed
    if (currentMillis - gameStartTime >= gameDuration) {
      gameStarted = false;
      displayWinner();
      resetGame();
      return;
    }

    // Alternate turns between players every round interval
    if (currentMillis - lastRoundTime >= roundInterval) {
      if (roundCounter < 30) {  // Maximum of 15 rounds per player
        if (isPlayer1Turn) {
          Serial.println(player1 + "'s Round");
        } else {
          Serial.println(player2 + "'s Round");
        }

        sendColorCommand();  // Send color command to slave
        awaitingResponse = true;  // Wait for response from slave
        lastRoundTime = currentMillis;
        isPlayer1Turn = !isPlayer1Turn;  // Switch player turn
        roundCounter++;
      }
    }

    // Process the response from the slave
    if (awaitingResponse) {
      response = sendCommand('#');  // Placeholder command to get response

      // Map the response to a score
      int score = getScoreFromResponse(response);
      if (score < 0) return;  // Ignore invalid responses

      // Update the score for the current player
      Serial.println((isPlayer1Turn ? player2 : player1) + " received score: " + String(score));
      updateScore(score);
      awaitingResponse = false;
    }
  }
}

// Send a random color command to the slave
void sendColorCommand() {
  char colorCommand = colors[random(0, 3)];
  sendCommand(colorCommand);
}

// Map the received character to a numeric score
int getScoreFromResponse(char response) {
  switch (response) {
    case 'a': return 50;  // Fast response
    case 'b': return 25;  // Moderate response
    case 'c': return 10;  // Slow response
    case 'i': return 0;   // Missed timing
    default: return -1;   // Invalid response
  }
}

// Update the score for the current player
void updateScore(int score) {
  if (!isPlayer1Turn) {
    player1Score += score;
  } else {
    player2Score += score;
  }
}

// Send a command to the slave and receive its response
char sendCommand(char command) {
  char response = '\0';  // Default response

  // Try sending the command up to 3 times
  for (int attempts = 0; attempts < 3; attempts++) {
    digitalWrite(SS, LOW);
    response = SPI.transfer(command);
    digitalWrite(SS, HIGH);

    // If the response is valid, exit the loop
    if (getScoreFromResponse(response) >= 0) break;

    delay(15);  // Allow slave time to stabilize
  }

  return response;
}

// Display the final scores and announce the winner
void displayWinner() {
  Serial.println(player1 + " score: " + String(player1Score));
  Serial.println(player2 + " score: " + String(player2Score));

  if (player1Score > player2Score) {
    Serial.println("Winner: " + player1);
  } else if (player2Score > player1Score) {
    Serial.println("Winner: " + player2);
  } else {
    Serial.println("It's a Draw!");
  }
}

// Reset the game state
void resetGame() {
  player1Score = 0;
  player2Score = 0;
  roundCounter = 0;
  isPlayer1Turn = true;
  Serial.println("Press the button to start!");
}
