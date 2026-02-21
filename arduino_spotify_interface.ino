#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int JOY_X = A0, JOY_Z = 2;

// CRASH-PROOF MEMORY BUFFERS
const byte numChars = 100;
char receivedChars[numChars]; 
boolean newData = false;

char trackInfo[80] = "Waiting for Sync..."; 
int scrollPos = 0;
unsigned long lastUpdate = 0;

long p_ms = 0, d_ms = 1000;
int isPlaying = 0;

void setup() {
  Serial.begin(115200);
  
  // 1. POWER STABILIZATION (The first missing fix)
  delay(500); 

  Wire.begin();
  Wire.setClock(100000); 
  
  // 2. DOUBLE LCD INIT (The anti-freeze/anti-square fix)
  lcd.init();
  delay(100);
  lcd.init(); 
  lcd.backlight();

  // 3. SAFE OLED BOOT
  if(oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oled.clearDisplay(); 
    oled.display();
  }

  // 4. THE BUFFER PURGE (Clears the Python flood so it can actually boot)
  while(Serial.available() > 0) { 
    Serial.read(); 
  }
  
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM SECURE");
  
  pinMode(JOY_Z, INPUT_PULLUP);
}

void loop() {
  // NON-BLOCKING SERIAL READ
  recvWithEndMarker();
  
  if (newData == true) {
    parseData();
  }

  // REFRESH RATE
  if (millis() - lastUpdate > 300) {
    updateDisplays();
    lastUpdate = millis();
  }

  // JOYSTICK COMMANDS
  if (analogRead(JOY_X) > 800) { Serial.println("NEXT"); delay(400); }
  if (analogRead(JOY_X) < 200) { Serial.println("PREV"); delay(400); }
  if (digitalRead(JOY_Z) == LOW) { Serial.println("TOGGLE"); delay(400); }
}

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

void parseData() {
  if (strncmp(receivedChars, "D:", 2) == 0) {
    char *strtokIndx; 
    
    strtokIndx = strtok(receivedChars + 2, ","); 
    if (strtokIndx != NULL) p_ms = atol(strtokIndx);
    
    strtokIndx = strtok(NULL, ","); 
    if (strtokIndx != NULL) d_ms = atol(strtokIndx);
    
    strtokIndx = strtok(NULL, ","); 
    
    strtokIndx = strtok(NULL, ","); 
    if (strtokIndx != NULL) isPlaying = atoi(strtokIndx);
    
    strtokIndx = strtok(NULL, "\n"); 
    if (strtokIndx != NULL) {
      if (strcmp(trackInfo, strtokIndx) != 0) {
        strncpy(trackInfo, strtokIndx, sizeof(trackInfo) - 1);
        trackInfo[sizeof(trackInfo) - 1] = '\0'; 
        scrollPos = 0;
      }
    }
  }
  newData = false; 
}

void updateDisplays() {
  // --- FLICKER-FREE LCD SCROLLING ---
  char lcdBuffer[17]; 
  int len = strlen(trackInfo);

  if (len <= 16) {
    snprintf(lcdBuffer, 17, "%-16s", trackInfo); 
  } else {
    for (int i = 0; i < 16; i++) {
      int charIndex = (scrollPos + i) % (len + 3); 
      if (charIndex < len) {
        lcdBuffer[i] = trackInfo[charIndex];
      } else {
        lcdBuffer[i] = ' ';
      }
    }
    lcdBuffer[16] = '\0';
    
    scrollPos++;
    if (scrollPos >= len + 3) scrollPos = 0;
  }

  lcd.setCursor(0, 0);
  lcd.print(lcdBuffer);
  
  lcd.setCursor(0, 1);
  lcd.print("                ");

  // --- OLED DASHBOARD ---
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setCursor(0, 0);
  oled.print(isPlaying ? "PLAYING" : "PAUSED");

  for(int i=0; i<15; i++) {
    int h = isPlaying ? random(5, 25) : 2;
    oled.fillRect(i*8 + 4, 35-h/2, 5, h, WHITE);
  }

  oled.drawRect(0, 52, 128, 10, WHITE);
  
  long duration_sec = d_ms / 1000;
  if (duration_sec <= 0) duration_sec = 1; 
  
  long current_sec = p_ms / 1000;
  long barWidth = map(constrain(current_sec, 0, duration_sec), 0, duration_sec, 0, 124);
  
  oled.fillRect(2, 54, barWidth, 6, WHITE);
  oled.display();
}