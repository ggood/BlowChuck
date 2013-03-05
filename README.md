ElectromusicBlowChuck
=====================

Code for an Arduino-based breath-controlled instrument using
Wii Nunchuck controllers

This sketch is for a breath-controlled MIDI musical instument
controller with the following features:

- Uses a pressure sensor to detect the player's breath. The
  values from the sensor are output as MIDI Breath Controller
  messages. MIDI Note/On messages are sent when the breath
  pressure exceeds/falls below a configurable threshhold
  value. Note and breath controller data is sent on MIDI
  channel 1.
- Uses a Wii Nunchuck controller to control performance
  metadata. The Nunchuck buttons (two of them) are configured
  to send two different MIDI notes on MIDI channel 2, and a
  third note is sent when both buttons are held and released.
  These three notes can be mapped, respectively, to
  Ableton Live's previous scene, next scene, and scene launch
  controls.
- Uses the Nunchuck's accelerometers to detect pitch and
  roll, and maps those values to MIDI continuous controller
  data sent on MIDI channel 1.
- Uses the Nunchuck's joystick to select one of 9 different
  MIDI notes. This allows the performer to choose which MIDI
  note sounds when the breath sensor activates.
  
Requirements:

- As written, the code expects to run on a PJRC Teensy
  microcontroller (www.pjrc.com), which supports USB over
  MIDI. The code can be adapted to an Arduino platform
  without USB MIDI support by replacing the calls to
  usbMIDI.* with calls into another MIDI library. You
  will also need to add a MIDI interface to your Ardiuno.
  
Future Ideas:
- Utilize a second Nunchuck controller. This turns out to
  be difficult since every Nunchuck has the same I2C bus
  address. It may be possible on a microcontroller platform
  with more than one I2C bus.

The BlowChuck.ino file is licensed with a BSD-style license.
The WiiChuck.h file is based on work found on the Internet,
and is not covered by the BlowChuck.ino file's license.
