// LED Programs and Cycler by Kyle Rush, May 2023
#include <FastLED.h>
#include <EEPROM.h>

FASTLED_USING_NAMESPACE

/****************************************************************/
/*                         Init Consts                          */
/****************************************************************/

#define PROGRAM_BUTTON      12

#define EEPROM_MODE_ADDRESS 0 // Used to check and set the current program
#define EEPROM_LOCK_ADDRESS 1 // Used for bypassing program change on future resets
#define DATA_PIN            3
#define CLK_PIN             5
#define LED_TYPE            WS2801
#define COLOR_ORDER         BGR
#define NUM_LEDS            64
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          255
#define FRAMES_PER_SECOND   120

/****************************************************************/
/*                    Program setup & config                    */
/****************************************************************/

typedef void(*callable)(void);
void empty(){} // empty func for setup and fps overrides
void normalFPS(){ // default FPS used by most programs
  FastLED.show();
  FastLED.delay(1000/FRAMES_PER_SECOND);
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

// TODO: turn programsets into stuct
Program programSet0[] = {
  { dot,        empty,          normalFPS,  serialCmdEmpty },
  { rainbow,    empty,          empty,      rainbow_serial },
  { rainbow2,   empty,          empty,      serialCmdEmpty },
  { Fire2012,   Fire2012Setup,  normalFPS,  serialCmdEmpty },
  { waves,      empty,          empty,      serialCmdEmpty },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr }
};
Program programSet1[] = {
  { backnforth, empty,          normalFPS,  rainbow_serial },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr },
  { nullptr,    nullptr,        nullptr,    nullptr }
};

unsigned short funcIndex = 0; // index of function to call retreived by EEPROM
// unsigned short lockState;
unsigned short currentProgramSet = 0;
const unsigned int programSetMaxMap[] = {
  4,
  0
};
Program programSetMap[][8] = {
  { programSet0[0], programSet0[1], programSet0[2], programSet0[3], programSet0[4], programSet0[5], programSet0[6], programSet0[7] },
  { programSet1[0], programSet1[1], programSet1[2], programSet1[3], programSet1[4], programSet1[5], programSet1[6], programSet1[7] }
};
unsigned int funcMax = programSetMaxMap[currentProgramSet]; // highest index of modes
Program currentProgram = programSetMap[currentProgramSet][4];
String lastSerialCommand = "help";

/*****************************************************************/
/*                      Program constants                        */
/*****************************************************************/

int globals[8];
int globalConstants[8];

uint8_t gHue = 0;

CRGBPalette16 wavesConstants[6] = {
  { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
    0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 },
  { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
    0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F },
  { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
    0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF },
  { },
  {},
  {}
};

/*****************************************************************/
/*                         Setup & Loop                          */
/*****************************************************************/

void setup() {
  pinMode(PROGRAM_BUTTON, INPUT);
  digitalWrite(PROGRAM_BUTTON, HIGH);

  // Serial used for debugging current program
  Serial.begin(9600);
  Serial.println("READY");

  // lockState = EEPROM.read(EEPROM_LOCK_ADDRESS);
  // funcIndex = EEPROM.read(EEPROM_MODE_ADDRESS); // read last known function
  // Serial.println(funcIndex);

  // if (lockState == 0) {
  //   funcIndex = funcIndex >= funcMax ? 0 : funcIndex + 1; // set next function
  //   // EEPROM.update(EEPROM_MODE_ADDRESS, funcIndex);
  //   Serial.println(funcIndex);
  //   // funcIndex = funcMax; // DEBUG - testing latest function
  // }

  funcIndex = 4;
  currentProgram = programSetMap[currentProgramSet][funcIndex];

  delay(2000); // wait for LED init

  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  currentProgram.setup(); // run current program setup (if any)
}

void parseSerialCommands(String incomingData) {
  if (!incomingData.startsWith("l")){ lastSerialCommand = incomingData; }
  if (incomingData.startsWith("help")) {
    Serial.println("Avaliable Commands: togglechange, set <int>");
  } else if (incomingData.startsWith("togglechange")) {
    // lockState = lockState == 1 ? 0 : 1;
    // EEPROM.update(EEPROM_LOCK_ADDRESS, lockState);
    // if (lockState == 1) { EEPROM.update(EEPROM_MODE_ADDRESS, funcIndex); }
    // Serial.println(lockState);
    Serial.println("Depracated.");
  } else if (incomingData.startsWith("set ")) {
    uint8_t value = incomingData.substring(4).toInt();
    funcIndex = value;
    currentProgram = programSetMap[currentProgramSet][funcIndex];
    currentProgram.setup();
    // if (lockState == 1) { EEPROM.update(EEPROM_MODE_ADDRESS, funcIndex); }
    // Serial.println("Set program to: " + value);
  } else if (incomingData.startsWith("pause")) {
    // FastLED.clear(true);
    while (Serial.available() == 0) {} // Want to stop the runtime and fps tick instead
  } else if (incomingData.startsWith("clear")) {
    FastLED.clear(true);
    delay(1000);
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

bool awaitingButton = false;
unsigned short buttonState = 1;

void parseProgramButton(unsigned short buttonState) {
  delay(50); // debounce time

  delay(250); // time to differentiate between short and long button presses
  if (awaitingButton && digitalRead(PROGRAM_BUTTON) == 0) {
    // do something every 500ms until the button is depressed
    Serial.println("Wait");
    delay(500);
  } else if (!awaitingButton && digitalRead(PROGRAM_BUTTON) == 0) {
    // long press of button
    awaitingButton = true; // used to later check if button is still pressed
    FastLED.clear();
    delay(350);
    currentProgramSet = currentProgramSet == 1 ? 0 : 1;
    funcMax = programSetMaxMap[currentProgramSet];
    currentProgram = programSetMap[currentProgramSet][0];
    currentProgram.setup();
  } else if (!awaitingButton) {
    // short press of button
    FastLED.clear();
    funcIndex = funcIndex >= funcMax ? 0 : funcIndex + 1;
    Serial.println(funcIndex);
    currentProgram = programSetMap[currentProgramSet][funcIndex];
    currentProgram.setup();
  }
}

void loop() {
  buttonState = digitalRead(PROGRAM_BUTTON);
  if (Serial.available() > 0) { parseSerialCommands(Serial.readString()); }
  if (buttonState == 0) { parseProgramButton(buttonState); }
  else { awaitingButton = false; }

  currentProgram.runtime(); // program loop runtime
  currentProgram.fps(); // fps delay (unless overriden) [default normalFPS()]
}

/*****************************************************************/
/*                       Helper Functions                        */
/*****************************************************************/

int boolConverter(bool b){
  return b ? 1 : 0;
}

/*****************************************************************/
/*                           Programs                            */
/*****************************************************************/

void dot() {
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
}

/*===============================================================*/

void rainbow() {
  fill_rainbow(leds, NUM_LEDS, gHue, 7);

  FastLED.show();

  FastLED.delay(1000/(FRAMES_PER_SECOND*2));

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
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 78, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);

  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
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
  const bool gReverseDirection = globalConstants[0] == 0;

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
  pacifica_one_layer( wavesConstants[0], sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer( wavesConstants[1], sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( wavesConstants[2], sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer( wavesConstants[2], sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();

  FastLED.show();
  FastLED.delay(20);
}

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
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
void pacifica_add_whitecaps()
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
void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145); 
    leds[i].green= scale8( leds[i].green, 200); 
    leds[i] |= CRGB( 2, 5, 7);
  }
}