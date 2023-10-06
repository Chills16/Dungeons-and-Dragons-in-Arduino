//Dávid Buko - Dungeons & Dragons

#include "funshield.h"

constexpr int pins[3] = {latch_pin, clock_pin, data_pin};
constexpr int buttonPins[3] = {button1_pin, button2_pin, button3_pin};
constexpr int multipliers[4] = {1000, 100, 10, 1};

constexpr int diceTypes[] = {4, 6, 8, 10, 12, 20, 100};
constexpr int len_diceTypes = sizeof(diceTypes) / sizeof(diceTypes[0]);

constexpr int len_pins = sizeof(pins) / sizeof(pins[0]);
constexpr int numOfPositions = 4;
constexpr int maxDiceValue = 100;

int diceIndex = 0; //current dice type
int numThrows = 1; //number of dice

enum class SelectedMode {
  NORMAL,
  CONFIG
};

SelectedMode selectedMode;

unsigned int counter = 0;

constexpr byte char_d = 0xA1;

int result = 0;
bool result_ready = false;

void display_result() { //function to display the result
  bool leading_zero = true;
  for (int i = 0; i < numOfPositions; i++) {
    int digit = (result / multipliers[i]) % 10;

    //check if this digit is a leading zero
    if (digit == 0 && leading_zero && i != 3) { //dont skip if its the last digit
      continue;
    }

    if (digit != 0) {
      leading_zero = false;
    }

    displayDigit(digit, i);
  }
}

void display_config() { //function to display the number of throws + dice type
  byte digit_value_throws = numThrows;
  byte digit_value_dieType = diceTypes[diceIndex] == maxDiceValue ? 0 : diceTypes[diceIndex] / 10;
  displayDigit(digit_value_throws, 0);
  displayDigit(char_d, 1);
  displayDigit(digit_value_dieType, 2);
  displayDigit(diceTypes[diceIndex] % 10, 3);
}

class Button {
public:
  Button(int pin) : pin(pin) {}
  virtual ~Button() {}
  virtual void setup() { pinMode(pin, INPUT_PULLUP); }
  virtual void loop();
  virtual bool isRolling() { return false; }

protected:
  virtual void down() = 0;
  virtual void up() {}

private:
  int pin;
  bool was_pressed = false;
};

void Button::loop() {
  bool is_pressed = digitalRead(pin) == LOW;
  if (is_pressed && !was_pressed) {
    down();
  }
  if (!is_pressed && was_pressed) {
    up();
  }
  was_pressed = is_pressed;
}

class Button1 : public Button {
public:
  Button1() : Button(buttonPins[0]) {}
  bool isRolling() override { return isRolling_; }
  void down() override {
    counter++;
    pressTime = millis(); //record the time of the button press
    isRolling_ = true;
  }

  void up() override {
    unsigned long pressDuration = millis() - pressTime; //duration of button press
    randomSeed(pressDuration); //seed the random number generator
    isRolling_ = false;
    rollDice();
    result_ready = true;
    selectedMode = SelectedMode::NORMAL;
  }
  void rollDice();
  unsigned long animationLastUpdate; 
  const unsigned long animationInterval = 50; //animation update interval in ms

private:
  bool isRolling_ = false;
  unsigned long pressTime;
};

void Button1::rollDice() {
  result = 0;
  for (int i = 0; i < numThrows; i++) { //roll the dice as many times as specified
    result += random(1, diceTypes[diceIndex] + 1); //add a random value to the result
  }
}

class Button2 : public Button {
public:
  Button2() : Button(buttonPins[1]) {}
  void down() override {
    result_ready = false;
    selectedMode = SelectedMode::CONFIG;
    numThrows = (numThrows % 9) + 1;
  }
};

class Button3 : public Button {
public:
  Button3() : Button(buttonPins[2]) {}
  void down() override {
    result_ready = false;
    selectedMode = SelectedMode::CONFIG;
    diceIndex = (diceIndex + 1) % len_diceTypes;
  }
};

Button1 button1;
Button2 button2;
Button3 button3;

void displayDigit(byte digit, byte pos) {
  digitalWrite(latch_pin, LOW);
  if (result_ready || button1.isRolling()) { //if the result is ready or the dice are rolling
    shiftOut(data_pin, clock_pin, MSBFIRST, digits[digit]); //send the byte for the current digit
  }
  else if (selectedMode == SelectedMode::CONFIG) { //if in config mode
    if (pos != 1) { //send byte for current digit, instead of the second position, because there we display a "d"
      shiftOut(data_pin, clock_pin, MSBFIRST, digits[digit]);
    }
    else {
      shiftOut(data_pin, clock_pin, MSBFIRST, char_d);
    }
  }
  else {
    shiftOut(data_pin, clock_pin, MSBFIRST, 0xFF); //if none of the conditions are met, turn off all the segments
  }
  shiftOut(data_pin, clock_pin, MSBFIRST, digit_muxpos[pos]); //current position
  digitalWrite(latch_pin, HIGH);
}

void setup() {
  for (int i = 0; i < len_pins; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }

  for (int i = 0; i < numOfPositions; i++) { //display nothing in the beginning
    displayDigit(i, i);
  }

  button1.setup();
  button2.setup();
  button3.setup();

  randomSeed(analogRead(0)); //seed the random generator with random value
}

void loop() {
  button1.loop();
  button2.loop();
  button3.loop();

  unsigned long currentTime = millis();

  if (button1.isRolling()) { //if the dice are currently rolling
    result_ready = false;
    if (currentTime - button1.animationLastUpdate >= button1.animationInterval) {
      button1.animationLastUpdate = currentTime;
      byte animationDigit = random(numOfPositions); //generate a random digit position
      byte randomDigit = random(10); //generate a random digit (0-9)
      displayDigit(randomDigit, animationDigit); //display the random digit at the random position
    }
  }
  else if (result_ready) {
    display_result();
  }
  else if (selectedMode == SelectedMode::CONFIG) {
    display_config(); //display current num of throws + dice type if in config mode
  }
}


















