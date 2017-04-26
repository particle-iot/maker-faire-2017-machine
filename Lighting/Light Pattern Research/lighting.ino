// LED patterns from http://funkboxing.com/wordpress/?p=2154

/*------------------------------------#
#----- FASTSPI2 EFFECTS EXAMPLES -----#
#-------------- v0.51 ----------------#
#------------ teldredge --------------#
#-------- www.funkboxing.com ---------#
#------ teldredge1979@gmail.com ------#
#-------------------------------------#
### THESE EFFECTS ARE MOSTLY DESIGNED FOR A LOOP/RING OF LEDS
### BUT PLENTY USEFUL FOR OTHER CONFIGURATIONS
### RECENT CHANGES v0.51
    -ADDED SOFTWARE SERIAL FOR BLUETOOTH CONTROL
    -ADDED (z) SHOW LED COMMAND (FOR BETTER SERIAL CONTROL)
    -ADDED SET (a)ll LEDS TO COLOR HSV (0-255)
    -ADDED (c)lear FUNCTION
    -ADDED (v) SET INDIVIDUAL LED HSV FUNCTION
    -ADDED (Q)UERY VERSION NUMBER FUNCTION
### NOTES
    -MAKE SURE YOU ARE USING FAST_SPI RC3 OR LATER
    -THIS IS AN EXAMPLE LIBRARY SO YOU'LL PRABALY WANT TO EDIT TO DO ANYTHING USEFUL WITH IT
    -GOTO FUNKBOXING FAST_SPI2 COMMENTS PAGE FOR HELP
    -DEMO MODE BLOCKS SERIAL COMMANDS
### LICENSE:::USE FOR WHATEVER YOU WANT, WHENEVER YOU WANT, HOWEVER YOU WANT, WHYEVER YOU WANT
### BUT YOU MUST YODEL ONCE FOR FREEDOM AND MAYBE DONATE TO SOMETHING WORTHWHILE

|-----------------------------------------------------|
|                                                     |
|           FROM THE FAST_SPI2 EXAMPLE FILE           |
|                                                     |
|----------------------------------------------------*/
// Uncomment this line if you have any interrupts that are changing pins - this causes the library to be a little bit more cautious
// #define FAST_SPI_INTERRUPTS_WRITE_PINS 1

// Uncomment this line to force always using software, instead of hardware, SPI (why?)
// #define FORCE_SOFTWARE_SPI 1

// Uncomment this line if you want to talk to DMX controllers
// #define FASTSPI_USE_DMX_SIMPLE 1
//-----------------------------------------------------

#include "neopixel.h"
#include "hsv.h"

SYSTEM_THREAD(ENABLED);

#define VERSION_NUMBER 0.51

//---LED SETUP STUFF
#define LED_COUNT 56          //FOR TESTING w/ SIGN
#define PIXEL_COUNT LED_COUNT
#define PIXEL_PIN A1            //NEOPIXEL PIN
#define PIXEL_TYPE WS2812B      //NEOPIXEL TYPE

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

uint32_t CHSV(uint8_t h, uint8_t s, uint8_t v) {
    HsvColor hsv(h, s, v);
    RgbColor rgb = HsvToRgb(hsv);
    return (rgb.r << 16) | (rgb.g << 8) | rgb.b;
}

int BOTTOM_INDEX = 0;
int TOP_INDEX = int(LED_COUNT/2);
int EVENODD = LED_COUNT%2;
int ledsX[LED_COUNT][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)
//int ledMode = 3;           //-START IN RAINBOW LOOP
//int ledMode = 888;         //-START IN DEMO MODE
int ledMode = 6;             //-MODE TESTING

int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR
int max_bright = 64;         //-SET MAX BRIGHTNESS TO 1/4

int thisindex = 0;           //-SET SINGLE LED VAR
int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

int btBOOL = 0;              //-ACTIVATE BLUETOOTH OR NOT
byte inbyte;                 //-SERIAL INPUT BYTE
int thisarg;                 //-SERIAL INPUT ARG

//---LED FX VARS
int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR


//------------------------------------- UTILITY FXNS --------------------------------------
//---SET THE COLOR OF A SINGLE RGB LED
void set_color_led(int adex, int cred, int cgrn, int cblu) {
  strip.setPixelColor(adex, cred, cgrn, cblu);
}

//---FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
  //-ONLY WORKS WITH INDEX < TOPINDEX
  if (i == BOTTOM_INDEX) {return BOTTOM_INDEX;}
  if (i == TOP_INDEX && EVENODD == 1) {return TOP_INDEX + 1;}
  if (i == TOP_INDEX && EVENODD == 0) {return TOP_INDEX;}
  return LED_COUNT - i;
}

//---FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {iN = ( i + TOP_INDEX ) % LED_COUNT; }
  return iN;
}

//---FIND ADJACENT INDEX CLOCKWISE
int adjacent_cw(int i) {
  int r;
  if (i < LED_COUNT - 1) {r = i + 1;}
  else {r = 0;}
  return r;
}

//---FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {r = i - 1;}
  else {r = LED_COUNT - 1;}
  return r;
}

void copy_led_array(){
  for(int i = 0; i < LED_COUNT; i++ ) {
    uint32_t c = strip.getPixelColor(i);
    ledsX[i][0] = (uint8_t)(c >> 16);
    ledsX[i][1] = (uint8_t)(c >> 8);
    ledsX[i][2] = (uint8_t)(c);
  }
}


//------------------------LED EFFECT FUNCTIONS------------------------
void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
    for(int i = 0 ; i < LED_COUNT; i++ ) {
      strip.setPixelColor(i, cred, cgrn, cblu);
    }
}

void one_color_allHSV(int ahue) {    //-SET ALL LEDS TO ONE COLOR (HSV)
    for(int i = 0 ; i < LED_COUNT; i++ ) {
      strip.setPixelColor(i, CHSV(ahue, thissat, 255));
    }
}

void rainbow_fade() {                         //-m2-FADE ALL LEDS THROUGH HSV RAINBOW
    ihue++;
    if (ihue > 255) {ihue = 0;}
    for(int idex = 0 ; idex < LED_COUNT; idex++ ) {
      strip.setPixelColor(idex, CHSV(ihue, thissat, 255));
    }
    strip.show();
    delay(thisdelay);
}

void rainbow_loop() {                        //-m3-LOOP HSV RAINBOW
  idex++;
  ihue = ihue + thisstep;
  if (idex >= LED_COUNT) {idex = 0;}
  if (ihue > 255) {ihue = 0;}
  strip.setPixelColor(idex, CHSV(ihue, thissat, 255));
  strip.show();
  delay(thisdelay);
}

void random_burst() {                         //-m4-RANDOM INDEX/COLOR
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  strip.setPixelColor(idex, CHSV(ihue, thissat, 255));
  strip.show();
  delay(thisdelay);
}

void color_bounce() {                        //-m5-BOUNCE COLOR (SINGLE LED)
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  for(int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      strip.setPixelColor(i, CHSV(thishue, thissat, 255));
    }
    else
    {
      strip.setPixelColor(i, CHSV(0, 0, 0));
    }
  }
  strip.show();
  delay(thisdelay);
}

void color_bounceFADE() {                    //-m6-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
  if (bouncedirection == 0) {
    idex = idex + 1;
    if (idex == LED_COUNT) {
      bouncedirection = 1;
      idex = idex - 1;
    }
  }
  if (bouncedirection == 1) {
    idex = idex - 1;
    if (idex == 0) {
      bouncedirection = 0;
    }
  }
  int iL1 = adjacent_cw(idex);
  int iL2 = adjacent_cw(iL1);
  int iL3 = adjacent_cw(iL2);
  int iR1 = adjacent_ccw(idex);
  int iR2 = adjacent_ccw(iR1);
  int iR3 = adjacent_ccw(iR2);
  for(int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {strip.setPixelColor(i, CHSV(thishue, thissat, 255));}
    else if (i == iL1) {strip.setPixelColor(i, CHSV(thishue, thissat, 150));}
    else if (i == iL2) {strip.setPixelColor(i, CHSV(thishue, thissat, 80));}
    else if (i == iL3) {strip.setPixelColor(i, CHSV(thishue, thissat, 20));}
    else if (i == iR1) {strip.setPixelColor(i, CHSV(thishue, thissat, 150));}
    else if (i == iR2) {strip.setPixelColor(i, CHSV(thishue, thissat, 80));}
    else if (i == iR3) {strip.setPixelColor(i, CHSV(thishue, thissat, 20));}
    else {strip.setPixelColor(i, CHSV(0, 0, 0));}
  }
  strip.show();
  delay(thisdelay);
}

void ems_lightsONE() {                    //-m7-EMERGENCY LIGHTS (TWO COLOR SINGLE LED)
  idex++;
  if (idex >= LED_COUNT) {idex = 0;}
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (thishue + 160) % 255;
  for(int i = 0; i < LED_COUNT; i++ ) {
    if (i == idexR) {strip.setPixelColor(i, CHSV(thishue, thissat, 255));}
    else if (i == idexB) {strip.setPixelColor(i, CHSV(thathue, thissat, 255));}
    else {strip.setPixelColor(i, CHSV(0, 0, 0));}
  }
  strip.show();
  delay(thisdelay);
}

void ems_lightsALL() {                  //-m8-EMERGENCY LIGHTS (TWO COLOR SOLID)
  idex++;
  if (idex >= LED_COUNT) {idex = 0;}
  int idexR = idex;
  int idexB = antipodal_index(idexR);
  int thathue = (thishue + 160) % 255;
  strip.setPixelColor(idexR, CHSV(thishue, thissat, 255));
  strip.setPixelColor(idexB, CHSV(thathue, thissat, 255));
  strip.show();
  delay(thisdelay);
}

void flicker() {                          //-m9-FLICKER EFFECT
  int random_bright = random(0,255);
  int random_delay = random(10,100);
  int random_bool = random(0,random_bright);
  if (random_bool < 10) {
    for(int i = 0 ; i < LED_COUNT; i++ ) {
      strip.setPixelColor(i, CHSV(thishue, thissat, random_bright));
    }
    strip.show();
    delay(random_delay);
  }
}

void pulse_one_color_all() {              //-m10-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    ibright++;
    if (ibright >= 255) {bouncedirection = 1;}
  }
  if (bouncedirection == 1) {
    ibright = ibright - 1;
    if (ibright <= 1) {bouncedirection = 0;}
  }
    for(int idex = 0 ; idex < LED_COUNT; idex++ ) {
      strip.setPixelColor(idex, CHSV(thishue, thissat, ibright));
    }
    strip.show();
    delay(thisdelay);
}

void pulse_one_color_all_rev() {           //-m11-PULSE SATURATION ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    isat++;
    if (isat >= 255) {bouncedirection = 1;}
  }
  if (bouncedirection == 1) {
    isat = isat - 1;
    if (isat <= 1) {bouncedirection = 0;}
  }
    for(int idex = 0 ; idex < LED_COUNT; idex++ ) {
      strip.setPixelColor(idex, CHSV(thishue, isat, 255));
    }
    strip.show();
    delay(thisdelay);
}

void fade_vertical() {                    //-m12-FADE 'UP' THE LOOP
  idex++;
  if (idex > TOP_INDEX) {idex = 0;}
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  ibright = ibright + 10;
  if (ibright > 255) {ibright = 0;}
  strip.setPixelColor(idexA, CHSV(thishue, thissat, ibright));
  strip.setPixelColor(idexB, CHSV(thishue, thissat, ibright));
  strip.show();
  delay(thisdelay);
}

void random_red() {                       //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED
  int temprand;
  for(int i = 0; i < LED_COUNT; i++ ) {
    temprand = random(0,100);
    strip.setPixelColor(i, (temprand > 50) ? 255 : 0, 0, 0);
  }
  strip.show();
}

void rule30() {                          //-m13-1D CELLULAR AUTOMATA - RULE 30 (RED FOR NOW)
  if (bouncedirection == 0) {
    random_red();
    bouncedirection = 1;
  }
  copy_led_array();
  int iCW;
  int iCCW;
  int y = 100;
  for(int i = 0; i < LED_COUNT; i++ ) {
    iCW = adjacent_cw(i);
    iCCW = adjacent_ccw(i);
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] > y) {strip.setPixelColor(i, 0);}
    if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {strip.setPixelColor(i, 0);}
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {strip.setPixelColor(i, 0);}
    if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {strip.setPixelColor(i, 255);}
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] > y) {strip.setPixelColor(i, 255);}
    if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {strip.setPixelColor(i, 255);}
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {strip.setPixelColor(i, 255);}
    if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {strip.setPixelColor(i, 0);}
  }
  strip.show();
  delay(thisdelay);
}

void random_march() {                   //-m14-RANDOM MARCH CCW
  copy_led_array();
  int iCCW;
  strip.setPixelColor(0, CHSV(random(0,255), 255, 255));
  for(int idex = 1; idex < LED_COUNT ; idex++ ) {
    iCCW = adjacent_ccw(idex);
    strip.setPixelColor(idex, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
  }
  strip.show();
  delay(thisdelay);
}

void rwb_march() {                    //-m15-R,W,B MARCH CCW
  copy_led_array();
  int iCCW;
  idex++;
  if (idex > 2) {idex = 0;}
  switch (idex) {
    case 0:
      strip.setPixelColor(0, 255, 0, 0);
    break;
    case 1:
      strip.setPixelColor(0, 255, 255, 255);
    break;
    case 2:
      strip.setPixelColor(0, 0, 0, 255);
    break;
  }
    for(int i = 1; i < LED_COUNT; i++ ) {
      iCCW = adjacent_ccw(i);
      strip.setPixelColor(i, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
    }
  strip.show();
  delay(thisdelay);
}

void radiation() {                   //-m16-SORT OF RADIATION SYMBOLISH-
  int N3  = int(LED_COUNT/3);
  int N6  = int(LED_COUNT/6);
  int N12 = int(LED_COUNT/12);
  for(int i = 0; i < N6; i++ ) {     //-HACKY, I KNOW...
    tcount = tcount + .02;
    if (tcount > 3.14) {tcount = 0.0;}
    ibright = int(sin(tcount)*255);
    int j0 = (i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0+N3) % LED_COUNT;
    int j2 = (j1+N3) % LED_COUNT;
    strip.setPixelColor(j0, CHSV(thishue, thissat, ibright));
    strip.setPixelColor(j1, CHSV(thishue, thissat, ibright));
    strip.setPixelColor(j2, CHSV(thishue, thissat, ibright));
  }
  strip.show();
  delay(thisdelay);
}

void color_loop_vardelay() {                    //-m17-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
  delay(1); return;
  // FIXME: crashes
  idex++;
  if (idex > LED_COUNT) {idex = 0;}
  int di = abs(TOP_INDEX - idex);
  int t = constrain((10/di)*10, 10, 500);
  for(int i = 0; i < LED_COUNT; i++ ) {
    if (i == idex) {
      strip.setPixelColor(i, CHSV(0, thissat, 255));
    }
    else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
  delay(t);
}

void white_temps() {                            //-m18-SHOW A SAMPLE OF BLACK BODY RADIATION COLOR TEMPERATURES
  int N9 = int(LED_COUNT/9);
  for (int i = 0; i < LED_COUNT; i++ ) {
    if (i >= 0 && i < N9) {strip.setPixelColor(i, 255, 147, 41);}         //-CANDLE - 1900 ));
    if (i >= N9 && i < N9*2) {strip.setPixelColor(i, 255, 197, 143);}     //-40W TUNG - 2600 ));
    if (i >= N9*2 && i < N9*3) {strip.setPixelColor(i, 255, 214, 170);}   //-100W TUNG - 2850 ));
    if (i >= N9*3 && i < N9*4) {strip.setPixelColor(i, 255, 241, 224);}   //-HALOGEN - 3200 ));
    if (i >= N9*4 && i < N9*5) {strip.setPixelColor(i, 255, 250, 244);}   //-CARBON ARC - 5200 ));
    if (i >= N9*5 && i < N9*6) {strip.setPixelColor(i, 255, 255, 251);}   //-HIGH NOON SUN - 5400 ));
    if (i >= N9*6 && i < N9*7) {strip.setPixelColor(i, 255, 255, 255);}   //-DIRECT SUN - 6000 ));
    if (i >= N9*7 && i < N9*8) {strip.setPixelColor(i, 201, 226, 255);}   //-OVERCAST SKY - 7000 ));
    if (i >= N9*8 && i < LED_COUNT) {strip.setPixelColor(i, 64, 156, 255);}//-CLEAR BLUE SKY - 20000   ));
  }
  strip.show();
  delay(100);
}

void sin_bright_wave() {        //-m19-BRIGHTNESS SINE WAVE
  for(int i = 0; i < LED_COUNT; i++ ) {
    tcount = tcount + .1;
    if (tcount > 3.14) {tcount = 0.0;}
    ibright = int(sin(tcount)*255);
    strip.setPixelColor(i, CHSV(thishue, thissat, ibright));
    strip.show();
    delay(thisdelay);
  }
}

void pop_horizontal() {        //-m20-POP FROM LEFT TO RIGHT UP THE RING
  int ix;
  if (bouncedirection == 0) {
    bouncedirection = 1;
    ix = idex;
  }
  else if (bouncedirection == 1) {
    bouncedirection = 0;
    ix = horizontal_index(idex);
    idex++;
    if (idex > TOP_INDEX) {idex = 0;}
  }
  for(int i = 0; i < LED_COUNT; i++ ) {
    if (i == ix) {
      strip.setPixelColor(i, CHSV(thishue, thissat, 255));
    }
    else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
  delay(thisdelay);
}

void quad_bright_curve() {      //-m21-QUADRATIC BRIGHTNESS CURVER
  int ax;
  for(int x = 0; x < LED_COUNT; x++ ) {
    if (x <= TOP_INDEX) {ax = x;}
    else if (x > TOP_INDEX) {ax = LED_COUNT-x;}
    int a = 1; int b = 1; int c = 0;
    int iquad = -(ax*ax*a)+(ax*b)+c; //-ax2+bx+c
    int hquad = -(TOP_INDEX*TOP_INDEX*a)+(TOP_INDEX*b)+c;
    ibright = int((float(iquad)/float(hquad))*255);
    strip.setPixelColor(x, CHSV(thishue, thissat, ibright));
  }
  strip.show();
  delay(thisdelay);
}

void flame() {                                    //-m22-FLAMEISH EFFECT
  int idelay = random(0,35);
  float hmin = 0.1; float hmax = 45.0;
  float hdif = hmax-hmin;
  int randtemp = random(0,3);
  float hinc = (hdif/float(TOP_INDEX))+randtemp;
  int ihue = hmin;
  for(int i = 0; i <= TOP_INDEX; i++ ) {
    ihue = ihue + hinc;
    strip.setPixelColor(i, CHSV(ihue, thissat, 255));
    int ih = horizontal_index(i);
    strip.setPixelColor(ih, CHSV(ihue, thissat, 255));
    strip.setPixelColor(TOP_INDEX, 255, 255, 255);
    strip.show();
    delay(idelay);
  }
}

void rainbow_vertical() {                        //-m23-RAINBOW 'UP' THE LOOP
  idex++;
  if (idex > TOP_INDEX) {idex = 0;}
  ihue = ihue + thisstep;
  if (ihue > 255) {ihue = 0;}
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  strip.setPixelColor(idexA, CHSV(ihue, thissat, 255));
  strip.setPixelColor(idexB, CHSV(ihue, thissat, 255));
  strip.show();
  delay(thisdelay);
}

void pacman() {                                  //-m24-REALLY TERRIBLE PACMAN CHOMPING EFFECT
  int s = int(LED_COUNT/4);
  lcount++;
  if (lcount > 5) {lcount = 0;}
  if (lcount == 0) {
    for(int i = 0 ; i < LED_COUNT; i++ ) {set_color_led(i, 255, 255, 0);}
    }
  if (lcount == 1 || lcount == 5) {
    for(int i = 0 ; i < LED_COUNT; i++ ) {set_color_led(i, 255, 255, 0);}
    strip.setPixelColor(s, 0, 0, 0);}
  if (lcount == 2 || lcount == 4) {
    for(int i = 0 ; i < LED_COUNT; i++ ) {set_color_led(i, 255, 255, 0);}
    strip.setPixelColor(s-1, 0, 0, 0);
    strip.setPixelColor(s, 0, 0, 0);
    strip.setPixelColor(s+1, 0, 0, 0);}
  if (lcount == 3) {
    for(int i = 0 ; i < LED_COUNT; i++ ) {set_color_led(i, 255, 255, 0);}
    strip.setPixelColor(s-2, 0, 0, 0);
    strip.setPixelColor(s-1, 0, 0, 0);
    strip.setPixelColor(s, 0, 0, 0);
    strip.setPixelColor(s+1, 0, 0, 0);
    strip.setPixelColor(s+2, 0, 0, 0);}
  strip.show();
  delay(thisdelay);
}

void random_color_pop() {                         //-m25-RANDOM COLOR POP
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  one_color_all(0, 0, 0);
  strip.setPixelColor(idex, CHSV(ihue, thissat, 255));
  strip.show();
  delay(thisdelay);
}

void ems_lightsSTROBE() {                  //-m26-EMERGENCY LIGHTS (STROBE LEFT/RIGHT)
  int thishue = 0;
  int thathue = (thishue + 160) % 255;
  for(int x = 0 ; x < 5; x++ ) {
    for(int i = 0 ; i < TOP_INDEX; i++ ) {
        strip.setPixelColor(i, CHSV(thishue, thissat, 255));
    }
    strip.show(); delay(thisdelay);
    one_color_all(0, 0, 0);
    strip.show(); delay(thisdelay);
  }
  for(int x = 0 ; x < 5; x++ ) {
    for(int i = TOP_INDEX ; i < LED_COUNT; i++ ) {
        strip.setPixelColor(i, CHSV(thathue, thissat, 255));
    }
    strip.show(); delay(thisdelay);
    one_color_all(0, 0, 0);
    strip.show(); delay(thisdelay);
  }
}

void rgb_propeller() {                           //-m27-RGB PROPELLER
  idex++;
  int ghue = (thishue + 80) % 255;
  int bhue = (thishue + 160) % 255;
  int N3  = int(LED_COUNT/3);
  int N12 = int(LED_COUNT/12);
  for(int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0+N3) % LED_COUNT;
    int j2 = (j1+N3) % LED_COUNT;
    strip.setPixelColor(j0, CHSV(thishue, thissat, 255));
    strip.setPixelColor(j1, CHSV(ghue, thissat, 255));
    strip.setPixelColor(j2, CHSV(bhue, thissat, 255));
  }
  strip.show();
  delay(thisdelay);
}

void kitt() {                                     //-m28-KNIGHT INDUSTIES 2000
  int rand = random(0, TOP_INDEX);
  for(int i = 0; i < rand; i++ ) {
    strip.setPixelColor(TOP_INDEX+i, CHSV(thishue, thissat, 255));
    strip.setPixelColor(TOP_INDEX-i, CHSV(thishue, thissat, 255));
    strip.show();
    delay(thisdelay/rand);
  }
  for(int i = rand; i > 0; i-- ) {
    strip.setPixelColor(TOP_INDEX+i, CHSV(thishue, thissat, 0));
    strip.setPixelColor(TOP_INDEX-i, CHSV(thishue, thissat, 0));
    strip.show();
    delay(thisdelay/rand);
  }
}

void matrix() {                                   //-m29-ONE LINE MATRIX
  int rand = random(0, 100);
  if (rand > 90) {
    strip.setPixelColor(0, CHSV(thishue, thissat, 255));
  }
  else {strip.setPixelColor(0, CHSV(thishue, thissat, 0));}
  copy_led_array();
    for(int i = 1; i < LED_COUNT; i++ ) {
      strip.setPixelColor(i, ledsX[i-1][0], ledsX[i-1][1], ledsX[i-1][2]);
  }
  strip.show();
  delay(thisdelay);
}

void strip_march_cw() {                        //-m50-MARCH STRIP CW
  copy_led_array();
  int iCW;
  for(int i = 0; i < LED_COUNT; i++ ) {
    iCW = adjacent_cw(i);
    strip.setPixelColor(i, ledsX[iCW][0], ledsX[iCW][1], ledsX[iCW][2]);
  }
  strip.show();
  delay(thisdelay);
}

void strip_march_ccw() {                        //-m51-MARCH STRIP CCW
  copy_led_array();
  int iCCW;
  for(int i = 0; i < LED_COUNT; i++ ) {
    iCCW = adjacent_ccw(i);
    strip.setPixelColor(i, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
  }
  strip.show();
  delay(thisdelay);
}

void new_rainbow_loop(){                       //-m88-RAINBOW FADE FROM FAST_SPI2
  strip.show();
  delay(thisdelay);
}

void demo_modeA(){
  int r = 10;
  thisdelay = 20; thisstep = 10; thishue = 0; thissat = 255;
  one_color_all(255,255,255); strip.show(); delay(1200);
  for(int i=0; i<r*25; i++) {rainbow_fade();}
  for(int i=0; i<r*20; i++) {rainbow_loop();}
  for(int i=0; i<r*20; i++) {random_burst();}
  for(int i=0; i<r*12; i++) {color_bounce();}
  thisdelay = 40;
  for(int i=0; i<r*12; i++) {color_bounceFADE();}
  for(int i=0; i<r*6; i++) {ems_lightsONE();}
  for(int i=0; i<r*5; i++) {ems_lightsALL();}
  thishue = 160; thissat = 50;
  for(int i=0; i<r*40; i++) {flicker();}
  one_color_all(0,0,0); strip.show();
  thisdelay = 15; thishue = 0; thissat = 255;
  for(int i=0; i<r*50; i++) {pulse_one_color_all();}
  for(int i=0; i<r*40; i++) {pulse_one_color_all_rev();}
  thisdelay = 60; thishue = 180;
  for(int i=0; i<r*5; i++) {fade_vertical();}
  random_red();
  thisdelay = 100;
  for(int i=0; i<r*5; i++) {rule30();}
  thisdelay = 40;
  for(int i=0; i<r*8; i++) {random_march();}
  thisdelay = 80;
  for(int i=0; i<r*5; i++) {rwb_march();}
  one_color_all(0,0,0); ; strip.show();
  thisdelay = 60; thishue = 95;
  for(int i=0; i<r*15; i++) {radiation();}
  for(int i=0; i<r*15; i++) {color_loop_vardelay();}
  for(int i=0; i<r*5; i++) {white_temps();}
  thisdelay = 35; thishue = 180;
  for(int i=0; i<r; i++) {sin_bright_wave();}
  thisdelay = 100; thishue = 0;
  for(int i=0; i<r*5; i++) {pop_horizontal();}
  thisdelay = 100; thishue = 180;
  for(int i=0; i<r*4; i++) {quad_bright_curve();}
  one_color_all(0,0,0); strip.show();
  for(int i=0; i<r*3; i++) {flame();}
  thisdelay = 50;
  for(int i=0; i<r*10; i++) {pacman();}
  thisdelay = 50; thisstep = 15;
  for(int i=0; i<r*12; i++) {rainbow_vertical();}
  thisdelay = 100;
  for(int i=0; i<r*3; i++) {strip_march_ccw();}
  for(int i=0; i<r*3; i++) {strip_march_cw();}
  demo_modeB();
  thisdelay = 5;
  for(int i=0; i<r*120; i++) {new_rainbow_loop();}
  one_color_all(255,0,0); strip.show(); delay(1200);
  one_color_all(0,255,0); strip.show(); delay(1200);
  one_color_all(0,0,255); strip.show(); delay(1200);
  one_color_all(255,255,0); strip.show(); delay(1200);
  one_color_all(0,255,255); strip.show(); delay(1200);
  one_color_all(255,0,255); strip.show(); delay(1200);
}

void demo_modeB(){
  int r = 10;
  one_color_all(0,0,0); strip.show();
  thisdelay = 35;
  for(int i=0; i<r*10; i++) {random_color_pop();}
  for(int i=0; i<r/2; i++) {ems_lightsSTROBE();}
  thisdelay = 50;
  for(int i=0; i<r*10; i++) {rgb_propeller();}
  one_color_all(0,0,0); strip.show();
  thisdelay = 100; thishue = 0;
  for(int i=0; i<r*3; i++) {kitt();}
  one_color_all(0,0,0); strip.show();
  thisdelay = 30; thishue = 95;
  for(int i=0; i<r*25; i++) {matrix();}
  one_color_all(0,0,0); strip.show();
}

void change_mode(int newmode){
  thissat = 255;
  switch (newmode) {
    case 0: one_color_all(0,0,0); strip.show(); break;   //---ALL OFF
    case 1: one_color_all(255,255,255); strip.show(); break;   //---ALL ON
    case 2: thisdelay = 20; break;                      //---STRIP RAINBOW FADE
    case 3: thisdelay = 20; thisstep = 10; break;       //---RAINBOW LOOP
    case 4: thisdelay = 20; break;                      //---RANDOM BURST
    case 5: thisdelay = 20; thishue = 0; break;         //---CYLON v1
    case 6: thisdelay = 40; thishue = 0; break;         //---CYLON v2
    case 7: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SINGLE
    case 8: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SOLID
    case 9: thishue = 160; thissat = 50; break;         //---STRIP FLICKER
    case 10: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR BRIGHTNESS
    case 11: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR SATURATION
    case 12: thisdelay = 60; thishue = 180; break;      //---VERTICAL SOMETHING
    case 13: thisdelay = 100; break;                    //---CELL AUTO - RULE 30 (RED)
    case 14: thisdelay = 40; break;                     //---MARCH RANDOM COLORS
    case 15: thisdelay = 80; break;                     //---MARCH RWB COLORS
    case 16: thisdelay = 60; thishue = 95; break;       //---RADIATION SYMBOL
    //---PLACEHOLDER FOR COLOR LOOP VAR DELAY VARS
    case 19: thisdelay = 35; thishue = 180; break;      //---SIN WAVE BRIGHTNESS
    case 20: thisdelay = 100; thishue = 0; break;       //---POP LEFT/RIGHT
    case 21: thisdelay = 100; thishue = 180; break;     //---QUADRATIC BRIGHTNESS CURVE
    //---PLACEHOLDER FOR FLAME VARS
    case 23: thisdelay = 50; thisstep = 15; break;      //---VERITCAL RAINBOW
    case 24: thisdelay = 50; break;                     //---PACMAN
    case 25: thisdelay = 35; break;                     //---RANDOM COLOR POP
    case 26: thisdelay = 25; thishue = 0; break;        //---EMERGECNY STROBE
    case 27: thisdelay = 25; thishue = 0; break;        //---RGB PROPELLER
    case 28: thisdelay = 100; thishue = 0; break;       //---KITT
    case 29: thisdelay = 50; thishue = 95; break;       //---MATRIX RAIN
    case 50: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 51: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 88: thisdelay = 5; break;                      //---NEW RAINBOW LOOP
    case 101: one_color_all(255,0,0); strip.show(); break;   //---ALL RED
    case 102: one_color_all(0,255,0); strip.show(); break;   //---ALL GREEN
    case 103: one_color_all(0,0,255); strip.show(); break;   //---ALL BLUE
    case 104: one_color_all(255,255,0); strip.show(); break;   //---ALL COLOR X
    case 105: one_color_all(0,255,255); strip.show(); break;   //---ALL COLOR Y
    case 106: one_color_all(255,0,255); strip.show(); break;   //---ALL COLOR Z
  }
  bouncedirection = 0;
  one_color_all(0,0,0);
  ledMode = newmode;
}


//------------------SETUP------------------
void setup()
{
  Serial.begin();

  strip.begin();
  strip.setBrightness(max_bright);

  one_color_all(0,0,0); //-CLEAR STRIP
  strip.show();
  Serial.println("---SETUP COMPLETE---");
}


//------------------MAIN LOOP------------------
void loop() {
    switch (ledMode) {
      case 999: break;
      case  2: rainbow_fade(); break;
      case  3: rainbow_loop(); break;
      case  4: random_burst(); break;
      case  5: color_bounce(); break;
      case  6: color_bounceFADE(); break;
      case  7: ems_lightsONE(); break;
      case  8: ems_lightsALL(); break;
      case  9: flicker(); break;
      case 10: pulse_one_color_all(); break;
      case 11: pulse_one_color_all_rev(); break;
      case 12: fade_vertical(); break;
      case 13: rule30(); break;
      case 14: random_march(); break;
      case 15: rwb_march(); break;
      case 16: radiation(); break;
      case 17: color_loop_vardelay(); break;
      case 18: white_temps(); break;
      case 19: sin_bright_wave(); break;
      case 20: pop_horizontal(); break;
      case 21: quad_bright_curve(); break;
      case 22: flame(); break;
      case 23: rainbow_vertical(); break;
      case 24: pacman(); break;
      case 25: random_color_pop(); break;
      case 26: ems_lightsSTROBE(); break;
      case 27: rgb_propeller(); break;
      case 28: kitt(); break;
      case 29: matrix(); break;
      case 50: strip_march_ccw(); break;
      case 51: strip_march_cw(); break;
      case 88: new_rainbow_loop(); break;
      case 888: demo_modeA(); break;
      case 889: demo_modeB(); break;
    }

  //---PROCESS HARDWARE SERIAL COMMANDS AND ARGS
  while (Serial.available() > 0) {inbyte = Serial.read();
    switch(inbyte) {
    case 59: break; //---BREAK IF INBYTE = ';'
    case 108:      //---"l" - SET SINGLE LED VALUE RGB
      thisindex = Serial.parseInt();
      thisRED = Serial.parseInt();
      thisGRN = Serial.parseInt();
      thisBLU = Serial.parseInt();
      if (ledMode != 999) {
        ledMode = 999;
        one_color_all(0,0,0);}
      strip.setPixelColor(thisindex, thisRED, thisGRN, thisBLU);
      break;
    case 118:      //---"v" - SET SINGLE LED VALUE HSV
      thisindex = Serial.parseInt();
      thishue = Serial.parseInt();
      thissat = Serial.parseInt();
      //thisVAL = Serial.parseInt();
      if (ledMode != 999) {
        ledMode = 999;
        one_color_all(0,0,0);}
      strip.setPixelColor(thisindex, CHSV(thishue, thissat, 255));
      break;
    case 100:      //---"d" - SET DELAY VAR
      thisarg = Serial.parseInt();
      thisdelay = thisarg;
      break;
    case 115:      //---"s" - SET STEP VAR
      thisarg = Serial.parseInt();
      thisstep = thisarg;
      break;
    case 104:      //---"h" - SET HUE VAR
      thisarg = Serial.parseInt();
      thishue = thisarg;
      break;
    case 116:      //---"t" - SET SATURATION VAR
      thisarg = Serial.parseInt();
      thissat = thisarg;
      break;
    case 98:      //---"b" - SET MAX BRIGHTNESS
      max_bright = Serial.parseInt();
      strip.setBrightness(max_bright);
      break;
    case 109:      //---"m" - SET MODE
      thisarg = Serial.parseInt();
      change_mode(thisarg);
      break;
    case 99:      //---"c" - CLEAR STRIP
      one_color_all(0,0,0);
      break;
    case 97:      //---"a" - SET ALL TO ONE COLOR BY HSV 0-255
      thisarg = Serial.parseInt();
      one_color_allHSV(thisarg);
      break;
    case 122:      //---"z" - COMMAND TO 'SHOW' LEDS
      strip.show();
      break;
    case 81:      //---"Q" - COMMAND RETURN VERSION NUMBER
      Serial.print(VERSION_NUMBER);
      break;
    }
  }
}
