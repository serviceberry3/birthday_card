#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

//the current position of the rotary encoder
int encoderPos = 0; 

//the encoder's last position
int lastReportedPos = 0; 

//whether or not the encoder is currently rotating
boolean rotating = false;

//where in the menu should the cursor be sitting
int menuCount = 0;

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

//items for main menu
char menuItems[][2] = {
  "Play song",
  "Change speed"
};

//set up the encoder pins
enum PinAssignments {
  encoderPinA = 2,  //encoder's A output pin mapped to Arduino pin 2
  encoderPinB = 3,  //encoder's B output pin mapped to Arduino pin 3
};

//different states of the interface
enum state {
  MAIN,
  SONG,
  SPD
};

//current state the interface is in
state curr_state = MAIN;

//instantiate an Adafruit display obj: 128x64 pixels, using I2C
Adafruit_SSD1306 disp(128, 64, &Wire);

//I2C address of OLED display
#define OLED_ADDR 0x3C

#define DESIRED_MAX_SPD_PX_PER_PULSE (float)14.0 //desired max ball speed in pixels per refresh
#define MIN_ENCODER_POS 0 //min of encoder pos
#define ENCODER_CLICK_VAL 1 //let each click of rotary encoder change 0-255 val by more than 1

//how many items are in main menu
#define MAIN_MENU_LEN 2

#define DUR_CUT 5

//number of notes in Happy Birthday
#define NUM_NOTES 28

//time delay between notes, in ms
#define TEMPO 100

//the max of encoder pos, starts at 1 for main menu
int max_encoder_pos = MAIN_MENU_LEN - 1;

//the string representing the song
char notes[] = "GGAGcB GGAGdc GGxecBA yyecdc";

//array indicating for how long each note should (relatively) be held (including rests)
int note_hold_lengths[] = {2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16};

int play() {
    //iterate through each note in song string
  for (int i = 0; i < NUM_NOTES; i++) {
    //if it's a space, just delay   
    if (notes[i] == ' ') {
        delay(note_hold_lengths[i] * TEMPO); // delay between notes
    } 

    //if there's a note that needs to be played, play given note for its intended duration
    else {
        playNote(notes[i], note_hold_lengths[i] * TEMPO);
    }

    //delay between notes
    delay(TEMPO);
  }
}

//main menu screen of interface
void mainMenu() {
  disp.setTextColor(WHITE);
  disp.clearDisplay();
  
  disp.setTextSize(2);
  disp.setCursor(15, 0);
  disp.println("Play song");

  disp.setCursor(15, 20);
  disp.println("Set speed");
  
  disp.setCursor(2, menuCount * 20);
  disp.println(">");

  //push content to display
  disp.display();
}


void disp_setup() {
  //open up comms with the display
  disp.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);  /* 0x3c is the I2C address */

  //clear display
  disp.clearDisplay();

  //push blank canvas to display
  disp.display();
}

void disp_loop() {
  //by default, always hold rotating at true so that the ISRs will debounce signal 
  rotating = true;

  //if the encoder position changed, print the new position
  if (lastReportedPos != encoderPos) {
    Serial.print("Encoder position: ");
    Serial.println(encoderPos);

    //save this position as the last encoder position
    lastReportedPos = encoderPos;
  }

  //menu cursor position should be set to the encoder position
  menuCount = encoderPos;

  //display main menu screen (live/dynamic)
  mainMenu();
}

//interrupt to be run when encoder output pin A changes state
void doEncoderA() {
  //debounce signal by delaying before checking state of encoder pin A
  if (rotating) { 
    delay(1);
  }
  
  //make sure the logic level of encoder's output pin A really did change (after debounce delay), else do nothing here
  if (digitalRead(encoderPinA) != a_state) {  // debounce once more
    //record new state of encoder's A pin
    a_state = !a_state;
    
    //if this interrupt was called, know A changed from 5V to 0 or from 0 to 5V 
    //if A just changed but B hasn't yet, then A is leading B, so encoder being turned clockwise
    if (a_state && !b_state && encoderPos < max_encoder_pos) { //saturate at MAX_ENCODER_POS
      //increment encoder position (but can't go above 255)
      encoderPos = min(max_encoder_pos, encoderPos + ENCODER_CLICK_VAL);
    }
    
    //shut off debouncing in case the interrupt for B is called immediately here 
    rotating = false;
  }
}

//interrupt to be run when encoder output pin B changes state
void doEncoderB() {
  //debounce signal by delaying before checking state of encoder pin B
  if (rotating) {
    delay(1);
  }

  //make sure the logic level of encoder's output pin B really did change (after debounce delay), else do nothing here
  if (digitalRead(encoderPinB) != b_state) {
    //record new state of encoder's A pin
    b_state = !b_state;
    
    //if this interrupt was called, know B changed from 5V to 0 or from 0 to 5V 
    //if B just changed but A hasn't yet, then B is leading A, so encoder being turned counterclockwise
    if (b_state && !a_state && encoderPos > MIN_ENCODER_POS) { //saturate at MIN_ENCODER_POS
      //decrement encoder position (but can't go below 0)
      encoderPos = max(MIN_ENCODER_POS, encoderPos - ENCODER_CLICK_VAL);
    }

    //shut off debouncing in case the interrupt for A is called immediately here 
    rotating = false;
  }
}

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


void encoder_setup() {
  //set both pins reading encoder to be passively pulled up to Vdd = 5V, so will trigger interrupts when rot encoder
  //strips brush ground
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);

  //encoder pin on interrupt 0 (pin 2)
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderA, CHANGE); //CHANGE means trigger interrupt whenever pin changes value
  
  //encoder pin on interrupt 1 (pin 3)
  attachInterrupt(digitalPinToInterrupt(encoderPinB), doEncoderB, CHANGE);
}


void setup() {
  //set buzzerPin on the Nano to OUTPUT mode
  pinMode(buzzerPin, OUTPUT);

  //open up serial comms
  Serial.begin(9600);

  disp_setup();

  encoder_setup();
}

//main program loop
void loop() {
  switch(curr_state) {
    case MAIN:
      
      disp_loop();
      break;
  }
}
