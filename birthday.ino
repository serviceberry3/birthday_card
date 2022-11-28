#include "disp.h"
#include "birth.h"

//the position of the rotary encoder
int encoderPos = 128; 

//the encoder's last position
int lastReportedPos = 1; 

//whether or not the encoder is currently rotating
boolean rotating = false;

//keep track of the current logic levels of the A and B output pins of the encoder
boolean a_state = false;
boolean b_state = false;

//define ball color
uint16_t blue = 0x0EFD;

//the current x and y speed components of the ball
float x_spd, y_spd = 0;

//current direction vector of ball
int x_dir = 1, y_dir = 1;

//current x, y position of ball
int x_pos = 10, y_pos = 10;

//buzzer pin
int buzzerPin = 7;


//instantiate an Adafruit display obj: 128x64 pixels, using I2C
Adafruit_SSD1306 disp(128, 64, &Wire);

//play a tone on the buzzer
void playTone(int tone, int duration) {
  //pulse the buzzer for the specified duration at the specified rate to create desired tone
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    //PWM on the buzzer
    digitalWrite(buzzerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(buzzerPin, LOW);
    delayMicroseconds(tone);
  }
}

//play the given note for the given duration
void playNote(char note, int duration) {
  //name of all possibly notes
  char names[9] = {'G', 'A', 'B', 'c', 'd', 'e', 'g', 'x', 'y'};

  //corresponding tones for the notes
  int tones[9] = {1275, 1136, 1014, 956, 834, 765, 468, 655, 715};
  
  //play the tone corresponding to the note name
  for (int i = 0; i < 9; i++) {
    //naive implementation: iterate over all notes, checking if the passed note is this note
    if (names[i] == note) {
      //play the tone
      playTone(tones[i], duration / DUR_CUT);
    }
  }
}


void setup() {
  //set buzzerPin on the Nano to OUTPUT mode
  pinMode(buzzerPin, OUTPUT);
}

//main program loop
void loop() {
  
}
