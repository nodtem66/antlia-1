#define ROWS 4
#define COLS 3
#define DEBOUNCE_TIME 30
#define GET_INDEX_ARRAY(x,y) (x * COLS + y + 1)
typedef enum keystate {NOKEY, KEYDOWN, KEYHOLD} keystate_t;

// pin 8 is green
// pin 2 is white
const byte rowPins[ROWS] = {8,7,6,5};
const byte colPins[COLS] = {4,3,2};

// Local Variables
keystate_t STATE_KEY = NOKEY;
byte INDEX_KEY = 0;
long timestamp = 0;

void initKeyPad() {
  //Set Output to ColPins
  //Set Input to RowPins
  
  //pinMode(8, INPUT_PULLUP);
}

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

void setup() {
  //initKeyPad();
  //Serial.begin(9600);
  //Serial.println("start");
}

void loop() {
    if ((keystate_t)getKeyPadState() == KEYDOWN) {
      //Serial.println((int)INDEX_KEY);
    }
}
