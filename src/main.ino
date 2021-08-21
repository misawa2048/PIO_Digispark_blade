#include <Arduino.h>
#include <EEPROM.h>
#include <FastLED.h>
#include <Button2.h>

#define LED_PIN (0)
#define GND_PIN (1)
#define GND2_PIN (4)
#define BUTTON_PIN (2)
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define NUM_LEDS (30)
#define EEPROM_ID (0)
#define FRAMES_PER_SECOND 60

#define BRIGHTNESS_L (64) // 0-255
#define BRIGHTNESS_H (200) // 0-255
#define NUM_BLADE_TYPE (3)

Button2 button = Button2(BUTTON_PIN);
uint16_t ptrR=30;
uint16_t ptrG=40;
uint16_t ptrB=50;
uint8_t linePattern=0;
CRGB leds[NUM_LEDS];
bool isEndBlade=false;

static uint8_t    sine8(uint8_t x) {
	static const uint8_t PROGMEM _NeoPixelSineTable[256] = {
		128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
		176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
		218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
		245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
		255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
		245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
		218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
		176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
		128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
		79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
		37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
		10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
			0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
		10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
		37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
		79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124
	};
	return pgm_read_byte(&_NeoPixelSineTable[x]); // 0-255 in, 0-255 out
}

uint8_t getSinVal(uint16_t _d4096, uint16_t _baseVal=128){
    uint16_t r = ((_d4096>>4)&0xff); // 実数部 0-255
    uint16_t p = (_d4096&0x0f); // 小数部 0-15
    uint16_t val = (sine8(r)*(15-p)+sine8((r+1)%256)*p)>>5;
    return min(val+_baseVal,(uint16_t)255);
}

void updateColorWater(uint8_t _numPixels){
  ptrR = (ptrR+21)%(1<<12);
  ptrG = (ptrG+23)%(2<<12);
  ptrB = (ptrB+25)%(4<<12);
  for(uint8_t i=0; i<_numPixels; i++) {
    uint16_t ofsv = (1<<12)*i/_numPixels;
    uint16_t dR = (((ptrR+ofsv)%(1<<12)));
    uint16_t dG = (((ptrG+ofsv)%(1<<12)));
    uint16_t dB = (((ptrB+ofsv)%(1<<12)));
    uint16_t valR = getSinVal(dR,8);
    uint16_t valG = getSinVal(dG,10);
    uint16_t valB = getSinVal(dB,20);
    leds[i] = CRGB(valR,valG,valB);
  }
}

void updateColorBugs(uint8_t _numPixels){
  #define BUG_BASE_COL (0xff2fcf)
  ptrR = (ptrR+(rand()&0x0f))%(1<<12);
  ptrG = (ptrG+(rand()&0x0f))%(1<<12);
  ptrB = (ptrB+(rand()&0x0f))%(1<<12);
  uint16_t randWidh = _numPixels*5;
  uint8_t vR,vG,vB;
  for(uint8_t i=0; i<_numPixels; i++) {
    if(((ptrR%randWidh)==(uint16_t)i)||((ptrG%randWidh)==(uint16_t)i)||((ptrB%randWidh)==(uint16_t)i)){
      vR=0x40+(rand()&0xbf);
      vG=0x40+(rand()&0xbf);
      vB=0x40+(rand()&0xbf);
    }else{
      static const uint8_t br[5]={
        0x10,0x10,0x10,0x3f,0x2f,
      };
      uint8_t tgtBr = br[(((int8_t)i-NUM_LEDS+5)<0) ? 0 : (((int8_t)i-NUM_LEDS+5))];
      vR = (((BUG_BASE_COL>>16)&0xff) * tgtBr)>>8;
      vG = (((BUG_BASE_COL>>8)&0xff) * tgtBr)>>8;
      vB = (((BUG_BASE_COL>>0)&0xff) * tgtBr)>>8;
    }
    leds[i] = CRGB(vR,vG,vB); //;
  }
}

void updateColorThunder(uint8_t _numPixels){
  uint32_t br=0x2f;
  uint32_t baseCol = 0xffff1f;
  ptrR = (ptrR+1)%(1<<12);
  ptrG = (ptrG+3)%(2<<12);
  ptrB = (ptrB+7)%(4<<12);
  uint32_t randWidh = _numPixels*15;
  for(uint8_t i=0; i<_numPixels; i++) {
    uint32_t vR = (((baseCol>>16)&0xff) * br)>>8;
    uint32_t vG = (((baseCol>>8)&0xff) * br)>>8;
    uint32_t vB = (((baseCol>>0)&0xff) * br)>>8;
    if(((ptrR%randWidh)==i)||((ptrG%randWidh)==i)||((ptrB%randWidh)==i)){
      vR=vG=vB=0x7f;
    }
    leds[i] = CRGB(vR,vG,vB); //;
  }
}

void updateColor(uint8_t _numPixels){
  switch(linePattern){
    case 0:  updateColorWater(_numPixels); break;
    case 1:  updateColorBugs(_numPixels); break;
    case 2:  updateColorThunder(_numPixels); break;
  }
}

void stripClear(){
	//memset(leds,0,NUM_LEDS);
  for(uint16_t i=0; i<NUM_LEDS; i++) {
    leds[i]=0;
  }
}
void startBlade(){
  isEndBlade=false;
  stripClear();
  for(uint16_t i=0; i<NUM_LEDS; i++) {
    updateColor(i);
    FastLED.show();
    delay(20);
  }
}

void endBlade(){
  for(uint8_t i=0; i<NUM_LEDS; i++) {
    updateColor(NUM_LEDS-i-1);
    FastLED.show();
    delay(20);
    stripClear();
  }
}

void btnClick(Button2& btn){
  linePattern=(linePattern+1)%NUM_BLADE_TYPE;
  EEPROM.write(EEPROM_ID,(uint8_t)linePattern);
}
void btnHold(Button2& btn) {
  if(!isEndBlade){
    endBlade();
    isEndBlade=true;
  }else{
    isEndBlade=false;
    startBlade();
  }
}

void setup() {
  delay(10);
  linePattern = EEPROM.read(EEPROM_ID) % NUM_BLADE_TYPE;
  pinMode(GND_PIN,  OUTPUT); digitalWrite(GND_PIN,  LOW);
  pinMode(GND2_PIN, OUTPUT); digitalWrite(GND2_PIN, LOW);

  button.setDoubleClickTime(0);
  button.setLongClickTime(1000);
  button.setClickHandler(btnClick);
  button.setLongClickDetectedHandler(btnHold);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(button.isPressedRaw() ? BRIGHTNESS_H : BRIGHTNESS_L );
  startBlade();
}

void loop() {
  button.loop();
  if(!isEndBlade){
    updateColor(NUM_LEDS);
    FastLED.show();
  }
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}
