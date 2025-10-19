#include <Arduino.h>
#include <Mouse.h>

#define ENABLEPIN 2
#define XPIN A1
#define YPIN A0
#define LCLICK A2
#define RCLICK A3
#define MAGIC 0.5

int centerX = 571;
int centerY = 547;

const int inputPin = 9;      // Use a GPIO pin for input, for example pin 2
int lastButtonState = HIGH;  // Initialize to the opposite of the active state to ensure initial check

int scaleAxis(int reading, int center) {
  int var = 0;
  int result = 0;

  var = reading - center;

if (abs(var) < 50)
    var = 0;

  result = map(var, -500, 500, -15, 15);
  return result;
}

void printAxis(int x, int y) {
  Serial.print(x); 
  Serial.print("  ");
  Serial.println(y);
}

void disableRoutine() {
  int x = 0;
  int y = 0;

  Serial.begin(9600);
  while (true) {
  x = analogRead(XPIN);
  y = analogRead(YPIN);

  delay(100);
  }


}

void setup()
{
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(inputPin, INPUT);
}

void loop() {
  int x = 0;
  int y = 0;
  int speed = 0;


  if (!digitalRead(ENABLEPIN))
    disableRoutine();


  Mouse.begin();
  while (true) {
    x = scaleAxis(analogRead(XPIN), centerX);
    y = scaleAxis(analogRead(YPIN), centerY);
    speed = floor(sqrt( pow(x,2)+ pow(y,2)));
    Mouse.move(x * -1, y, 0);

    if (RCLICK == 1) {
      Mouse.click(MOUSE_RIGHT);
    }

    if (LCLICK == 1) {
      Mouse.click(MOUSE_LEFT);
    }
    
    delay(20 - (speed * MAGIC));

    int buttonState = digitalRead(inputPin);
    // Serial.println(buttonState);

    // Check if the button state has changed since the last loop iteration
    if (buttonState != lastButtonState) {
      // Serial.println("Button state changed.");

      // If the button state is HIGH (meaning it's pressed/activated)
      if (buttonState == HIGH) {
        // Serial.println("Input Pin is HIGH - Pressing Mouse Button");
        Mouse.press(MOUSE_LEFT);
      } else { // If the button state is LOW (meaning it's released)
        Serial.println("Input Pin is LOW - Releasing Mouse Button");
        Mouse.release(MOUSE_LEFT);
      }
      
      // Update the last known button state
      lastButtonState = buttonState;
    }

  }


}
