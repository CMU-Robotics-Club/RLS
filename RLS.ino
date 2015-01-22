#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_NeoPixel.h>
#include <RC.h>

// SET THESE
#define PUBLIC_KEY ""
#define PRIVATE_KEY ""
byte mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


#define STRIP_CHANNEL_ID 12
#define LOGO_CHANNEL_ID 11


#define STRIP_PIN 6
#define STRIP_NUM_PIXELS 150

#define LOGO_PIN 5
#define LOGO_NUM_PIXELS 8

#define INACTIVE_STRIP_COLOR COLOR(20, 0, 0)
#define INACTIVE_LOGO_COLOR COLOR(20, 0, 0  )

// 1000 * 30 seconds
#define INACTIVE_MILLIS_THRESHOLD 30000


#define COLOR(r, g, b) r, g, b

Adafruit_NeoPixel stripPixels = Adafruit_NeoPixel(STRIP_NUM_PIXELS, STRIP_PIN, NEO_GRB + NEO_KHZ800); 
Adafruit_NeoPixel logoPixels = Adafruit_NeoPixel(LOGO_NUM_PIXELS, LOGO_PIN, NEO_GRB + NEO_KHZ800); 

EthernetClient ethernetClient;

APIClient api(ethernetClient, PUBLIC_KEY, PRIVATE_KEY);

typedef struct Pixels_t {
  boolean written;
  unsigned long lastWrite;
  Channel oldChannel;
} Pixels;

Pixels strip;
Pixels logo;


void setPixelsColor(Adafruit_NeoPixel& pixels, uint8_t r, uint8_t g, uint8_t b);
boolean parseColor(String& value, int& red, int& green, int& blue);

void updateStrip();
void updateLogo();


void setup() {
    
  Serial.begin(9200);
  
  Serial.println("Welcome to RLS");
  
  stripPixels.begin();
  logoPixels.begin();
  
  if(!Ethernet.begin(mac)) {
    Serial.println("Failed to initialize ethernet shield");
    return; 
  }
  
  delay(1000);
  
  Serial.println("Connected to Internet");
}

void loop() {
  updateStrip();
  updateLogo();
  //delay(2000); 
}

void setPixelsColor(Adafruit_NeoPixel& pixels, uint8_t r, uint8_t g, uint8_t b) {
  for(int i = 0; i < pixels.numPixels(); i++){
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();
  } 
}

boolean parseColor(String& value, int& red, int& green, int& blue) {
  int spaceIndex = value.indexOf(' ');
  int spaceIndex2 = value.indexOf(' ', spaceIndex+1);
        
  if(spaceIndex >= 0 && spaceIndex2 > spaceIndex) {
    String redS = value.substring(0, spaceIndex);
    String greenS = value.substring(spaceIndex+1, spaceIndex2);
    String blueS = value.substring(spaceIndex2);     
          
    red = redS.toInt();
    green = greenS.toInt();
    blue = blueS.toInt();
    
    return true;
  } else {
    return false;
  } 
}

void updateStrip() {  
  Channel stripChannel;

  if(!api.channel(STRIP_CHANNEL_ID, stripChannel)) {
    Serial.println("Strip channel read failed"); 
  } else {
    if(!strip.written || CHANNEL_UPDATED(strip.oldChannel, stripChannel)) {
      int red, green, blue;
        
      if(parseColor(stripChannel.value, red, green, blue)) {
        Serial.print("Valid Color: ");  
        Serial.print(red);
        Serial.print(" ");
        Serial.print(green);
        Serial.print(" ");
        Serial.println(blue);
        
        setPixelsColor(stripPixels, COLOR(red, green, blue));
        
        strip.written = true;
        strip.oldChannel = stripChannel;
        strip.lastWrite = millis();
        
      } else {
        Serial.print("Invalid color: ");
        Serial.println(stripChannel.value);
      }
    }
  }
  
  if(millis() - strip.lastWrite > INACTIVE_MILLIS_THRESHOLD) {
    setPixelsColor(stripPixels, INACTIVE_STRIP_COLOR);
  }
}

void updateLogo(){  
  Channel logoChannel;

  if(!api.channel(LOGO_CHANNEL_ID, logoChannel)) {
    Serial.println("Logo channel read failed"); 
  } else {
    if(!logo.written || CHANNEL_UPDATED(logo.oldChannel, logoChannel)) {
      int red, green, blue;
        
      if(parseColor(logoChannel.value, red, green, blue)) {
        Serial.print("Valid Color: ");  
        Serial.print(red);
        Serial.print(" ");
        Serial.print(green);
        Serial.print(" ");
        Serial.println(blue);
        
        setPixelsColor(logoPixels, COLOR(red, green, blue));
        
        logo.written = true;
        logo.oldChannel = logoChannel;
        logo.lastWrite = millis();
        
      } else {
        Serial.print("Invalid color: ");
        Serial.println(logoChannel.value);
      }
    }
  }
  
  if(millis() - logo.lastWrite > INACTIVE_MILLIS_THRESHOLD) {
    setPixelsColor(logoPixels, INACTIVE_LOGO_COLOR);
  } 
}
