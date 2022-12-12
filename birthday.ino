#include <Adafruit_SSD1306.h>
#include <CapacitiveSensor.h>

#define ENCODER_CLICK_VAL 1 //let each click of rotary encoder change 0-255 val by more than 1

//how many items are in main menu
#define MAIN_MENU_LEN 3
#define MAX_SPD 10
#define MIN_SPD 1

//number of tones available
#define NUM_TONES 8

//keyboard stuff
#define NUM_OF_SAMPLES 10   // Higher number whens more delay but more consistent readings
#define CAP_THRESHOLD 150  // Capactive reading that triggers a note (adjust to fit your needs)
#define NUM_KEYS 8 // Number of keys that are on the keyboard
#define COMMON_PIN 5    // The common 'send' pin for all keys

// This macro creates a capacitance "key" sensor object for each key on the piano keyboard:
#define CS(Y) CapacitiveSensor(COMMON_PIN, Y)

CapacitiveSensor keys[NUM_KEYS] = {CS(6), CS(8), CS(9), CS(10), CS(11), CS(12), CS(A0), CS(A1)};

//instantiate an Adafruit display obj: 128x64 pixels, using I2C
Adafruit_SSD1306 disp(128, 64, &Wire);

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

//bounds for text 
int x, y, w, h;

//buzzer pin
int buzzerPin = 7;

//the current speed setting that the user selected
//char curr_spd[4];

//the last time the encoder button state changed
long lastStateChangeTime = 0; 

//the time after an encoder button state change detecion for which we'll ignore any input (to avoide bounce noise)
long debounceDelay = 250; //in ms

//items for main menu
char* menuItems[MAIN_MENU_LEN] = {
  "Song",
  "Speed",
  "Hearts"
};

//set up the encoder pins
enum PinAssignments {
  encoderPinA = 2,  //encoder's A output pin mapped to Arduino pin 2
  encoderPinB = 3,  //encoder's B output pin mapped to Arduino pin 3
  switchPin = 4 //encoder's switch pin mapped to Arduino pin 4, which will be used as input pin,
};

//different states of the interface
enum state {
  MAIN,
  SONG,
  SPD,
  KEYBOARD
};

//corresponding tones for the notes
int tones[NUM_TONES] = {1275, 1136, 1014, 956, 834, 765, 655, 715};

//name of all notes used here
//char names[NUM_TONES] = {'G', 'A', 'B', 'c', 'd', 'e', 'x', 'y'};

int piezo_tones[NUM_TONES] = {131, 147, 165, 175, 196, 220, 233, 262};

//current state the interface is in
state curr_state = MAIN;

//time delay between notes, in ms
int TEMPO = 200;

//int top_marg = 0;

//the max of encoder pos, starts at the appropriate max for main menu
int max_encoder_pos = MAIN_MENU_LEN - 1;

//min of encoder pos
int min_encoder_pos = 0;

//the string representing the song
//char notes[] = "GGAGcB GGAGdc GGxecBA yyecdc";

int notes[] = {0, 0, 1, 0, 3, 2, -1, 0, 0, 1, 0, 4, 3, -1, 0, 0, 6, 5, 3, 2, 1, -1, 7, 7, 5, 3, 4, 3};
char* sylls[28] = {"ha", "ppy", "birth", "day", "to", "you", "", "ha", "ppy", "birth", "day", "to", "you", "", "ha", "ppy", "birth", "day", "dear", "No", "a", "", "ha", "ppy", "birth", "day", "to", "you"};

//array indicating for how long each note should (relatively) be held (including rests)
int note_hold_lengths[28] = {2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 8, 16, 1, 2, 2, 8, 8, 8, 16};

//play the birthday song sequence
int play_song() {
  //iterate through each note in song string
  for (int i = 0; i < 28; i++) {
    //if it's a space, just delay   
    if (notes[i] == -1) {
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

  int top_marg = 0;

  for (int i = 0; i < MAIN_MENU_LEN; i++) {
    disp.setCursor(15, top_marg);
    disp.println(menuItems[i]);
    top_marg += 20;
  }

  disp.setCursor(2, menuCount * 20);
  disp.println(">");

  //push content to display
  disp.display();
}


void disp_setup() {
  //open up comms with the display
  disp.begin(SSD1306_SWITCHCAPVCC, 0x3c);  /* 0x3c is the I2C address */

  //clear display
  disp.clearDisplay();

  //push blank canvas to display
  disp.display();
}

//the main menu loop function
void disp_loop() {
  //by default, always hold rotating at true so that the ISRs will debounce signal 
  rotating = true;

  //if the encoder position changed, print the new position
  if (lastReportedPos != encoderPos) {
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

//return to main menu
void return_to_main(int pos) {
  min_encoder_pos = 0;
  max_encoder_pos = MAIN_MENU_LEN - 1;
  encoderPos = pos;

  //set state back to main to trigger the main menu loop
  curr_state = MAIN; 
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
    //if we're in main menu
    case MAIN:
      //if we're on main menu, check what's selected
      switch(menuCount) {
        case 0:
          //switch state to SONG, play song, and switch state back to MAIN
          play_song();
          break;
        case 1:
          //switch to speed selection screen
          encoderPos = int((TEMPO - 439) / -39);
          min_encoder_pos = MIN_SPD;
          max_encoder_pos = MAX_SPD;
          curr_state = SPD;
          break;
        //switch to keyboard activity
        case 2:
          curr_state = KEYBOARD;
          break;
      }
      break;

    //if we're in speed selection activity
    case SPD:
      //if we're in speed setting screen, return to main menu

      //change tempo -- use y = mx+b with points (1, 400) and (10, 50)
      TEMPO = -39 * encoderPos + 439;
      
      return_to_main(1);     
      break;

    //if we're in keyboard activity
    case KEYBOARD:
      //Serial.println("SWITCH HANDLR FOUND KEYBOARD");
      //if we're in keyboard screen, return to main menu
      return_to_main(2);   
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
  disp.setTextSize(3);
  
  //center the text
  //disp.getTextBounds(syll, 0, 0, &x, &y, &w, &h);
  
  //display the syll on the screen
  disp.clearDisplay();
  
  //disp.setCursor(64 - int(w / 2), 32 - int(h / 2));
  disp.setCursor(10, 20);
  
  disp.println(syll);
  disp.display();
}

void playNote(int note, char* syll, int duration) {
  displaySyll(syll);
  
  //play the tone corresponding to the note name
  playTone(tones[note], duration / 5);
}


//activity for setting the speed
void set_speed() {
  //by default, always hold rotating at true so that the ISRs will debounce signal 
  rotating = true;
  
  //disp.clearDisplay();
  //disp.setTextSize(1);

  //center the text
  //disp.getTextBounds("Speed (1-10):", 0, 0, &x, &y, &w, &h);

  //we have 128 width to work with, 64 height to work with
  //int start_x = 64 - int(60 / 2);
  //int start_y = 20;
  
  //display the syll on the screen
  //disp.setCursor(34, 20);
  //disp.println("Speed (1-10):");

  //now display speed setting
  //disp.setTextSize(2);

  //while we use encoderPos, disable interrupts
  //String curr_spd_str = String(encoderPos);
  //curr_spd_str.toCharArray(curr_spd, 3);
  
  //disp.getTextBounds(curr_spd, 0, 0, &x, &y, &w, &h);

  //start_x = 64 - int(20 / 2);
  //start_y += 10;

  //int test = encoderPos;
  //disp.setCursor(54, 30);
  //disp.println(encoderPos);
  
  //disp.display();

  /*
  digitalWrite(13, HIGH);
  delayMicroseconds(encoderPos * 100);
  digitalWrite(13, LOW);
  delayMicroseconds(encoderPos * 100);
  */

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

  encoderPos = 0;
}

// Turn off autocalibrate on all channels:
void cap_setup() {
  for (int i = 0; i < NUM_KEYS; i++) {
    keys[i].set_CS_AutocaL_Millis(0xFFFFFFFF);
  }
}

//function for playing keys activity
void keyboard() {
  int active = 0;

  //check keys
  for (int i = 0; i < NUM_KEYS; i++) {
    //if the capacitance reading is greater than the threshold, play a note:
    if (keys[i].capacitiveSensor(NUM_OF_SAMPLES) > CAP_THRESHOLD) {
      //plays the note corresponding to the key pressed
      tone(buzzerPin, piezo_tones[i]); 
      active = 1;
    }
  }

  //if no key being pressed, make sure buzzer off
  if (active == 0) {
    noTone(buzzerPin);
  }

  //monitor the switch
  if (!digitalRead(switchPin)) {
    handleSwitch();
  }
}


void setup() {  
  //set buzzerPin on the Nano to OUTPUT mode
  pinMode(buzzerPin, OUTPUT);

  //set up display and encoder (register interrupts, etc)
  disp_setup();
  encoder_setup();

  cap_setup();
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
   case KEYBOARD:
      keyboard();
      break;
  }
}
