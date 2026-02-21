# Dual-Display Arduino Spotify Controller

A hardware-based Spotify controller built with an Arduino, a 16x2 LCD, an SSD1306 OLED, and an analog joystick. This project uses a Python script to bridge data between the Spotify Web API and the Arduino over a serial connection.



## Overview

The system consists of two main components:
1. **Python Bridge:** Connects to the Spotify Web API, retrieves the current playback state, formats the data, and sends it to the Arduino via USB serial.
2. **Arduino Firmware:** Reads the incoming serial data, parses it, and updates the dual I2C displays. It also reads analog joystick inputs to send playback commands (Next, Previous, Play/Pause) back to Python.

## Core Architecture

### 1. Serial Parsing (Arduino/C++)
To manage the Arduino's limited SRAM, this firmware uses fixed character arrays and standard C string functions (`strtok`) instead of Arduino `String` objects. Incoming serial data is read asynchronously into a buffer.

```cpp
// Non-blocking serial read into a fixed 100-byte buffer
void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) ndx = numChars - 1; 
    } else {
      receivedChars[ndx] = '\0'; 
      ndx = 0;
      newData = true;
    }
  }
}

// Tokenizing the data array
void parseData() {
  if (strncmp(receivedChars, "D:", 2) == 0) {
    char *strtokIndx; 
    
    strtokIndx = strtok(receivedChars + 2, ","); 
    if (strtokIndx != NULL) p_ms = atol(strtokIndx); // Progress (ms)
    
    strtokIndx = strtok(NULL, ","); 
    if (strtokIndx != NULL) d_ms = atol(strtokIndx); // Duration (ms)
    
    // ... continues parsing playback state and track info
  }
}
