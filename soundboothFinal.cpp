#include "Timer.h"
#include "Adafruit_LiquidCrystal.h"
// Depending on the LiquidCrystal library you are able to install, it might be:
// #include "LiquidCrystal.h"
#include "pitches.h"

// Sound Variables  
int buzzer = 8;

int song_note_length = 20; 

// == Song 1 ==
int song1[] = {
NOTE_E4, NOTE_C5, NOTE_B1, NOTE_F3, NOTE_C4, 
NOTE_A4, NOTE_A4, NOTE_GS5, NOTE_C5, NOTE_CS4, 
NOTE_AS4, NOTE_C5, NOTE_DS4, NOTE_CS5, NOTE_GS4, 
NOTE_C3, NOTE_E3, NOTE_DS5, NOTE_D4, NOTE_D3
};
int song1_time[] = {
2, 1, 2, 1, 1, 4, 8, 16, 8, 4, 4, 1, 8, 4, 2, 4, 4, 16, 4, 2
};

// == Song 2 ==

int song2[] = {
  NOTE_FS5, NOTE_D2, NOTE_DS5, NOTE_G2, NOTE_B3, 
  NOTE_C2, NOTE_G5, NOTE_D6, NOTE_CS5, NOTE_AS4, 
  NOTE_DS6, NOTE_D3, NOTE_CS4, NOTE_E5, NOTE_DS6,
   NOTE_E4, NOTE_B4, NOTE_F4, NOTE_E6, NOTE_DS4
};

int song2_time[] = {
  2, 2, 4, 8, 1, 8, 4, 4, 16, 8, 2, 4, 16, 8, 2, 4, 16, 4, 8, 1
};

// == Song 3 == 

int song3[] = {
  NOTE_A5, NOTE_D4, NOTE_D6, NOTE_DS3, NOTE_G4, 
  NOTE_B2, NOTE_F2, NOTE_A3, NOTE_AS2, NOTE_B5, 
  NOTE_C6, NOTE_C3, NOTE_GS3, NOTE_G2, NOTE_FS5, 
  NOTE_AS4, NOTE_GS2, NOTE_CS3, NOTE_C3, NOTE_AS2
};

int song3_time[] = {
  1, 2, 16, 4, 16, 2, 16, 1, 1, 2, 1, 8, 2, 16, 8, 1, 16, 4, 1, 2
};


// LCD variables
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
Adafruit_LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// Depending on the LiquidCrystal library you are able to install, it might be:
// LiquidCrystal lcd(rs, en, d4, d5, d6, d7);



// Task Structure Definition
typedef struct task {
   int state;                  // Tasks current state
   unsigned long period;       // Task period
   unsigned long elapsedTime;  // Time elapsed since last task tick
   int (*TickFct)(int);        // Task tick function
} task;


const unsigned char tasksNum = 4;
task tasks[tasksNum]; // We have 4 tasks

// Task Periods

const unsigned long periodLCDOutput = 100;
const unsigned long periodJoystickInput = 100;
const unsigned long periodSoundOutput = 100;
const unsigned long periodController = 500;


// GCD 
const unsigned long tasksPeriodGCD = 100;

// Task Function Definitions
int TickFct_LCDOutput(int state);
int TickFct_JoystickInput(int state);
int TickFct_SoundOutput(int state);
int TickFct_Controller(int state);

// Task Enumeration Definitions
enum LO_States {LO_init, LO_MenuOptionA, LO_MenuOptionB, LO_MenuOptionC, LO_MenuOptionD};
enum JI_States {JI_init, JI_NoMovement, JI_Movement};
enum SO_States {SO_init, SO_SoundOn, SO_SoundOff, SO_SoundPause};
enum C_States {C_init, C_Main, C_Song1, C_Song2, C_Song3};



void TimerISR() { // TimerISR actually defined here
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += tasksPeriodGCD;
  }
}

int readStickY() { //returns 1 if the joystick was up, 2 if it is down, 0 for anything else
  // you may have to read from A0 instead of A1 depending on how you orient the joystick
  if (analogRead(A1) > 800) {
    return 2; // down
  }
  else if (analogRead(A1) < 200) {
    return 1; // up
  }
  else {
    return 0;
  }

}

int readStickX() { //returns 1 if the joystick was right, 2 if it is left, 0 for anything else
  // you may have to read from A0 instead of A1 depending on how you orient the joystick
  if (analogRead(A0) > 800) {
    return 2; // left
  }
  else if (analogRead(A0) < 200) {
    return 1; // right
  }
  else {
    return 0;
  }

}

int cursorArr[] = {0, 0, 0, 0}; // 0 if no '*' and 1 if '*'

void ClearCursorArr(){
  for (int i = 0; i < 4; i++){
    cursorArr[i] = 0;
  }
}

void LCDWriteLines(String line1, String line2) {
  lcd.clear();          
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

// Task Function Definitions

int menuOption = 0; //0 is menu, 
int cursor_pos = 0; //0 topleft, 1 topright, 2 bottomleft, 3 bottomright
int js_pressed = 0; //shared variable between joystick and controller

// Task 1
// ONLY HANDLES output of text, not cursor
int TickFct_LCDOutput(int state) {
  switch (state) { // State Transitions
    case LO_init:
      state = LO_MenuOptionA;
      LCDWriteLines("Song 1  Song 2  ", "Song 3  Select ");
    break;
    case LO_MenuOptionA: //this is the main menu, so by switching to a different menu option, 
      // we get different screens saying which song is playing
      if(menuOption == 1) {
        state = LO_MenuOptionB;
        LCDWriteLines("Playing   Song 1", "Pause   Play    ");
      }
      else if (menuOption == 2){
        state = LO_MenuOptionC;
        LCDWriteLines("Playing   Song 2", "Pause   Play    ");
      }
      else if (menuOption == 3){
        state = LO_MenuOptionD;
        LCDWriteLines("Playing   Song 3", "Pause   Play    ");
      }
    break;
    case LO_MenuOptionB:
      if(menuOption == 0) {
        state = LO_MenuOptionA;
        LCDWriteLines("Song 1  Song 2  ", "Song 3  Select ");
      }
      else if (menuOption == 2){
        state = LO_MenuOptionC;
        LCDWriteLines("Playing   Song 2", "Pause   Play    ");
      }
      else if (menuOption == 3){
        state = LO_MenuOptionD;
        LCDWriteLines("Playing   Song 3", "Pause   Play    ");
      }
    break;
    case LO_MenuOptionC:
      if(menuOption == 0) {
        state = LO_MenuOptionA;
        LCDWriteLines("Song 1  Song 2  ", "Song 3  Select ");
      }
      else if (menuOption == 1){
        state = LO_MenuOptionB;
        LCDWriteLines("Playing   Song 1", "Pause   Play    ");
      }
      else if (menuOption == 3){
        state = LO_MenuOptionD;
        LCDWriteLines("Playing   Song 3", "Pause   Play    ");
      }
    break;
    case LO_MenuOptionD:
      if(menuOption == 0) {
        state = LO_MenuOptionA;
        LCDWriteLines("Song 1  Song 2  ", "Song 3  Select ");
      }
      else if (menuOption == 1){
        state = LO_MenuOptionB;
        LCDWriteLines("Playing   Song 1", "Pause   Play    ");
      }
      else if (menuOption == 2){
        state = LO_MenuOptionC;
        LCDWriteLines("Playing   Song 2", "Pause   Play    ");
      }
    break;    
  }

  /* KEEP CODE IN HERE JUST TO SHOW WE DO NOT USE STATE ACTIONS
  switch (state) { // State Action
    case LO_MenuOptionA:
    
    break;
    case LO_MenuOptionB:
   
    break;
  }
*/
  return state;
}

// Task 2
int TickFct_JoystickInput(int state) {
  //returns 1 if the joystick was up, 2 if it is down, 0 for anything else
  //returns 1 if the joystick was right, 2 if it is left, 0 for anything else
  switch (state) { // State Transitions
    case JI_init:
      state = JI_NoMovement;
    break;
    case JI_NoMovement: // waiting for an input...
      if ((readStickY() != 0) || (readStickX() != 0)){
        state = JI_Movement;
        if (readStickX() == 1 && (cursor_pos != 1 && cursor_pos != 3)){
          cursor_pos++;
        }
        else if (readStickX() == 2 && (cursor_pos != 0 && cursor_pos != 2)){
          cursor_pos--;
        }
        
        if (readStickY() == 1 && (cursor_pos > 1)){
          cursor_pos -= 2;
        }
        else if (readStickY() == 2 && (cursor_pos < 2)){
          cursor_pos += 2;
        }
      }
      else if (!digitalRead(10)){
        state = JI_Movement;
        js_pressed = 1; //set to 0 in controller
      } 
    break;
    case JI_Movement: // we have movement at this point, but we do not want constant readings,
      //so we will wait for no movement so we can wait for an input again
      if (!(readStickY() != 0) || (readStickX() != 0)){
        state = JI_NoMovement;
      }
    break;
  }

  /* KEEP CODE IN HERE JUST TO SHOW WE DO NOT USE STATE ACTIONS
   switch (state) { // State Actions
    case JI_init:
    break;
    case JI_NoMovement:
    break;
    case JI_Movement:
    break;
    default:
    break;    
  }
*/
  return state;
}

// Sound Output
// THIS FUNCTION IS NOT MY CODE! PROVIDED BY TA'S
int counter = 0;
int note = 0;
int TickFct_SoundOutput(int state) {
  switch (state) { // State Transitions
    case SO_init:
      state = SO_SoundOff;
    break;
    case SO_SoundOn:
      if (menuOption <= 0){
        state = SO_SoundOff;
      }
      else{
        if (menuOption == 1){
          if(counter >= song1_time[note]) {
            state = SO_SoundOff;
            note++;
            counter = 0;
            if (song_note_length <= note){
              menuOption = 0;
            }
            note = note % 20;
          }
        }
        else if (menuOption == 2){
          if(counter >= song2_time[note]) {
            state = SO_SoundOff;
            note++;
            counter = 0;
            if (song_note_length <= note){
              menuOption = 0;
            }
            note = note % 20;
            //menuOption = 0;
          }
        }
        else if (menuOption == 3){
          if(counter >= song3_time[note]) {
            state = SO_SoundOff;
            note++;
            counter = 0;
            if (song_note_length <= note){
              menuOption = 0;
            }
            note = note % 20;
            //menuOption = 0;
          }
        }

        if (cursorArr[2] == 1 && cursorArr[3] == 0){
          state = SO_SoundPause;          
        }        
      }
    break;
    case SO_SoundOff:
      if (menuOption > 0){
        state = SO_SoundOn;
      }
    break;
    case SO_SoundPause:
      if (cursorArr[2] == 0 && cursorArr[3] == 1){
        state = SO_SoundOn;
      }
    break;
    default:
      state = SO_SoundOff;
    break;
    
  }
   switch (state) { // State Actions
    case SO_SoundOn:
      if (menuOption == 1){
        tone(buzzer, song1[note], periodSoundOutput * song1_time[note]);
        counter++;
      }
      else if (menuOption == 2){
        tone(buzzer, song2[note], periodSoundOutput * song2_time[note]);
        counter++;
      }
      else if (menuOption == 3){
        tone(buzzer, song3[note], periodSoundOutput * song3_time[note]);
        counter++;
      }
    break;
    case SO_SoundOff:
      noTone(buzzer);
    break;

  }
  return state;
}

void PrintCursor(){ //set *'s if necessary. At the end, set cursor back to where it belongs.
  // clear the cursors
  lcd.noCursor();
  if (menuOption == 0){
    lcd.setCursor(6,0);
    lcd.print("  ");
  }
  lcd.setCursor(6,1);
  lcd.print("  ");

  // apply *'s if necessary
  if (menuOption == 0){
    if (cursorArr[0] == 1){
      lcd.setCursor(6,0);
      lcd.print("*");
    }
    else if (cursorArr[1] == 1){
      lcd.setCursor(7,0);
      lcd.print("*");
    }
    else if (cursorArr[2] == 1){
      lcd.setCursor(6,1);
      lcd.print("*");
    }
    else if (cursorArr[3] == 1){
      lcd.setCursor(7,1);
      lcd.print("*");
    }
  }
  else{
    if (cursorArr[2] == 1){
      lcd.setCursor(6,1);
      lcd.print("*");
    }
    else if (cursorArr[3] == 1){
      lcd.setCursor(7,1);
      lcd.print("*");
    }
  }

  if (menuOption == 0){
    switch (cursor_pos){
      case 0:
        lcd.setCursor(6,0);
      break;
      case 1:
        lcd.setCursor(7,0);
      break;
      case 2:
        lcd.setCursor(6,1);
      break;
      case 3:
        lcd.setCursor(7,1);
      break;
      default:
        lcd.setCursor(6,0);
      break;
    }
  }
  else{
    if (cursor_pos <= 0 || cursor_pos == 2){
      lcd.setCursor(6,1);
    }
    else{
      lcd.setCursor(7,1);
    }
  }
  lcd.cursor();
}

// Handles the cursor 
int TickFct_Controller(int state) {

  /* KEEP JUST TO SHOW WE DO NOT USE STATE TRANSITIONS
  switch (state) { // State Transitions
    case C_init:
      state = C_Main;
    break;
    case C_Main:
      
    break;
    default:
      state = C_Main;
    break;
  }
*/

   switch (state) { // State Actions
     /*
     I found that setting and printing our cursor was constantly a hassle,
     so I used a separate state machine to get place our cursor based on cursor_pos which should only change
     mainly from joystick input.

     That is all this state machine really does. Because each menu does not have consistent placing for the cursor,
     a state machine was used.
     For example, the menu has four different areas where the cursor can be,
     while for a song, the cursor can only be on the bottom row.
       */
    case C_init:
    break;
    case C_Main:
      lcd.cursor();
      if (menuOption == 0){
        if (cursor_pos == 0){
          lcd.setCursor(6, 0);        
        }
        else if (cursor_pos == 1){
          lcd.setCursor(7, 0);
        }
        else if (cursor_pos == 2){
          lcd.setCursor(6, 1);
        }
        else if (cursor_pos == 3){
          lcd.setCursor(7, 1);
        }
      }
      else{
        if (cursor_pos <= 0 || cursor_pos == 2){
          lcd.setCursor(6,1);
        }
        else{
          lcd.setCursor(7,1);
        }
      }
      //Serial.println(cursor_pos);

      if (js_pressed == 1){
        if (cursorArr[cursor_pos] == 1){ // just to deselect anything
          cursorArr[cursor_pos] = 0;
          lcd.print(" ");
        }
        else{ // to either enter a new menu, play song, and/or select
          if (menuOption == 0){
            if (cursor_pos >= 3){ //play song
              for (int i = 0; i < 3; i++){
                if (cursorArr[i] == 1){
                  menuOption = i+1;
                  Serial.println(menuOption);
                }
              }
            }
            else{
              if (cursorArr[cursor_pos] == 0){
                ClearCursorArr(); //only one song can be selected
                cursorArr[cursor_pos] = 1;
                lcd.print("*");
              }
              else{
                ClearCursorArr();
                cursorArr[cursor_pos] = 0;
              }

              PrintCursor();
              
              if (cursor_pos == 0){
                lcd.setCursor(6, 0);        
              }
              else if (cursor_pos == 1){
                lcd.setCursor(7, 0);
              }
              else if (cursor_pos == 2){
                lcd.setCursor(6, 1);
              }
              else if (cursor_pos == 3){
                lcd.setCursor(7, 1);
              }
            }
          }
          else{ // if not menuOption == 0
            // in this case, cursor_pos can still be between 0-3, but cursorArr must keep values 0 and 1 at 0.
            if (cursor_pos <= 0 || cursor_pos == 2){
              lcd.setCursor(6,1);
              if (cursorArr[2] == 0){
                ClearCursorArr(); // can only select either pause or play
                cursorArr[2] = 1;
                lcd.print("*");
              }
              else{
                ClearCursorArr();
                cursorArr[2] = 0;
              }
            }
            else{
              lcd.setCursor(7,1);
              if (cursorArr[3] == 0){
                ClearCursorArr(); // can only select either pause or play
                cursorArr[3] = 1;
                lcd.print("*");
              }
              else{
                ClearCursorArr();
                cursorArr[3] = 0;
              }
            }
            PrintCursor();          
          }
        }
        js_pressed = 0;
      }
    break;
  }
  return state;
}



void InitializeTasks() {
  unsigned char i=0;
  tasks[i].state = LO_init;
  tasks[i].period = periodLCDOutput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_LCDOutput;
  ++i;
  tasks[i].state = JI_init;
  tasks[i].period = periodJoystickInput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_JoystickInput;
  ++i;
  tasks[i].state = SO_init;
  tasks[i].period = periodSoundOutput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_SoundOutput;
  ++i;
  tasks[i].state = C_init;
  tasks[i].period = periodController;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_Controller;

}

void setup() {
  // put your setup code here, to run once:
  InitializeTasks();
  pinMode(10, INPUT_PULLUP);
  TimerSet(tasksPeriodGCD);
  TimerOn();
  Serial.begin(9600);
  // Initialize Outputs
  lcd.begin(16, 2);
  // Initialize Inputs

}

void loop() {
  // put your main code here, to run repeatedly:
  // Task Scheduler with Timer.h
  
}
