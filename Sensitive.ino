#include <Arduino.h>
#include <Arduboy2.h>
//#include <ATMlib.h>
#include "song.h"
#include "gfx.h"

unsigned char * screenBuffer;
byte px, py, pd, ps = 0;
int frameNumber = 0;
byte curLev[16 * 8];
long int myDelay;
unsigned char numTiles = 0;
unsigned char levelNum = 0;
unsigned char maxLevels = 4;
long int tempTime;
int score = 0;
int lives = 0;
int hiscore = 0;
int gameMode = 0;
char satTime = 0;
byte trkNum = 0;
byte tileAnim=0;
byte menuItem=0;
byte ok2render=0;

// 0: 4-level in 2 frames using contrast (slight noise at border between 01 and 10)
// 1: 4-level in 3 frames (slight flicker due to lower refresh rate)
// 2: 3-level in 2 frames (no visual artifacts but 3 levels instead of 4)
static uint8_t grayscale_option = 2;

// explosion
byte expX[10], expY[10], expF[10], expU[10];
byte explode = 0;

#define _SFX 0
#define _MUSIC 1
byte settings[3]; // settings


// Make an instance of arduboy used for many functions
Arduboy2 arduboy;
//ATMsynth ATM;


#define ROW_DRIVE_WAIT_US 150
static constexpr uint8_t FBW = 128;
static constexpr uint8_t FBH = 64;
int FULLBUFF = FBH * FBW / 8;
static constexpr uint8_t FBP = 8;
// good range seems to be about 387 to ... anything!
static uint16_t timer_counter = 415;

static void send_cmds(uint8_t const* d, uint8_t n)
{
    Arduboy2::LCDCommandMode();
    while(n-- != 0)
        Arduboy2::SPItransfer(*d++);
    Arduboy2::LCDDataMode();
}

static void send_cmds_prog(uint8_t const* d, uint8_t n)
{
    Arduboy2::LCDCommandMode();
    while(n-- != 0)
        Arduboy2::SPItransfer(pgm_read_byte(d++));
    Arduboy2::LCDDataMode();
}



// Button updates
#define HELD 0
#define NEW 1
#define RELEASE 2
byte CompletePad, ExPad, TempPad;
bool _A[3], _B[3], _Up[3], _Down[3], _Left[3], _Right[3];

void UPDATEPAD(int pad, int var) {
  _B[pad] = (var & B_BUTTON) >> 2; // 4
  _A[pad] = (var & A_BUTTON) >> 3; // 8
  _Down[pad] = (var & DOWN_BUTTON) >> 4; // 16
  _Left[pad] = (var & LEFT_BUTTON) >> 5; // 32
  _Right[pad] = (var & RIGHT_BUTTON) >> 6; // 64
  _Up[pad] = (var & UP_BUTTON) >> 7; // 128
}
void UpdatePad(int joy_code)
{
  ExPad = CompletePad;
  CompletePad = joy_code;
  UPDATEPAD(HELD, CompletePad); // held
  UPDATEPAD(RELEASE, (ExPad & (~CompletePad))); // released
  UPDATEPAD(NEW, (CompletePad & (~ExPad))); // newpress
}

void drawTile(int16_t x, int16_t y, const uint8_t *bitmap) {
  for (byte t = 0; t < 8; t++) {
    screenBuffer[(x * 8) + (y * 128) + t] = pgm_read_byte(bitmap + t);
  }
}
void drawTileOffset(int16_t x, int16_t y, const uint8_t *bitmap) {
  for (byte t = 0; t < 8; t++) {
    if (y > 0) {
      byte temp = screenBuffer[(x * 8) + ((y - 1) * 128) + t];
      byte myByte = (pgm_read_byte(bitmap + t) & 0x0F) << 4;
      int offset = (x * 8) + ((y - 1) * 128) + t;
      //screenBuffer[(x * 8) + ((y - 1) * 128) + t] |= myByte;
      screenBuffer[offset] = (screenBuffer[offset]&0x0f) | myByte;
    }
    if (y < 8) {
      byte temp = screenBuffer[(x * 8) + (y * 128) + t];
      byte myByte = (pgm_read_byte(bitmap + t) & 0xF0) >> 4;
      int offset = (x * 8) + (y * 128) + t;
      //screenBuffer[(x * 8) + (y * 128) + t] |= myByte;
      screenBuffer[offset] = (screenBuffer[offset]&0xf0) | myByte;
    }
  }
}
// print text
void print(char x, char y, const char* text, bool color = 1) {
  for (char t = 0; t < strlen(text); t++) {
    char character = text[t] - 32;
    for (byte t1 = 0; t1 < 4; t1++) {
      if(x+t1>=0 && x+t1<=127){screenBuffer[x + (y * 128) + t1] = (pgm_read_byte(myFont + ((character * 3) + t1)) << 2) & 255;}
    }
    if(x+3>=0 && x+3<=127){screenBuffer[x + (y * 128) + 3] = 0;}
    x += 4;
  }
}

void titleprint(char x, char y, const char* text, bool color = 1) {
  for (char t = 0; t < strlen(text); t++) {
    char character = text[t] - 32;
    for (byte t1 = 0; t1 < 4; t1++) {
      if(x+t1>=0 && x+t1<=127){screenBuffer[x + (y * 128) + t1] = pgm_read_byte(myFont + ((character * 3) + t1));}
    }
    if(x+3>=0 && x+3<=127){screenBuffer[x + (y * 128) + 3] = 0;}
    x += 4;
  }
}

// change brightness?
void setContrast(uint8_t contrast)
{
//  arduboy.LCDCommandMode();
//  SPI.transfer(0x81); // contrast command
//  SPI.transfer(contrast);
//  arduboy.LCDDataMode();
}

void fadeOut() {
/*  // set screen to funky settings to allow fade
  arduboy.LCDCommandMode();
  SPI.transfer(0xD9); // charge time command
  SPI.transfer(0x00); // 0 - 16
  //SPI.transfer(0x81); // contrast command
  //SPI.transfer(127);
  SPI.transfer(0xdb); // VCOM deselect level mode
  SPI.transfer(0x00); // set vcomh = 0.83 * VCC
  SPI.transfer(0xa4); // set entire display on/off
  SPI.transfer(0xa6); // set normal display
  SPI.transfer(0xaf); // set display on
  arduboy.LCDDataMode();

  for (int t = 127; t > 0; t -= 2) {
    setContrast(t);
    frameNumber++;
    switch (gameMode) {
      case 10:
        renderLevel();
        break;
    }

  if(TVOUT){
    Serial.write(screenBuffer, 128 * 8);
  }
    
    arduboy.display();
  }
  setContrast(0);
*/
}
void fadeIn() {
  for (int t = 0; t < 127; t += 2) {
    setContrast(t);
    frameNumber++;
    switch (gameMode) {
      case 10:
//        renderLevel();
        break;
    }
    arduboy.display();
  }
  // restore screen settings to default
 /*
  arduboy.LCDCommandMode();

  SPI.transfer(0xD9); // charge time command
  SPI.transfer(0xF1); // 0 - 16
  //SPI.transfer(0x81); // contrast command
  //SPI.transfer(0xCF);
  SPI.transfer(0xdb); // VCOM deselect level mode
  SPI.transfer(0x40); // set vcomh = 0.83 * VCC
  SPI.transfer(0xa4); // set entire display on/off
  SPI.transfer(0xa6); // set normal display
  SPI.transfer(0xaf); // set display on
  arduboy.LCDDataMode();
*/
}


void paint(uint8_t image[], bool clear)
{
   uint16_t count;
  
   asm volatile (
       "   ldi   %A[count], %[len_lsb]               \n\t" //for (len = WIDTH * HEIGHT / 8)
       "   ldi   %B[count], %[len_msb]               \n\t"
       "1: ld    __tmp_reg__, %a[ptr]      ;2        \n\t" //tmp = *(image)
       "   out   %[spdr], __tmp_reg__      ;1        \n\t" //SPDR = tmp
       "   cpse  %[clear], __zero_reg__    ;1/2      \n\t" //if (clear) tmp = 0;
       "   mov   __tmp_reg__, __zero_reg__ ;1        \n\t"
       "2: sbiw  %A[count], 1              ;2        \n\t" //len --
       "   sbrc  %A[count], 0              ;1/2      \n\t" //loop twice for cheap delay
       "   rjmp  2b                        ;2        \n\t"
       "   st    %a[ptr]+, __tmp_reg__     ;2        \n\t" //*(image++) = tmp
       "   brne  1b                        ;1/2 :18  \n\t" //len > 0
       "   in    __tmp_reg__, %[spsr]                \n\t" //read SPSR to clear SPIF
       : [ptr]     "+&e" (image),
         [count]   "=&w" (count)
       : [spdr]    "I"   (_SFR_IO_ADDR(SPDR)),
         [spsr]    "I"   (_SFR_IO_ADDR(SPSR)),
         [len_msb] "M"   ((FBP * FBW * 2) >> 8),   // 8: pixels per byte
         [len_lsb] "M"   ((FBP * FBW * 2) & 0xFF), // 2: for delay loop multiplier
         [clear]   "r"   (clear)
    );
}


void loadLevel(int number) {
  // wait for no dpad input
  while(_Up[HELD]||_Down[HELD]||_Left[HELD]||_Right[HELD]){
    UpdatePad(arduboy.buttonsState());
  }
  
  numTiles = 0;
  satTime = 0;
  for (int t = 63; t > 0; t--) {
    int s = pgm_read_byte(levels + t + (66 * number));
    curLev[t * 2] = pgm_read_byte(lookUpTile + (s >> 4));
    curLev[(t * 2) + 1] = pgm_read_byte(lookUpTile + (s & 15));
    s = t * 2;
    switch (curLev[s]) {
      case 1:
        numTiles++;
        break;
      case 2:
        numTiles += 2;
        break;
      case 4:
        curLev[s + 1] = 5;
        curLev[s + 16] = 6;
        curLev[s + 17] = 7;
        break;
      case 8:
        curLev[s + 1] = 9;
        break;
      case 10:
        curLev[s + 1] = 11;
        break;
      case 12:
        curLev[s + 16] = 13;
        break;
    }

    s = (t * 2) + 1;
    switch (curLev[s]) {
      case 1:
        numTiles++;
        break;
      case 2:
        numTiles += 2;
        break;
      case 4:
        curLev[s + 1] = 5;
        curLev[s + 16] = 6;
        curLev[s + 17] = 7;
        break;
      case 8:
        curLev[s + 1] = 9;
        break;
      case 10:
        curLev[s + 1] = 11;
        break;
      case 12:
        curLev[s + 16] = 13;
        break;
    }

  }
  int t = (number + 1) * 66;
  px = pgm_read_byte(levels + (t - 2)) * 8;
  py = pgm_read_byte(levels + (t - 1)) * 8;
    ps = 0; pd = 0;
    satTime = 0;
}

void checkTile(int x, int y) {
  int t = curLev[x + 16 * y];
  switch (t) {
    case 1:
      curLev[x + 16 * y] = 0;
      numTiles--;
      for (int t = 0; t < 8; t++) {
        if (expU[t] == 0) {
          expX[t] = px / 8;
          expY[t] = py / 8;
          expF[t] = 0;
          expU[t] = 1;
          score++;
          break;
        }
      }
      break;
    case 2:
      curLev[x + 16 * y] = 1;
      numTiles--;
      score++;
      break;
  }
  if(score > 9999) {
    score = 9999;
  }
  if(score > hiscore) {
    hiscore = score;
  }
}

void movePlayer() {
  if (ps == 0) { // not moving
    // sneeky exit check
    if (curLev[(px / 8) + 16 * (py / 8)] == 10 || curLev[(px / 8) + 16 * (py / 8)] == 11) {
      if (numTiles == 0) {
        levelNum++;
        if (levelNum >= maxLevels) {
          levelNum = 0;
        }
        fadeOut();
        loadLevel(levelNum);
        fadeIn();
      }
    }
    // sneeky water check
    int p = curLev[(px / 8) + 16 * (py / 8)];
    if (p == 0 || (p>=24 && p<=32)) {
      lives--;
      if (lives >= 0) {
        fadeOut();
        loadLevel(levelNum);
        fadeIn();
      } else {
        setContrast(127);
/*
        arduboy.LCDCommandMode();
        SPI.transfer(0xD9);
        SPI.transfer(0x16);
        SPI.transfer(0xa4); // set entire display on/off
        SPI.transfer(0xa6); // set normal display
        SPI.transfer(0xaf); // set display on
        arduboy.LCDDataMode();  
*/
        gameMode = 5;
      }
    }


    pd = 0;
    if (!_B[HELD]) {
      if (_Up[HELD]) {
        if (curLev[(px / 8) + 16 * (py / 8)] != 14 && curLev[(px / 8) + 16 * ((py-8) / 8)] != 14) {
          pd = 1;
          checkTile(px / 8, py / 8);
        }
      }
      if (_Down[HELD]) {
        if (curLev[(px / 8) + 16 * (py / 8)] != 14 && curLev[(px / 8) + 16 * (py / 8)] != 14) {
          pd = 2;
          checkTile(px / 8, py / 8);
        }
      }
      if (_Left[HELD]) {
        if (curLev[(px / 8) + 16 * (py / 8)] != 15 && curLev[((px-8) / 8) + 16 * (py / 8)] != 15) {
          pd = 3;
          checkTile(px / 8, py / 8);
        }
      }
      if (_Right[HELD]) {
        if (curLev[(px / 8) + 16 * (py / 8)] != 15 && curLev[((px+8) / 8) + 16 * (py / 8)] != 15) {
          pd = 4;
          checkTile(px / 8, py / 8);
        }
      }
      if (_A[NEW]) {
        if (curLev[(px / 8) + 16 * (py / 8)] == 16) { // teleport
          int t = (px / 8) + 16 * (py / 8) + 1;
          while (curLev[t] != 16) {
            t++;
            if (t == 128) {
              t = 0;
            }
          }
          px = (t % 16) * 8;
          py = (t / 16) * 8;
        }

      }
    }
  }

  switch (pd) {
    case 0:
      break;
    case 1:
      py--;
      ps++;
      break;
    case 2:
      py++;
      ps++;
      break;
    case 3:
      px--;
      ps++;
      break;
    case 4:
      px++;
      ps++;
      break;
  }
  if (ps == 8) {
    ps = 0; pd = 0;
    satTime = 0;
  }

}

void renderLevel(int fn) {

  if(gameMode==10){
    // we clear our screen to black
    //arduboy.clear();
    for (int t = 0; t < 8; t++) {
      if (expU[t] == 1) {
        int tl = expX[t] + 16 * expY[t];
        if (curLev[tl] == 0 || curLev[tl] >= 24) {
          curLev[tl] = expF[t] + 24;
        }
        if (fn % 6 == 0) {
          expF[t]++;
          if (expF[t] == 8) {
            expU[t] = 0;
            if (curLev[tl] >= 24) {
              curLev[tl] = 0;
            }
          }
        }
      }
    }
  
    // animate tiles
    if(fn%6==0){
      tileAnim++;
      if(tileAnim==4){tileAnim=0;}
    }
  
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 16; x++) {
        char tn = (y << 4) + x;
        if (fn & 1) {
          if(curLev[tn]==2){
            drawTileOffset(x, y, animTILE + (tileAnim * 8));
          }else{
            drawTileOffset(x, y, tileGFX + (curLev[tn] * 8));
          }
        } else {
          if(curLev[tn]==2){
            drawTileOffset(x, y, animTILE + (tileAnim * 8) + 32);
          }else{
            drawTileOffset(x, y, tileGFXneg + (curLev[tn] * 8));
          }
        }
      }
    }
    // player sprite
    arduboy.drawBitmap(px, py - 4, tileGFX + (22 * 8), 8, 8, 0);
    arduboy.drawBitmap(px, py - 4, tileGFX + (20 * 8), 8, 8, 1);
  
    // the 'HUD'
    memset(&screenBuffer[7 * 128], 0, 128);
    char text[] = "                               ";
    int temp=levelNum+1;
    if(temp<0){temp=0;}
    sprintf(text, "S:%04d  LVS:%02d  LVL:%02d  Hi:%04d", score, lives, temp, hiscore);
    print(0, 7 , text);
//    arduboy.display();
  }//gameMode==10
}

int myInt=0;
byte scroller=0;
int temp=0;
void titleScreen() {
  
  //arduboy.clear();

  for(char y=0; y<3; y++){
    for(char x=0; x<14; x++){
      //int mt = pgm_read_byte(titleMap+(x+14*y)+((frameNumber%3)*42));
      int mt = pgm_read_byte(titleMap+temp);
      if(mt!=0){
        drawTile(x+1,y+3, titleTiles + (mt*8));
      }
      if(temp++>=125){temp=0;}
    }
  }

  char text1[34];
  memcpy_P(text1, &text[myInt],33);
  text1[32]=0;
  titleprint(-scroller, 6 , text1);
  if(frameNumber%6==0){
    scroller++;
    if(scroller==4){
      scroller=0;
      myInt++;
      if(myInt==strlen(text)){myInt=0;}
    }
  }
  // sides of scroller
  drawTile(0,6, titleTiles + (101*8));
  drawTile(31,5, titleTiles + (101*8));

  if (_A[NEW]) {
    lives = 4;
    score = 0;
    levelNum = 0;
    fadeOut();
    loadLevel(levelNum);
    gameMode = 10;
    scroller=0;
    fadeIn();
  }
  
  arduboy.display();
}

void playLevel(int fn) {
  if (fn % 4 == 0) {
    movePlayer();
  }

  if (satTime > 16 && (curLev[px / 8 + 16 * py / 8] == 1 || curLev[px / 8 + 16 * py / 8] == 2)) {
    arduboy.setRGBled(4, 0, 0);
  } else {
    arduboy.setRGBled(0, 0, 0);
  }
  if (numTiles == 0) {
    arduboy.setRGBled(0, 4, 0);
  }
  if (pd == 0) {
    if (fn % 4 == 0) {
      satTime++;
      if (satTime == 32) {
        checkTile(px / 8, py / 8);
        satTime = 0;
      }
    }
  }


//  renderLevel();
  
}


void setup() {
  arduboy.begin();
  screenBuffer = arduboy.getBuffer();
  arduboy.setFrameRate(125);
  //Serial.begin(9600);
  arduboy.audio.on();
  // Initializes ATMSynth and samplerate
  // Begin playback of song.
//  ATM.play(music);

 
  gameMode = 5; // titlescreen
  maxLevels = sizeof(levels) / 66;
/*
    arduboy.LCDCommandMode();
    SPI.transfer(0xD9);
    SPI.transfer(0x16);
    SPI.transfer(0xa4); // set entire display on/off
    SPI.transfer(0xa6); // set normal display
    SPI.transfer(0xaf); // set display on
    arduboy.LCDDataMode();  
*/

    static uint8_t const SETUP_CMDS[] PROGMEM =
    {
        0x21, (128 - FBW) / 2, 127 - (128 - FBW) / 2,
        0x22, 0, FBP - 1,
        0x7E,
        0xD3, 1,
        0xD9, 0x31, // boost phase 2
        0xA8, 0,
    };
    send_cmds_prog(SETUP_CMDS, sizeof(SETUP_CMDS));

    // disable timer0 overflow ISR (cannot use micros/millis/delay)
    bitWrite(TIMSK0, TOIE0, 0);
    
    TCCR3A = 0;
    TCCR3B = _BV(WGM32) | _BV(CS32); // CTC mode, prescaler /256
    OCR3A = timer_counter;
    bitWrite(TIMSK3, OCIE3A, 1);

}

void loop() {

    if (WDTCSR & _BV(WDE))
    {
        // disable ints and set magic key
        cli();
        *(volatile uint8_t*)0x800 = 0x77;
        *(volatile uint8_t*)0x801 = 0x77;
        for(;;);
    }

    // timer3 will still wake CPU
    Arduboy2::idle();
//  if(TIMSK4==0){ATM.play(music);} // loop music.
}

ISR(TIMER3_COMPA_vect)
{
      static uint8_t n = 0;
        static uint8_t const PRE_CMDS[] PROGMEM = { 0xA8, 63, };
        static uint8_t const POST_CMDS[] PROGMEM = { 0x81, 0xf0, 0xA8, 0x0, };

      paint(screenBuffer, false);
        send_cmds_prog(PRE_CMDS, sizeof(PRE_CMDS));
        delayMicroseconds(ROW_DRIVE_WAIT_US);
        send_cmds_prog(POST_CMDS, sizeof(POST_CMDS));

      if((n & 15) == 0){
        UpdatePad(arduboy.buttonsState());
      }

      switch (gameMode) {
        case 5:
          titleScreen();
          break;
        case 10:
          playLevel(n);
          break;
      }

      renderLevel(n++);
}
