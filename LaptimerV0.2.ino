#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#define BLACK   0x0000
#define RED     0xF800
#define GREEN   0x07E0
#define WHITE   0xFFFF
#define GREY    0x8410
#define BLUE     0x001F
#define RED      0xF800
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define SECTORS 1
#define REEDPIN 31
#define LEDPIN  6
#define LONGLAP  120000
#define MAX_SECTORS SECTORS
#define LAP 0
#define NOTIME 0x7FED
#define DEBUG
#ifdef DEBUG
#define PRINT(x)    Serial.print (x)
#define PRINTDEC(x) Serial.print (x, DEC)
#define PRINTLN(x)  Serial.println (x)
#define BEGIN(x)    Serial.begin(x)
#define DEBOUNCE 500
#else
#define PRINT(x)
#define PRINTDEC(x)
#define PRINTLN(x) 
#define BEGIN(x)
#define DEBOUNCE 50
#endif
#define CONFADDR 20
#define CONFVER  2
volatile int newlap;
volatile long reedready;
long lasttime;
int  sectors;
int  cursector;
int  lastsector;
int  curlap;
int  fastestlap;
int  state;
struct timing_struct {
    unsigned int best;    
    unsigned int last;
    int diff; //last - previous best
    } 
timing_struct;
struct timing_struct timing[MAX_SECTORS+1];

void setup() {

  tft.begin(0x9481);
    tft.setRotation(3);
    tft.fillScreen(BLACK);
    tft.setTextColor(RED, BLACK);
    tft.setTextSize(4);
    tft.setCursor(45, 130);
    tft.print("Wurstbrot Racing");
    tft.setCursor(45, 170);
    tft.print("Laptimer V0.2");
    delay(3000);
    tft.fillScreen(BLACK);
    tft.drawRect(0, 0, 480, 320, WHITE);
    tft.drawRect(1, 1, 479, 319, WHITE);
    tft.drawFastHLine(0, 116, 480, WHITE);
    tft.drawFastHLine(0, 117, 480, WHITE);
    tft.drawFastHLine(0, 229, 480, WHITE);
    tft.drawFastHLine(0, 230, 480, WHITE);
    tft.drawFastVLine(285, 0, 320, WHITE);
    tft.drawFastVLine(286, 0, 320, WHITE);
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(3);
    tft.setCursor(75, 87);
    tft.print("Last Lap");
    tft.setCursor(120, 200);
    tft.print("Gap");
    tft.setCursor(52, 290);
    tft.print("Fastest Lap");
    tft.setTextColor(RED, BLACK);
    tft.setTextSize(9);
    tft.setCursor(14, 13);
    tft.print("99.99");
    tft.setTextColor(YELLOW, BLACK);
    tft.setTextSize(9);
    tft.setCursor(14, 130);
    tft.print("+9.99");
    tft.setTextColor(GREEN, BLACK);
    tft.setTextSize(6);
    tft.setCursor(14, 242);
    tft.print("90.99");
    tft.setTextColor(GREEN, BLACK);
    tft.setTextSize(4);
    tft.setCursor(230, 254);
    tft.print("99");
    tft.setTextColor(GREEN, BLACK);
    tft.setTextSize(4);
    tft.setCursor(300, 13);
    tft.print("Current");
    tft.setCursor(300, 73);
    tft.print("Lap");
    tft.setCursor(388, 54);
    tft.setTextSize(7);
    tft.print("99");
    delay(1000);
    tft.setCursor(388, 54);
    tft.setTextSize(7);
    tft.print(" 0");

  BEGIN(9600);
  //Activate internal pullup
  pinMode(REEDPIN , INPUT_PULLUP);

  //this fires once and never more ...
  //attachInterrupt(0, trigger, FALLING);
  cleardata();
  sectors   = SECTORS;
  //getconf();


  show();
}

void cleardata() {
  newlap    = 0;
  reedready = 0;
  lasttime  = 0;
  //sectors   = SECTORS; config setting
  cursector = 0;
  lastsector= 0;
  curlap    = 0;
  state     = 0; //wait for start impulse
  timing[0] = { 
    NOTIME, NOTIME, NOTIME    }; //lap
  timing[1] = { 
    NOTIME, NOTIME, NOTIME    }; //sec1
  timing[2] = { 
    NOTIME, NOTIME, NOTIME    }; //sec2
  timing[3] = { 
    NOTIME, NOTIME, NOTIME    }; //sec3  
}

void getconf() {
  if ( EEPROM.read(CONFADDR) == 'L' and
    EEPROM.read(CONFADDR+1) == 'T' ) {
    int confver = EEPROM.read(CONFADDR+2);
    if ( confver == 1 ) {
      sectors = EEPROM.read(CONFADDR+3);
    }
  }
}

void saveconf() {
  EEPROM.write(CONFADDR, 'L');
  EEPROM.write(CONFADDR+1,'T' );
  EEPROM.write(CONFADDR+2, 1);
  EEPROM.write(CONFADDR+3, sectors);
}


void loop() {
  //Just wait for an interrupt

  if (newlap > 0) {
    //calculate
    //Don't process interrupts during calculations

    if (state == 0) {
      //first hit
      lastsector = sectors;
      cursector  = 1;
      curlap     = 1;
    } 
    else {
      savetime(cursector, reedready - lasttime);

      lastsector = cursector;
      if (cursector == sectors) {
        //start/finish passing
        cursector = 1;
        curlap++;
      } 
      else {
        cursector++;
      }
    }

    state      = 1; //running

    show();
    lasttime = reedready;     
    newlap = 0;

  }
  if ( state < 2 and millis() - reedready > LONGLAP) {
    state=2;
    show();
  }

  for (int i=0; i <= 1024; i++) {
    if (digitalRead(REEDPIN)==0) {

      if( millis() > reedready + DEBOUNCE){
        newlap++;
        reedready = millis();

        break;
      } 
      else {
        //parking over magnet
        reedready = millis();
      }
    }
  }

}


void savetime(int sectoridx, long time) {
  PRINT("Sector,time:");
  PRINT(sectoridx);
  PRINT(",");
  PRINTLN(time);
  //avoid overflow
  int sectortime = int((time+5)/10);
  if ( time > LONGLAP ) {
    sectortime = NOTIME + 1;
  }

  if (timing[sectoridx].last == NOTIME) {
    //first time
    timing[sectoridx].best = sectortime;
    timing[sectoridx].last = sectortime;
    timing[sectoridx].diff = 0;
  } 
  else {
    timing[sectoridx].diff = sectortime - timing[sectoridx].best;
    timing[sectoridx].last = sectortime;

    if ( sectortime < timing[sectoridx].best ) {
      timing[sectoridx].best = sectortime;
    }
  }

  PRINT("best, last, diff:");
  PRINT(timing[sectoridx].best);
  PRINT(",");
  PRINT(timing[sectoridx].last);
  PRINT(",");
  PRINTLN(timing[sectoridx].diff);

  if (sectoridx != LAP) {   
    //calculate running lap
    long runlap = 0;
    //sumup all sectors
    for (int i=1; i <= sectors; i++) {
      if (timing[i].last == NOTIME) {
        runlap = NOTIME;
        break;
      } 
      runlap += timing[i].last;
    }
    //timing is in millis
    if (runlap != NOTIME) {
      savetime(LAP,(long)(runlap*10));
    }
  }
}

void show() {

  char buff[8];

    PRINT("sec: best, last, diff:");
    PRINT(timing[lastsector].best);
    PRINT(",");
    PRINT(timing[lastsector].last);
    PRINT(",");
    PRINTLN(timing[lastsector].diff);
    PRINT("lap: best, last, diff:");
    PRINT(timing[LAP].best);
    PRINT(",");
    PRINT(timing[LAP].last);
    PRINT(",");
    PRINTLN(timing[LAP].diff);
    PRINT(curlap);
    PRINTLN();

    //fastest lap
    if ( (timing[LAP].diff) < 0 ) {
        fastestlap = curlap;
        }  
   
    //last lap
    if ( (timing[LAP].diff) < 0 ) {
        tft.setTextColor(GREEN, BLACK);
        }  
     if ( (timing[LAP].diff) >= 0 ) {
        tft.setTextColor(RED, BLACK);
        } 
        tft.setTextSize(9);
        tft.setCursor(14, 13);
        tft.print( ftime(timing[lastsector].last,buff));
            
    //lap diff
    tft.setTextColor(YELLOW, BLACK);
    tft.setTextSize(9);
    tft.setCursor(14, 130);
    tft.print( fdiff(timing[lastsector].diff,buff));
    
    //fastest lap time
    tft.setTextColor(GREEN, BLACK);
    tft.setTextSize(6);
    tft.setCursor(14, 242);
    tft.print( ftime(timing[lastsector].best,buff));
    
    //fastest lap
    if (fastestlap < 10) {
             tft.setTextColor(GREEN, BLACK);
             tft.setCursor(230, 254);
             tft.setTextSize(4);
             tft.print(" ");
             tft.setCursor(253, 254);
             tft.print( fastestlap );
             }
     if (fastestlap >= 10) {
             tft.setTextColor(GREEN, BLACK);
             tft.setCursor(230, 254);
             tft.setTextSize(4);
             tft.print( fastestlap );
             }
   
    //actual lapnumber
    if ( state > 0 ) {
        tft.setTextColor(GREEN, BLACK);
           if (curlap < 2) {
             tft.setTextColor(BLUE, BLACK);
             tft.setCursor(300, 140);
             tft.setTextSize(5);
             tft.print("Warmup");
             }
             if (curlap > 2) {
             tft.setTextColor(RED, BLACK);
             tft.setCursor(300, 140);
             tft.setTextSize(5);
             tft.print("      ");
             tft.setCursor(300, 155);
             tft.setTextSize(2);
             tft.print("floor it! ");
             tft.setCursor(300, 180);
             tft.print("full throttle");
             }
           if (curlap < 10) {
             tft.setTextColor(GREEN, BLACK);
             tft.setCursor(388, 54);
             tft.setTextSize(7);
             tft.print(" ");
             tft.setCursor(430, 54);
             tft.setTextSize(7);
             tft.print(curlap);
            }
            if (curlap >= 10) {
            tft.setTextColor(GREEN, BLACK);
            tft.setCursor(388, 54);
            tft.setTextSize(7);
            tft.print(curlap);
            }
            if (curlap > 40) {
            tft.setCursor(300, 13);
            tft.setTextSize(5);
            tft.setTextColor(RED, BLACK);
            tft.print("Pause!");
            }
  }
    tft.drawRect(0, 0, 480, 320, WHITE);
    tft.drawRect(1, 1, 479, 319, WHITE);
    tft.drawFastVLine(285, 0, 320, WHITE);
    tft.drawFastVLine(286, 0, 320, WHITE);

}

char* fdiff(int diff, char* buf) {
  int num = fabs(diff);
  if ( diff == NOTIME ) {
    strcpy(buf,"--.--");
    return(buf);
  } 
  if (diff >= 1000) {
    num = 999;
  }
  int secs = int(num/100);
  int hund = num % 100;
  sprintf(buf, "%c%01d.%02d", diff>0?'+':'-', secs, hund); 
  return(buf);   
}

char* ftime(int time, char* buf) {
  int num = fabs(time);
  if ( time == NOTIME ) {
    strcpy(buf,"--.--");
    return(buf);
  } 
  else if (time >= 10000) {
    num = 9999;     
  } 
  else if (time < 0) {
    num = 0;
  }
  int secs = int(num/100);
  int hund = num % 100;
  sprintf(buf, "%2d.%02d", secs, hund); 
  return(buf);   
}
