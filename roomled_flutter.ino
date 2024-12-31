/*
* Led program cycler
* 
* Written by Kyle Rush
* Created May 2023
* Updated December 2024 :)
*/

#include <FastLED.h>
#include <EEPROM.h>

FASTLED_USING_NAMESPACE

/****************************************************************/
/*                         Init Consts                          */
/****************************************************************/

#define PROGRAM_BUTTON          12
#define SECONDARY_BUTTON        11

#define EEPROM_MODE_ADDRESS     0 // Used to check and set the current program
#define EEPROM_PROGSET_ADDRESS  1 // Used to get current program set from last known program
#define DATA_PIN                3
#define CLK_PIN                 5
#define LED_TYPE                WS2801
#define COLOR_ORDER             BGR
#define NUM_LEDS                64
CRGB leds[NUM_LEDS];

#define BRIGHTNESS              255

#define BUTTON_DEBOUNCE_MS      50
#define BUTTON_LONGDELAY_MS     350

#define BUTTONS_ENABLED
#define EEPROM_ENABLED
// #define DEBUG_ENABLED
// TODO: implement allowing disable of eeprom
// TODO: add flags for disabling buttons

/****************************************************************/
/*                    Program setup & config                    */
/****************************************************************/

uint16_t fps = 120;

typedef void(*callable)(void);
void normalFPS(){ // default FPS used by most programs
  FastLED.show();
  FastLED.delay(1000/fps);
}
void serialCmdEmpty(String command) { Serial.println("There are no avaliable commands for this program!"); }

struct Program {
  void (*runtime)(void);
  void (*setup)(void);
  void (*fps)(void);
  void (*parseCommands)(String);

  Program(callable run, callable setupCommand, callable fpsOverride, callable serialCommands) {
    runtime = run;
    setup = setupCommand;
    fps = fpsOverride;
    parseCommands = serialCommands;
  }
};

Program initProgram = { setupSequenceLED, [](){}, [](){}, serialCmdEmpty };
// TODO: turn programsets into stuct
Program programSet0[] = {
  { holidayTwinkle,   holidayTwinkleSetup,  [](){},     holidayTwinkleSerial  },
  { dot,              [](){},               normalFPS,  dot_serial            },
  { rainbow,          [](){},               [](){},     rainbow_serial        },
  { waves,            [](){},               [](){},     serialCmdEmpty        },
  { nullptr,          nullptr,              nullptr,    nullptr },
  { nullptr,          nullptr,              nullptr,    nullptr },
  { nullptr,          nullptr,              nullptr,    nullptr },
  { nullptr,          nullptr,              nullptr,    nullptr }
};
Program programSet1[] = {
  { backnforth,       [](){},               [](){},     rainbow_serial },
  { Fire2012,         Fire2012Setup,        normalFPS,  serialCmdEmpty },
  { rainbow2,         [](){},               [](){},     serialCmdEmpty },
  { strobe,           [](){},               normalFPS,  serialCmdEmpty },
  initProgram,
  { nullptr,          nullptr,              nullptr,    nullptr },
  { nullptr,          nullptr,              nullptr,    nullptr },
  { nullptr,          nullptr,              nullptr,    nullptr }
};

unsigned short funcIndex = 0; // index of function to call retreived by EEPROM
unsigned short currentProgramSet = 0; // current program set retrieved by EEPROM
unsigned short programSetMax = 1;
const unsigned int programSetMaxMap[] = { 3, 4 };
Program programSetMap[][8] = {
  { programSet0[0], programSet0[1], programSet0[2], programSet0[3], programSet0[4], programSet0[5], programSet0[6], programSet0[7] },
  { programSet1[0], programSet1[1], programSet1[2], programSet1[3], programSet1[4], programSet1[5], programSet1[6], programSet1[7] }
};
unsigned int funcMax = programSetMaxMap[currentProgramSet]; // highest index of modes
Program currentProgram = programSetMap[currentProgramSet][funcIndex];

void setProgram(uint8_t programSet, uint8_t program) {
  currentProgramSet = programSet;
  funcMax = programSetMaxMap[currentProgramSet];
  funcIndex = program <= 0 || program > funcMax ? 0 : program;
  currentProgram = programSetMap[currentProgramSet][program];
  // return currentProgram;
}
// void swapProgramBank(short program) {
//   currentProgramSet = currentProgramSet >= programSetMax ? 0 : currentProgramSet + 1;
//   return setProgram(currentProgramSet, program);
// }

void save() {
  #ifdef EEPROM_ENABLED
  EEPROM.update(EEPROM_MODE_ADDRESS, (uint8_t)funcIndex);
  EEPROM.update(EEPROM_PROGSET_ADDRESS, (uint8_t)currentProgramSet);
  #endif
  for (int i=0;i<2;i++){
    // TODO: make a chasing pattern with all leds
    fill_solid(leds, 16, CRGB(100,100,100));
    FastLED.show();
    delay(250);
    fill_solid(leds, 16, CRGB(0,0,0));
    FastLED.show();
    delay(200);
  }
}

String lastSerialCommand = "help";

/*****************************************************************/
/*                      Program constants                        */
/*****************************************************************/

int globalConstants[8]; // TODO: remove

uint8_t gHue = 0;
uint8_t gSaturation = 200;

CRGBPalette16 wavesConstants[6] = {
  { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
    0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 },
  { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
    0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F },
  { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
    0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF },
  {},
  {},
  {}
};

// A mostly red palette with green accents and white trim.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedGreenWhite_p FL_PROGMEM =
{  CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
  CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
  CRGB::Red, CRGB::Red, CRGB::Gray, CRGB::Gray, 
  CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green };

// A mostly (dark) green palette with red berries.
#define Holly_Green 0x00580c
#define Holly_Red   0xB00402
const TProgmemRGBPalette16 Holly_p FL_PROGMEM =
{  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
  Holly_Green, Holly_Green, Holly_Green, Holly_Red 
};

// A red and white striped palette
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 RedWhite_p FL_PROGMEM =
{  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray,
  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A mostly blue palette with white accents.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const TProgmemRGBPalette16 BlueWhite_p FL_PROGMEM =
{  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
  CRGB::Blue, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A pure "fairy light" palette with some brightness variations
#define HALFFAIRY ((CRGB::FairyLight & 0xFEFEFE) / 2)
#define QUARTERFAIRY ((CRGB::FairyLight & 0xFCFCFC) / 4)
const TProgmemRGBPalette16 FairyLight_p FL_PROGMEM =
{  CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, 
  HALFFAIRY,        HALFFAIRY,        CRGB::FairyLight, CRGB::FairyLight, 
  QUARTERFAIRY,     QUARTERFAIRY,     CRGB::FairyLight, CRGB::FairyLight, 
  CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight };

// A palette of soft snowflakes with the occasional bright one
const TProgmemRGBPalette16 Snow_p FL_PROGMEM =
{  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0xE0F0FF };

// A palette reminiscent of large 'old-school' C9-size tree lights
// in the five classic colors: red, orange, green, blue, and white.
#define C9_Red    0xB80400
#define C9_Orange 0x902C02
#define C9_Green  0x046002
#define C9_Blue   0x070758
#define C9_White  0x606820
const TProgmemRGBPalette16 RetroC9_p FL_PROGMEM =
{  C9_Red,    C9_Orange, C9_Red,    C9_Orange,
  C9_Orange, C9_Red,    C9_Orange, C9_Red,
  C9_Green,  C9_Green,  C9_Green,  C9_Green,
  C9_Blue,   C9_Blue,   C9_Blue,
  C9_White
};

// A cold, icy pale blue palette
#define Ice_Blue1 0x0C1040
#define Ice_Blue2 0x182080
#define Ice_Blue3 0x5080C0
const TProgmemRGBPalette16 Ice_p FL_PROGMEM =
{
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue2, Ice_Blue2, Ice_Blue2, Ice_Blue3
};


// Add or remove palette names from this list to control which color
// palettes are used, and in what order.
const TProgmemRGBPalette16* ActivePaletteList[] = {
  &RetroC9_p,
  &BlueWhite_p,
  &RainbowColors_p,
  &FairyLight_p,
  &RedGreenWhite_p,
  &PartyColors_p,
  &RedWhite_p,
  &Snow_p,
  &Holly_p,
  &Ice_p  
};

CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;

const int songData[] = {};
// const unsigned float songDataLength[] = {};

/*****************************************************************/
/*                         Setup & Loop                          */
/*****************************************************************/

void setup() {
  #ifdef BUTTONS_ENABLED
  pinMode(PROGRAM_BUTTON, INPUT);
  digitalWrite(PROGRAM_BUTTON, HIGH);
  pinMode(SECONDARY_BUTTON, INPUT);
  digitalWrite(SECONDARY_BUTTON, HIGH);
  #endif

  #ifdef DEBUG_ENABLED
  Serial.begin(9600);
  #endif

  delay(250); // wait for LED init

  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  currentProgram = initProgram;
  currentProgram.setup(); // run current program setup (if any)
}

#ifdef DEBUG_ENABLED
void parseSerialCommands(String incomingData) {
  if (!incomingData.startsWith("l")){ lastSerialCommand = incomingData; }
  if (incomingData.startsWith("help")) {
    Serial.println("Avaliable Commands: pause, clear, bank <int>, set <int>, cmd <cmd>, l, bright <int>");
  } /*else if (incomingData.startsWith("togglechange")) {
    // lockState = lockState == 1 ? 0 : 1;
    // EEPROM.update(EEPROM_PROGSET_ADDRESS, 0);
    // if (lockState == 1) { EEPROM.update(EEPROM_MODE_ADDRESS, funcIndex); }
    // Serial.println();
  } */
  else if (incomingData.startsWith("save")) {
    save();
  } else if (incomingData.startsWith("bank ")) {
    uint8_t value = incomingData.substring(5).toInt();
    setProgram(value, 0);
    currentProgram.setup();
  } else if (incomingData.startsWith("set ")) {
    uint8_t value = incomingData.substring(4).toInt();
    setProgram(currentProgramSet, value);
    currentProgram.setup();
    // if (lockState == 1) { EEPROM.update(EEPROM_MODE_ADDRESS, funcIndex); }
    // Serial.println("Set program to: " + value);
  } else if (incomingData.startsWith("pause")) {
    // FastLED.clear(true);
    while (Serial.available() == 0) {} // Want to stop the runtime and fps tick instead
  } else if (incomingData.startsWith("clear")) {
    FastLED.clear(true);
    delay(500);
  } else if (incomingData.startsWith("cmd ")) {
    currentProgram.parseCommands(incomingData.substring(4));
  } else if (incomingData.startsWith("l")) {
    parseSerialCommands(lastSerialCommand);
  } else if (incomingData.startsWith("bright")) {
    FastLED.setBrightness(incomingData.substring(7).toInt());
  } else {
    Serial.println("Unknown command. Type 'help' for help.");
  }
}
#endif

#ifdef BUTTONS_ENABLED
bool awaitingButton1 = false; // set true when detecting long presses
bool deadButton1 = false; // used to ignore parsing the button (after action completed until released again)
unsigned short buttonState1 = 1;
bool awaitingButton2 = false;
bool deadButton2 = false;
unsigned short buttonState2 = 1;

// TODO: revise this whole button system i dont like it
void parseProgramButton() {
  delay(BUTTON_DEBOUNCE_MS); // debounce time (not really needed for this style of button but whatever)
  if (deadButton1) { return; }

  delay(BUTTON_LONGDELAY_MS); // time to differentiate between short and long button presses
  if (awaitingButton1 && digitalRead(PROGRAM_BUTTON) == 0) {
    // actually wait 1000ms then ignore button until it is depressed ~~do something every 500ms until the button is depressed~~
    delay(900);
    deadButton1 = true;

    save();
  } else if (!awaitingButton1 && digitalRead(PROGRAM_BUTTON) == 0) {
    // little bit of a funny but this is here i guess idk what im thinking tbh
    awaitingButton1 = true; // used to later check if button is still pressed
    
  } else if (!awaitingButton1) {
    // short press of button
    FastLED.clear();
    funcIndex = funcIndex >= funcMax ? 0 : funcIndex + 1;
    setProgram(currentProgramSet, funcIndex);
    currentProgram.setup();
  }
}
void parseSecondaryButton() {
  delay(BUTTON_DEBOUNCE_MS);
  if (deadButton2) { return; }

  delay(BUTTON_LONGDELAY_MS);
  if (awaitingButton2 && digitalRead(SECONDARY_BUTTON) == 0) {
    deadButton2 = true;
    delay(300);

    FastLED.clear();
    funcIndex = 0;
    currentProgramSet = currentProgramSet >= programSetMax ? 0 : currentProgramSet + 1;
    funcMax = programSetMaxMap[currentProgramSet];
    // TODO: update eeprom
    currentProgram = programSetMap[currentProgramSet][funcIndex];
    currentProgram.setup();

  } else if (!awaitingButton2 && digitalRead(SECONDARY_BUTTON) == 0) {
    awaitingButton2 = true;
  } else if (!awaitingButton2) {
    // TODO: run current program button press method (also todo)
  }
}
#endif

void loop() {
  #ifdef BUTTONS_ENABLED
  buttonState1 = digitalRead(PROGRAM_BUTTON);
  buttonState2 = digitalRead(SECONDARY_BUTTON);
  #endif

  #ifdef DEBUG_ENABLED
  if (Serial.available() > 0) { parseSerialCommands(Serial.readString()); }
  #endif

  #ifdef BUTTONS_ENABLED
  if (buttonState1 == 0 && !deadButton1) {
    parseProgramButton();
  } else if (buttonState1 == 1) {
    awaitingButton1 = false;
    deadButton1 = false;
  }
  if (buttonState2 == 0 && !deadButton2) {
    parseSecondaryButton();
  } else if (buttonState2 == 1) {
    awaitingButton2 = false;
    deadButton2 = false;
  }
  #endif

  currentProgram.runtime(); // program loop runtime
  currentProgram.fps(); // fps delay (unless overriden) [default normalFPS()]
}

/*****************************************************************/
/*                           Programs                            */
/*****************************************************************/

void setupSequenceLED() {
  static uint8_t currentLedIndex = 0;
  static bool startFadeOut = false;
  currentLedIndex = currentLedIndex >= NUM_LEDS ? 0 : currentLedIndex + 1;
  
  if (startFadeOut && currentLedIndex == 0) {
    // Swap to program from EEPROM

    FastLED.clear();

    #ifdef EEPROM_ENABLED
    setProgram(
      EEPROM.read(EEPROM_PROGSET_ADDRESS),
      EEPROM.read(EEPROM_MODE_ADDRESS)
    );
    #else
    setProgram(0, 0);
    #endif

    startFadeOut = false;

    currentProgram.setup();

  } else if (currentLedIndex != 0) {
    if (startFadeOut) {
      fill_solid(leds, NUM_LEDS, CRGB((NUM_LEDS - currentLedIndex), (NUM_LEDS - currentLedIndex), (NUM_LEDS - currentLedIndex)));
    } else {
      fill_solid(leds, currentLedIndex, CRGB(NUM_LEDS, NUM_LEDS, NUM_LEDS));
    }

    FastLED.show();
    FastLED.delay(500/fps);
  } else if (!startFadeOut && currentLedIndex == 0) {
    startFadeOut = true;
  }
}

/*===============================================================*/

void dot() {
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), gSaturation, 255);

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
}
void dot_serial(String command) {
  if (command.startsWith("sat ")) {
    gSaturation = command.substring(4).toInt();
  } else if (command.startsWith("fps ")) {
    fps = command.substring(4).toInt();
  } else {
    Serial.println("Unkown Program Comamnd");
  }
}

/*===============================================================*/

void rainbow() {
  fill_rainbow(leds, NUM_LEDS, gHue, 7);

  FastLED.show();

  FastLED.delay(1000/(fps*2));

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
}
void rainbow_serial(String command) {
  if (command.startsWith("hue ")) {
    gHue = command.substring(4).toInt();
  } else {
    Serial.println("Unknown Program Command");
  }
}

/*===============================================================*/

void rainbow2() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }

  FastLED.show();
}

/*===============================================================*/

void backnforth()
{
  // // a colored dot sweeping back and forth, with fading trails
  // fadeToBlackBy( leds, NUM_LEDS, 20);
  // int pos = beatsin16( 78, 0, NUM_LEDS-1 );
  // leds[pos] += CHSV( gHue, 255, 192);

  // EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  for(int i = 0; i < NUM_LEDS; i++) {
        // Set the i'th led to red 
        leds[i] = CHSV(gHue++, 255, 255);
        // Show the leds
        FastLED.show(); 
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); }
        // Wait a little bit before we loop around and do it again
        delay(10);
    }
 
    // Now go in the other direction.  
    for(int i = (NUM_LEDS)-1; i >= 0; i--) {
        // Set the i'th led to red 
        leds[i] = CHSV(gHue++, 255, 255);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); }
        // Wait a little bit before we loop around and do it again
        delay(10);
    }
}

/*===============================================================*/

void Fire2012Setup()
{
  globalConstants[0] = 65; // COOLING
  globalConstants[1] = 130; // SPARKING
  globalConstants[2] = 0; // reverse?
}
void Fire2012()
{
  int COOLING = globalConstants[0];
  int SPARKING = globalConstants[1];
  const bool gReverseDirection = globalConstants[2] == 0;

  // Array of temperature readings at each simulation cell
  static uint8_t heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}

/*===============================================================*/

void waves()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  waves_one_layer( wavesConstants[0], sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  waves_one_layer( wavesConstants[1], sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  waves_one_layer( wavesConstants[2], sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  waves_one_layer( wavesConstants[2], sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  waves_add_whitecaps();

  // Deepen the blues and greens a bit
  waves_deepen_colors();

  FastLED.show();
  FastLED.delay(20);
}
// Add one layer of waves into the led array
void waves_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}
// Add extra 'white' to areas where the four layers of light have lined up brightly
void waves_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );
  
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}
// Deepen the blues and greens
void waves_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145); 
    leds[i].green= scale8( leds[i].green, 200); 
    leds[i] |= CRGB( 2, 5, 7);
  }
}

/*===============================================================*/

void strobe() {
  static uint16_t sPseudotime = 0;

}

/*===============================================================*/

void holidayTwinkleSetup() {
  globalConstants[0] = 6; // TWINKLE_SPEED 1-8
  globalConstants[1] = 6; // TWINKLE_DENSITY 1-8
  globalConstants[2] = 60; // SECONDS_PER_PALLETTE
  globalConstants[3] = 0; // AUTO_SELECT_BACKGROUND_COLOR
  globalConstants[4] = 1; // COOL_LIKE_INCANDESCENT

  gTargetPalette = *(ActivePaletteList[0]);
  holidayTwinkleNextPalette( gTargetPalette );

}
void holidayTwinkle() {
  EVERY_N_SECONDS( globalConstants[2] ) { 
    holidayTwinkleNextPalette( gTargetPalette ); 
  }

  EVERY_N_MILLISECONDS( 10 ) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 12);
  }

  // "PRNG16" is the pseudorandom number generator
  // It MUST be reset to the same starting value each time
  // this function is called, so that the sequence of 'random'
  // numbers that it generates is (paradoxically) stable.
  uint16_t PRNG16 = 11337;
  
  uint32_t clock32 = millis();
 
  // Set up the background color, "bg".
  // if AUTO_SELECT_BACKGROUND_COLOR == 1, and the first two colors of
  // the current palette are identical, then a deeply faded version of
  // that color is used for the background color
  CRGB bg;
  if( (globalConstants[3] == 1) &&
      (gCurrentPalette[0] == gCurrentPalette[1] )) {
    bg = gCurrentPalette[0];
    uint8_t bglight = bg.getAverageLight();
    if( bglight > 64) {
      bg.nscale8_video( 16); // very bright, so scale to 1/16th
    } else if( bglight > 16) {
      bg.nscale8_video( 64); // not that bright, so scale to 1/4th
    } else {
      bg.nscale8_video( 86); // dim, scale to 1/3rd.
    }
  } else {
    bg = CRGB::Black;//CRGB(CRGB::FairyLight).nscale8_video(16);// just use the explicitly defined background color
  }
 
  uint8_t backgroundBrightness = bg.getAverageLight();
  
  // for( CRGB& pixel: L) {
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    uint16_t myclockoffset16= PRNG16; // use that number as clock offset
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    // use that number as clock speed adjustment factor (in 8ths, from 8/8ths to 23/8ths)
    uint8_t myspeedmultiplierQ5_3 =  ((((PRNG16 & 0xFF)>>4) + (PRNG16 & 0x0F)) & 0x0F) + 0x08;
    uint32_t myclock30 = (uint32_t)((clock32 * myspeedmultiplierQ5_3) >> 3) + myclockoffset16;
    uint8_t  myunique8 = PRNG16 >> 8; // get 'salt' value for this pixel
 
    // We now have the adjusted 'clock' for this pixel, now we call
    // the function that computes what color the pixel should be based
    // on the "brightness = f( time )" idea.
    uint32_t ms = myclock30;
    uint8_t salt = myunique8;

    uint16_t ticks = ms >> (8-globalConstants[0]);
    uint8_t fastcycle8 = ticks;
    uint16_t slowcycle16 = (ticks >> 8) + salt;
    slowcycle16 += sin8( slowcycle16);
    slowcycle16 =  (slowcycle16 * 2053) + 1384;
    uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);
    
    uint8_t bright = 0;
    if( ((slowcycle8 & 0x0E)/2) < globalConstants[1]) {
      uint8_t j = fastcycle8;
      if( j < 86) {
        bright = j * 3;
      } else {
        j -= 86;
        bright = 255 - (j + (i/2));
      }
    }
  
    uint8_t hue = slowcycle8 - salt;
    CRGB c;
    if( bright > 0) {
      c = ColorFromPalette( gCurrentPalette, hue, bright, NOBLEND);
      if( globalConstants[4] == 1 ) {
        if ( fastcycle8 >= 128) {
          uint8_t cooling = (fastcycle8 - 128) >> 4;
          c.g = qsub8( c.g, cooling);
          c.b = qsub8( c.b, cooling * 2);
        }
  
      }
    } else {
      c = CRGB::Black;
    }
 
    uint8_t cbright = c.getAverageLight();
    int16_t deltabright = cbright - backgroundBrightness;
    if( deltabright >= 32 || (!bg)) {
      // If the new pixel is significantly brighter than the background color, 
      // use the new color.
      leds[i] = c;
    } else if( deltabright > 0 ) {
      // If the new pixel is just slightly brighter than the background color,
      // mix a blend of the new color and the background color
      leds[i] = blend( bg, c, deltabright * 8);
    } else { 
      // if the new pixel is not at all brighter than the background color,
      // just use the background color.
      leds[i] = bg;
    }
  }

  FastLED.show();
}
void holidayTwinkleNextPalette( CRGBPalette16& pal)
{
  const uint8_t numberOfPalettes = sizeof(ActivePaletteList) / sizeof(ActivePaletteList[0]);
  static uint8_t whichPalette = -1; 
  whichPalette = addmod8( whichPalette, 1, numberOfPalettes);
 
  pal = *(ActivePaletteList[whichPalette]);
}
void holidayTwinkleSerial(String command) {
  if (command.startsWith("spd ")) {
    globalConstants[0] = command.substring(4).toInt();
  } else if (command.startsWith("dens ")) {
    globalConstants[1] = command.substring(5).toInt();
  }
  // else if (command.startsWith("tpp ")) {
  //   globalConstants[2] = command.substring(4).toInt();
  // }
  else if (command.startsWith("set ")) {
    gTargetPalette = *(ActivePaletteList[command.substring(4).toInt()]);
  } else {
    Serial.println("Unknown Program Command");
  }
}