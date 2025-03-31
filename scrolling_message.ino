#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 10

// PIN definitions
#define RESET_PIN 8
#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10
#define POT_PIN A0
#define VERT_PIN A1
#define HORZ_PIN A2
#define SEL_PIN  2

// HARDWARE SPI
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

//scrolling parameters
uint8_t scrollSpeed = 10;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_CENTER;
uint16_t scrollPause = 0; // no pause
int oldValue = LOW; // default/idle value for pin 8 is low.
bool displayFrozen = false;

// for message
#define	BUF_SIZE	100
char curMessage[BUF_SIZE] = { "Hello, please insert your message " };
char defaultMessage[BUF_SIZE] = { "Hello, please insert your message " };
bool newMessageAvailable = true;

void readSerial(void)
{
  static char buffer[BUF_SIZE];
  static char *cp = buffer;

  while (Serial.available())
  {
    *cp = (char)Serial.read();
    if ((*cp == '\n') || (cp - buffer >= BUF_SIZE - 2))
    {
      *cp = '\0';
      strcpy(curMessage, buffer);
      cp = buffer;
      newMessageAvailable = true;
    }
    else
      cp++;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600); // initilizing bit transfer speed
  pinMode(RESET_PIN, INPUT);
  pinMode(DATA_PIN, OUTPUT); 
  pinMode(CLK_PIN, OUTPUT); 
  pinMode(CS_PIN, OUTPUT); 
  pinMode(VERT_PIN, INPUT);
  pinMode(HORZ_PIN, INPUT);
  pinMode(SEL_PIN, INPUT_PULLUP); // High default input
  pinMode(POT_PIN, INPUT);
  P.begin();
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);  
}

void loop() {
  //main loop code
  int vert = analogRead(VERT_PIN);
  int horz = analogRead(HORZ_PIN);
  static bool lastSelState = HIGH;
  bool selPressed = (digitalRead(SEL_PIN) == LOW);
  int newValue = digitalRead(RESET_PIN);

  // Check for SEL button press (toggle freeze state)
  if (selPressed && lastSelState == HIGH) {
      while (digitalRead(SEL_PIN) == LOW) {
    }
    displayFrozen = !displayFrozen;
    P.displaySuspend(displayFrozen); // This immediately freezes/unfreezes
  }
  lastSelState = selPressed; // Update last button state

  if (!displayFrozen) {
    if (P.displayAnimate()) {
      if (newMessageAvailable) {
        Serial.print("Please type a new message\n");
        newMessageAvailable = false;
        P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
      }
      // Reset the animation to start again
      P.displayReset();
    }
  }

  // on the fly speed setting
  scrollSpeed = 10 + 70*(analogRead(POT_PIN)/1023.0);
  P.setSpeed(scrollSpeed);

  // resets displayed message upon button press
  if (newValue != oldValue) {
    if (newValue == HIGH) {
      P.displayClear();
      strcpy(curMessage, defaultMessage);
      displayFrozen = false; // Unfreeze when resetting
      P.displaySuspend(false); // Make sure to resume display
      P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
      P.displayReset();
    }
    // Remember the value for the next time.
    oldValue = newValue;
  }

  // Handle joystick direction changes with better thresholds
  textEffect_t newEffect = scrollEffect; // Store current effect
  
  // Use center with deadzone (400-600) to avoid accidental changes
  if (vert > 600) {
    newEffect = PA_SCROLL_RIGHT;
    if (displayFrozen) {
      displayFrozen = false;
      P.displaySuspend(false); // Resume display when direction changes
    }
  }
  else if (vert < 400) {
    newEffect = PA_SCROLL_LEFT;
    if (displayFrozen) {
      displayFrozen = false;
      P.displaySuspend(false); // Resume display when direction changes
    }
  }
  else if (horz > 600) {
    newEffect = PA_SCROLL_UP;
    if (displayFrozen) {
      displayFrozen = false;
      P.displaySuspend(false); // Resume display when direction changes
    }
  }
  else if (horz < 400) {
    newEffect = PA_SCROLL_DOWN;
    if (displayFrozen) {
      displayFrozen = false;
      P.displaySuspend(false); // Resume display when direction changes
    }
  }
  
  // Only update effect if it changed
  if (newEffect != scrollEffect) {
    scrollEffect = newEffect;
    P.displayClear();
    P.setTextEffect(scrollEffect, scrollEffect);
    if (!displayFrozen) {
      P.displayReset();
    }
  }

  readSerial();
}