#include "disp.h"

//I2C address of OLED display
#define OLED_ADDR 0x3C

#define DESIRED_MAX_SPD_PX_PER_PULSE (float)14.0 //desired max ball speed in pixels per refresh
#define MAX_ENCODER_CLICKS (float)128.0 //the max magnitude of encoder input as it deviates from the origin of 128 (255 or 0)
#define SPD_SCALER (float)(DESIRED_MAX_SPD_PX_PER_PULSE / MAX_ENCODER_CLICKS) //how much to increase ball speed for each encoder step
#define ENCODER_CLICK_VAL 3 //let each click of rotary encoder change 0-255 val by more than 1


//set up the encoder pins
enum PinAssignments {
  encoderPinA = 2,  //encoder's A output pin mapped to Arduino pin 2
  encoderPinB = 3,  //encoder's B output pin mapped to Arduino pin 3
};

void setup_func() {
  //set both pins reading encoder to be passively pulled up to Vdd = 5V, so will trigger interrupts when rot encoder
  //strips brush ground
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);

  //encoder pin on interrupt 0 (pin 2)
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderA, CHANGE); //CHANGE means trigger interrupt whenever pin changes value
  
  //encoder pin on interrupt 1 (pin 3)
  attachInterrupt(digitalPinToInterrupt(encoderPinB), doEncoderB, CHANGE);

  //open up serial comms
  Serial.begin(9600);

  //open up comms with the display
  disp.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);  /* 0x3c is the I2C address */

  //clear display
  disp.clearDisplay();

  //push blank canvas to display
  disp.display();
}

void main_func() {
  //by default, always hold rotating at true so that the ISRs will debounce signal 
  rotating = true;

  //if the encoder position changed, print the new position
  if (lastReportedPos != encoderPos) {
    Serial.print("Encoder position: ");
    Serial.println(encoderPos);

    //save this position as the last encoder position
    lastReportedPos = encoderPos;
  }

  //set up new ball kinematics

  //get desired ball speed based on rot encoder pos
  x_spd = y_spd = (encoderPos - 128) * SPD_SCALER;

  //Serial.print(", setting ball speed to ");
  //Serial.println((int)x_spd);

  //x and y directions will be either -1 or 1, just scale direction by spd
  x_pos += (int)x_spd * x_dir;
  y_pos += (int)y_spd * y_dir;

  //clear display canvas
  disp.clearDisplay();

  //draw ball at desired position then push to screen
  disp.fillCircle(x_pos, y_pos, 4, WHITE);
  disp.display();

  //if ball hitting a wall, need switch x and maybe y direction
  if (x_pos + 4 > 124 || x_pos - 4 < 4) {
    x_dir = -x_dir;
  }
  if (y_pos + 4 > 59 || y_pos - 4 < 4) {
    y_dir = -y_dir;
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
    if (a_state && !b_state && encoderPos < 255) { //saturate at 255
      //increment encoder position
      encoderPos = min(255, encoderPos + ENCODER_CLICK_VAL);
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
    if (b_state && !a_state && encoderPos > 0) { //saturate at 0
      //decrement encoder position
      encoderPos = max(0, encoderPos - ENCODER_CLICK_VAL);
    }

    //shut off debouncing in case the interrupt for A is called immediately here 
    rotating = false;
  }
}