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


int16_t x,y;
uint16_t w,h;

//buzzer pin
int buzzerPin = 7;

char curr_spd[4];

//the last time the encoder button state changed
long lastStateChangeTime = 0; 

//the time after an encoder button state change detecion for which we'll ignore any input (to avoide bounce noise)
long debounceDelay = 250; //in ms

//items for main menu
char menuItems[][2] = {
  "Play song",
  "Change speed"
};

//set up the encoder pins
enum PinAssignments {
  encoderPinA = 2,  //encoder's A output pin mapped to Arduino pin 2
  encoderPinB = 3,  //encoder's B output pin mapped to Arduino pin 3
  switchPin = 4 //encoder's switch pin mapped to Arduino pin 4, which will be used as input pin
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
#define ENCODER_CLICK_VAL 1 //let each click of rotary encoder change 0-255 val by more than 1

//how many items are in main menu
#define MAIN_MENU_LEN 2

#define DUR_CUT 5

//number of notes in Happy Birthday
#define NUM_NOTES 28

//time delay between notes, in ms
int TEMPO = 200;

//the max of encoder pos, starts at 1 for main menu
int max_encoder_pos = MAIN_MENU_LEN - 1;

//min of encoder pos
int min_encoder_pos = 0;

//the string representing the song
char notes[] = "GGAGcB GGAGdc GGxecBA yyecdc";
char* sylls[28] = {"ha", "ppy", "birth", "day", "to", "you", "", "ha", "ppy", "birth", "day", "to", "you", "", "ha", "ppy", "birth", "day", "dear", "No", "a", "", "ha", "ppy", "birth", "day", "to", "you"};

//array indicating for how long each note should (relatively) be held (including rests)
int note_hold_lengths[28] = {2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16};

//play the birthday song sequence
int play_song() {
  //iterate through each note in song string
  for (int i = 0; i < NUM_NOTES; i++) {
    //if it's a space, just delay   
    if (notes[i] == ' ') {
        delay(note_hold_lengths[i] * TEMPO); // delay between notes
    } 
  
    //if there's a note that needs to be played, play given note for its intended duration
    else {
        playNote(notes[i], sylls[i], note_hold_lengths[i] * TEMPO);
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
    //Serial.print("Encoder position: ");
    //Serial.println(encoderPos);

    //save this position as the last encoder position
    lastReportedPos = encoderPos;
  }

  //menu cursor position should be set to the encoder position
  menuCount = encoderPos;

  //display main menu screen (live/dynamic)
  mainMenu();

  if (!digitalRead(switchPin)) {
    handleSwitch();
  }
}

//switch handler fxn (when encoder button is clicked)
void handleSwitch() {  
  //only accept input if it's been long enough since last button state change
  if (millis() - lastStateChangeTime < debounceDelay) {
    return;
  }
  
  //save clock time at which button last changed state, for debounce purposes
  lastStateChangeTime = millis();

  //do actual handling

  switch(curr_state) {
    case MAIN:
      //if we're on main menu, check what's selected
      switch(menuCount) {
        case 0:
          //switch state to SONG, play song, and switch state back to MAIN
          curr_state = SONG;
          play_song();
          curr_state = MAIN;
          break;
        case 1:
          //switch to speed selection screen
          encoderPos = int((TEMPO - 439) / -39);
          min_encoder_pos = 1;
          max_encoder_pos = 10;
          curr_state = SPD;
          break;
      }
      break;
      
    case SPD:
      //if we're in speed setting screen, return to main menu

      //change tempo -- use y = mx+b with points (1, 400) and (10, 50)
      TEMPO = -39 * encoderPos + 439;
      
      min_encoder_pos = 0;
      max_encoder_pos = 1;
      encoderPos = 0;
      curr_state = MAIN;      
      break;
  }
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
    if (b_state && !a_state && encoderPos > min_encoder_pos) { //saturate at MIN_ENCODER_POS
      //decrement encoder position (but can't go below 0)
      encoderPos = max(min_encoder_pos, encoderPos - ENCODER_CLICK_VAL);
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

//display a syllable of the Happy Birthday song at center of OLED display screen
void displaySyll(char* syll) {
  disp.setTextSize(4);
  
  //center the text
  disp.getTextBounds(syll, 0, 0, &x, &y, &w, &h);

  //we have 128 width to work with, 64 height to work with
  int start_x = 64 - int(w / 2);
  int start_y = 32 - int(h / 2);
  
  //display the syll on the screen
  disp.clearDisplay();
  disp.setCursor(start_x, start_y);
  disp.println(syll);
  disp.display();
}

//play the given note for the given duration
void playNote(char note, char* syll, int duration) {
  if (syll == NULL) {
    return;
  }
  
  //name of all possibly notes
  char names[9] = {'G', 'A', 'B', 'c', 'd', 'e', 'g', 'x', 'y'};

  //corresponding tones for the notes
  int tones[9] = {1275, 1136, 1014, 956, 834, 765, 468, 655, 715};

  if (strcmp(syll, "")) {
    displaySyll(syll);
  }
  
  //play the tone corresponding to the note name
  for (int i = 0; i < 9; i++) {
    //naive implementation: iterate over all notes, checking if the passed note is this note
    if (names[i] == note) {
      //play the tone
      playTone(tones[i], duration / DUR_CUT);
    }
  }
}

//activity for setting the speed
void set_speed() {
  //by default, always hold rotating at true so that the ISRs will debounce signal 
  rotating = true;
  
  disp.clearDisplay();

  disp.setTextSize(1);

  //center the text
  disp.getTextBounds("Speed (1-10):", 0, 0, &x, &y, &w, &h);

  //we have 128 width to work with, 64 height to work with
  int start_x = 64 - int(w / 2);
  int start_y = 20;
  
  //display the syll on the screen
  disp.setCursor(start_x, start_y);
  disp.println("Speed (1-10):");

  //now display speed setting
  disp.setTextSize(2);

  //while we use encoderPos, disable interrupts
  String curr_spd_str = String(encoderPos);
  curr_spd_str.toCharArray(curr_spd, 3);
  
  disp.getTextBounds(curr_spd, 0, 0, &x, &y, &w, &h);

  start_x = 64 - int(w/2);
  start_y += 10;

  disp.setCursor(start_x, start_y);
  disp.println(curr_spd);
  
  disp.display();

  if (!digitalRead(switchPin)) {
    handleSwitch();
  }
}

void encoder_setup() {
  //set both pins reading encoder to be passively pulled up to Vdd = 5V, so will trigger interrupts when rot encoder strips brush ground
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);

  //set switch pin as input, passively pulled up to Vdd = 5V
  pinMode(switchPin, INPUT_PULLUP);

  //encoder pin on interrupt 0 (pin 2)
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderA, CHANGE); //CHANGE means trigger interrupt whenever pin changes value
  
  //encoder pin on interrupt 1 (pin 3)
  attachInterrupt(digitalPinToInterrupt(encoderPinB), doEncoderB, CHANGE);

  //attach switch pin to interrupt -- NOT ENOUGH INTERRUPT PINS
  //attachInterrupt(digitalPinToInterrupt(switchPin), handleSwitch, CHANGE);

  encoderPos = 0;
}


void setup() {
  //set buzzerPin on the Nano to OUTPUT mode
  pinMode(buzzerPin, OUTPUT);

  //open up serial comms
  Serial.begin(9600);

  //set up display and encoder (register interrupts, etc)
  disp_setup();
  encoder_setup();
}

//main program loop
void loop() {
  switch(curr_state) {
    case MAIN:      
      //if in main menu, just loop
      disp_loop();
      break;
    case SPD:
      set_speed();
      break;
  }
}
