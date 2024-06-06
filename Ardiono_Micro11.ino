#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include <Keypad.h>

#define BZPIN 9
#define LEDPIN 10 // Define the pin where the 1W LED is connected
#define DHTPIN 13
#define DHTTYPE DHT11
#define POTPIN A0 // Define the pin where the potentiometer is connected

const int LED = 11;
const int JOYSTICK_H = A1; // Define the pin where the joystick horizontal is connected
const int JOYSTICK_V = A3; // Define the pin where the joystick vertical is connected
const int JOYSTICK_PB = A2; // Define the pin where the joystick push button is connected
const int SERVO_PIN_1 = 5; // Define the pin where the first servo is connected
const int SERVO_PIN_2 = 3; // Define the pin where the second servo is connected

const int motorEN = 12;
const int motorPin1 = 3;
const int motorPin2 = 2;

// Keypad configuration
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'0', '1', '2', '3'},
  {'4', '5', '6', '7'},
  {'8', '9', 'A', 'B'},
  {'C', 'D', 'E', 'F'}
};
byte rowPins[ROWS] = {A1, A2, A3, A4}; // Connect to the row pinouts of the keypad: R0, R1, R2, R3
byte colPins[COLS] = {A5, 6, 7, 8}; // Connect to the column pinouts of the keypad: C0, C1, C2, C3

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String correctPIN = "123"; // Set your desired PIN here
String inputPIN = "";
bool accessGranted = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(20, LED, NEO_GRB + NEO_KHZ800); // NEO_GRB + NEO_KHZ800 is the standard for most NeoPixels
Servo servo1;
Servo servo2;

void readSensor() {
  float t = dht.readTemperature();
  int potValue = analogRead(POTPIN); // Read the potentiometer value
  int frequency = map(potValue, 0, 1023, 0, 100); // Map potentiometer value to frequency range

  if (t >= 26) {
    tone(BZPIN, 500); // Use the mapped frequency for the buzzer
    digitalWrite(LEDPIN, LOW); // Turn on the 1W LED
    setStripColor(255, 0, 0); // Red for high temperature
    digitalWrite(motorEN, HIGH); // Turn on the motor
    analogWrite(motorEN, map(potValue, 0, 1023, 0, 255)); // Set motor speed based on potentiometer value
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
  } else if (t >= 24 && t >= 25) {
    noTone(BZPIN);
    digitalWrite(LEDPIN, HIGH); // Turn off the 1W LED
    setStripColor(0, 0, 255); // Green for normal temperature
    analogWrite(motorEN, map(potValue, 0, 1023, 0, 255)); // Adjust motor speed based on potentiometer value
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
  } else {
    noTone(BZPIN);
    digitalWrite(LEDPIN, HIGH); // Turn off the 1W LED
    setStripColor(0, 255, 0); // Blue for low temperature
    digitalWrite(motorEN, LOW); // Turn off the motor
  }
  
  lcd.clear();
  lcd.setCursor(2, 0); // Set the cursor on the third column and first row.
  lcd.print("SUHU: " + String(t) + "°C");
  lcd.setCursor(0, 1); // Set the cursor on the first column and second row.
  lcd.print("DC M: " + String(frequency));
}

void setStripColor(int r, int g, int b) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show(); // Update the strip to show the new color
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  servo1.attach(SERVO_PIN_1); // Attach the first servo to the pin
  servo2.attach(SERVO_PIN_2); // Attach the second servo to the pin

  // Motor setup
  pinMode(motorEN, OUTPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  digitalWrite(motorEN, LOW);

  // LCD
  pinMode(BZPIN, OUTPUT); // Set buzzer - pin 9 as an output
  pinMode(LEDPIN, OUTPUT); // Set 1W LED - pin 10 as an output
  pinMode(JOYSTICK_PB, INPUT_PULLUP); // Set joystick push button as input with internal pull-up

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);

  dht.begin();
}

void loop() {
  if (!accessGranted) {
    checkPIN();
  } else {
    char key = keypad.getKey();
    if (key == 'D') {
      accessGranted = false; // Revoke access
      inputPIN = ""; // Clear input PIN
      setStripColor(0, 0, 0); // Green for normal temperature
      lcd.clear();
      lcd.print("Logged Out");
      delay(2000);
      lcd.clear();
    } else {
      readSensor();
      controlServos();
    }
  }
}

void checkPIN() {
  lcd.setCursor(0, 0);
  lcd.print("Enter PIN:");
  char key = keypad.getKey();

  if (key) {
    if (key == 'E') { // Change here to 'E' for enter key
      if (inputPIN == correctPIN) {
        accessGranted = true;
        lcd.clear();
        lcd.print("Access Granted");
        delay(2000);
        lcd.clear();
      } else {
        lcd.clear();
        lcd.print("Wrong PIN");
        inputPIN = ""; // Reset input PIN
        delay(2000);
        lcd.clear();
      }
    } else if (key == '*') {
      inputPIN = ""; // Clear input PIN if '*' is pressed
      lcd.clear();
    } else {
      inputPIN += key; // Append key to input PIN
      lcd.setCursor(0, 1);
      lcd.print(inputPIN);
    }
  }
}

void controlServos() {
  // Read joystick values and map them to servo positions
  int joystickHValue = analogRead(JOYSTICK_H);
  int joystickVValue = analogRead(JOYSTICK_V);
  int servoPosition1 = map(joystickVValue, 0, 1023, 0, 180); // Map joystick horizontal value to servo angle
  int servoPosition2 = map(joystickHValue, 0, 1023, 0, 180); // Map joystick vertical value to servo angle

  servo1.write(servoPosition1); // Set first servo position
  servo2.write(servoPosition2); // Set second servo position
  
  delay(100); // Shorter delay for smoother servo movement
}
