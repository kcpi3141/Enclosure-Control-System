/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
--------------------- Initializations -----------------------
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

// PIN DEFINITIONS
  // MEGA interrupt pins: 2,3,18,19,20,21
  // SDA, SCL: 20, 21
  // MISO, MOSI, SCK: 50, 51, 52
// Auxiliary Pins
const int lightDimming_PIN = 26;            // PWM Signal (analog OUT)
const int terrariumLevelLow_PIN = 32;       // Optical water level switches (digital IN)
const int reservoirLevelLow_PIN = 31;       // Optical water level switches (digital IN)
const int terrariumWaterTemp_PIN = 5;       // Temp probe (analog)
const int fanExhaustSpeed_PIN = 9;          // Exhaust fan PWM signal (analog OUT)
const int fanCirculationSpeed_PIN = 10;     // Circulation fan PWM signal (analog OUT)
const int backSwitch_PIN = 2;               // Turns menu ON/back button (digital IN) (interrupt)
const int rotarySwitch_PIN = 3;             // Switch button on the rotary encoder (digital IN) (interrupt)
const int rotaryCLK_PIN = 18;               // CLK input of rotary encoder (digital IN) (interrupt)
const int rotaryDT_PIN = 45;                // DT input of rotary encoder (digital IN)
// Relay Pins
const int LCDbackLight_PIN = 21;        // Turns LCD display backlight ON/OFF. Need 3.3V divider.
const int waterfallRelay_PIN = 14;      // Dripwall (AC Relay)
const int fanExhaust_PIN = 9;           // Exhaust fan (FET)
const int fanCirculation_PIN = 10;      // Circulation fan (FET)
const int lightRelay_PIN = 11;          // LED light (AC Relay)
const int mistRelay_PIN = 12;           // Mist King (AC Relay)
const int heaterRelay_PIN = 13;         // Heater (for emergency shut off) (AC Relay). Might be unnecessary.

// LIBRARIES -----------------------------------------------
#include <SPI.h>        // Serial Peripheral Interface
#include <Wire.h>       // I2C Ports
#include <Adafruit_GFX.h>

// OLED DISPLAY --------------------------------------------
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// LCD DISPLAY ---------------------------------------------
  // Connect SDA to MOSI not I2C
#include <Adafruit_ST7735.h>
#define CS_PIN  10      
#define DC_PIN  9           // A0 on the display
#define RESET_LCD_PIN  -1   // Any pin or set to -1 and connect to Arduino RESET pin
Adafruit_ST7735 tft = Adafruit_ST7735(CS_PIN, DC_PIN, RESET_LCD_PIN);
// Colors
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
void initializeLCD () {
  tft.initR(INITR_144GREENTAB);         // Init ST7735R chip, green tab (LCD Display)
  tft.fillScreen(ST77XX_BLACK);         // Fills the screen black
  digitalWrite(LCDbackLight_PIN, HIGH); // Turns on LCD backlight
  // Make pretty
}

// CLOCK -----------------------------------------
#include "RTClib.h"
RTC_DS3231 rtc;  // Define module
DateTime now;    // Create DateTime object
boolean timeCheck () {         // Checks if the RTC is working
  if (now.year() == 2165) {       // 2165 is the default error year
    return (false);
  } else {
    return (true);
  }
}

// BME280 SENSOR --------------------------------------------
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define BME_SCK 13    // SCL
#define BME_MISO 12   
#define BME_MOSI 11   // SDA
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1016)   // Change as needed
Adafruit_BME280 bme;      // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

// Arduino Functions --------------------------------------------
void(* resetFunc) (void) = 0;     //declare reset function at address 0




/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
-------------------------- VALUES ---------------------------
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

// DEVICE STATES -----------------------------------------
int lightRelayState = 0;    // If 1, relay turns ON, 0 turns OFF
int mistRelayState = 0;
int fanRelayState = 0;

// LOOP FUNCTION -----------------------------------------------
const int loopDelayBaseline = 3000;
const int menuLoopDelay = 50;        // Menu Function
const int inputLoopDelay = 50;       // input Mode Function
int loopDelay = loopDelayBaseline;   // Allows modification of loop delay. Initializes at baseline

// PARAMETER VARIABLES -------------------------------------------
// Light schedule (hrs)
int lightStart = 10;           // Time when light turns ON (hrs) (0-23) (USER INPUT)
int lightPeriod = 12;          // Length of the time the light is ON (hrs) (0-24) (USER INPUT)
int maxLightIntensity = 50;    // Max light intensity on the dimming schedule and intensity for constant schedule (0-100%) (USER INPUT)
int dimmingMode = 0;           // Turns on dimming schedule. See Menu Navigation for key (USER INPUT)

// Relative humidity (% RH) & Air Pressure
int humidityDayMax = 80;    // All adjustable (0-100) (USER INPUT)
int humidityDayMin = 60;
int humidityNightMax = 90;
int humidityNightMin = 70;
float currentHumidity = 0;  // Records the current RH reading
float currentPressure = 0;  // Extra info
int fanSpeed = 0;           // PWM signal for fan speed (0-100)

// Temperature (C)
int tempMax = 23;           // Difficult to control, so no difference between night and day
int tempMin = 13;
float currentAirTemp = 0;   // Records the current Temp reading in the air (BME280)
float currentWaterTemp = 0; // Records the current Temp reading in the water (DS18B20)

// Water Level
boolean terrariumLevelLow = false;
boolean reservoirLevelLow = false;




/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
--------------------- PARAMETER FUNCTIONS --------------------
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

int dimmingConversion (int percentBright) { // percentBright is 0-100
  float dimVar = percentBright;   // Duty cycle to dim the light
  dimVar = -dimVar + 100;         // Duty cycle is inverse of the brightness (100% = dimmest, 0% = brightest)
  dimVar = dimVar * 2.55;         // Turn duty cycle % into PWM signal value (0-255)
  dimVar = round(dimVar);         // Round to int
  return dimVar;
}




/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  -------------------------- LCD MENU -------------------------
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

// LCD MENU VARIABLES ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// MENU NAVIGATION
int menuLevel = 0;                    // Depth in the submenus
int lastMenuLevel = 0;                // Records the last menuLevel value for the next loop

// MENU DEVICE STATES ---------------------------
int menuLightIntensity = 50;

// INPUTS -------------------------
volatile int backSwitch = 0;          // Increases each time the go-back switch is pressed
volatile int rotarySwitch = 0;        // Increases each time the rotary encoder switch is pressed
volatile int rotaryCounter = 0;       // Increases if rotated CCW, decreases CW     
int lastStateCLK;                     // Determines direction of rotation in encoder
int cursorPosition;                   // Records the current cursor position (0 for first)
int lastCursorPosition;
int inputValueUpperLimit = 0;         // Maximum number the rotary counter can add up to from 0
int inputValueLowerLimit = 0;
int* inputValueVariable;              // Determines which values are being modified   
int inputValuePin;                    // For directly controlling outputs
boolean isPWM = false;                // To differentiate between analog and digital outputs
boolean isText = false;               // Display text instead of numbers for the input         

// DISPLAY -----------------------------
  // (determines different bitmaps to display) 
boolean showContents = true;          // Show content bitmap. Directory and header always show
boolean showCursor = true;            // Show cursor bitmap
boolean inputModeActive = false;   // TRUE to stay in settings mode
boolean overrideModeActive = false;  // TRUE to stay in override mode

// TIMERS -------------------------
int idleTimeLimit_LCD = 2;            // Maximum time allowed to stay idle on menu (min) (1440 max) (USER INPUT)
long idleTime_LCD = 0;                // Record the time passed since last action
long idleTimeStart_LCD = 0;           // Time of begin idle
int inputModeTimeLimit = 1;           // Maximum time allowed to stay idle on inputMode (min)



// LCD Menu bitmaps +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// BITMAP DEFINITIONS
  // Adjust array sizes according to LCD screen dimensions
const int maxOptionNum = 4;           // Max number of items each menu page can hold/fit onto the screen (max 4 for textsize 2)
const int maxMenuLevelNum = 6;        // Max number of menu subpage levels (no limit)
typedef struct bitmap {
  int x;            // Starting coordinates (x,y)
  int y;
  int w;            // Width and height of the bitmap (w,h)
  int h;
  String Str[maxOptionNum];    // Words to display. Only the contents bitmap uses more array elements
};
struct bitmap directory = {0,0,128,8};
struct bitmap header = {0,15,128,16};
struct bitmap contents = {0,30,110,98};
struct bitmap cursors = {110,30,18,98};     // "cursor" is a key word, so must use "cursors"
const char cursor_char = '<';               // Symbol for the cursor
struct bitmap inputModeMap = {0,25,128,98}; // Used for both inputMode and overrideMode

// GFXcanvas1 FRAME BUFFER ---------------------------------------
GFXcanvas1 bitmaps[] = {
  GFXcanvas1(directory.w, directory.h),
  GFXcanvas1(header.w, header.h),
  GFXcanvas1(contents.w, contents.h),
  GFXcanvas1(cursors.w, cursors.h),
  GFXcanvas1(inputModeMap.w, inputModeMap.h)
};



// Menu Page Navigation ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* To add new menu item:
    1. Determine where it will be logically and place it in the diagram. If it does not have options (UI) then go to 2. If it does:
      1a. Create another const char array with a name that corresponds to its directory
      1b. Add elements into the array to make options
      1c. Comment and explain each option's purpose
    2. Add an entry into findAndReturnOptions(). Update any submenu contents it is contained in
      2a. Set CDString to its directory
      2b. Insert its header. If it does not have options (UI) then go to 3. If it does:
      2c. Insert the desired contents that lists out the different options
      2d. Return the address of the const char array made earlier to use the directory to display the proper bitmap later. Go to 4
    3. If UI, set inputModeActive to TRUE
      3a. Set inputValueVariable to the address of the variable you are changing
      3b. Set inputValueUpperLimit to the maximum value inputValueVariable can go. Default for inputValueLowerLimit is 0, but can also change
      3c. If the value to be changed is a state that is represented by text, set isText to TRUE and insert the words to each element of inputModeMap.Str[i]
      3d. If the value to be changed controls a specific pin and you want to see the effects of that in real time, set inputValuePin to the pin you want to modify
      3e. Return nothing
    4. Test to see if everything is properly displayed and adjust. Done.
*/

/* Menu Logic Chart. Each row is a menuLevel, column are options. (UI) = USER input, (UP) = USER PROGRAMMED, '+' = ON, '-' = OFF
  'Settings'
  'Humidity'                                             ||'Light'                                                                                            || 'Timer' (UI)
  'H_Day'                    |'H_Night'                  ||'L_Start Time'(UI)|'L_Period ON'(US)|'L_Intensity',                       |'L_Dimming Duration'(UI)||
  'H_D_Max'(UI),'H_D_min'(UI)|'H_N_Max'(UI),'H_N_min'(UI)||                                    |'L_I_Constant'(UI),'L_I_Variable'(UP)|                        ||

  'Override'
  'Light','Mist','Fan'
*/

// MAIN MENU -------------------------------
const char mainMenuOptions[maxOptionNum] = {'S','O'};  // 'Settings','Override'
  // 1. Settings: To change parameters
  // 2. Override: Allows user to change enclosure settings temporarily, which will return to normal after exiting override and back into mainMenu

// SETTINGS --------------------------------
const char settingsOptions[maxOptionNum] = {'L','H','T'}; // 'Humidity','Light Cycle'
const char S_H_Options [maxOptionNum] = {'D','N'};    // 'Day','Night'
const char S_H_D_Options [maxOptionNum] = {'M','m'};  // 'Max','min' (USER INPUT)
const char S_H_N_Options [maxOptionNum] = {'M','m'};  // 'Max','min' (USER INPUT)
const char S_L_Options [maxOptionNum] = {'S','P','I','D'}; // 'Start Time','Photoperiod','Intensity','Dimming Mode'
  // 1. Hour to begin (USER INPUT)
  // 2. How long light will be ON (USER INPUT)
  // 3. Maximum intensity the light will be at (USER INPUT)
  // 4. What kind of dimming cycle to use (USER INPUT)
    // 4a. if dimmingMode = 0, Sine Curve [0,pi]
    // 4b. if dimmingMode = 1, Constant Intensity @maxLightIntensity

// OVERRIDE --------------------------------
const char overrideOptions[maxOptionNum] = {'L','M','F'}; // 'Light','Mist','Fan' (Mist: USER INPUT)
const char O_L_Options [maxOptionNum] = {'R','I'};        // 'Relay State','Intensity' (USER INPUT)

// WORKSPACE -----------------------------
char currentDirectory[maxMenuLevelNum];            // Array recording the current menu page directory. Each element represents a menuLevel
char* currentDirectory_ptr = currentDirectory;
char* currentOptions;                              // Returns the menu subpage options for currentDirectory



// Menu Page Sorting ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char* findAndReturnOptions () {
  inputValuePin = 0;        // Turn on real time adjustments only if called for. 0 is non-existent pin
  inputValueLowerLimit = 0; // Minimum a value to be changed can go
  isPWM = false;            // Differentiates between analog and digital outputs
  isText = false;           // Default input display is for numbers
  String CDString = charToString(currentDirectory_ptr, sizeof(currentDirectory));
  
  if (*currentDirectory_ptr == 0) {       // Main menu
    header.Str[0] = "+++ Main Menu +++";
    contents.Str[0] = "Settings";
    contents.Str[1] = "Override";
    contents.Str[2] = "";
    contents.Str[3] = "";
    overrideModeActive = false;
    return &mainMenuOptions[0];
  }

  //  ---------------------------------------------------------
  
  if (CDString == "S") {                   // Settings
    header.Str[0] = "+++ Settings +++";
    contents.Str[0] = "Light";
    contents.Str[1] = "Humidity";
    contents.Str[2] = "Timer";
    contents.Str[3] = "";
    return &settingsOptions[0];
  }if (CDString == "SL") {                 // Settings-Light
    header.Str[0] = "+ Set Lighting +";
    contents.Str[0] = "Start";
    contents.Str[1] = "Period";
    contents.Str[2] = "Intensity";
    contents.Str[3] = "Dimming";
    return &S_L_Options[0];
  }
  if (CDString == "SLS") {                // Settings-Light-Start(Time)
    header.Str[0] = "+ Set Start Time +";
    inputValueVariable = &lightStart;
    inputValueUpperLimit = 23;
    inputModeActive = true;
    return;
  }
  if (CDString == "SLP") {                // Settings-Light-Period
    header.Str[0] = "+ Set Photoperiod +";
    inputValueVariable = &lightPeriod;
    inputValueUpperLimit = 24;
    inputModeActive = true;
    return;
  }
  if (CDString == "SLI") {                // Settings-Light-Intensity
    header.Str[0] = "Set Max Intensity";
    inputValueVariable = &maxLightIntensity;
    inputValueUpperLimit = 100;
    inputModeActive = true;
    return;
  }
  if (CDString == "SLD") {                // Settings-Light-Dimming
    header.Str[0] = "+ Set Dimming +";
    inputModeMap.Str[0] = "Half Sine";
    inputModeMap.Str[1] = "Constant";
    inputValueVariable = &dimmingMode;
    inputValueUpperLimit = 1;
    isText = true;
    inputModeActive = true;
    return;
  }
  if (CDString == "SH") {                 // Settings-Humidity
    header.Str[0] = "+ Set Humidity +";
    contents.Str[0] = "Day";
    contents.Str[1] = "Night";
    contents.Str[2] = "";
    contents.Str[3] = "";
    return &S_H_Options[0];
  }
  if (CDString == "SHD") {                // Settings-Humidity-Day
    header.Str[0] = "+ Set Day Humidity +";
    contents.Str[0] = "Max";
    contents.Str[1] = "Min";
    contents.Str[2] = "";
    contents.Str[3] = "";
    return &S_H_D_Options[0];
  }
  if (CDString == "SHDM") {               // Settings-Humidity-Day-Max
    header.Str[0] = "Set Day Max Humidity";
    inputValueVariable = &humidityDayMax;  // Records which values to change
    inputValueUpperLimit = 100;                 // Max allowed value
    inputModeActive = true;                // Turn on inputMode
    return;
  }
  if (CDString == "SHDm") {               // Settings-Humidity-Day-min
    header.Str[0] = "Set Day Min Humidity";
    inputValueVariable = &humidityDayMax;  // Records which values to change
    inputValueUpperLimit = 100;                 // Max allowed value
    inputModeActive = true;                // Turn on inputMode
    return;
  }
  if (CDString == "SHN") {                // Settings-Humidity-Night
    header.Str[0] = "Set Night Humidity";
    contents.Str[0] = "Max";
    contents.Str[1] = "Min";
    contents.Str[2] = "";
    contents.Str[3] = "";
    return &S_H_N_Options[0];
  }
  if (CDString == "SHNM") {               // Settings-Humidity-Night-Max
    header.Str[0] = "Set Max Humidity";
    inputValueVariable = &humidityNightMax; // Records which values to change
    inputValueUpperLimit = 100;                  // Max allowed value
    inputModeActive = true;                 // Turn on inputMode
    return;
  }
  if (CDString == "SHNm") {               // Settings-Humidity-Night-min
    header.Str[0] = "Set Min Humidity";
    inputValueVariable = &humidityNightMin; // Records which values to change
    inputValueUpperLimit = 100;                  // Max allowed value
    inputModeActive = true;                 // Turn on inputMode
    return;
  }
  

  // -----------------------------------------------------

  if (CDString == "O") {                  // Override
    header.Str[0] = "+++ Override +++";
    contents.Str[0] = "Light";
    contents.Str[1] = "Mist";
    contents.Str[2] = "Fan";
    contents.Str[3] = "";
    return &overrideOptions[0];
  }
  if (CDString == "OL") {                 // Override-Light
    header.Str[0] = "+ Override Light +";
    contents.Str[0] = "Relay";
    contents.Str[1] = "Intensity";
    contents.Str[2] = "";
    contents.Str[3] = "";
    return &O_L_Options[0];
  }
  if (CDString == "OLR") {                // Override-Light-Relay(State)
    header.Str[0] = "+ Light Relay State +";
    inputModeMap.Str[0] = "ON";
    inputModeMap.Str[1] = "OFF";
    inputValueVariable = &lightRelayState;
    inputValueUpperLimit = 1;
    isText = true;
    inputValuePin = lightRelay_PIN;
    inputModeActive = true;
    return;
  }
  if (CDString == "OLI") {                // Override-Light-Intensity
    header.Str[0] = "+ Light Intensity +";
    inputValueVariable = &menuLightIntensity;
    inputValueUpperLimit = 100;
    inputValuePin = lightDimming_PIN;
    isPWM = true;
    inputModeActive = true;
    return;
  }
  if (CDString == "OM") {                 // Override-Mist
    header.Str[0] = "+ Override Mist +";
    inputModeMap.Str[0] = "ON";
    inputModeMap.Str[1] = "OFF";
    inputValueVariable = &mistRelayState;
    inputValueUpperLimit = 1;
    isText = true;
    inputValuePin = mistRelay_PIN;
    inputModeActive = true;
    return;
  }
  if (CDString == "OF") {                 // Override-Fan
    //O_F_Page();
    //return &O_F_Options[0];
  }

  // --------------------------------------------------

  if (CDString == "ST") {
    header.Str[0] = "Max Menu Idle Time";
    inputValueVariable = &idleTimeLimit_LCD;  // Records which values to change
    inputValueUpperLimit = 1440;              // Max allowed value
    inputValueLowerLimit = 1;                 // Min allowed value
    inputModeActive = true;                   // Turn on inputMode
    return;
  }
  
  return;
}


// Print Bitmaps +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Centers and prints a String in screen of defined size
  // (Width)
    // whichBitmap: 0 - directory, 1 - header, 2 - contents, 3 - cursor, 4 - inputMode
int centerText_x (String text, int whichBitmap, int screenWidth) {
  int x1, y1;
  unsigned int w, h;
  int newcursor_x;
  bitmaps[whichBitmap].getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return newcursor_x = (screenWidth - w) / 2;  //  128 is the display width
}

  // (Height)
int centerText_y (String text, int whichBitmap, int screenHeight) {
  int x1, y1;
  unsigned int w, h;
  int newcursor_y;
  bitmaps[whichBitmap].getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return newcursor_y = (screenHeight - h) / 2;
}

// Prints the directory bitmap. bitmaps[0] -------------------------------------------------
void printDirectory () {
  // Changes currentDirectory to a String to be displayed
  if (menuLevel == 0) {       // Displays nothing for main menu
    directory.Str[0] = '\0';
  } else {                    // Otherwise turn currentDirectory[] into a String
    for (int i = 0; i < itemCount(currentDirectory_ptr, sizeof(currentDirectory)); i++) {  
      if (i == 0) {       // Prints first char without '>' before it
        directory.Str[0] = currentDirectory[0];
      } else {
        directory.Str[0] = String(directory.Str[0] + " > " + currentDirectory[i]);
      }
    } 
  }
  // Print
  bitmaps[0].fillScreen(BLACK);                     // Clear buffer
  bitmaps[0].setCursor(0,0);                        // Set cursor to beginning
  bitmaps[0].print(directory.Str[0]);               // Print text
  tft.fillRect(directory.x, directory.y, directory.w, directory.h, BLACK);  // Clear screen area
  tft.drawBitmap(directory.x, directory.y, bitmaps[0].getBuffer(), directory.w, directory.h, GREEN);  // Print
}

// Prints the header bitmap. Bitmaps[1] ------------------------------------------------
void printHeader () {
  bitmaps[1].fillScreen(BLACK);               // Clear buffer
  bitmaps[1].setTextSize(1);
  bitmaps[1].setCursor(centerText_x(header.Str[0], 1, header.w), 0);     
  bitmaps[1].print(header.Str[0]);               // Print text
  tft.fillRect(header.x, header.y, header.w, header.h, BLACK);  // Clear screen area
  tft.drawBitmap(header.x, header.y, bitmaps[1].getBuffer(), header.w, header.h, WHITE);  // Print
}

// Prints the contents bitmap. Bitmaps[2]. contents.Str[] has 4 elements ---------------
void printContents () {
  bitmaps[2].fillScreen(BLACK);               // Clear buffer
  bitmaps[2].setTextSize(2);
  bitmaps[2].setCursor(0, 0);     
  for (int i = 0; i < maxOptionNum; i++) {    // Print text
    bitmaps[2].println(contents.Str[i]);
  }
  tft.fillRect(contents.x, contents.y, contents.w, contents.h, BLACK);  // Clear screen area
  tft.drawBitmap(contents.x, contents.y, bitmaps[2].getBuffer(), contents.w, contents.h, CYAN);  // Print  
}

// Prints the cursor bitmap. Bitmaps[3] ----------------------------------------------------
void printCursor () {
  // Determines which row to put the cursor
  arrayToNull(&cursors.Str[0], maxOptionNum);   // Clears all rows first
  cursors.Str[cursorPosition] = cursor_char;
  // Print
  bitmaps[3].fillScreen(BLACK);                 // Clear buffer
  bitmaps[3].setTextSize(2);
  bitmaps[3].setCursor(0, 0);     
  for (int i = 0; i < maxOptionNum; i++) {      // Print text
    bitmaps[3].println(cursors.Str[i]);
  }
  tft.fillRect(cursors.x, cursors.y, cursors.w, cursors.h, BLACK);  // Clear screen area
  tft.drawBitmap(cursors.x, cursors.y, bitmaps[3].getBuffer(), cursors.w, cursors.h, CYAN);  // Print
}

// Prints the inputMode bitmap. Bitmaps[4] -------------------------------------------------
void printInputMode () {
    String toDisplay;
    // Differentiates between displaying words vs text
    if (isText) {   // If words
      toDisplay = inputModeMap.Str[*inputValueVariable];
      bitmaps[4].setTextSize(2);      
    } else {        // If numbers
      toDisplay = String(*inputValueVariable);      
      bitmaps[4].setTextSize(4);
    }
    // Print
    bitmaps[4].fillScreen(BLACK);     // Clear buffer 
    bitmaps[4].setCursor(centerText_x(toDisplay, 4, inputModeMap.w), centerText_y(toDisplay, 4, inputModeMap.h));     
    bitmaps[4].print(toDisplay);      // Print text
    tft.fillRect(inputModeMap.x, inputModeMap.y, inputModeMap.w, inputModeMap.h, BLACK);  // Clear screen area
    tft.drawBitmap(inputModeMap.x, inputModeMap.y, bitmaps[4].getBuffer(), inputModeMap.w, inputModeMap.h, YELLOW);  // Print
}

// Print Bitmaps onto LCD Display ----------------------------------------------------------
void printBitmaps () {
  printDirectory();          // Always print directory
  printHeader();             // Always print header
  if (inputModeActive) {  // If activated, does not display the content or cursor bitmap
    showContents = false;
    showCursor = false;
  } else {
    showContents = true;
    showCursor = true;
    Serial.println("a");
  }
  if (showContents) {
    printContents();
  }
  // Cursor is printed in the master menu function
}



// Miscellaneous functions +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Returns TRUE if array1 is exactly the same as array2
  // <String>
boolean compareArrays (String* array1, String* array2, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    if (*array1 != *array2) {
      return false;
    }
    array1++;
    array2++;
  }
  return true;
}
  // <char w/ pointers>
boolean compareArrays (char* array1, char* array2, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    if (*array1 != *array2) {
      return false;
    }
    array1++;
    array2++;
  }
  return true;
}

// Prints array to Serial (int) -------------------------------------------------------
  // <int>
void printArray(int* arrayToPrint, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    Serial.print(*arrayToPrint);
    arrayToPrint++;
  }
  Serial.println();
}
  // <String>
void printArray(String* arrayToPrint, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    Serial.print(*arrayToPrint);
    arrayToPrint++;
  }
  Serial.println();
}
  // <char>
void printArray(char* arrayToPrint, int arraySize) {
  //Serial.println("printArray");
  for (int i = 0; i < arraySize; i++) {
    Serial.print(*arrayToPrint);
    arrayToPrint++;
  }
  Serial.println();
  return;
}

// Copies one array into another with pointers ----------------------------------------
  // <int>
void copyArray (int* copyTo, int* copyFrom, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    *copyTo = *copyFrom;
    copyTo++;
    copyFrom++;
  }
  return;
}
  // <String>
void copyArray (String* copyTo, String* copyFrom, int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    *copyTo = *copyFrom;
    copyTo++;
    copyFrom++;
  }
  return;
}
  // <char>
void copyArray (char* copyTo, char* copyFrom, int arraySize) {
  //Serial.println("copyArray");
  for (int i = 0; i < arraySize; i++) {
    *copyTo = *copyFrom;
    copyTo++;
    copyFrom++;
  }
  return;
}

// Turns char[] into String -----------------------------------------------------------
String charToString (char* copyFrom, int arraySize) {
 String newString;
 for (int i = 0; i < arraySize; i++) {
    newString += *copyFrom;
    copyFrom++;
 }
 return newString;
}

// Sets all elements in an array to NULL ----------------------------------------------
  // <char>
void arrayToNull (char* toNull, int arraySize) {
  //Serial.println("arrayToNull");
  for (int i = 0; i < arraySize; i++) {  // Resets the cursor to starting position
    *toNull = 0;  
    toNull++;
  }
  return;
}
  // <String>
void arrayToNull (String* toNull, int arraySize) {
  for (int i = 0; i < arraySize; i++) {  // Resets the cursor to starting position
    *toNull = "";  
    toNull++;
  }
}

// Counts how many elements are in an array (char) ------------------------------------
int itemCount (char* theArray, int arraySize) {
  int count = 0;
  for (int i = 0; i < arraySize; i++) {
    if (*theArray != 0) {
      count++;
      theArray++;
    }
  }
  return count;
}

// Processes the rotary encoder to allow cursor to stay within menu bounds ------------
int rotaryCounterLimit (int upperLimit, int lowerLimit, int currentPosition) {
  int newPosition = constrain(currentPosition + rotaryCounter, lowerLimit, upperLimit);
  rotaryCounter = 0;                   // Resets for next reading
  return newPosition;
}



// Menu Integration ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// MENU DEVICE DEFAULTS
void mainMenuControlDefault (int isLightON) {
  analogWrite(lightDimming_PIN, dimmingConversion(70)); // Turns on light to 70%
  digitalWrite(lightRelay_PIN, isLightON);
  digitalWrite(mistRelay_PIN, LOW);
  digitalWrite(fanCirculation_PIN, LOW);
  analogWrite(fanCirculationSpeed_PIN, 0);
  digitalWrite(waterfallRelay_PIN, LOW);
}

// INITILIZATION ------------------------------------------------------------------
void initializeMenu (int isLightON) {
  // Reset/set variables
  lastMenuLevel = 0;
  backSwitch = 0;           
  rotarySwitch = 0;
  cursorPosition = 0;           // Array position. 0 is first position.
  lastCursorPosition = -1;      // To initiate print cursor
  idleTimeStart_LCD = millis(); // Timer for menu
  //lightRelayState = isLightON;  // Defaults override light settings to pre-menu states
  showCursor = true;            // Displays the cursor initially
  showContents = true;          // Displays menu contents intially
 
  // Reset/set display
  powerUp_LCD();
  arrayToNull(currentDirectory_ptr, maxOptionNum);    // Resets directory to main menu (all null)
  currentOptions = findAndReturnOptions();            // Sets display to main menu options
  printBitmaps();
}

// MENU EXIT ------------------------------------------------------------
boolean exitMenu_LCD () {
  if ((cursorPosition == lastCursorPosition) && (menuLevel == lastMenuLevel)) {   // Continues counting idle time if there is no activity
    idleTime_LCD = millis();
  } else {                                      // Resets the idle time starting point when there is a change
    idleTimeStart_LCD = millis();
  }
  if ((idleTime_LCD - idleTimeStart_LCD) > (idleTimeLimit_LCD * 60000)) {  // If time exceeds the idle limit, exit menu
    return true;              // To immediately exit out of the master menu function
  }
  if (menuLevel <= -1) {      // Exits the menu function until next activation when pressing backSwitch on main menu page
    return true;
  }
  return false;               // Don't exit the master menu function
}

// POWER UP ---------------------------------------------------------------------------
void powerUp_LCD () {
  digitalWrite(LCDbackLight_PIN, HIGH);         // Turns on LCD backlight
}

// POWER DOWN -----------------------------------------------------------
void powerDown_LCD () {
  tft.fillScreen(BLACK);
  digitalWrite(LCDbackLight_PIN, LOW);          // Turns off LCD backlight
}

// MENU NAVIGATION SELECTION ---------------------------------------------------------------
void menuNav () {
  if (menuLevel == 0) {              // Return to main menu
    arrayToNull(currentDirectory_ptr, maxMenuLevelNum);     // Resets directory to main menu
    currentOptions = findAndReturnOptions(); // Retrieve and activate options
  } else if (lastMenuLevel < menuLevel) {   // If a selection is made
    currentDirectory[menuLevel - 1] = *(currentOptions + cursorPosition);   // -1 is to account for array positioning
    currentOptions = findAndReturnOptions();  // Retrieve and activate options
  } else if (lastMenuLevel > menuLevel) {   // If returning to last page
    currentDirectory[menuLevel] = '\0';       // Goes up a level
    currentOptions = findAndReturnOptions();  // Retrieve and activate options
  }
  cursorPosition = 0;   // Resets the cursor position whenever there is a change in menu page
}

// INPUT MODE ------------------------------------------------------------------------------
void inputMode (int isLightON) {
  int lastInputMode = -1;      // To allow a first printInputMode
  lightRelayState = isLightON;
  float inputModeStartTime = millis();
  while (inputModeActive) {  
    // Receives user input
    *inputValueVariable = rotaryCounterLimit(inputValueUpperLimit, inputValueLowerLimit, *inputValueVariable);  
    menuLevel = rotarySwitch - backSwitch;
    
    // Exit inputMode if bS is pressed or if passed the time limit
    if (lastMenuLevel > menuLevel) {  
      Serial.println("exit inputMode");
      return;
    }
    if ((millis() - inputModeStartTime) > (inputModeTimeLimit * 60000)) {
      backSwitch++;    // As if backSwitch is getting pressed
      return;
    }
    
    // Changes in input
    if (*inputValueVariable != lastInputMode) {
      printInputMode();           // Print
      if (inputValuePin > 0) {    // Real time adjustments for override (0 is default which is no pin)
        if (isPWM) {
          if (inputValuePin == lightDimming_PIN) {    // PWM for light dimming
          analogWrite(inputValuePin, dimmingConversion(*inputValueVariable));
          } else {                                       // PWM for fan, etc.
            analogWrite(inputValuePin, (*inputValueVariable) * 2.55); 
          }
        } else {                                              // For relays
          digitalWrite(inputValuePin, *inputValueVariable);
        }
      }    
    }
    lastInputMode = *inputValueVariable;
    delay(inputLoopDelay);
  }
}

// MASTER MENU FUNCTION -------------------------------------------------------------------------
void menu_LCD (int isLightON) {
  while (1) {  // Keeps looping until conditions are met to exit
    // User input
    cursorPosition = rotaryCounterLimit(itemCount(currentOptions, maxOptionNum) - 1, 0, cursorPosition); // "-1" to account for array indexing
    menuLevel = rotarySwitch - backSwitch;  // How deep in the submenu pages we are
      // 0 is main menu
      // -1 exits menu. rS is forward, bS is backward
    // Determine whether or not to exit the menu system
    if (exitMenu_LCD()) {
      return;
    }
    // Sets predetermined relay values for main menu. Allows for quick pausing to work on the enclosure
    if (menuLevel == 0) {
      mainMenuControlDefault(isLightON);
    }
    // For user to change the settings
    if (inputModeActive) {
      inputMode(isLightON);         // Enters into inputMode loop
      inputModeActive = false;
      lastCursorPosition = -1;      // Allows the cursor to print after exiting inputMode
    }
    // Displays new menu page if there is a selection
    if (menuLevel != lastMenuLevel) {
      menuNav();        // Determine new content to display
      printBitmaps();   // Prints determined content      
    }
    // Displays the cursor if there is a change
    if ((cursorPosition != lastCursorPosition) && showCursor) {
      printCursor(); 
    }
    // Update
    lastCursorPosition = cursorPosition;
    lastMenuLevel = menuLevel;
    delay(menuLoopDelay);
  }
}




/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ------------------------ INTERRUPTS ------------------------
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

// DEBOUNCING
const int debounceTime = 180;  // millis
long lastSwitchDetectedMillis = 0;

// OLED Menu Page change switch
void OLEDmenuSwitch_ISP () {
  //OLEDmenuSwitch++;
}

// ROTARY ENCODER ------------------------------------------------------------
void rotaryEncoder_ISP () {   // Rotary encoder
  int currentStateCLK = digitalRead(rotaryCLK_PIN);
  if ((currentStateCLK != lastStateCLK)  && (currentStateCLK == 1)) {  // If states different, pulse occurred. React to only 1 state change to avoid double count
    if (digitalRead(rotaryDT_PIN) != currentStateCLK) {       // If the DT state != CLK state then the encoder is rotating CCW so increment
      rotaryCounter ++;
    } else {         // Encoder is rotating CW so decrement
      rotaryCounter --;
    }
  }
  lastStateCLK = currentStateCLK;
}

// ROTARY ENCODER SWITCH -----------------------------------------------------
void rotarySwitch_ISP () {    // Switch button on the rotary encoder
  if (millis() - lastSwitchDetectedMillis > debounceTime) {
    rotarySwitch ++;
  }
  lastSwitchDetectedMillis = millis();
}

// MENU BACKWARD AND ACTIVATION SWITCH ---------------------------------------
void backSwitch_ISP () {      // Separate switch button
  if (millis() - lastSwitchDetectedMillis > debounceTime) {
    backSwitch++;
  }
  lastSwitchDetectedMillis = millis();
}




/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  ----------------------- Setup and Loop ----------------------
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

void setup() {
  // Data Initialization
  Serial.begin(9600);

  // Pin Setup
  pinMode(LCDbackLight_PIN, OUTPUT);
  pinMode(backSwitch_PIN, INPUT_PULLUP);
  pinMode(rotarySwitch_PIN, INPUT_PULLUP);

  // LCD Display
  initializeLCD();
  powerDown_LCD();

  // Special Value Initilizations
  lastStateCLK = digitalRead(rotaryCLK_PIN);

  // INTERRUPTS
  //attachInterrupt(digitalPinToInterrupt(OLEDmenuSwitch_PIN), OLEDmenuSwitch_ISP, RISING);
  attachInterrupt(digitalPinToInterrupt(rotaryCLK_PIN), rotaryEncoder_ISP, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rotarySwitch_PIN), rotarySwitch_ISP, RISING);
  attachInterrupt(digitalPinToInterrupt(backSwitch_PIN), backSwitch_ISP, RISING);
}

// -----------------------------------------------------------------------------------------------

void loop() {

  
  // LCD Menu
  if (backSwitch > 0) {         // Menu Activated by backSwitch
    int isLightON = digitalRead(lightRelay_PIN);  // Keeps light the same when entering menu
    initializeMenu(isLightON);  // Sets up menu
    menu_LCD(isLightON);        // Master Menu function
    powerDown_LCD();            // Turns the LCD to lower power mode    
    backSwitch = 0;             // Resets switch counter for next menu activation
  }

  delay(loopDelay);
}
