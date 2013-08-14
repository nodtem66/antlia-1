#include<SPI.h>

/* LCD pin for SPI
 * DIN --> Pin 11
 * CLK --> Pin 13
 * PIN_CE --> Pin 10 
 */
#define PIN_CE 10
#define PIN_DC 12
#define PIN_RESET 9
#define DATA_OUTPUT HIGH
#define COMM_OUTPUT LOW
#define LCD_WIDTH 84
#define LCD_HEIGHT 6 // (48bit / 8bit)

#define ROWS 4
#define COLS 3
#define DEBOUNCE_TIME 500

//State of instrument
typedef enum _pump_state {PREPARE, SETTING, RUNNING, PAUSE, PUMP_ERROR} pump_state;
//Position of menu
typedef enum _menu_position {TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT} menu_position;
//State of Keypad
typedef enum keystate {NOKEY, KEYDOWN, KEYHOLD} keystate_t;

/*======= Function helpers =========*/
#define GET_INDEX_ARRAY(x,y) (x * COLS + y + 1)
#define Contrast(x) (0x80 | ((x > 0x7F)? 0x7F : (x)))
#define Pixel(x,y) lcd_buffer[LCD_WIDTH * (y) + (x)]
#define PixelGroup(a,b,row,x) for(i=(a);i<=(b);i++) Pixel(i,(row))= (x);
#define PixelR(x,y) Pixel(positionX+(x), positionY+(y))
#define PixelGroupR(a,b,row,x) PixelGroup(positionX+(a),positionY+(b),row,x)

/*======= Global Variables =========*/
// pin 8 is green
// pin 2 is white
const byte rowPins[ROWS] = {8,7,6,5};
const byte colPins[COLS] = {4,3,2};
byte lcd_buffer[LCD_WIDTH * LCD_HEIGHT] = {0x00};

/*======= Local Variables ==========*/
keystate_t STATE_KEY = NOKEY;
byte INDEX_KEY = 0;
long timestamp = 0;
int buffer_size = LCD_WIDTH * LCD_HEIGHT, i;
byte positionX=0, positionY=0, positionSelection = -1, pumperState = PREPARE;
boolean timeUnit = 0, volumnUnit = 0;

/*======= Predefined Function ======*/
//Get Keypad
byte getKeyPadState() {
  if (timestamp++ > DEBOUNCE_TIME) {  
    timestamp = 0;
    byte i,j;
    for(i=0; i<ROWS; i++) pinMode(rowPins[i], INPUT_PULLUP);
    for(i=0; i<COLS; i++) {
      pinMode(colPins[i], OUTPUT);
      digitalWrite(colPins[i], LOW);
      
      for(j=0; j<ROWS; j++) {
        if (digitalRead(rowPins[j]) == LOW) {
          if (INDEX_KEY == GET_INDEX_ARRAY(j,i)) {
            STATE_KEY = KEYHOLD;
          } else {
            INDEX_KEY = GET_INDEX_ARRAY(j,i);
            STATE_KEY = KEYDOWN;
          }
          break;
        } else if (INDEX_KEY == GET_INDEX_ARRAY(j,i)) {
          //Clear old key
          STATE_KEY = NOKEY;
          INDEX_KEY = 0;
        }
      }
      pinMode(colPins[i], INPUT);
      digitalWrite(colPins[i], HIGH);
      if (j < ROWS) break;
    }
    if (i == COLS) STATE_KEY = NOKEY;
    return STATE_KEY;
  }
  return NOKEY;
}
void changeTimeUnit(boolean isMinute) {
  if (isMinute) {
    Pixel(68, 0) = 0xF0;
    Pixel(69, 0) = 0x10;
    Pixel(70, 0) = 0xB0;
    Pixel(71, 0) = 0x70;
    Pixel(72, 0) = 0xB0;
    Pixel(73, 0) = 0x10;
    Pixel(74, 0) = 0xF0;
    Pixel(75, 0) = 0x70;
    Pixel(76, 0) = 0x50;
    Pixel(77, 0) = 0xF0;
    Pixel(78, 0) = 0xF0;
    Pixel(79, 0) = 0x70;
    Pixel(80, 0) = 0x70;
    Pixel(81, 0) = 0x70;
    Pixel(82, 0) = 0xF0;
    Pixel(83, 0) = 0xF0;
    Pixel(68, 1) = 0x07;
    Pixel(69, 1) = 0x04;
    Pixel(70, 1) = 0x07;
    Pixel(71, 1) = 0x06;
    Pixel(72, 1) = 0x07;
    Pixel(73, 1) = 0x04;
    Pixel(74, 1) = 0x07;
    Pixel(75, 1) = 0x05;
    Pixel(76, 1) = 0x04;
    Pixel(77, 1) = 0x05;
    Pixel(78, 1) = 0x07;
    Pixel(79, 1) = 0x04;
    Pixel(80, 1) = 0x07;
    Pixel(81, 1) = 0x07;
    Pixel(82, 1) = 0x04;
    Pixel(83, 1) = 0x07;
  } else {
    PixelGroup(68,70,0, 0xF0);
    Pixel(71,0) = 0x10;
    PixelGroup(72,74,0, 0x70);
    Pixel(75,0) = 0x10;
    Pixel(76,0) = 0xF0;
    Pixel(77,0) = 0xF0;
    Pixel(78,0) = 0x30;
    Pixel(79,0) = 0x70;
    Pixel(80,0) = 0xB0;
    Pixel(81,0) = 0xB0;
    Pixel(82,0) = 0xF0;
    Pixel(83,0) = 0xF0;
    PixelGroup(68,70,1, 0x07);
    Pixel(71,1) = 0x04;
    PixelGroup(72,74,1, 0x07);
    Pixel(75,1) = 0x04;
    PixelGroup(76,77,1, 0x07);
    Pixel(78,1) = 0x04;
    PixelGroup(79,83,1,0x07);
  }
  timeUnit = isMinute;
  if (positionSelection == TOP_RIGHT) printUnderLine(TOP_RIGHT);
  
}

void changeVolumnUnit(boolean isMilliliter) {
  if (isMilliliter) {
    Pixel(68,2) = 0xFC;
    Pixel(69,2) = 0x04;
    Pixel(70,2) = 0xEC;
    Pixel(71,2) = 0x9C;
    Pixel(72,2) = 0xEC;
    Pixel(73,2) = 0x04;
    Pixel(74,2) = 0xFC;
    Pixel(75,2) = 0xFC;
    Pixel(76,2) = 0x84;
    PixelGroup(77,79,2, 0x7C);
    PixelGroup(80,83,2, 0xFC);
  } else {
    PixelGroup(68,72,2, 0xFC);
    Pixel(73,2) = 0x84;
    PixelGroup(74,77,2, 0x7C);
    PixelGroup(78,83,2, 0xFC);
    PixelGroup(68,83,3, 0x01);
  }
  volumnUnit = isMilliliter;
  if (positionSelection == BOTTOM_RIGHT) printUnderLine(BOTTOM_RIGHT);
}

//For Setting LCD config
void initialScreen() {
  PixelGroup(0,LCD_WIDTH-1,5, 0xFE);
  Pixel(3, 0) = 0x10;
  Pixel(4, 0) = 0x10;
  Pixel(5, 0) = 0xF0;
  Pixel(6, 0) = 0x10;
  Pixel(7, 0) = 0x10;
  Pixel(8, 0) = 0x40;
  Pixel(9, 0) = 0xD0;
  Pixel(10, 0) = 0x00;
  Pixel(11, 0) = 0x00;
  Pixel(12, 0) = 0xC0;
  Pixel(13, 0) = 0x40;
  Pixel(14, 0) = 0x80;
  Pixel(15, 0) = 0x40;
  Pixel(16, 0) = 0xC0;
  Pixel(17, 0) = 0x00;
  Pixel(18, 0) = 0x80;
  Pixel(19, 0) = 0x40;
  Pixel(20, 0) = 0x40;
  Pixel(21, 0) = 0x40;
  Pixel(22, 0) = 0x80;
  Pixel(5, 1) = 0x07;
  Pixel(8, 1) = 0x04;
  Pixel(9, 1) = 0x07;
  Pixel(10, 1) = 0x04;
  Pixel(12, 1) = 0x07;
  Pixel(14, 1) = 0x03;
  Pixel(16, 1) = 0x07;
  Pixel(18, 1) = 0x03;
  Pixel(19, 1) = 0x05;
  Pixel(20, 1) = 0x05;
  Pixel(21, 1) = 0x05;
  Pixel(22, 1) = 0x05;
  changeTimeUnit(0);
  
  Pixel(4, 2) = 0xF8;
  Pixel(8, 2) = 0xF8;
  Pixel(11, 2) = 0xF0;
  Pixel(12, 2) = 0x08;
  Pixel(13, 2) = 0x08;
  Pixel(14, 2) = 0x08;
  Pixel(15, 2) = 0xF0;
  Pixel(18, 2) = 0xF8;

  Pixel(5, 3) = 0x01;
  Pixel(6, 3) = 0x02;
  Pixel(7, 3) = 0x01;
  Pixel(11, 3) = 0x01;
  Pixel(12, 3) = 0x02;
  Pixel(13, 3) = 0x02;
  Pixel(14, 3) = 0x02;
  Pixel(15, 3) = 0x01;
  Pixel(18, 3) = 0x03;
  PixelGroup(19,22,3, 0x02);
  
  changeVolumnUnit(0);
}

void printState(byte mode) {
  PixelGroup(2,45, 4, 0x00);
  switch (mode) {
    case RUNNING:
      //R
      Pixel(4, 4) = 0x7F;
      Pixel(5, 4) = 0x09;
      Pixel(6, 4) = 0x19;
      Pixel(7, 4) = 0x29;
      Pixel(8, 4) = 0x46;
      //U
      Pixel(10,4) = 0x3F;
      PixelGroup(11,13,4, 0x40);
      Pixel(14,4) = 0x3F;
      //N
      Pixel(16,4) = 0x7F;
      Pixel(17,4) = 0x04;
      Pixel(18,4) = 0x08;
      Pixel(19,4) = 0x10;
      Pixel(20,4) = 0x7F;
      //N
      Pixel(22,4) = 0x7F;
      Pixel(23,4) = 0x04;
      Pixel(24,4) = 0x08;
      Pixel(25,4) = 0x10;
      Pixel(26,4) = 0x7F;
      //I
      Pixel(28,4) = 0x41;
      Pixel(29,4) = 0x7F;
      Pixel(30,4) = 0x41;
      //N
      Pixel(32,4) = 0x7F;
      Pixel(33,4) = 0x04;
      Pixel(34,4) = 0x08;
      Pixel(35,4) = 0x10;
      Pixel(36,4) = 0x7F;
      //G
      Pixel(38,4) = 0x7E;
      Pixel(39,4) = 0x41;
      Pixel(40,4) = 0x41;
      Pixel(41,4) = 0x51;
      Pixel(42,4) = 0x71;
      break;
    case PREPARE:
      //P
      Pixel(4, 4) = 0x7F;
      PixelGroup(5,7,4, 0x09);
      Pixel(8, 4) = 0x06;
      //R
      Pixel(10, 4) = 0x7F;
      Pixel(11, 4) = 0x09;
      Pixel(12, 4) = 0x19;
      Pixel(13, 4) = 0x29;
      Pixel(14, 4) = 0x46;
      //E
      Pixel(16, 4) = 0x7F;
      PixelGroup(17,19,4, 0x49);
      Pixel(20, 4) = 0x41;
      //P
      Pixel(22, 4) = 0x7F;
      PixelGroup(23,25,4, 0x09);
      Pixel(26, 4) = 0x06;
      //A
      Pixel(28,4) = 0x7C;
      Pixel(29,4) = 0x12;
      Pixel(30,4) = 0x11;
      Pixel(31,4) = 0x12;
      Pixel(32,4) = 0x7C;
      //R
      Pixel(34, 4) = 0x7F;
      Pixel(35, 4) = 0x09;
      Pixel(36, 4) = 0x19;
      Pixel(37, 4) = 0x29;
      Pixel(38, 4) = 0x46;
      //E
      Pixel(40, 4) = 0x7F;
      PixelGroup(41,43,4, 0x49);
      Pixel(44, 4) = 0x41;
      break;
    case SETTING:
      //S
      Pixel(4, 4) = 0x26;
      PixelGroup(5,7,4, 0x49);
      Pixel(8, 4) = 0x32;
      //E
      Pixel(10, 4) = 0x7F;
      PixelGroup(11,13,4, 0x49);
      Pixel(14, 4) = 0x41;
      //T
      Pixel(16, 4) = 0x01;
      Pixel(17, 4) = 0x01;
      Pixel(18, 4) = 0x7F;
      Pixel(19, 4) = 0x01;
      Pixel(20, 4) = 0x01;
      //T
      Pixel(22, 4) = 0x01;
      Pixel(23, 4) = 0x01;
      Pixel(24, 4) = 0x7F;
      Pixel(25, 4) = 0x01;
      Pixel(26, 4) = 0x01;
      //I
      Pixel(28,4) = 0x41;
      Pixel(29,4) = 0x7F;
      Pixel(30,4) = 0x41;
      //N
      Pixel(32,4) = 0x7F;
      Pixel(33,4) = 0x04;
      Pixel(34,4) = 0x08;
      Pixel(35,4) = 0x10;
      Pixel(36,4) = 0x7F;
      //G
      Pixel(38,4) = 0x7E;
      Pixel(39,4) = 0x41;
      Pixel(40,4) = 0x41;
      Pixel(41,4) = 0x51;
      Pixel(42,4) = 0x71;
      break;
    case PAUSE:
      //P
      Pixel(4, 4) = 0x7F;
      PixelGroup(5,7,4, 0x09);
      Pixel(8, 4) = 0x06;
      //A
      Pixel(10,4) = 0x7C;
      Pixel(11,4) = 0x12;
      Pixel(12,4) = 0x11;
      Pixel(13,4) = 0x12;
      Pixel(14,4) = 0x7C;
      //U
      Pixel(16,4) = 0x3F;
      PixelGroup(17,19,4, 0x40);
      Pixel(20,4) = 0x3F;
      //S
      Pixel(22, 4) = 0x26;
      PixelGroup(23,25,4, 0x49);
      Pixel(26, 4) = 0x32;
      //E
      Pixel(28, 4) = 0x7F;
      PixelGroup(29,31,4, 0x49);
      Pixel(32, 4) = 0x41;
      //Circle
      Pixel(36, 4) = 0x1C;
      Pixel(37, 4) = 0x3E;
      Pixel(38, 4) = 0x7F;
      Pixel(39, 4) = 0x3E;
      Pixel(40, 4) = 0x1C;
      break;
    case PUMP_ERROR:
      //E
      Pixel(4, 4) = 0x7F;
      PixelGroup(5,7,4, 0x49);
      Pixel(8, 4) = 0x41;
      Pixel(9, 4) = 0x00;
      //R
      Pixel(10, 4) = 0x7F;
      Pixel(11, 4) = 0x09;
      Pixel(12, 4) = 0x19;
      Pixel(13, 4) = 0x29;
      Pixel(14, 4) = 0x46;
      Pixel(15, 4) = 0x00;
      //R
      Pixel(16, 4) = 0x7F;
      Pixel(17, 4) = 0x09;
      Pixel(18, 4) = 0x19;
      Pixel(19, 4) = 0x29;
      Pixel(20, 4) = 0x46;
      Pixel(21, 4) = 0x00;
      //O
      Pixel(22, 4) = 0x3E;
      Pixel(23, 4) = 0x41;
      Pixel(24, 4) = 0x41;
      Pixel(25, 4) = 0x41;
      Pixel(26, 4) = 0x3E;
      Pixel(27, 4) = 0x00;
      //R
      Pixel(28, 4) = 0x7F;
      Pixel(29, 4) = 0x09;
      Pixel(30, 4) = 0x19;
      Pixel(31, 4) = 0x29;
      Pixel(32, 4) = 0x46;
      PixelGroup(33,35, 4, 0x00);
      //!
      Pixel(36, 4) = 0x5F;
      Pixel(37, 4) = 0x00;
      Pixel(38, 4) = 0x00;
      Pixel(39, 4) = 0x5F;
      break;
    default:;
  }
}

void printNumber(byte num, byte isShadowed) {
  if (positionX + 7 >= LCD_WIDTH || positionY + 1 >= LCD_HEIGHT) return; 
  if (isShadowed == 0) {
    switch (num) {
      case 0:
        PixelR(0, 0) = 0xFC; PixelR(0, 1) = 0x03;
        PixelR(1, 0) = 0xFE; PixelR(1, 1) = 0x07;
        PixelR(2, 0) = 0xC7; PixelR(2, 1) = 0x0D;
        PixelR(3, 0) = 0xE3; PixelR(3, 1) = 0x0C;
        PixelR(4, 0) = 0x73; PixelR(4, 1) = 0x0C;
        PixelR(5, 0) = 0x3F; PixelR(5, 1) = 0x0E; 
        PixelR(6, 0) = 0xFE; PixelR(6, 1) = 0x07;
        PixelR(7, 0) = 0xFC; PixelR(7, 1) = 0x03;
        break;
      case 1:
        PixelR(0, 0) = 0x00; PixelR(0, 1) = 0x00;
        PixelR(1, 0) = 0x0C; PixelR(1, 1) = 0x0C;
        PixelR(2, 0) = 0x0E; PixelR(2, 1) = 0x0C;
        PixelR(3, 0) = 0xFF; PixelR(3, 1) = 0x0F;
        PixelR(4, 0) = 0xFF; PixelR(4, 1) = 0x0F;
        PixelR(5, 0) = 0x00; PixelR(5, 1) = 0x0C;
        PixelR(6, 0) = 0x00; PixelR(6, 1) = 0x0C;
        PixelR(7, 0) = 0x00; PixelR(7, 1) = 0x00;
        break;
      case 2:
        PixelR(0, 0) = 0x0C; PixelR(0, 1) = 0x0E;
        PixelR(1, 0) = 0x0E; PixelR(1, 1) = 0x0F;
        PixelR(2, 0) = 0x87; PixelR(2, 1) = 0x0F;
        PixelR(3, 0) = 0xC3; PixelR(3, 1) = 0x0D;
        PixelR(4, 0) = 0xE3; PixelR(4, 1) = 0x0C;
        PixelR(5, 0) = 0x77; PixelR(5, 1) = 0x0C; 
        PixelR(6, 0) = 0x3E; PixelR(6, 1) = 0x0C;
        PixelR(7, 0) = 0x1C; PixelR(7, 1) = 0x0C;
        break;
      case 3:
        PixelR(0, 0) = 0x0C; PixelR(0, 1) = 0x03;
        PixelR(1, 0) = 0x0E; PixelR(1, 1) = 0x07;
        PixelR(2, 0) = 0x07; PixelR(2, 1) = 0x0E;
        PixelR(3, 0) = 0x63; PixelR(3, 1) = 0x0C;
        PixelR(4, 0) = 0x63; PixelR(4, 1) = 0x0C;
        PixelR(5, 0) = 0xF7; PixelR(5, 1) = 0x0E; 
        PixelR(6, 0) = 0xFE; PixelR(6, 1) = 0x07;
        PixelR(7, 0) = 0x9C; PixelR(7, 1) = 0x03;
        break;
      case 4:
        PixelR(0, 0) = 0xF0; PixelR(0, 1) = 0x00;
        PixelR(1, 0) = 0xF8; PixelR(1, 1) = 0x00;
        PixelR(2, 0) = 0xDC; PixelR(2, 1) = 0x00;
        PixelR(3, 0) = 0xCE; PixelR(3, 1) = 0x00;
        PixelR(4, 0) = 0xCF; PixelR(4, 1) = 0x00;
        PixelR(5, 0) = 0xFF; PixelR(5, 1) = 0x0F; 
        PixelR(6, 0) = 0xFF; PixelR(6, 1) = 0x0F;
        PixelR(7, 0) = 0xC0; PixelR(7, 1) = 0x00;
        break;
      case 5:
        PixelR(0, 0) = 0x3F; PixelR(0, 1) = 0x03;
        PixelR(1, 0) = 0x3F; PixelR(1, 1) = 0x07;
        PixelR(2, 0) = 0x33; PixelR(2, 1) = 0x0E;
        PixelR(3, 0) = 0x33; PixelR(3, 1) = 0x0C;
        PixelR(4, 0) = 0x33; PixelR(4, 1) = 0x0C;
        PixelR(5, 0) = 0x73; PixelR(5, 1) = 0x0E; 
        PixelR(6, 0) = 0xE3; PixelR(6, 1) = 0x07;
        PixelR(7, 0) = 0xC3; PixelR(7, 1) = 0x03;
        break;
      case 6:
        PixelR(0, 0) = 0xFC; PixelR(0, 1) = 0x03;
        PixelR(1, 0) = 0xFE; PixelR(1, 1) = 0x07;
        PixelR(2, 0) = 0x77; PixelR(2, 1) = 0x0E;
        PixelR(3, 0) = 0x33; PixelR(3, 1) = 0x0C;
        PixelR(4, 0) = 0x33; PixelR(4, 1) = 0x0C;
        PixelR(5, 0) = 0x73; PixelR(5, 1) = 0x0E; 
        PixelR(6, 0) = 0xE6; PixelR(6, 1) = 0x07;
        PixelR(7, 0) = 0xC6; PixelR(7, 1) = 0x03;
        break;
      case 7:
        PixelR(0, 0) = 0x03; PixelR(0, 1) = 0x00;
        PixelR(1, 0) = 0x03; PixelR(1, 1) = 0x00;
        PixelR(2, 0) = 0x03; PixelR(2, 1) = 0x00;
        PixelR(3, 0) = 0x83; PixelR(3, 1) = 0x0F;
        PixelR(4, 0) = 0xE3; PixelR(4, 1) = 0x0F;
        PixelR(5, 0) = 0x7B; PixelR(5, 1) = 0x00; 
        PixelR(6, 0) = 0x1F; PixelR(6, 1) = 0x00;
        PixelR(7, 0) = 0x07; PixelR(7, 1) = 0x00;
        break;
      case 8:
        PixelR(0, 0) = 0x9C; PixelR(0, 1) = 0x03;
        PixelR(1, 0) = 0xFE; PixelR(1, 1) = 0x07;
        PixelR(2, 0) = 0xF7; PixelR(2, 1) = 0x0E;
        PixelR(3, 0) = 0x63; PixelR(3, 1) = 0x0C;
        PixelR(4, 0) = 0x63; PixelR(4, 1) = 0x0C;
        PixelR(5, 0) = 0xF7; PixelR(5, 1) = 0x0E; 
        PixelR(6, 0) = 0xFE; PixelR(6, 1) = 0x07;
        PixelR(7, 0) = 0x9C; PixelR(7, 1) = 0x03;
        break;
      case 9:
        PixelR(0, 0) = 0x3C; PixelR(0, 1) = 0x06;
        PixelR(1, 0) = 0x7E; PixelR(1, 1) = 0x0E;
        PixelR(2, 0) = 0xE7; PixelR(2, 1) = 0x0C;
        PixelR(3, 0) = 0xC3; PixelR(3, 1) = 0x0C;
        PixelR(4, 0) = 0xC3; PixelR(4, 1) = 0x0C;
        PixelR(5, 0) = 0xE7; PixelR(5, 1) = 0x0E; 
        PixelR(6, 0) = 0xFE; PixelR(6, 1) = 0x07;
        PixelR(7, 0) = 0xFC; PixelR(7, 1) = 0x03;
        break;
      default:;
    }
    positionX += 8;
  } else {
    switch (num) {
      case 0:
        break;
      case 1:
        break;
      case 2:
        break;
      case 3:
        break;
      case 4:
        break;
      case 5:
        break;
      case 6:
        break;
      case 7:
        break;
      case 8:
        break;
      case 9:
        break;
      default:;
    }
  }
}

void clearUnderLine(byte order) {
  int j;
  if (order < 0 || order > 3) return;
  switch (order) {
    case TOP_LEFT:
      for(j = 5; j < 23; j++)
      {
        bitClear(Pixel(j, 1), 4);
        bitClear(Pixel(j, 1), 5);
      }
      break;
    case TOP_RIGHT:
      for(j = 68; j < 84; j++)
      {
        bitClear(Pixel(j, 1), 4);
        bitClear(Pixel(j, 1), 5);
      }
      break;
    case BOTTOM_LEFT:
      for(j = 4; j < 23; j++)
      {
        bitClear(Pixel(j, 3), 3);
        bitClear(Pixel(j, 3), 4);
      }
      break;
    case BOTTOM_RIGHT:
      for(j = 68; j < 84; j++)
      {
        bitClear(Pixel(j, 3), 2);
        bitClear(Pixel(j, 3), 3);
      }
      break;
    default:;
  }
}

void printUnderLine(byte order) {
  if (order < 0 || order > 3) return;
  int j;
  switch (order) {
    case TOP_LEFT:
      for(j = 5; j < 23; j++)
      {
        bitSet(Pixel(j, 1), 4);
        bitSet(Pixel(j, 1), 5);
      }
      break;
    case TOP_RIGHT:
      for(j = 68; j < 84; j++)
      {
        bitSet(Pixel(j, 1), 4);
        bitSet(Pixel(j, 1), 5);
      }
      break;
    case BOTTOM_LEFT:
      for(j = 4; j < 23; j++)
      {
        bitSet(Pixel(j, 3), 3);
        bitSet(Pixel(j, 3), 4);
      }
      break;
    case BOTTOM_RIGHT:
      for(j = 68; j < 84; j++)
      {
        bitSet(Pixel(j, 3), 2);
        bitSet(Pixel(j, 3), 3);
      }
      break;
    default:;
  }
  positionSelection = order;
}
void printNextUnderLine() {
  clearUnderLine(positionSelection);
  positionSelection++;
  if (positionSelection > 3) positionSelection = 0;
  printUnderLine(positionSelection);
}
void sendToLCD() {
  digitalWrite(PIN_DC, DATA_OUTPUT);
  digitalWrite(PIN_CE, LOW);
  for (i = 0; i < buffer_size; i++) 
  {
    SPI.transfer(lcd_buffer[i]); 
  }
  digitalWrite(PIN_CE, HIGH);
}

void clearLCD(void)
{
  digitalWrite(PIN_DC, DATA_OUTPUT);
  for (i = 0; i < buffer_size; i++)
  {
    SPI.transfer(0x00);
  }
}

void setup() {
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_CE, OUTPUT);
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  //Reset LCD - prevent from crashing
  digitalWrite(PIN_RESET, LOW);
  digitalWrite(PIN_RESET, HIGH);
  digitalWrite(PIN_DC, COMM_OUTPUT);
  digitalWrite(PIN_CE, LOW);
  //LCD Extended Option
  SPI.transfer(0x21); //Extented Option config mode
  SPI.transfer(Contrast(40)); //Set Vop; Contrast Setting (0-127)
  SPI.transfer(0x04);
  SPI.transfer(0x14); //Set Bias value; 1-48 Mux  
  //LCD Basic Option
  SPI.transfer(0x20); //Basic option config mode
  SPI.transfer(0x0C); //Normal mode display
  digitalWrite(PIN_DC, DATA_OUTPUT);

  //Write example
  clearLCD();
  initialScreen();
  printState(PREPARE);
  sendToLCD();
}

void loop() {
  if (pumperState == PREPARE) {
    delay(2000);
    pumperState = SETTING;
    printState(SETTING);
    printUnderLine(TOP_LEFT);
    sendToLCD();
  } else {
    if((keystate_t)getKeyPadState() == KEYDOWN) {
      if (pumperState == SETTING) {
        if (INDEX_KEY == 10) {
          printNextUnderLine();
        } else if (INDEX_KEY == 12) {
          pumperState = RUNNING;
          printState(RUNNING);
          clearUnderLine(positionSelection);
          positionSelection = -1; 
        } else {
          if (positionX > 64) positionX = 26;
          if (positionSelection == TOP_LEFT) {
            if (positionX < 26 || positionY != 0) {
              positionX = 26;
              positionY = 0;
            }
            printNumber((INDEX_KEY==11)?0:INDEX_KEY, 0);
            positionX += 2;
          } else if (positionSelection == BOTTOM_LEFT) {
            if (positionX < 26 || positionY != 2) {
              positionX = 26;
              positionY = 2;
            }
            printNumber((INDEX_KEY==11)?0:INDEX_KEY, 0);
            positionX += 2;
          } else if (positionSelection == TOP_RIGHT) {
             changeTimeUnit(~timeUnit);
          } else if (positionSelection == BOTTOM_RIGHT) {
             changeVolumnUnit(~volumnUnit);
          } 
        }
      }
      sendToLCD();
    }
  }
  if (pumperState == RUNNING) {
  
  }
}
