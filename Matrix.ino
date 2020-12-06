#define USE_OCTOWS2811
#include <OctoWS2811.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>

#define MATRIX_WIDTH  107
#define MATRIX_HEIGHT 8
#define MATRIX_TYPE   HORIZONTAL_MATRIX

FASTLED_USING_NAMESPACE

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define HWSERIAL Serial1
#define HWSERIALEVENT serialEvent1

#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define NUM_LEDS    (MATRIX_WIDTH*MATRIX_HEIGHT)

cLEDMatrix<-MATRIX_WIDTH, -MATRIX_HEIGHT, MATRIX_TYPE> matrix;
CRGB *leds = matrix[0];
cLEDText ScrollingMsg;

#define DEFAULT_BRIGHTNESS  32
#define FRAMES_PER_SECOND  120
#define DEFAULT_SLOW         5

#define BUFFER_LEN (400)

unsigned char TxtScreen[BUFFER_LEN] = {
  EFFECT_FRAME_RATE "\x00"
  EFFECT_SCROLL_LEFT
  "               "
  "               "
  "Merry Christmas"
  "               "
  "               "
};
int txtScreenLen = 78;
String serialString = "";
unsigned char txtChristmas[] = " Merry Christmas  ";

// List of patterns to cycle through.  Each is defined as a separate function below.
/* No static pattern - rotate */
void off() {}
void text();
void flash();
void white();
void cylon();
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();
void snowfall();
void christmas();
void text(); // TODO: these don't seem to be necessary until things break

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { snowfall, christmas, /*rainbow, rainbowWithGlitter,*/ snowfall, confetti, /*sinelon, juggle, bpm*/ };

void (*static_pattern)() = off;

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
bool active = true;
bool serialWaiting = false;
  
void setup() {
  delay(3000); // 3 second delay for recovery

  Serial.begin(115200);
  Serial.println("Serial init");
  HWSERIAL.begin(115200);
  HWSERIAL.println("HWSERIAL init");
  serialString.reserve(200);

  FastLED.addLeds<OCTOWS2811>(leds, MATRIX_WIDTH);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);

  ScrollingMsg.SetFont(MatriseFontData);
  ScrollingMsg.Init(&matrix, matrix.Width(), ScrollingMsg.FontHeight() + 1);
  ScrollingMsg.SetText(TxtScreen, txtScreenLen);
  ScrollingMsg.SetTextColrOptions(COLR_HSV | COLR_GRAD_AV, /*0x4*/0, 0xff, 0xff, 0xe0, 0xff, 0xff);
  ScrollingMsg.SetOptionsChangeMode(INSTANT_OPTIONS_MODE);
}

// Serial control codes
#define SERIAL_TEXT   'T'
#define SERIAL_TIME   '0'
#define SERIAL_BRIGHT 'B'
#define SERIAL_WHITE  'W'
#define SERIAL_CLEAR  'Z'
#define SERIAL_OFF    'X'
#define SERIAL_NEXT   'N'
#define SERIAL_FLASH  'F'

void processSerial()
{
  int val;

  switch (serialString.charAt(0))
  {
    case SERIAL_NEXT:
      static_pattern = off; // continue looping round patterns
      nextPattern();
      break;
    case SERIAL_TEXT:
      static_pattern = text;
      serialString.substring(1).toCharArray(TxtScreen, BUFFER_LEN);
      txtScreenLen = serialString.substring(1).length();
      Serial.println(txtScreenLen);
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      ScrollingMsg.SetText(TxtScreen, txtScreenLen);
      break;
     case SERIAL_TIME:
      // TODO: get time, and set it
      break;
     case SERIAL_BRIGHT:
      val = serialString.substring(1).toInt();
      FastLED.setBrightness(val);
      break;
     case SERIAL_WHITE:
      static_pattern = white;
      break;
     case SERIAL_FLASH:
      static_pattern = flash;
      break;
     case SERIAL_CLEAR:
      static_pattern = clear;
      break;
     case SERIAL_OFF:
      fill_solid( leds, NUM_LEDS, CRGB::Black);
      static_pattern = cylon;
      break;
     default:
      Serial.print("Unexpected input:");
      Serial.println(serialString);
  }

  serialString = "";
  serialWaiting = false;
}

void loop()
{
  if(static_pattern == off) {
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
  } else {
    //FastLED.clear();
    static_pattern();
  }
  
  FastLED.show(); // send the 'leds' array out to the actual LED strip
  FastLED.delay(1000/FRAMES_PER_SECOND); // insert a delay to keep the framerate modest

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 20 ) { nextPattern(); } // change patterns periodically
  EVERY_N_SECONDS( 60 ) { checkTime(); } // check time to disable out of hours

  if(serialWaiting)
  {
    processSerial();
  }
  
}

/* Turn on at sunset; turn off at 1am */
void checkTime()
{
  // TODO: check we have a valid time else assume active
  
  // TODO: fetch sunset from somewhere
  // sunset = 16:00
  
  /*if(now > sunset || now < 0100)
  {
    active = true;
  }
  else
  {
    active = false;
  }*/
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    serialString += inChar;
    
    if (inChar == '\n') {
      serialWaiting = true;
    }
  }
}

void HWSERIALEVENT()
{
  while (HWSERIAL.available()) {
    char inChar = (char)HWSERIAL.read();
    serialString += inChar;
    
    if (inChar == '\n') {
      serialWaiting = true;
    }
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void flash()
{
  static byte slow = 0;
  if(slow--) {
    return;
  } else {
    slow = DEFAULT_SLOW * 4;
    
    if(leds[0].r > 0)
    {
      fill_solid( leds, NUM_LEDS, CRGB::Black);
    } else {
      fill_rainbow( leds, NUM_LEDS, gHue, 7);
    }
  }
}

void white()
{
  fill_solid( leds, NUM_LEDS, CRGB::White);
}

void clear()
{
  fill_solid( leds, NUM_LEDS, CRGB::Black);
}

void cylon()
{
  static byte slow = 0;
  if(slow--) {
    return;
  } else {
    slow = DEFAULT_SLOW * 8;
    
    if(leds[0].r > 0)
    {
      leds[0] = CRGB::Black;
      leds[1] = CRGB::Red;
    } else {
      leds[0] = CRGB::Red;
      leds[1] = CRGB::Black;
    }
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

/* Create snowfall effect
 * 
 *  [ ] Black
 *  [+] Gray
 *  [*] GhostWhite
 *  [-] DimGray
 *  [ ] Black
*/
void snowfall() {  
  static byte slow = 0;
  if(slow--) {
    return;
  } else {
    slow = DEFAULT_SLOW;
  }
  
  // move everything down a row
  for (byte col = 0; col < MATRIX_WIDTH; col++) {
    for (byte row = MATRIX_HEIGHT-1; row > 0; row--) {
      leds[row * MATRIX_WIDTH + col] = leds[(row-1) * MATRIX_WIDTH + col];
    }

    // Yes, it's a bit of a hack hanging off just the red value but it works
    switch (leds[1 * MATRIX_WIDTH + col].r) {
      case 0x69: // DimGray
        leds[col] = CRGB::GhostWhite;
        break;
      case 0xF8: // GhostWhite
        leds[col] = CRGB::Gray;
        break;
      case 0x80: // Gray
      default:
        leds[col] = CRGB::Black;
    }
  }

  // fill in (new) top row 60% of the time
  if(random8()<60) {
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
  }
  if(random8()<40) {
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
  }
  if(random8()<20) {
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
    leds[random8(MATRIX_WIDTH)] = CRGB::DimGray;
  }
}

/* Static Merry Christmas */
void christmas() {
  ScrollingMsg.SetTextColrOptions(COLR_HSV | COLR_GRAD_AV, gHue, 0xff, 0xff, gHue + 0xe0, 0xff, 0xff);
  ScrollingMsg.UpdateText();
  ScrollingMsg.SetText(txtChristmas, sizeof(txtChristmas));
  // TODO: can we put the snow background behind this?
}

void text() {
  if (ScrollingMsg.UpdateText() == -1)
  {
    ScrollingMsg.SetTextColrOptions(COLR_HSV | COLR_GRAD_AV, gHue, 0xff, 0xff, gHue + 0xe0, 0xff, 0xff);
    ScrollingMsg.SetText(TxtScreen, txtScreenLen);
  }
}
