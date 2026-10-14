/*
  GitHub Command Selector
  -----------------------
  Turn the potentiometer to scroll through six GitHub commands on a 16x2 LCD.
  Press the button to "run" the command: the LCD shows a confirmation and
  the green LED lights up. Turning the dial to another command turns it off.

  Wiring (standard HD44780 16x2 LCD, 4-bit mode):
    LCD VSS -> GND
    LCD VDD -> 5V
    LCD V0  -> ~1k resistor to GND (or a contrast pot)
    LCD RS  -> D12
    LCD RW  -> GND
    LCD E   -> D11
    LCD D4  -> D5
    LCD D5  -> D4
    LCD D6  -> D3
    LCD D7  -> D2
    LCD A   -> 5V through a 220 ohm resistor (backlight +)
    LCD K   -> GND (backlight -)

  Selector potentiometer:
    one outer pin -> 5V
    other outer pin -> GND
    middle pin -> A0

  Button:
    one leg -> D7
    other leg -> GND
    (no resistor needed, the sketch uses the internal pull-up)

  Green LED:
    long leg -> D9 through a 220 ohm resistor
    short leg -> GND
*/

#include <LiquidCrystal.h>

// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int POT_PIN = A0;
const int BUTTON_PIN = 7;
const int GREEN_LED_PIN = 9;

const int NUM_COMMANDS = 6;

// Edit these to change which commands appear.
// Names should be 10 characters or fewer so the "1/6" counter fits.
const char* commandNames[NUM_COMMANDS] = {
  "git commit",
  "git push",
  "git pull",
  "git merge",
  "git stash",
  "git branch"
};

// Descriptions must be 16 characters or fewer.
const char* commandDescriptions[NUM_COMMANDS] = {
  "Save a snapshot",
  "Send to remote",
  "Get from remote",
  "Combine branches",
  "Set work aside",
  "Start a new line"
};

// Shown on the bottom row after the button is pressed (16 characters max).
const char* commandConfirmations[NUM_COMMANDS] = {
  "Snapshot saved",
  "Sent to remote",
  "Pulled changes",
  "Branches joined",
  "Work set aside",
  "Branch created"
};

const int ZONE_WIDTH = 1024 / NUM_COMMANDS;  // width of each pot zone
const int HYSTERESIS = 12;                   // stops flicker at zone edges
const unsigned long DEBOUNCE_MS = 25;        // ignores button contact bounce

int currentIndex = -1;  // -1 forces the first draw

// Button state tracking for debouncing.
int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastButtonChange = 0;

// Average a few readings to smooth out pot noise.
int readPot() {
  long total = 0;
  for (int i = 0; i < 4; i++) {
    total += analogRead(POT_PIN);
    delay(2);
  }
  return total / 4;
}

// Returns true once each time the button is pressed down.
bool buttonPressed() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonChange = millis();
    lastButtonReading = reading;
  }

  if ((millis() - lastButtonChange) > DEBOUNCE_MS && reading != stableButtonState) {
    stableButtonState = reading;
    if (stableButtonState == LOW) {  // LOW means pressed (pull-up wiring)
      return true;
    }
  }
  return false;
}

// Print text on a row and pad with spaces so old characters are erased.
void printPadded(int row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print(text);
  int len = strlen(text);
  for (int i = len; i < 16; i++) {
    lcd.print(' ');
  }
}

void showCommand(int index) {
  digitalWrite(GREEN_LED_PIN, LOW);  // moving the dial turns the LED off

  printPadded(0, commandNames[index]);
  printPadded(1, commandDescriptions[index]);

  // Position counter in the top right corner, e.g. "1/6"
  lcd.setCursor(13, 0);
  lcd.print(index + 1);
  lcd.print('/');
  lcd.print(NUM_COMMANDS);
}

// Runs when the button is pressed on the current command.
void runCommand(int index) {
  printPadded(1, commandConfirmations[index]);
  digitalWrite(GREEN_LED_PIN, HIGH);
}

void setup() {
  lcd.begin(16, 2);
  lcd.print("Turn the dial");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void loop() {
  int reading = readPot();

  int newIndex = reading / ZONE_WIDTH;
  if (newIndex >= NUM_COMMANDS) newIndex = NUM_COMMANDS - 1;

  if (currentIndex == -1) {
    // First reading: draw whatever the dial is pointing at.
    currentIndex = newIndex;
    showCommand(currentIndex);
  } else if (newIndex != currentIndex) {
    // Only switch once the dial is clearly past the zone edge.
    int zoneLow = currentIndex * ZONE_WIDTH;
    int zoneHigh = (currentIndex + 1) * ZONE_WIDTH;
    if (reading < zoneLow - HYSTERESIS || reading >= zoneHigh + HYSTERESIS) {
      currentIndex = newIndex;
      showCommand(currentIndex);
    }
  }

  if (buttonPressed() && currentIndex != -1) {
    runCommand(currentIndex);
  }

  delay(10);
}
