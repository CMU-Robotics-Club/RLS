#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_NeoPixel.h>
#include <RC.h>

// SET THESE
#define PUBLIC_KEY ""
#define PRIVATE_KEY ""
//byte mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


#define STRIP_CHANNEL_ID 12
#define LOGO_CHANNEL_ID 11


#define STRIP_PIN 6
#define STRIP_NUM_PIXELS 150

#define LOGO_PIN 5
#define LOGO_NUM_PIXELS 8

#define INACTIVE_STRIP_COLOR 20, 0, 0
#define INACTIVE_LOGO_COLOR 20, 0, 0

// 1000 * 30 seconds
#define INACTIVE_MILLIS_THRESHOLD 30000


Adafruit_NeoPixel stripPixels = Adafruit_NeoPixel(STRIP_NUM_PIXELS, STRIP_PIN, NEO_GRB + NEO_KHZ800); 
Adafruit_NeoPixel logoPixels = Adafruit_NeoPixel(LOGO_NUM_PIXELS, LOGO_PIN, NEO_GRB + NEO_KHZ800); 

EthernetClient ethernetClient;

APIClient api(ethernetClient, PUBLIC_KEY, PRIVATE_KEY);

/*
 * Data structure used to keep track of
 * if the pixels have beeen written to and
 * if so, when was it.
 */
typedef struct Pixels_t {
  /*
   * set to true first time channel for
   * LEDs is read, its color(s) parsed,
   * and sent to LEDs.
   */
  boolean written;
  
  /*
   * Last time LEDs were written to.
   */
  unsigned long lastWrite;
  
  /*
   * Channel state that caused last update.
   */
  Channel oldChannel;
} Pixels;


Pixels strip;
Pixels logo;

/*
 * Sets the pixels to a specific color.
 */
void setPixelsColor(Adafruit_NeoPixel& pixels, uint32_t color);

/*
 * Sets the pixels to the specific pattern.
 */
void setPixelsColor(Adafruit_NeoPixel& pixels, uint32_t* colors, size_t num_colors);

/*
 * Returns true if vzlue is a valid color (pattern).  Colors is set
 * to an array of parsed colors, with num_colors being set to the number
 * of colors in colors.
 * NOTE: YOU MUST CALL FREE yourself on the color array *colors, not the pointer to the array!
 */
boolean parseColor(String& value, uint32_t** colors, size_t* num_colors);

// Update each set of LEDs by checking the respective channel,
// and if new parsing it and setting the LEDs to the respective color
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
}

void setPixelsColor(Adafruit_NeoPixel& pixels, uint32_t color) {
  for(int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, color);
    pixels.show();
  }
}

void setPixelsColor(Adafruit_NeoPixel& pixels, uint32_t* colors, size_t num_colors) {
  size_t j = 0;
  
  for(int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, colors[j]);
    pixels.show();
    
    j++;
    
    /*
     * if we are at the end of the color array
     * reset to the beginning
     */
    if(j >= num_colors) {
      j = 0; 
    }
  }  
}

boolean parseColor(String& value, uint32_t** colors, size_t* num_colors) {
  // each color is 6 hexadecimal characters
  if(value.length() % 6 != 0) {
    return false;
  }
  
  // ensure each character is a hexadecimal character
  for(int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if(!isHexadecimalDigit(c)) {
      return false; 
    }
  }
  
  size_t n = value.length() / 6;
  (*colors) = (uint32_t*)malloc(sizeof(uint32_t) * n);
  
  // convert each color string to decimal value
  for(int i = 0; i < n; i++) {
    String colorString = value.substring(i*6, (i+1)*6);
    
    colorString.toLowerCase();
    
    colorString = "0x" + colorString;
    
    Serial.print("Color String:");
    Serial.println(colorString);

    char buffer[colorString.length() + 1];
    colorString.toCharArray(buffer, colorString.length() + 1);
    
    Serial.print("Buffer:");
    Serial.println(buffer);
    
    long color = strtol(buffer, NULL, 16);
    
    Serial.print("Color:");
    Serial.println(color);
    
    (*colors)[i] = color;
  }
  
  (*num_colors) = n;

  return true;
}


void updateStrip() {  
  Channel stripChannel;

  if(!api.channel(STRIP_CHANNEL_ID, stripChannel)) {
    Serial.println("Strip channel read failed"); 
  } else {
    if(!strip.written || CHANNEL_UPDATED(strip.oldChannel, stripChannel)) {
      uint32_t* colors;
      size_t num_colors;
      
      if(parseColor(stripChannel.value, &colors, &num_colors)) {
        for(int i = 0; i < num_colors; i++){
          Serial.print("Valid Color: ");  
          Serial.println(colors[i]);
        }
        
        setPixelsColor(stripPixels, colors, num_colors);
        
        free(colors);
        
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
    setPixelsColor(stripPixels, stripPixels.Color(INACTIVE_STRIP_COLOR));
  }
}

void updateLogo(){  
  Channel logoChannel;

  if(!api.channel(LOGO_CHANNEL_ID, logoChannel)) {
    Serial.println("Logo channel read failed"); 
  } else {
    if(!logo.written || CHANNEL_UPDATED(logo.oldChannel, logoChannel)) {
      uint32_t* colors;
      size_t num_colors;
        
      if(parseColor(logoChannel.value, &colors, &num_colors)) {
        for(int i = 0; i < num_colors; i++){
          Serial.print("Valid Color: ");  
          Serial.println(colors[i]);
        }
        
        setPixelsColor(logoPixels, colors, num_colors);
        
        free(colors);
        
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
    setPixelsColor(logoPixels, logoPixels.Color(INACTIVE_LOGO_COLOR));
  } 
}
