// A basic everyday NeoPixel strip test program.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_DS1307 rtc;

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN   6

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 165

// Set BRIGHTNESS to about 1/5 (max = 255)
#define BRIGHTNESS 50

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

struct Config {
  // total LEDs
  int total = LED_COUNT;

  // main clock
  int totalHours = 13;
  int hoursStart = 6;
  int intervalHours = 12;
  int intervalHalves = intervalHours / 2;
  int intervalQuarters = intervalHours / 4;
  int clockStart = 0;
  int clockEnd = totalHours * intervalHours;

  // minutes counter
  int intervalMinutes = 5;
  int minutesStart = clockEnd;
  int minutesEnd = minutesStart + intervalMinutes;

  // seconds counter
  int intervalSeconds = 4;
  int secondsRepresentation = 15;
  int secondsStart = minutesEnd;
  int secondsEnd = secondsStart + intervalSeconds;

  // colors
  uint32_t colorTicksHours     = Adafruit_NeoPixel::Color(255, 255,   0); // yellow
  uint32_t colorTicksHalves    = Adafruit_NeoPixel::Color(  0,   0, 255); // blue
  uint32_t colorTicksQuarters  = Adafruit_NeoPixel::Color(255,   0,   0); // red
  uint32_t colorFuture         = Adafruit_NeoPixel::Color(255, 255, 255); // white
  uint32_t colorPast           = Adafruit_NeoPixel::Color(  0, 255,   0); // green
  uint32_t colorCounterMinutes = Adafruit_NeoPixel::Color(  0,   0, 255); // blue
  uint32_t colorCounterSeconds = Adafruit_NeoPixel::Color(  0, 255,   0); // green

  uint32_t colors[LED_COUNT];
};

Config cfg;
DateTime now;
DateTime last;
bool alreadyOff = false;

DateTime getNow(RTC_DS1307);
int getHours(DateTime);
int getMinutes(DateTime);
int getSeconds(DateTime);
void initColors(Config *);
void logConfig(Config *);
bool inClockRange(Config *, int);
void renderTime(Config *, DateTime);

void colorWipe(uint32_t, int);
void theaterChase(uint32_t, int);

// setup() function -- runs once at startup --------------------------------

void setup() {
  Serial.begin(9600);

#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  initColors(&cfg);
  logConfig(&cfg);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)
}


// loop() function -- runs repeatedly as long as board is on ---------------

void loop() {
  // Fill along the length of the strip in various colors...
  //colorWipe(strip.Color(255,   0,   0), 50); // Red
  //colorWipe(strip.Color(  0, 255,   0), 50); // Green
  //colorWipe(strip.Color(  0,   0, 255), 50); // Blue

  // Do a theater marquee effect in various colors...
  //theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness
  //theaterChase(strip.Color(127,   0,   0), 50); // Red, half brightness
  //theaterChase(strip.Color(  0,   0, 127), 50); // Blue, half brightness

  now = getNow(rtc);
  if (now != last) { 
    if (inClockRange(&cfg, getHours(now))) {
      renderTime(&cfg, now);
      strip.show();
      alreadyOff = false;
    } else {
      Serial.println("not in clock range!");
      if (!alreadyOff) {
        for (int i = cfg.clockStart; i < cfg.total; i++) {
          strip.clear();
          strip.show();
        }
        alreadyOff = true;
      }
    }
    last = now;
    }
}

DateTime getNow(RTC_DS1307 rtc) {
  return rtc.now();
}

int getHours(DateTime dateTime)   { return dateTime.hour();   }
int getMinutes(DateTime dateTime) { return dateTime.minute(); }
int getSeconds(DateTime dateTime) { return dateTime.second(); }

void initColors(Config *cfg) {
  // initialize colors
  for (int i = cfg->clockStart; i < cfg->clockEnd; i++) {
    if (i % cfg->intervalHours == 0) {
      cfg->colors[i] = cfg->colorTicksHours;
    } else if (i % cfg->intervalHalves == 0) {
      cfg->colors[i] = cfg->colorTicksHalves;
    } else if (i % cfg->intervalQuarters == 0) {
      cfg->colors[i] = cfg->colorTicksQuarters;
    } else {
      cfg->colors[i] = cfg->colorFuture;
    }
  }

  for (int i = cfg->minutesStart; i < cfg->minutesEnd; i++) {
    cfg->colors[i] = cfg->colorFuture;
  }

  for (int i = cfg->secondsStart; i < cfg->secondsEnd; i++) {
    cfg->colors[i] = cfg->colorFuture;
  }
}

void logConfig(Config *cfg) {
  Serial.print("                total: ");
  Serial.println(cfg->total                );
  Serial.print("           totalHours: ");
  Serial.println(cfg->totalHours           );
  Serial.print("           hoursStart: ");
  Serial.println(cfg->hoursStart           );
  Serial.print("        intervalHours: ");
  Serial.println(cfg->intervalHours        );
  Serial.print("       intervalHalves: ");
  Serial.println(cfg->intervalHalves       );
  Serial.print("     intervalQuarters: ");
  Serial.println(cfg->intervalQuarters     );
  Serial.print("           clockStart: ");
  Serial.println(cfg->clockStart           );
  Serial.print("             clockEnd: ");
  Serial.println(cfg->clockEnd             );
  Serial.print("      intervalMinutes: ");
  Serial.println(cfg->intervalMinutes      );
  Serial.print("         minutesStart: ");
  Serial.println(cfg->minutesStart         );
  Serial.print("           minutesEnd: ");
  Serial.println(cfg->minutesEnd           );
  Serial.print("      intervalSeconds: ");
  Serial.println(cfg->intervalSeconds      );
  Serial.print("secondsRepresentation: ");
  Serial.println(cfg->secondsRepresentation);
  Serial.print("         secondsStart: ");
  Serial.println(cfg->secondsStart         );
  Serial.print("           secondsEnd: ");
  Serial.println(cfg->secondsEnd           );
  Serial.print("      colorTicksHours: ");
  Serial.println(cfg->colorTicksHours      );
  Serial.print("     colorTicksHalves: ");
  Serial.println(cfg->colorTicksHalves     );
  Serial.print("   colorTicksQuarters: ");
  Serial.println(cfg->colorTicksQuarters   );
  Serial.print("          colorFuture: ");
  Serial.println(cfg->colorFuture          );
  Serial.print("            colorPast: ");
  Serial.println(cfg->colorPast            );
  Serial.print("  colorCounterMinutes: ");
  Serial.println(cfg->colorCounterMinutes  );
  Serial.print("  colorCounterSeconds: ");
  Serial.println(cfg->colorCounterSeconds  );
}

void logTime(DateTime dateTime) {
  Serial.print(getHours(dateTime), DEC);
  Serial.print(':');
  Serial.print(getMinutes(dateTime), DEC);
  Serial.print(':');
  Serial.print(getSeconds(dateTime), DEC);
  Serial.println();
}

bool inClockRange(Config *cfg, int hours) {
  return hours >= cfg->hoursStart && hours < (cfg->hoursStart + cfg->totalHours);
}

void renderTime(Config *cfg, DateTime dateTime) {
  logTime(dateTime);

  int hours   = getHours(dateTime);
  int minutes = getMinutes(dateTime);
  int seconds = getSeconds(dateTime);

  int currentTimeIndex = cfg->clockStart;
  currentTimeIndex += (hours - cfg->hoursStart) * cfg->intervalHours;
  currentTimeIndex += minutes / cfg->intervalMinutes;

  int currentMinuteIndex = cfg->minutesStart;
  currentMinuteIndex += minutes % cfg->intervalMinutes;

  int currentSecondIndex = cfg->secondsStart;
  currentSecondIndex += seconds / cfg->secondsRepresentation;

  for (int i = cfg->clockStart; i < cfg->clockEnd; i++) {
    if (i < currentTimeIndex) {
      strip.setPixelColor(i, cfg->colorPast);
    } else {
      strip.setPixelColor(i, cfg->colors[i]);
    }
  }

  for (int i = cfg->minutesStart; i < cfg->minutesEnd; i++) {
    if (i < currentMinuteIndex) {
      strip.setPixelColor(i, cfg->colorCounterMinutes);
    } else {
      strip.setPixelColor(i, cfg->colors[i]);
    }
  }

  for (int i = cfg->secondsStart; i < cfg->secondsEnd; i++) {
    if (i < currentSecondIndex) {
      strip.setPixelColor(i, cfg->colorCounterSeconds);
    } else {
      strip.setPixelColor(i, cfg->colors[i]);
    }
  }
}

// Some functions of our own for creating animated effects -----------------

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}