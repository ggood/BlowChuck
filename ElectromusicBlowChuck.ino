#include "Wire.h"
#include "WiiChuck.h"

/*
Notes: 3/3/13 - switched scene launch to happen when both
Z and C buttons released, rather than by shaking the
nunchuck.



*/
// The MIDI channel on which we send note data from
// breath actions
#define MIDI_CHANNEL 1
// The MIDI channel we use for Ableton Live control
#define SCENE_MGMT_MIDI_CHANNEL 2
// The MIDI note for Live scene launch
#define SCENE_LAUNCH_MIDI_NOTE 0
// The MIDI note for the Live previous scene action
#define SCENE_PREV_MIDI_NOTE 1
// The MIDI note for the Live next scene action
#define SCENE_NEXT_MIDI_NOTE 2
// Suppress multiple scene lanches if < 100 ms apart
#define SCENE_LAUNCH_DELAY 100

// The threshold level for sending a note on event. If the
// sensor is producing a level above this, we should be sounding
// a note.
#define NOTE_ON_THRESHOLD 80
// The maximum raw pressure value you can generate by
// blowing into the tube.
#define MAX_PRESSURE 500

// The three states of our state machine
// No note is sounding
#define NOTE_OFF 1
// We've observed a transition from below to above the
// threshold value. We wait a while to see how fast the
// breath velocity is increasing
#define RISE_TIME 10
// A note is sounding
#define NOTE_ON 3
// Send aftertouch data no more than every AT_INTERVAL
// milliseconds
#define AT_INTERVAL 70

// The five notes, from which we choose one at random
unsigned int notes[5] = {
  60, 62, 65, 67, 69};

// We keep track of which note is sounding, so we know
// which note to turn off when breath stops.
int noteSounding;
// The value read from the sensor
int sensorValue;
// The state of our state machine
int state;
// The time that we noticed the breath off -> on transition
unsigned long breath_on_time = 0L;
// The breath value at the time we observed the transition
int initial_breath_value;
// The aftertouch value we will send
int atVal;
// The last time we sent an aftertouch value
unsigned long atSendTime = 0L;

// The nunchuck object
WiiChuck chuck = WiiChuck(); // The nunchuck controller
// Current button state from nunchuck
byte buttonState = 0;
byte prevButtonState = 0;
// True when both buttons pressed
boolean sceneLaunchArmed;
// Accelerometer history
int xVal, yVal, zVal, zSum, zAvg;
int zValues[10] = {0};
unsigned long noteOnTime;
int i;


void setup() {
  state = NOTE_OFF;  // initialize state machine
  // Initialize the nunchuck-related things
  prevButtonState = buttonState = 0;
  chuck.begin();
}

int get_note() {
  return notes[random(0,4)];
}

int get_velocity(int initial, int final, unsigned long time_delta) {
  //return map(final, NOTE_ON_THRESHOLD, MAX_PRESSURE, 0, 127);
  return map(constrain(final, NOTE_ON_THRESHOLD, MAX_PRESSURE), NOTE_ON_THRESHOLD, MAX_PRESSURE, 0, 127);
}

// TODO(ggood) rewrite in terms of bit shift operations
byte get_button_state() {
  if (chuck.buttonC) {
    if (chuck.buttonZ) {
      return 0x03;
    } else {
      return 0x02;
    }
  } else {
    if (chuck.buttonZ) {
      return 0x01;
    } else {
      return 0x0;
    }
  }
}

void scene_next() {
  usbMIDI.sendNoteOn(SCENE_NEXT_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL);
  usbMIDI.sendNoteOff(SCENE_NEXT_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL);
}

void scene_prev() {
  usbMIDI.sendNoteOn(SCENE_PREV_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL);
  usbMIDI.sendNoteOff(SCENE_PREV_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL); 
}

void scene_launch() {
  usbMIDI.sendNoteOn(SCENE_LAUNCH_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL);
  usbMIDI.sendNoteOff(SCENE_LAUNCH_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL);
}

void loop() {
  // Process nuncheck data
  chuck.update(); 
  delay(1);

  // Deal with accelerometer
  zVal = chuck.readAccelZ();
  zSum -= zValues[i];
  zSum += zVal;
  zValues[i] = zVal;
  i = (i + 1) % 10;
  zAvg = zSum / 10;
  if (zAvg > 500) {
    if (millis() - noteOnTime > SCENE_LAUNCH_DELAY) {
      //usbMIDI.sendNoteOn(SCENE_LAUNCH_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL);
      //usbMIDI.sendNoteOff(SCENE_LAUNCH_MIDI_NOTE, 100, SCENE_MGMT_MIDI_CHANNEL);
      //noteOnTime = millis();
    }
  }

  // Check for scene up/down and launch. All actions occur on
  // button release.
  prevButtonState = buttonState;
  buttonState = get_button_state();
  sceneLaunchArmed = (buttonState == 0x03) || sceneLaunchArmed;
  if (buttonState == 0x0) {
    if (sceneLaunchArmed) {
      scene_launch();
      sceneLaunchArmed = false;
    } else if (prevButtonState != buttonState) {
      // Button state transitioned
      switch (prevButtonState) {
      case 0x01:
        scene_next();
        break;
      case 0x02:
        scene_prev();
        break;
      case 0x03:
        //scene_launch();
        break;
      }
    }
  }

  // Process pressure sensor data
  // read the input on analog pin 0
  sensorValue = analogRead(A0);
  if (state == NOTE_OFF) {
    if (sensorValue > NOTE_ON_THRESHOLD) {
      // Value has risen above threshold. Move to the RISE_TIME
      // state. Record time and initial breath value.
      breath_on_time = millis();
      initial_breath_value = sensorValue;
      state = RISE_TIME;  // Go to next state
    }
  } 
  else if (state == RISE_TIME) {
    if (sensorValue > NOTE_ON_THRESHOLD) {
      // Has enough time passed for us to collect our second
      // sample?
      if (millis() - breath_on_time > RISE_TIME) {
        // Yes, so calculate MIDI note and velocity, then send a note on event
        noteSounding = get_note();
        int velocity = get_velocity(initial_breath_value, sensorValue, RISE_TIME);
        usbMIDI.sendNoteOn(noteSounding, velocity, MIDI_CHANNEL);
        state = NOTE_ON;
      }
    } 
    else {
      // Value fell below threshold before RISE_TIME passed. Return to
      // NOTE_OFF state (e.g. we're ignoring a short blip of breath)
      state = NOTE_OFF;
    }
  } 
  else if (state == NOTE_ON) {
    if (sensorValue < NOTE_ON_THRESHOLD) {
      // Value has fallen below threshold - turn the note off
      usbMIDI.sendNoteOff(noteSounding, 100, MIDI_CHANNEL);  
      state = NOTE_OFF;
    } 
    else {
      // Is it time to send more aftertouch data?
      if (millis() - atSendTime > AT_INTERVAL) {
        // Map the sensor value to the aftertouch range 0-127
        atVal = map(sensorValue, NOTE_ON_THRESHOLD, 1023, 0, 127);
        usbMIDI.sendAfterTouch(atVal, MIDI_CHANNEL);
        atSendTime = millis();
      }
    }
  }
}

