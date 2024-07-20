/*
   @title     StarLeds
   @file      LedEffects.h
   @date      20240228
   @repo      https://github.com/MoonModules/StarLeds
   @Authors   https://github.com/MoonModules/StarLeds/commits/main
   @Copyright © 2024 Github StarLeds Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#ifdef STARLEDS_USERMOD_WLEDAUDIO
  #include "../User/UserModWLEDAudio.h"
#endif
#ifdef STARBASE_USERMOD_E131
  #include "../User/UserModE131.h"
#endif

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
unsigned long call = 0; //not used at the moment (don't use in effect calculations), well this is not entirely true.

//utility function
float distance(float x1, float y1, float z1, float x2, float y2, float z2) {
  return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

//should not contain variables/bytes to keep mem as small as possible!!
class Effect {
public:
  virtual const char * name() {return "noname";}
  virtual const char * tags() {return "";}
  virtual uint8_t dim() {return _1D;};

  virtual void setup(Leds &leds) {}

  virtual void loop(Leds &leds) {}

  virtual void controls(Leds &leds, JsonObject parentVar) {
    ui->initSelect(parentVar, "pal", 4, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Palette");
        JsonArray options = ui->setOptions(var);
        options.add("CloudColors");
        options.add("LavaColors");
        options.add("OceanColors");
        options.add("ForestColors");
        options.add("RainbowColors");
        options.add("RainbowStripeColors");
        options.add("PartyColors");
        options.add("HeatColors");
        options.add("RandomColors");
        return true; }
      case f_ChangeFun:
        switch (var["value"][rowNr].as<uint8_t>()) {
          case 0: leds.palette = CloudColors_p; break;
          case 1: leds.palette = LavaColors_p; break;
          case 2: leds.palette = OceanColors_p; break;
          case 3: leds.palette = ForestColors_p; break;
          case 4: leds.palette = RainbowColors_p; break;
          case 5: leds.palette = RainbowStripeColors_p; break;
          case 6: leds.palette = PartyColors_p; break;
          case 7: leds.palette = HeatColors_p; break;
          case 8: { //randomColors
            for (int i=0; i < sizeof(leds.palette.entries) / sizeof(CRGB); i++) {
              leds.palette[i] = CHSV(random8(), 255, 255); //take the max saturation, max brightness of the colorwheel
            }
            break;
          }
          default: leds.palette = PartyColors_p; //should never occur
        }
        return true;
      default: return false;
    }});
  }

};

class SolidEffect: public Effect {
  const char * name() {return "Solid";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t red = leds.sharedData.read<uint8_t>();
    uint8_t green = leds.sharedData.read<uint8_t>();
    uint8_t blue = leds.sharedData.read<uint8_t>();

    CRGB color = CRGB(red, green, blue);
    leds.fill_solid(color);
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "Red", leds.sharedData.write<uint8_t>(182));
    ui->initSlider(parentVar, "Green", leds.sharedData.write<uint8_t>(15));
    ui->initSlider(parentVar, "Blue", leds.sharedData.write<uint8_t>(98));
  }
};

class RainbowEffect: public Effect {
public:
  const char * name() {return "Rainbow";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "⚡";}

  void loop(Leds &leds) {
    // FastLED's built-in rainbow generator
    leds.fill_rainbow(gHue, 7);
  }

  void controls(Leds &leds, JsonObject parentVar) {} //so no palette control is created
};

class RainbowWithGlitterEffect: public RainbowEffect {
  const char * name() {return "Rainbow with glitter";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "⚡";}

  void loop(Leds &leds) {
    // built-in FastLED rainbow, plus some random sparkly glitter
    RainbowEffect::loop(leds);
    addGlitter(leds, 80);
  }
  void addGlitter(Leds &leds, fract8 chanceOfGlitter) 
  {
    if( random8() < chanceOfGlitter) {
      leds[ random16(leds.nrOfLeds) ] += CRGB::White;
    }
  }
};

// a colored dot sweeping back and forth, with fading trails
class SinelonEffect: public Effect {
  const char * name() {return "Sinelon";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "⚡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t bpm = leds.sharedData.read<uint8_t>();

    leds.fadeToBlackBy(20);

    int pos = beatsin16( bpm, 0, leds.nrOfLeds-1 );
    leds[pos] = leds.getPixelColor(pos) + CHSV( gHue, 255, 255);
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "BPM", leds.sharedData.write<uint8_t>(60));
  }
}; //Sinelon

class ConfettiEffect: public Effect {
  const char * name() {return "Confetti";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "⚡";}

  void loop(Leds &leds) {
    // random colored speckles that blink in and fade smoothly
    leds.fadeToBlackBy(10);
    int pos = random16(leds.nrOfLeds);
    leds[pos] += CHSV( gHue + random8(64), 200, 255);
  }

  void controls(Leds &leds, JsonObject parentVar) {} //so no palette control is created
};

// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
class BPMEffect: public Effect {
  const char * name() {return "Beats per minute";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "⚡";}

  void loop(Leds &leds) {
    uint8_t BeatsPerMinute = 62;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for (forUnsigned16 i = 0; i < leds.nrOfLeds; i++) { //9948
      leds[i] = ColorFromPalette(leds.palette, gHue+(i*2), beat-gHue+(i*10));
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
  }
};

// eight colored dots, weaving in and out of sync with each other
class JuggleEffect: public Effect {
  const char * name() {return "Juggle";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "⚡";}

  void loop(Leds &leds) {
    leds.fadeToBlackBy(20);
    uint8_t dothue = 0;
    for (unsigned i = 0; i < 8; i++) {
      leds[beatsin16( i+7, 0, leds.nrOfLeds-1 )] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {} //so no palette control is created
};

//https://www.perfectcircuit.com/signal/difference-between-waveforms
class RunningEffect: public Effect {
  const char * name() {return "Running";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t bpm = leds.sharedData.read<uint8_t>();
    uint8_t fade = leds.sharedData.read<uint8_t>();

    leds.fadeToBlackBy(fade); //physical leds
    int pos = map(beat16( bpm), 0, UINT16_MAX, 0, leds.nrOfLeds-1 ); //instead of call%leds.nrOfLeds
    // int pos2 = map(beat16( bpm, 1000), 0, UINT16_MAX, 0, leds.nrOfLeds-1 ); //one second later
    leds[pos] = CHSV( gHue, 255, 255); //make sure the right physical leds get their value
    // leds[leds.nrOfLeds -1 - pos2] = CHSV( gHue, 255, 255); //make sure the right physical leds get their value
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "BPM", leds.sharedData.write<uint8_t>(60), 0, 255, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setComment(var, "in BPM!");
        return true;
      default: return false;
    }});
    //tbd: check if memory is freed!
    ui->initSlider(parentVar, "fade", leds.sharedData.write<uint8_t>(128));
  }
};

class RingEffect: public Effect {
  protected:

  void setRing(Leds &leds, int ring, CRGB colour) { //so britisch ;-)
    leds[ring] = colour;
  }

};

class RingRandomFlow: public RingEffect {
  const char * name() {return "RingRandomFlow";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💫";}

  void loop(Leds &leds) {
    //binding of loop persistent values (pointers)
    uint8_t *hue = leds.sharedData.readWrite<uint8_t>(leds.nrOfLeds); //array

    hue[0] = random(0, 255);
    for (int r = 0; r < leds.nrOfLeds; r++) {
      setRing(leds, r, CHSV(hue[r], 255, 255));
    }
    for (int r = (leds.nrOfLeds - 1); r >= 1; r--) {
      hue[r] = hue[(r - 1)]; // set this ruing based on the inner
    }
    // FastLED.delay(SPEED);
  }

  void controls(Leds &leds, JsonObject parentVar) {} //so no palette control is created
};

//BouncingBalls inspired by WLED
#define maxNumBalls 16
//each needs 12 bytes
struct Ball {
  unsigned long lastBounceTime;
  float impactVelocity;
  float height;
};

class BouncingBalls: public Effect {
  const char * name() {return "Bouncing Balls";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t grav = leds.sharedData.read<uint8_t>();
    uint8_t numBalls = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers)
    Ball *balls = leds.sharedData.readWrite<Ball>(maxNumBalls); //array

    leds.fill_solid(CRGB::Black);

    // non-chosen color is a random color
    const float gravity = -9.81f; // standard value of gravity
    // const bool hasCol2 = SEGCOLOR(2);
    const unsigned long time = sys->now;

    //not necessary as sharedData is cleared at setup(Leds &leds)
    // if (call == 0) {
    //   for (size_t i = 0; i < maxNumBalls; i++) balls[i].lastBounceTime = time;
    // }

    for (size_t i = 0; i < numBalls; i++) {
      float timeSinceLastBounce = (time - balls[i].lastBounceTime)/((255-grav)/64 + 1);
      float timeSec = timeSinceLastBounce/1000.0f;
      balls[i].height = (0.5f * gravity * timeSec + balls[i].impactVelocity) * timeSec; // avoid use pow(x, 2) - its extremely slow !

      if (balls[i].height <= 0.0f) {
        balls[i].height = 0.0f;
        //damping for better effect using multiple balls
        float dampening = 0.9f - float(i)/float(numBalls * numBalls); // avoid use pow(x, 2) - its extremely slow !
        balls[i].impactVelocity = dampening * balls[i].impactVelocity;
        balls[i].lastBounceTime = time;

        if (balls[i].impactVelocity < 0.015f) {
          float impactVelocityStart = sqrtf(-2.0f * gravity) * random8(5,11)/10.0f; // randomize impact velocity
          balls[i].impactVelocity = impactVelocityStart;
        }
      } else if (balls[i].height > 1.0f) {
        continue; // do not draw OOB ball
      }

      // uint32_t color = SEGCOLOR(0);
      // if (SEGMENT.palette) {
      //   color = SEGMENT.color_wheel(i*(256/MAX(numBalls, 8)));
      // } 
      // else if (hasCol2) {
      //   color = SEGCOLOR(i % NUM_COLORS);
      // }

      int pos = roundf(balls[i].height * (leds.nrOfLeds - 1));

      CRGB color = ColorFromPalette(leds.palette, i*(256/max(numBalls, (uint8_t)8)), 255); //error: no matching function for call to 'max(uint8_t&, int)'

      leds[pos] = color;
      // if (leds.nrOfLeds<32) leds.setPixelColor(indexToVStrip(pos, stripNr), color); // encode virtual strip into index
      // else           leds.setPixelColor(balls[i].height + (stripNr+1)*10.0f, color);
    } //balls
  }

  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "gravity", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "balls", leds.sharedData.write<uint8_t>(8), 1, 16);
  }
}; // BouncingBalls

void mode_fireworks(Leds &leds, uint16_t *aux0, uint16_t *aux1, uint8_t speed, uint8_t intensity, bool useAudio = false) {
  // fade_out(0);
  leds.fadeToBlackBy(10);
  // if (SEGENV.call == 0) {
  //   *aux0 = UINT16_MAX;
  //   *aux1 = UINT16_MAX;
  // }
  bool valid1 = (*aux0 < leds.nrOfLeds);
  bool valid2 = (*aux1 < leds.nrOfLeds);
  CRGB sv1 = 0, sv2 = 0;
  if (valid1) sv1 = leds.getPixelColor(*aux0);
  if (valid2) sv2 = leds.getPixelColor(*aux1);

  // WLEDSR
  uint8_t blurAmount   = 255 - speed;
  uint8_t my_intensity = 129 - intensity;
  bool addPixels = true;                        // false -> inhibit new pixels in silence
  int soundColor = -1;                          // -1 = random color; 0..255 = use as palette index

  // if (useAudio) {
  //   if (FFT_MajorPeak < 100)    { blurAmount = 254;} // big blobs
  //   else {
  //     if (FFT_MajorPeak > 3200) { blurAmount = 1;}   // small blobs
  //     else {                                         // blur + color depends on major frequency
  //       float musicIndex = logf(FFT_MajorPeak);            // log scaling of peak freq
  //       blurAmount = mapff(musicIndex, 4.60, 8.08, 253, 1);// map to blur range (low freq = more blur)
  //       blurAmount = constrain(blurAmount, 1, 253);        // remove possible "overshot" results
  //       soundColor = mapff(musicIndex, 4.6, 8.08, 0, 255); // pick color from frequency
  //   } }
  //   if (sampleAgc <= 1.0) {      // silence -> no new pixels, just blur
  //     valid1 = valid2 = false;   // do not copy last pixels
  //     addPixels = false;         
  //     blurAmount = 128;
  //   }
  //   my_intensity = 129 - (speed >> 1); // dirty hack: use "speed" slider value intensity (no idea how to _disable_ the first slider, but show the second one)
  //   if (samplePeak == 1) my_intensity -= my_intensity / 4;    // inclease intensity at peaks
  //   if (samplePeak > 1) my_intensity = my_intensity / 2;      // double intensity at main peaks
  // }
  // // WLEDSR end

  leds.blur1d(blurAmount);
  if (valid1) leds.setPixelColor(*aux0, sv1);
  if (valid2) leds.setPixelColor(*aux1, sv2);

  if (addPixels) {                                                                             // WLEDSR
    for(uint16_t i=0; i<max(1, leds.nrOfLeds/20); i++) {
      if(random8(my_intensity) == 0) {
        uint16_t index = random(leds.nrOfLeds);
        if (soundColor < 0)
          leds.setPixelColor(index, ColorFromPalette(leds.palette, random8()));
        else
          leds.setPixelColor(index, ColorFromPalette(leds.palette, soundColor + random8(24))); // WLEDSR
        *aux1 = *aux0;
        *aux0 = index;
      }
    }
  }
  // return FRAMETIME;
}

class RainEffect: public Effect {
  const char * name() {return "Rain";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t intensity = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    uint16_t *aux0 = leds.sharedData.readWrite<uint16_t>();
    uint16_t *aux1 = leds.sharedData.readWrite<uint16_t>();
    uint16_t *step = leds.sharedData.readWrite<uint16_t>();

    // if(SEGENV.call == 0) {
      // SEGMENT.fill(BLACK);
    // }
    *step += 1000 / 40;// FRAMETIME;
    if (*step > (5U + (50U*(255U - speed))/leds.nrOfLeds)) { //SPEED_FORMULA_L) {
      *step = 1;
      // if (strip.isMatrix) {
      //   //uint32_t ctemp[leds.size.x];
      //   //for (int i = 0; i<leds.size.x; i++) ctemp[i] = SEGMENT.getPixelColorXY(i, leds.size.y-1);
      //   SEGMENT.move(6, 1, true);  // move all pixels down
      //   //for (int i = 0; i<leds.size.x; i++) leds.setPixelColorXY(i, 0, ctemp[i]); // wrap around
      //   *aux0 = (*aux0 % leds.size.x) + (*aux0 / leds.size.x + 1) * leds.size.x;
      //   *aux1 = (*aux1 % leds.size.x) + (*aux1 / leds.size.x + 1) * leds.size.x;
      // } else 
      {
        //shift all leds left
        CRGB ctemp = leds.getPixelColor(0);
        for (int i = 0; i < leds.nrOfLeds - 1; i++) {
          leds.setPixelColor(i, leds.getPixelColor(i+1));
        }
        leds.setPixelColor(leds.nrOfLeds -1, ctemp); // wrap around
        *aux0++;  // increase spark index
        *aux1++;
      }
      if (*aux0 == 0) *aux0 = UINT16_MAX; // reset previous spark position
      if (*aux1 == 0) *aux0 = UINT16_MAX; // reset previous spark position
      if (*aux0 >= leds.size.x*leds.size.y) *aux0 = 0;     // ignore
      if (*aux1 >= leds.size.x*leds.size.y) *aux1 = 0;
    }
    mode_fireworks(leds, aux0, aux1, speed, intensity);
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(128), 1, 255);
    ui->initSlider(parentVar, "intensity", leds.sharedData.write<uint8_t>(64), 1, 128);
  }
}; // RainEffect

//each needs 19 bytes
//Spark type is used for popcorn, 1D fireworks, and drip
struct Spark {
  float pos, posX;
  float vel, velX;
  unsigned16 col;
  uint8_t colIndex;
};

#define maxNumDrops 6
class DripEffect: public Effect {
  const char * name() {return "Drip";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t grav = leds.sharedData.read<uint8_t>();
    uint8_t drips = leds.sharedData.read<uint8_t>();
    uint8_t swell = leds.sharedData.read<uint8_t>();
    bool invert = leds.sharedData.read<bool>();

    //binding of loop persistent values (pointers)
    Spark* drops = leds.sharedData.readWrite<Spark>(maxNumDrops);

    // leds.fadeToBlackBy(90);
    leds.fill_solid(CRGB::Black);

    float gravity = -0.0005f - (grav/25000.0f); //increased gravity (50000 to 25000)
    gravity *= max(1, leds.nrOfLeds-1);
    int sourcedrop = 12;

    for (int j=0;j<drips;j++) {
      if (drops[j].colIndex == 0) { //init
        drops[j].pos = leds.nrOfLeds-1;    // start at end
        drops[j].vel = 0;           // speed
        drops[j].col = sourcedrop;  // brightness
        drops[j].colIndex = 1;      // drop state (0 init, 1 forming, 2 falling, 5 bouncing)
        drops[j].velX = (uint32_t)ColorFromPalette(leds.palette, random8()); // random color
      }
      CRGB dropColor = drops[j].velX;

      leds.setPixelColor(invert?0:leds.nrOfLeds-1, blend(CRGB::Black, dropColor, sourcedrop));// water source
      if (drops[j].colIndex==1) {
        if (drops[j].col>255) drops[j].col=255;
        leds.setPixelColor(invert?leds.nrOfLeds-1-drops[j].pos:drops[j].pos, blend(CRGB::Black, dropColor, drops[j].col));

        drops[j].col += swell; // swelling

        if (random16() <= drops[j].col * swell * swell / 10) {               // random drop
          drops[j].colIndex=2;               //fall
          drops[j].col=255;
        }
      }
      if (drops[j].colIndex > 1) {           // falling
        if (drops[j].pos > 0) {              // fall until end of segment
          drops[j].pos += drops[j].vel;
          if (drops[j].pos < 0) drops[j].pos = 0;
          drops[j].vel += gravity;           // gravity is negative

          for (int i=1;i<7-drops[j].colIndex;i++) { // some minor math so we don't expand bouncing droplets
            uint16_t pos = constrain(uint16_t(drops[j].pos) +i, 0, leds.nrOfLeds-1); //this is BAD, returns a pos >= leds.nrOfLeds occasionally
            leds.setPixelColor(invert?leds.nrOfLeds-1-pos:pos, blend(CRGB::Black, dropColor, drops[j].col/i)); //spread pixel with fade while falling
          }

          if (drops[j].colIndex > 2) {       // during bounce, some water is on the floor
            leds.setPixelColor(invert?leds.nrOfLeds-1:0, blend(dropColor, CRGB::Black, drops[j].col));
          }
        } else {                             // we hit bottom
          if (drops[j].colIndex > 2) {       // already hit once, so back to forming
            drops[j].colIndex = 0;
            // drops[j].col = sourcedrop;

          } else {

            if (drops[j].colIndex==2) {      // init bounce
              drops[j].vel = -drops[j].vel/4;// reverse velocity with damping
              drops[j].pos += drops[j].vel;
            }
            drops[j].col = sourcedrop*2;
            drops[j].colIndex = 5;           // bouncing
          }
        }
      }
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "gravity", leds.sharedData.write<uint8_t>(128), 1, 255);
    ui->initSlider(parentVar, "drips", leds.sharedData.write<uint8_t>(4), 1, 6);
    ui->initSlider(parentVar, "swell", leds.sharedData.write<uint8_t>(4), 1, 6);
    ui->initCheckBox(parentVar, "invert", leds.sharedData.write<bool>(false));
  }
}; // DripEffect

class HeartBeatEffect: public Effect {
  const char * name() {return "HeartBeat";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡💫♥";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t intensity = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    bool *isSecond = leds.sharedData.readWrite<bool>();
    uint16_t *bri_lower = leds.sharedData.readWrite<uint16_t>();
    unsigned long *step = leds.sharedData.readWrite<unsigned long>();

    uint8_t bpm = 40 + (speed);
    uint32_t msPerBeat = (60000L / bpm);
    uint32_t secondBeat = (msPerBeat / 3);
    unsigned long beatTimer = sys->now - *step;

    *bri_lower = *bri_lower * 2042 / (2048 + intensity);

    if ((beatTimer > secondBeat) && !*isSecond) { // time for the second beat?
      *bri_lower = UINT16_MAX; //3/4 bri
      *isSecond = true;
    }

    if (beatTimer > msPerBeat) { // time to reset the beat timer?
      *bri_lower = UINT16_MAX; //full bri
      *isSecond = false;
      *step = sys->now;
    }

    for (int i = 0; i < leds.nrOfLeds; i++) {
      leds.setPixelColor(i, ColorFromPalette(leds.palette, map(i, 0, leds.nrOfLeds, 0, 255), 255 - (*bri_lower >> 8)));
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(15), 0, 31);
    ui->initSlider(parentVar, "intensity", leds.sharedData.write<uint8_t>(128));
  }
}; // HeartBeatEffect

class FreqMatrix: public Effect {
  const char * name() {return "FreqMatrix";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "♪💡";}

  void setup(Leds &leds) {
    leds.fadeToBlackBy(16);
  }

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t fx = leds.sharedData.read<uint8_t>();
    uint8_t lowBin = leds.sharedData.read<uint8_t>();
    uint8_t highBin = leds.sharedData.read<uint8_t>();
    uint8_t sensitivity10 = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    uint8_t *aux0 = leds.sharedData.readWrite<uint8_t>();

    uint8_t secondHand = (speed < 255) ? (micros()/(256-speed)/500 % 16) : 0;
    if((speed > 254) || (*aux0 != secondHand)) {   // WLEDMM allow run run at full speed
      *aux0 = secondHand;

      // Pixel brightness (value) based on volume * sensitivity * intensity
      // uint_fast8_t sensitivity10 = map(sensitivity, 0, 31, 10, 100); // reduced resolution slider // WLEDMM sensitivity * 10, to avoid losing precision
      int pixVal = wledAudioMod->sync.volumeSmth * (float)fx * (float)sensitivity10 / 2560.0f; // WLEDMM 2560 due to sensitivity * 10
      if (pixVal > 255) pixVal = 255;  // make a brightness from the last avg

      CRGB color = CRGB::Black;

      if (wledAudioMod->sync.FFT_MajorPeak > MAX_FREQUENCY) wledAudioMod->sync.FFT_MajorPeak = 1;
      // MajorPeak holds the freq. value which is most abundant in the last sample.
      // With our sampling rate of 10240Hz we have a usable freq range from roughtly 80Hz to 10240/2 Hz
      // we will treat everything with less than 65Hz as 0

      if ((wledAudioMod->sync.FFT_MajorPeak > 80.0f) && (wledAudioMod->sync.volumeSmth > 0.25f)) { // WLEDMM
        // Pixel color (hue) based on major frequency
        int upperLimit = 80 + 42 * highBin;
        int lowerLimit = 80 + 3 * lowBin;
        //uint8_t i =  lowerLimit!=upperLimit ? map(FFT_MajorPeak, lowerLimit, upperLimit, 0, 255) : FFT_MajorPeak;  // (original formula) may under/overflow - so we enforce uint8_t
        int freqMapped =  lowerLimit!=upperLimit ? map(wledAudioMod->sync.FFT_MajorPeak, lowerLimit, upperLimit, 0, 255) : wledAudioMod->sync.FFT_MajorPeak;  // WLEDMM preserve overflows
        uint8_t i = abs(freqMapped) & 0xFF;  // WLEDMM we embrace overflow ;-) by "modulo 256"

        color = CHSV(i, 240, (uint8_t)pixVal); // implicit conversion to RGB supplied by FastLED
      }

      // shift the pixels one pixel up
      leds.setPixelColor(0, color);
      for (int i = leds.nrOfLeds - 1; i > 0; i--) leds.setPixelColor(i, leds.getPixelColor(i-1));
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(255));
    ui->initSlider(parentVar, "Sound effect", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "Low bin", leds.sharedData.write<uint8_t>(18));
    ui->initSlider(parentVar, "High bin", leds.sharedData.write<uint8_t>(48));
    ui->initSlider(parentVar, "Sensivity", leds.sharedData.write<uint8_t>(30), 10, 100);
  }
};

#define maxNumPopcorn 21 // max 21 on 16 segment ESP8266
#define NUM_COLORS       3 /* number of colors per segment */

class PopCorn: public Effect {
  const char * name() {return "PopCorn";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "♪💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t numPopcorn = leds.sharedData.read<uint8_t>();
    uint8_t useaudio = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers)
    Spark *popcorn = leds.sharedData.readWrite<Spark>(maxNumPopcorn); //array

    leds.fill_solid(CRGB::Black);

    float gravity = -0.0001f - (speed/200000.0f); // m/s/s
    gravity *= leds.nrOfLeds;

    if (numPopcorn == 0) numPopcorn = 1;

    for (int i = 0; i < numPopcorn; i++) {
      if (popcorn[i].pos >= 0.0f) { // if kernel is active, update its position
        popcorn[i].pos += popcorn[i].vel;
        popcorn[i].vel += gravity;
      } else { // if kernel is inactive, randomly pop it
        bool doPopCorn = false;  // WLEDMM allows to inhibit new pops
        // WLEDMM begin
        if (useaudio) {
          if (  (wledAudioMod->sync.volumeSmth > 1.0f)                      // no pops in silence
              // &&((wledAudioMod->sync.samplePeak > 0) || (wledAudioMod->sync.volumeRaw > 128))  // try to pop at onsets (our peek detector still sucks)
              &&(random8() < 4) )                        // stay somewhat random
            doPopCorn = true;
        } else {         
          if (random8() < 2) doPopCorn = true; // default POP!!!
        }
        // WLEDMM end

        if (doPopCorn) { // POP!!!
          popcorn[i].pos = 0.01f;

          uint16_t peakHeight = 128 + random8(128); //0-255
          peakHeight = (peakHeight * (leds.nrOfLeds -1)) >> 8;
          popcorn[i].vel = sqrtf(-2.0f * gravity * peakHeight);

          // if (SEGMENT.palette)
          // {
            popcorn[i].colIndex = random8();
          // } else {
          //   byte col = random8(0, NUM_COLORS);
          //   if (!SEGCOLOR(2) || !SEGCOLOR(col)) col = 0;
          //   popcorn[i].colIndex = col;
          // }
        }
      }
      if (popcorn[i].pos >= 0.0f) { // draw now active popcorn (either active before or just popped)
        // uint32_t col = SEGMENT.color_wheel(popcorn[i].colIndex);
        // if (!SEGMENT.palette && popcorn[i].colIndex < NUM_COLORS) col = SEGCOLOR(popcorn[i].colIndex);
        uint16_t ledIndex = popcorn[i].pos;
        CRGB col = ColorFromPalette(leds.palette, popcorn[i].colIndex*(256/maxNumPopcorn), 255);
        if (ledIndex < leds.nrOfLeds) leds.setPixelColor(ledIndex, col);
      }
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "corns", leds.sharedData.write<uint8_t>(maxNumPopcorn/2), 1, maxNumPopcorn);
    ui->initCheckBox(parentVar, "useaudio", leds.sharedData.write<bool>(false));
  }
}; //PopCorn

class NoiseMeter: public Effect {
  const char * name() {return "NoiseMeter";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "♪💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t fadeRate = leds.sharedData.read<uint8_t>();
    uint8_t width = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    uint8_t *aux0 = leds.sharedData.readWrite<uint8_t>();
    uint8_t *aux1 = leds.sharedData.readWrite<uint8_t>();

    leds.fadeToBlackBy(fadeRate);

    float tmpSound2 = wledAudioMod->sync.volumeRaw * 2.0 * (float)width / 255.0;
    int maxLen = map(tmpSound2, 0, 255, 0, leds.nrOfLeds); // map to pixels availeable in current segment              // Still a bit too sensitive.
    // if (maxLen <0) maxLen = 0;
    // if (maxLen >leds.nrOfLeds) maxLen = leds.nrOfLeds;

    for (int i=0; i<maxLen; i++) {                                    // The louder the sound, the wider the soundbar. By Andrew Tuline.
      uint8_t index = inoise8(i*wledAudioMod->sync.volumeSmth+*aux0, *aux1+i*wledAudioMod->sync.volumeSmth);  // Get a value from the noise function. I'm using both x and y axis.
      leds.setPixelColor(i, ColorFromPalette(leds.palette, index));//, 255, PALETTE_SOLID_WRAP));
    }

    *aux0+=beatsin8(5,0,10);
    *aux1+=beatsin8(4,0,10);
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "fadeRate", leds.sharedData.write<uint8_t>(248), 200, 254);
    ui->initSlider(parentVar, "width", leds.sharedData.write<uint8_t>(128));
  }
}; //NoiseMeter

class AudioRings: public RingEffect {
  const char * name() {return "AudioRings";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "♫💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    bool inWards = leds.sharedData.read<bool>();
    uint8_t nrOfRings = leds.sharedData.read<uint8_t>();

    for (int i = 0; i < nrOfRings; i++) {

      uint8_t band = map(i, 0, nrOfRings-1, 0, 15);

      byte val;
      if (inWards) {
        val = wledAudioMod->fftResults[band];
      }
      else {
        val = wledAudioMod->fftResults[15 - band];
      }
  
      // Visualize leds to the beat
      CRGB color = ColorFromPalette(leds.palette, val, val);
//      CRGB color = ColorFromPalette(currentPalette, val, 255, currentBlending);
//      color.nscale8_video(val);
      setRing(leds, i, color);
//        setRingFromFtt((i * 2), i); 
    }

    setRingFromFtt(leds, 2, 7); // set outer ring to bass
    setRingFromFtt(leds, 0, 8); // set outer ring to bass

  }
  void setRingFromFtt(Leds &leds, int index, int ring) {
    byte val = wledAudioMod->fftResults[index];
    // Visualize leds to the beat
    CRGB color = ColorFromPalette(leds.palette, val, 255);
    color.nscale8_video(val);
    setRing(leds, ring, color);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initCheckBox(parentVar, "inWards", leds.sharedData.write<bool>(true));
    ui->initSlider(parentVar, "rings", leds.sharedData.write<uint8_t>(7), 1, 50);
  }
};

class DJLight: public Effect {
  const char * name() {return "DJLight";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "♫💡";}

  void setup(Leds &leds) {
    leds.fill_solid(CRGB::Black, true); //no blend
  }

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    bool candyFactory = leds.sharedData.read<bool>();
    uint8_t fade = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    uint8_t *aux0 = leds.sharedData.readWrite<uint8_t>();

    const int mid = leds.nrOfLeds / 2;

    uint8_t *fftResult = wledAudioMod->fftResults;
    float volumeSmth   = wledAudioMod->volumeSmth;

    uint8_t secondHand = (speed < 255) ? (micros()/(256-speed)/500 % 16) : 0;
    if((speed > 254) || (*aux0 != secondHand)) {   // WLEDMM allow run run at full speed
      *aux0 = secondHand;

      CRGB color = CRGB(0,0,0);
      // color = CRGB(fftResult[15]/2, fftResult[5]/2, fftResult[0]/2);   // formula from 0.13.x (10Khz): R = 3880-5120, G=240-340, B=60-100
      if (!candyFactory) {
        color = CRGB(fftResult[12]/2, fftResult[3]/2, fftResult[1]/2);    // formula for 0.14.x  (22Khz): R = 3015-3704, G=216-301, B=86-129
      } else {
        // candy factory: an attempt to get more colors
        color = CRGB(fftResult[11]/2 + fftResult[12]/4 + fftResult[14]/4, // red  : 2412-3704 + 4479-7106 
                    fftResult[4]/2 + fftResult[3]/4,                     // green: 216-430
                    fftResult[0]/4 + fftResult[1]/4 + fftResult[2]/4);   // blue:  46-216
        if ((color.getLuma() < 96) && (volumeSmth >= 1.5f)) {             // enhance "almost dark" pixels with yellow, based on not-yet-used channels 
          unsigned yello_g = (fftResult[5] + fftResult[6] + fftResult[7]) / 3;
          unsigned yello_r = (fftResult[7] + fftResult[8] + fftResult[9] + fftResult[10]) / 4;
          color.green += (uint8_t) yello_g / 2;
          color.red += (uint8_t) yello_r / 2;
        }
      }

      if (volumeSmth < 1.0f) color = CRGB(0,0,0); // silence = black

      // make colors less "pastel", by turning up color saturation in HSV space
      if (color.getLuma() > 32) {                                      // don't change "dark" pixels
        CHSV hsvColor = rgb2hsv_approximate(color);
        hsvColor.v = min(max(hsvColor.v, (uint8_t)48), (uint8_t)204);  // 48 < brightness < 204
        if (candyFactory)
          hsvColor.s = max(hsvColor.s, (uint8_t)204);                  // candy factory mode: strongly turn up color saturation (> 192)
        else
          hsvColor.s = max(hsvColor.s, (uint8_t)108);                  // normal mode: turn up color saturation to avoid pastels
        color = hsvColor;
      }
      //if (color.getLuma() > 12) color.maximizeBrightness();          // for testing

      //leds.setPixelColor(mid, color.fadeToBlackBy(map(fftResult[4], 0, 255, 255, 4)));     // 0.13.x  fade -> 180hz-260hz
      uint8_t fadeVal = map(fftResult[3], 0, 255, 255, 4);                                      // 0.14.x  fade -> 216hz-301hz
      if (candyFactory) fadeVal = constrain(fadeVal, 0, 176);  // "candy factory" mode - avoid complete fade-out
      leds.setPixelColor(mid, color.fadeToBlackBy(fadeVal));

      for (int i = leds.nrOfLeds - 1; i > mid; i--)   leds.setPixelColor(i, leds.getPixelColor(i-1)); // move to the left
      for (int i = 0; i < mid; i++)            leds.setPixelColor(i, leds.getPixelColor(i+1)); // move to the right

      leds.fadeToBlackBy(fade);

    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(255));
    ui->initCheckBox(parentVar, "candyFactory", leds.sharedData.write<bool>(true));
    ui->initSlider(parentVar, "fade", leds.sharedData.write<uint8_t>(4), 0, 10);
  }
}; //DJLight




//2D Effects
//==========

class Lines: public Effect {
  const char * name() {return "Lines";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t bpm = leds.sharedData.read<uint8_t>();
    bool vertical = leds.sharedData.read<bool>();

    leds.fadeToBlackBy(100);

    Coord3D pos = {0,0,0};
    if (vertical) {
      pos.x = map(beat16( bpm), 0, UINT16_MAX, 0, leds.size.x-1 ); //instead of call%width

      for (pos.y = 0; pos.y <  leds.size.y; pos.y++) {
        leds[pos] = CHSV( gHue, 255, 255);
      }
    } else {
      pos.y = map(beat16( bpm), 0, UINT16_MAX, 0, leds.size.y-1 ); //instead of call%height
      for (pos.x = 0; pos.x <  leds.size.x; pos.x++) {
        leds[pos] = CHSV( gHue, 255, 255);
      }
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "BPM", leds.sharedData.write<uint8_t>(60));
    ui->initCheckBox(parentVar, "Vertical", leds.sharedData.write<bool>(true));
  }
}; // Lines

// By: Stepko https://editor.soulmatelights.com/gallery/1012 , Modified by: Andrew Tuline
class BlackHole: public Effect {
  const char * name() {return "BlackHole";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t fade = leds.sharedData.read<uint8_t>();
    uint8_t outX = leds.sharedData.read<uint8_t>();
    uint8_t outY = leds.sharedData.read<uint8_t>();
    uint8_t inX = leds.sharedData.read<uint8_t>();
    uint8_t inY = leds.sharedData.read<uint8_t>();

    uint16_t x, y;

    leds.fadeToBlackBy(16 + (fade)); // create fading trails
    unsigned long t = sys->now/128;                 // timebase
    // outer stars
    for (size_t i = 0; i < 8; i++) {
      x = beatsin8(outX,   0, leds.size.x - 1, 0, ((i % 2) ? 128 : 0) + t * i);
      y = beatsin8(outY, 0, leds.size.y - 1, 0, ((i % 2) ? 192 : 64) + t * i);
      leds.addPixelColor(leds.XY(x, y), CHSV(i*32, 255, 255));
    }
    // inner stars
    for (size_t i = 0; i < 4; i++) {
      x = beatsin8(inX, leds.size.x/4, leds.size.x - 1 - leds.size.x/4, 0, ((i % 2) ? 128 : 0) + t * i);
      y = beatsin8(inY, leds.size.y/4, leds.size.y - 1 - leds.size.y/4, 0, ((i % 2) ? 192 : 64) + t * i);
      leds.addPixelColor(leds.XY(x, y), CHSV(i*32, 255, 255));
    }
    // central white dot
    leds.setPixelColor(leds.XY(leds.size.x/2, leds.size.y/2), CHSV(0, 0, 255));

    // blur everything a bit
    leds.blur2d(16);

  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "fade", leds.sharedData.write<uint8_t>(16), 0, 32);
    ui->initSlider(parentVar, "outX", leds.sharedData.write<uint8_t>(16), 0, 32);
    ui->initSlider(parentVar, "outY", leds.sharedData.write<uint8_t>(16), 0, 32);
    ui->initSlider(parentVar, "inX", leds.sharedData.write<uint8_t>(16), 0, 32);
    ui->initSlider(parentVar, "inY", leds.sharedData.write<uint8_t>(16), 0, 32);
  }
}; // BlackHole

// dna originally by by ldirko at https://pastebin.com/pCkkkzcs. Updated by Preyy. WLED conversion by Andrew Tuline.
class DNA: public Effect {
  const char * name() {return "DNA";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💡💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t blur = leds.sharedData.read<uint8_t>();
    uint8_t phases = leds.sharedData.read<uint8_t>();

    leds.fadeToBlackBy(64);

    for (int i = 0; i < leds.size.x; i++) {
      //256 is a complete phase
      // half a phase is dna is 128
      uint8_t phase = leds.size.x * i / 8; 
      //32: 4 * i
      //16: 8 * i
      phase = i * 127 / (leds.size.x-1) * phases / 64;
      leds.setPixelColor(leds.XY(i, beatsin8(speed, 0, leds.size.y-1, 0, phase    )), ColorFromPalette(leds.palette, i*5+ sys->now /17, beatsin8(5, 55, 255, 0, i*10), LINEARBLEND));
      leds.setPixelColor(leds.XY(i, beatsin8(speed, 0, leds.size.y-1, 0, phase+128)), ColorFromPalette(leds.palette, i*5+128+ sys->now /17, beatsin8(5, 55, 255, 0, i*10+128), LINEARBLEND));
    }
    leds.blur2d(blur);
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(16), 0, 32);
    ui->initSlider(parentVar, "blur", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "phases", leds.sharedData.write<uint8_t>(64));
  }
}; // DNA


//DistortionWaves inspired by WLED, ldirko and blazoncek, https://editor.soulmatelights.com/gallery/1089-distorsion-waves
uint8_t gamma8(uint8_t b) { //we do nothing with gamma for now
  return b;
}
class DistortionWaves: public Effect {
  const char * name() {return "DistortionWaves";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>(); 
    uint8_t scale = leds.sharedData.read<uint8_t>(); 

    uint8_t  w = 2;

    uint16_t a  = sys->now/32;
    uint16_t a2 = a/2;
    uint16_t a3 = a/3;

    uint16_t cx =  beatsin8(10-speed,0,leds.size.x-1)*scale;
    uint16_t cy =  beatsin8(12-speed,0,leds.size.y-1)*scale;
    uint16_t cx1 = beatsin8(13-speed,0,leds.size.x-1)*scale;
    uint16_t cy1 = beatsin8(15-speed,0,leds.size.y-1)*scale;
    uint16_t cx2 = beatsin8(17-speed,0,leds.size.x-1)*scale;
    uint16_t cy2 = beatsin8(14-speed,0,leds.size.y-1)*scale;
    
    uint16_t xoffs = 0;
    Coord3D pos = {0,0,0};
    for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
      xoffs += scale;
      uint16_t yoffs = 0;

      for (pos.y = 0; pos.y < leds.size.y; pos.y++) {
        yoffs += scale;

        byte rdistort = cos8((cos8(((pos.x<<3)+a )&255)+cos8(((pos.y<<3)-a2)&255)+a3   )&255)>>1; 
        byte gdistort = cos8((cos8(((pos.x<<3)-a2)&255)+cos8(((pos.y<<3)+a3)&255)+a+32 )&255)>>1; 
        byte bdistort = cos8((cos8(((pos.x<<3)+a3)&255)+cos8(((pos.y<<3)-a) &255)+a2+64)&255)>>1; 

        byte valueR = rdistort+ w*  (a- ( ((xoffs - cx)  * (xoffs - cx)  + (yoffs - cy)  * (yoffs - cy))>>7  ));
        byte valueG = gdistort+ w*  (a2-( ((xoffs - cx1) * (xoffs - cx1) + (yoffs - cy1) * (yoffs - cy1))>>7 ));
        byte valueB = bdistort+ w*  (a3-( ((xoffs - cx2) * (xoffs - cx2) + (yoffs - cy2) * (yoffs - cy2))>>7 ));

        valueR = gamma8(cos8(valueR));
        valueG = gamma8(cos8(valueG));
        valueB = gamma8(cos8(valueB));

        leds[pos] = CRGB(valueR, valueG, valueB);
      }
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(4), 0, 8);
    ui->initSlider(parentVar, "scale", leds.sharedData.write<uint8_t>(4), 0, 8);
  }
}; // DistortionWaves

//Octopus inspired by WLED, Stepko and Sutaburosu and blazoncek 
//Idea from https://www.youtube.com/watch?v=HsA-6KIbgto&ab_channel=GreatScott%21 (https://editor.soulmatelights.com/gallery/671-octopus)
class Octopus: public Effect {
  const char * name() {return "Octopus";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💡";}

  struct Map_t {
    uint8_t angle;
    uint8_t radius;
  };

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t offsetX = leds.sharedData.read<uint8_t>();
    uint8_t offsetY = leds.sharedData.read<uint8_t>();
    uint8_t legs = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    Map_t    *rMap = leds.sharedData.readWrite<Map_t>(leds.size.x * leds.size.y); //array
    uint8_t *offsX = leds.sharedData.readWrite<uint8_t>();
    uint8_t *offsY = leds.sharedData.readWrite<uint8_t>();
    uint16_t *aux0 = leds.sharedData.readWrite<uint16_t>();
    uint16_t *aux1 = leds.sharedData.readWrite<uint16_t>();
    uint32_t *step = leds.sharedData.readWrite<uint32_t>();

    const uint8_t mapp = 180 / max(leds.size.x,leds.size.y);

    Coord3D pos = {0,0,0};

    // re-init if SEGMENT dimensions or offset changed
    if (*aux0 != leds.size.x || *aux1 != leds.size.y || offsetX != *offsX || offsetY != *offsY) {
      // *step = 0;
      *aux0 = leds.size.x;
      *aux1 = leds.size.y;
      *offsX = offsetX;
      *offsY = offsetY;
      const uint8_t C_X = leds.size.x / 2 + (offsetX - 128)*leds.size.x/255;
      const uint8_t C_Y = leds.size.y / 2 + (offsetY - 128)*leds.size.y/255;
      for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
        for (pos.y = 0; pos.y < leds.size.y; pos.y++) {
          rMap[leds.XY(pos.x, pos.y)].angle = 40.7436f * atan2f(pos.y - C_Y, pos.x - C_X); // avoid 128*atan2()/PI
          rMap[leds.XY(pos.x, pos.y)].radius = hypotf(pos.x - C_X, pos.y - C_Y) * mapp; //thanks Sutaburosu
        }
      }
    }

    *step = sys->now * speed / 32 / 10;//mdl->getValue("realFps").as<int>();  // WLEDMM 40fps

    for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
      for (pos.y = 0; pos.y < leds.size.y; pos.y++) {
        byte angle = rMap[leds.XY(pos.x,pos.y)].angle;
        byte radius = rMap[leds.XY(pos.x,pos.y)].radius;
        //CRGB c = CHSV(*step / 2 - radius, 255, sin8(sin8((angle * 4 - radius) / 4 + *step) + radius - *step * 2 + angle * (SEGMENT.custom3/3+1)));
        uint16_t intensity = sin8(sin8((angle * 4 - radius) / 4 + *step/2) + radius - *step + angle * legs);
        intensity = map(intensity*intensity, 0, UINT16_MAX, 0, 255); // add a bit of non-linearity for cleaner display
        CRGB color = ColorFromPalette(leds.palette, *step / 2 - radius, intensity);
        leds[pos] = color;
      }
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {

    Effect::controls(leds, parentVar); //palette

    // uint8_t *speed = ; 
    // uint8_t *offsetX = ; 
    // uint8_t *offsetY = ; 
    // uint8_t *legs = ; 

    // ppf("controls ptr %p,%p,%p,%p\n", speed, offsetX, offsetY, legs);
    // ppf("controls before %d,%d,%d,%d\n", *speed, *offsetX, *offsetY, *legs);
  
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(128), 1, 255);
    ui->initSlider(parentVar, "Offset X", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "Offset Y", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "Legs", leds.sharedData.write<uint8_t>(4), 1, 8);
    // ppf("controls %d,%d,%d,%d\n", *speed, *offsetX, *offsetY, *legs);
  }
}; // Octopus

//Lissajous inspired by WLED, Andrew Tuline 
class Lissajous: public Effect {
  const char * name() {return "Lissajous";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t freqX = leds.sharedData.read<uint8_t>();
    uint8_t fadeRate = leds.sharedData.read<uint8_t>();
    uint8_t speed = leds.sharedData.read<uint8_t>();
    bool smooth = leds.sharedData.read<bool>(); 

    leds.fadeToBlackBy(fadeRate);

    uint_fast16_t phase = sys->now * speed / 256;  // allow user to control rotation speed, speed between 0 and 255!

    Coord3D locn = {0,0,0};
    if (smooth) { // WLEDMM: this is the original "float" code featuring anti-aliasing
        int maxLoops = max(192U, 4U*(leds.size.x+leds.size.y));
        maxLoops = ((maxLoops / 128) +1) * 128; // make sure whe have half or full turns => multiples of 128
        for (int i=0; i < maxLoops; i++) {
          locn.x = float(sin8(phase/2 + (i* freqX)/64)) / 255.0f;  // WLEDMM align speed with original effect
          locn.y = float(cos8(phase/2 + i*2)) / 255.0f;
          //leds.setPixelColorXY(xlocn, ylocn, SEGMENT.color_from_palette(sys->now/100+i, false, PALETTE_SOLID_WRAP, 0)); // draw pixel with anti-aliasing
          unsigned palIndex = (256*locn.y) + phase/2 + (i* freqX)/64;
          // leds.setPixelColorXY(xlocn, ylocn, SEGMENT.color_from_palette(palIndex, false, PALETTE_SOLID_WRAP, 0)); // draw pixel with anti-aliasing - color follows rotation
          leds[locn] = ColorFromPalette(leds.palette, palIndex);
        }
    } else
    for (int i=0; i < 256; i ++) {
      //WLEDMM: stick to the original calculations of xlocn and ylocn
      locn.x = sin8(phase/2 + (i*freqX)/64);
      locn.y = cos8(phase/2 + i*2);
      locn.x = (leds.size.x < 2) ? 1 : (map(2*locn.x, 0,511, 0,2*(leds.size.x-1)) +1) /2;    // softhack007: "*2 +1" for proper rounding
      locn.y = (leds.size.y < 2) ? 1 : (map(2*locn.y, 0,511, 0,2*(leds.size.y-1)) +1) /2;    // "leds.size.y > 2" is needed to avoid div/0 in map()
      // leds.setPixelColorXY((uint8_t)xlocn, (uint8_t)ylocn, SEGMENT.color_from_palette(sys->now/100+i, false, PALETTE_SOLID_WRAP, 0));
      leds[locn] = ColorFromPalette(leds.palette, sys->now/100+i);
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);

    // uint8_t *freqX = ; 
    // uint8_t *fadeRate = ; 
    // uint8_t *speed = ; 
    // bool *smooth = ; 

    ui->initSlider(parentVar, "x frequency", leds.sharedData.write<uint8_t>(64));
    ui->initSlider(parentVar, "fade rate", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(128));
    ui->initCheckBox(parentVar, "smooth", leds.sharedData.write<bool>(false));
  }
}; // Lissajous

//Frizzles inspired by WLED, Stepko, Andrew Tuline, https://editor.soulmatelights.com/gallery/640-color-frizzles
class Frizzles: public Effect {
  const char * name() {return "Frizzles";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t bpm = leds.sharedData.read<uint8_t>();
    uint8_t intensity = leds.sharedData.read<uint8_t>();
    uint8_t blur = leds.sharedData.read<uint8_t>();

    leds.fadeToBlackBy(16);

    for (int i = 8; i > 0; i--) {
      Coord3D pos = {0,0,0};
      pos.x = beatsin8(bpm/8 + i, 0, leds.size.x - 1);
      pos.y = beatsin8(intensity/8 - i, 0, leds.size.y - 1);
      CRGB color = ColorFromPalette(leds.palette, beatsin8(12, 0, 255), 255);
      leds[pos] = color;
    }
    leds.blur2d(blur);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "BPM", leds.sharedData.write<uint8_t>(60));
    ui->initSlider(parentVar, "intensity", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "blur", leds.sharedData.write<uint8_t>(128));
  }
}; // Frizzles

class ScrollingText: public Effect {
  const char * name() {return "Scrolling Text";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t font = leds.sharedData.read<uint8_t>();
    const char * text = mdl->getValue("text"); //sharedData to be implemented!

    // text might be nullified by selecting other effects and if effect is selected, controls are run afterwards  
    // tbd: this should be removed and fx.changeFUn (setEffect) must make sure this cannot happen!!
    if (text && strlen(text)>0) {
      leds.fadeToBlackBy();
      leds.drawText(text, 0, 0, font, CRGB::Red, - (call*speed/256));
    }

  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initText(parentVar, "text", "StarLeds"); //sharedData to be implemented!
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(128));
    ui->initSelect(parentVar, "font", leds.sharedData.write<uint8_t>(0), false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun: {
        JsonArray options = ui->setOptions(var);
        options.add("4x6");
        options.add("5x8");
        options.add("5x12");
        options.add("6x8");
        options.add("7x9");
        return true;
      }
      default: return false;
    }});
  }
}; //ScrollingText

class Noise2D: public Effect {
  const char * name() {return "Noise2D";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t scale = leds.sharedData.read<uint8_t>();

    for (int y = 0; y < leds.size.y; y++) {
      for (int x = 0; x < leds.size.x; x++) {
        uint8_t pixelHue8 = inoise8(x * scale, y * scale, sys->now / (16 - speed));
        leds.setPixelColor(leds.XY(x, y), ColorFromPalette(leds.palette, pixelHue8));
      }
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);

    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(8), 0, 15);
    ui->initSlider(parentVar, "scale", leds.sharedData.write<uint8_t>(128), 2, 255);
  }
}; //Noise2D


static bool getBitValue(const byte* byteArray, size_t n) {
    size_t byteIndex = n / 8;
    size_t bitIndex = n % 8;
    uint8_t byte = byteArray[byteIndex];
    return (byte >> bitIndex) & 1;
}
static void setBitValue(byte* byteArray, size_t n, bool value) {
    size_t byteIndex = n / 8;
    size_t bitIndex = n % 8;
    if (value)
        byteArray[byteIndex] |= (1 << bitIndex);
    else
        byteArray[byteIndex] &= ~(1 << bitIndex);
}
//utiltity function?
uint16_t crc16(const unsigned char* data_p, size_t length) {
  uint8_t x;
  uint16_t crc = 0xFFFF;
  if (!length) return 0x1D0F;
  while (length--) {
    x = crc >> 8 ^ *data_p++;
    x ^= x>>4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
  }
  return crc;
}

uint16_t gcd(uint16_t a, uint16_t b) {
  while (b != 0) {
    uint16_t t = b;
    b = a % b;
    a = t;
  }
  return a;
}

uint16_t lcm(uint16_t a, uint16_t b) {
  return a / gcd(a, b) * b;
}

// Written by Ewoud Wijma in 2022, inspired by https://natureofcode.com/book/chapter-7-cellular-automata/ and https://github.com/DougHaber/nlife-color ,
// Modified By: Brandon Butler in 2024
class GameOfLife: public Effect {
  const char * name() {return "GameOfLife";}
  uint8_t dim() {return _3D;} //supports 3D but also 2D (1D as well?)
  const char * tags() {return "💡💫";}

  void drawGrid(Leds &leds, byte *cells, CRGB bgColor, byte overlay = 0, bool drawAliveRandomColor = false, bool drawDead =false, bool blur = false) {
    // Redraws the grid. drawAliveRandomColor is used when palettes change or new game is started drawDead is used to blur dead cells further
    for (int x = 0; x < leds.size.x; x++) for (int y = 0; y < leds.size.y; y++) for (int z = 0; z < leds.size.z; z++){
      if (!leds.isMapped(leds.XYZNoSpin({x,y,z}))) continue;
      bool alive = getBitValue(cells, leds.XYZNoSpin({x,y,z}));
      if (overlay) {
        if (overlay == 1 && alive) leds.setPixelColor({x,y,z}, bgColor, 0);
        else if (overlay == 2 && !alive) leds.setPixelColor({x,y,z}, bgColor);
        continue;
      }
      else if (drawAliveRandomColor && alive) leds.setPixelColor({x,y,z}, ColorFromPalette(leds.palette, random8()), 0);
      else if (drawDead && !alive) blur ? leds.setPixelColor({x,y,z}, bgColor) : leds.setPixelColor({x,y,z}, bgColor, 0);
    }
  }

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    byte ruleset      = leds.sharedData.read<byte>();
    uint8_t speed     = leds.sharedData.read<uint8_t>();
    byte lifeChance   = leds.sharedData.read<byte>();
    uint8_t mutation  = leds.sharedData.read<uint8_t>();
    bool wrap         = leds.sharedData.read<bool>();
    bool disablePause = leds.sharedData.read<bool>();

    //Binding of loop persistent values (pointers)
    const uint16_t dataSize = ((leds.size.x * leds.size.y * leds.size.z + 7) / 8);
    uint16_t *gliderLength     = leds.sharedData.readWrite<uint16_t>();
    uint16_t *cubeGliderLength = leds.sharedData.readWrite<uint16_t>();
    uint16_t *oscillatorCRC    = leds.sharedData.readWrite<uint16_t>();
    uint16_t *spaceshipCRC     = leds.sharedData.readWrite<uint16_t>();
    uint16_t *cubeGliderCRC    = leds.sharedData.readWrite<uint16_t>();
    byte *cells       = leds.sharedData.readWrite<byte>(dataSize);
    byte *futureCells = leds.sharedData.readWrite<byte>(dataSize);
    uint16_t *generation = leds.sharedData.readWrite<uint16_t>();
    unsigned long *step  = leds.sharedData.readWrite<unsigned long>();
    bool *birthNumbers   = leds.sharedData.readWrite<bool>(9);
    bool *surviveNumbers = leds.sharedData.readWrite<bool>(9);
    byte *prevRuleset    = leds.sharedData.readWrite<byte>();
    byte *setUp       = leds.sharedData.readWrite<byte>(); // call == 0 not working temp fix
    CRGB *prevPalette = leds.sharedData.readWrite<CRGB>();

    CRGB bgColor = CRGB::Black;
    CRGB color   = ColorFromPalette(leds.palette, random8()); // Used if all parents died

    // Start New Game of Life
    if (call == 0  || *setUp != 123|| (*generation == 0 && *step < sys->now)) {
      *setUp = 123; // quick fix for effect starting up
      *prevPalette = ColorFromPalette(leds.palette, 0);
      *generation = 1;
      disablePause ? *step = sys->now : *step = sys->now + 1500;

      // Setup Grid
      memset(cells, 0, dataSize);
      for (int x = 0; x < leds.size.x; x++) for (int y = 0; y < leds.size.y; y++) for (int z = 0; z < leds.size.z; z++){
        if (leds.projectionDimension == _3D && !leds.isMapped(leds.XYZNoSpin({x,y,z}))) continue;
        if (random8(100) < lifeChance) setBitValue(cells, leds.XYZNoSpin({x,y,z}), true);
      }
      memcpy(futureCells, cells, dataSize); 
      drawGrid(leds, cells, bgColor, false, true, true, false);

      // Change CRCs
      uint16_t crc = crc16((const unsigned char*)cells, dataSize);
      *oscillatorCRC = crc;
      *spaceshipCRC  = crc;
      *cubeGliderCRC = crc;
      *gliderLength  = lcm(leds.size.y, leds.size.x) * 4;
      *cubeGliderLength = *gliderLength * 6; // change later for rectangular cuboid
      return;
    }

    byte blendVal = leds.fixture->globalBlend; // Used for different blend mode
    bool bgBlendMode = blendVal > 200;
    if (*prevPalette != ColorFromPalette(leds.palette, 0)) {   // Palette changed, redraw grid
      drawGrid(leds, cells, bgColor, false, true, true);
      *prevPalette = ColorFromPalette(leds.palette, 0);
    } 
    if (*step > sys->now && !bgBlendMode) drawGrid(leds, cells, bgColor, 0, false, true, true); // Blend dead cells while paused

    if (!speed || *step > sys->now || sys->now - *step < 1000 / speed) return; // Check if enough time has passed for updating

    //Rule set for game of life
    if (ruleset != *prevRuleset || ruleset == 0) { // Custom rulestring always parsed
      String ruleString = "";
      if      (ruleset == 0) ruleString = mdl->getValue("Custom Rule String").as<String>(); //Custom
      else if (ruleset == 1) ruleString = "B3/S23";         //Conway's Game of Life
      else if (ruleset == 2) ruleString = "B36/S23";        //HighLife
      else if (ruleset == 3) ruleString = "B0123478/S34678";//InverseLife
      else if (ruleset == 4) ruleString = "B3/S12345";      //Maze
      else if (ruleset == 5) ruleString = "B3/S1234";       //Mazecentric
      else if (ruleset == 6) ruleString = "B367/S23";       //DrighLife

      *prevRuleset = ruleset;
      memset(birthNumbers, 0, sizeof(bool) * 9);
      memset(surviveNumbers, 0, sizeof(bool) * 9);

      //Rule String Parsing
      int slashIndex = ruleString.indexOf('/');
      for (int i = 0; i < ruleString.length(); i++) {
        int num = ruleString.charAt(i) - '0';
        if (num >= 0 && num < 9) {
          if (i < slashIndex) birthNumbers[num] = true;
          else surviveNumbers[num] = true;
        }
      }
    }
    //Update Game of Life
    bool cellChanged = false; // Detect still live and dead grids

    //Loop through all cells. Count neighbors, apply rules, setPixel
    for (int x = 0; x < leds.size.x; x++) for (int y = 0; y < leds.size.y; y++) for (int z = 0; z < leds.size.z; z++){
      Coord3D cPos = {x, y, z}; //current cells position
      uint16_t cIndex = leds.XYZNoSpin(cPos);
      if (leds.projectionDimension == _3D && !leds.isMapped(leds.XYZ(x,y,z))) continue; //skip if not physical led
      byte neighbors = 0;
      byte colorCount = 0; //track number of valid colors
      CRGB nColors[9];     //track up to 9 colors (3D / alt ruleset), dying cells may overwrite but this wont be used

      for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) for (int k = -1; k <= 1; k++) { // iterate through 3*3*3 matrix
        if (i==0 && j==0 && k==0) continue; // ignore itself
        Coord3D nPos = {x+i, y+j, z+k};     // neighbor position
        if (!wrap || leds.size.z > 1 || (*generation) % 1500 == 0) { //no wrap, never wrap 3D, disable wrap every 1500 generations to prevent undetected repeats
          if (nPos.isOutofBounds(leds.size)) continue;
        } else { // wrap around 2D
          if (k != 0) continue; //no z axis (wrap around only for x and y
          nPos = (nPos + leds.size) % leds.size;
        }
        uint16_t nIndex = leds.XYZNoSpin(nPos);
        // count neighbors and store up to 9 neighbor colors
        if (getBitValue(cells, nIndex)) { //if alive
          neighbors++;
          if (!getBitValue(futureCells, nIndex)) continue; //skip if parent died in this loop (color lost or blended)
          nColors[colorCount % 9] = leds.getPixelColor(nPos);
          colorCount++;
        }
      }
      // Rules of Life
      bool cellValue = getBitValue(cells, cIndex);
      if (cellValue && !surviveNumbers[neighbors]) {
        // Loneliness or Overpopulation
        cellChanged = true;
        setBitValue(futureCells, cIndex, false);
        leds.setPixelColor(cPos, bgColor, bgBlendMode ? blendVal - 190 : blendVal);
      }
      else if (!cellValue && birthNumbers[neighbors]){
        // Reproduction
        setBitValue(futureCells, cIndex, true);
        cellChanged = true;
        CRGB randomParentColor = color; // last seen color, overwrite if colors are found
        if (colorCount) randomParentColor = nColors[random8(colorCount)];
        if (randomParentColor == bgColor) randomParentColor = ColorFromPalette(leds.palette, random8()); // needed for tilt, pan, roll
        if (random8(100) < mutation) randomParentColor = ColorFromPalette(leds.palette, random8());      // mutate
        leds.setPixelColor(cPos, randomParentColor, 0);
      }
      else {
        // Blending, fade dead cells further causing blurring effect to moving cells
        if (!cellValue && !bgBlendMode) leds.setPixelColor(cPos, bgColor);
      }
    }

    // Update cell values from futureCells
    memcpy(cells, futureCells, dataSize);
    // Get current crc value
    uint16_t crc = crc16((const unsigned char*)cells, dataSize);

    bool repetition = false;
    if (!cellChanged || crc == *oscillatorCRC || crc == *spaceshipCRC || crc == *cubeGliderCRC) repetition = true; //check if cell changed this gen and compare previous stored crc values
    if (repetition) {
      *generation = 0; //reset on next call
      disablePause ? *step = sys->now : *step = sys->now + 1000;
      return;
    }
    // Update CRC values
    if (*generation % 16 == 0) *oscillatorCRC = crc;
    if (*gliderLength     && *generation % *gliderLength     == 0) *spaceshipCRC = crc;
    if (*cubeGliderLength && *generation % *cubeGliderLength == 0) *cubeGliderCRC = crc;
    (*generation)++;
    *step = sys->now;
  }

  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSelect(parentVar, "ruleset", leds.sharedData.write<uint8_t>(1), false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) {
      case f_UIFun: {
        JsonArray options = ui->setOptions(var);
        options.add("Custom B/S");
        options.add("Conway's Game of Life B3/S23");
        options.add("HighLife B36/S23");
        options.add("InverseLife B0123478/S34678");
        options.add("Maze B3/S12345");
        options.add("Mazecentric B3/S1234");
        options.add("DrighLife B367/S23");
        return true;
      }
      default: return false;
    }});
    ui->initText(parentVar, "Custom Rule String", "B/S");
    ui->initSlider(parentVar, "Game Speed (FPS)", leds.sharedData.write<uint8_t>(20), 0, 60);    
    ui->initSlider(parentVar, "Starting Life Density", leds.sharedData.write<uint8_t>(32), 10, 90);
    ui->initSlider(parentVar, "Mutation Chance", leds.sharedData.write<uint8_t>(2), 0, 100);    
    ui->initCheckBox(parentVar, "Wrap", leds.sharedData.write<bool>(true));
    ui->initCheckBox(parentVar, "Disable Pause", leds.sharedData.write<bool>(false));
  }
}; //GameOfLife

#ifdef STARLEDS_USERMOD_WLEDAUDIO

class Waverly: public Effect {
  const char * name() {return "Waverly";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "♪💡";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t amplification = leds.sharedData.read<uint8_t>();
    uint8_t sensitivity = leds.sharedData.read<uint8_t>();
    bool noClouds = leds.sharedData.read<bool>();
    // bool soundPressure = leds.sharedData.read<bool>();
    // bool agcDebug = leds.sharedData.read<bool>();

    leds.fadeToBlackBy(amplification);
    // if (agcDebug && soundPressure) soundPressure = false;                 // only one of the two at any time
    // if ((soundPressure) && (wledAudioMod->sync.volumeSmth > 0.5f)) wledAudioMod->sync.volumeSmth = wledAudioMod->sync.soundPressure;    // show sound pressure instead of volume
    // if (agcDebug) wledAudioMod->sync.volumeSmth = 255.0 - wledAudioMod->sync.agcSensitivity;                    // show AGC level instead of volume

    long t = sys->now / 2; 
    Coord3D pos;
    for (pos.x = 0; pos.x < leds.size.x; pos.x++) {
      uint16_t thisVal = wledAudioMod->sync.volumeSmth*sensitivity/64 * inoise8(pos.x * 45 , t , t)/64;      // WLEDMM back to SR code
      uint16_t thisMax = min(map(thisVal, 0, 512, 0, leds.size.y), (long)leds.size.x);

      for (pos.y = 0; pos.y < thisMax; pos.y++) {
        CRGB color = ColorFromPalette(leds.palette, map(pos.y, 0, thisMax, 250, 0), 255, LINEARBLEND);
        if (!noClouds)
          leds.addPixelColor(pos, color);
        leds.addPixelColor(leds.XY((leds.size.x - 1) - pos.x, (leds.size.y - 1) - pos.y), color);
      }
    }
    leds.blur2d(16);

  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "Amplification", leds.sharedData.write<uint8_t>(128));
    ui->initSlider(parentVar, "Sensitivity", leds.sharedData.write<uint8_t>(128));
    ui->initCheckBox(parentVar, "No Clouds", leds.sharedData.write<bool>(false));
    // ui->initCheckBox(parentVar, "Sound Pressure", leds.sharedData.write<bool>(false));
    // ui->initCheckBox(parentVar, "AGC debug", leds.sharedData.write<bool>(false));
  }
}; //Waverly

class GEQEffect: public Effect {
  const char * name() {return "GEQ";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "♫💡";}

  void setup(Leds &leds) {
    leds.fadeToBlackBy(16);
  }

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t fadeOut = leds.sharedData.read<uint8_t>();
    uint8_t ripple = leds.sharedData.read<uint8_t>();
    bool colorBars = leds.sharedData.read<bool>();
    bool smoothBars = leds.sharedData.read<bool>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    uint16_t *previousBarHeight = leds.sharedData.readWrite<uint16_t>(leds.size.x); //array
    unsigned long *step = leds.sharedData.readWrite<unsigned long>();

    const int NUM_BANDS = NUM_GEQ_CHANNELS ; // map(SEGMENT.custom1, 0, 255, 1, 16);

    #ifdef SR_DEBUG
    uint8_t samplePeak = *(uint8_t*)um_data->u_data[3];
    #endif

    bool rippleTime = false;
    if (sys->now - *step >= (256U - ripple)) {
      *step = sys->now;
      rippleTime = true;
    }

    int fadeoutDelay = (256 - fadeOut) / 64; //256..1 -> 4..0
    size_t beat = map(beat16( fadeOut), 0, UINT16_MAX, 0, fadeoutDelay-1 ); // instead of call%fadeOutDelay

    if ((fadeoutDelay <= 1 ) || (beat == 0)) leds.fadeToBlackBy(fadeOut);

    uint16_t lastBandHeight = 0;  // WLEDMM: for smoothing out bars

    //evenly distribute see also Funky Plank/By ewowi/From AXI
    float bandwidth = (float)leds.size.x / NUM_BANDS;
    float remaining = bandwidth;
    uint8_t band = 0;
    Coord3D pos = {0,0,0};
    for (pos.x=0; pos.x < leds.size.x; pos.x++) {
      //WLEDMM if not enough remaining
      if (remaining < 1) {band++; remaining+= bandwidth;} //increase remaining but keep the current remaining
      remaining--; //consume remaining

      // ppf("x %d b %d n %d w %f %f\n", x, band, NUM_BANDS, bandwidth, remaining);
      uint8_t frBand = ((NUM_BANDS < 16) && (NUM_BANDS > 1)) ? map(band, 0, NUM_BANDS - 1, 0, 15):band; // always use full range. comment out this line to get the previous behaviour.
      // frBand = constrain(frBand, 0, 15); //WLEDMM can never be out of bounds (I think...)
      uint16_t colorIndex = frBand * 17; //WLEDMM 0.255
      uint16_t bandHeight = wledAudioMod->fftResults[frBand];  // WLEDMM we use the original ffResult, to preserve accuracy

      // WLEDMM begin - smooth out bars
      if ((pos.x > 0) && (pos.x < (leds.size.x-1)) && (smoothBars)) {
        // get height of next (right side) bar
        uint8_t nextband = (remaining < 1)? band +1: band;
        nextband = constrain(nextband, 0, 15);  // just to be sure
        frBand = ((NUM_BANDS < 16) && (NUM_BANDS > 1)) ? map(nextband, 0, NUM_BANDS - 1, 0, 15):nextband; // always use full range. comment out this line to get the previous behaviour.
        uint16_t nextBandHeight = wledAudioMod->fftResults[frBand];
        // smooth Band height
        bandHeight = (7*bandHeight + 3*lastBandHeight + 3*nextBandHeight) / 12;   // yeees, its 12 not 13 (10% amplification)
        bandHeight = constrain(bandHeight, 0, 255);   // remove potential over/underflows
        colorIndex = map(pos.x, 0, leds.size.x-1, 0, 255); //WLEDMM
      }
      lastBandHeight = bandHeight; // remember BandHeight (left side) for next iteration
      uint16_t barHeight = map(bandHeight, 0, 255, 0, leds.size.y); // Now we map bandHeight to barHeight. do not subtract -1 from leds.size.y here
      // WLEDMM end

      if (barHeight > leds.size.y) barHeight = leds.size.y;                      // WLEDMM map() can "overshoot" due to rounding errors
      if (barHeight > previousBarHeight[pos.x]) previousBarHeight[pos.x] = barHeight; //drive the peak up

      CRGB ledColor = CRGB::Black;

      for (pos.y=0; pos.y < barHeight; pos.y++) {
        if (colorBars) //color_vertical / color bars toggle
          colorIndex = map(pos.y, 0, leds.size.y-1, 0, 255);

        ledColor = ColorFromPalette(leds.palette, (uint8_t)colorIndex);

        leds.setPixelColor(leds.XY(pos.x, leds.size.y - 1 - pos.y), ledColor);
      }

      if ((ripple > 0) && (previousBarHeight[pos.x] > 0) && (previousBarHeight[pos.x] < leds.size.y))  // WLEDMM avoid "overshooting" into other segments
        leds.setPixelColor(leds.XY(pos.x, leds.size.y - previousBarHeight[pos.x]), CHSV( gHue, 255, 255)); // take gHue color for the time being

      if (rippleTime && previousBarHeight[pos.x]>0) previousBarHeight[pos.x]--;    //delay/ripple effect

    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "fadeOut", leds.sharedData.write<uint8_t>(255));
    ui->initSlider(parentVar, "ripple", leds.sharedData.write<uint8_t>(128));
    ui->initCheckBox(parentVar, "colorBars", leds.sharedData.write<bool>(false));
    ui->initCheckBox(parentVar, "smoothBars", leds.sharedData.write<bool>(true));

    // Nice an effect can register it's own DMX channel, but not a fan of repeating the range and type of the param

    // #ifdef STARBASE_USERMOD_E131

    //   if (e131mod->isEnabled) {
    //     e131mod->patchChannel(3, "fadeOut", 255); // TODO: add constant for name
    //     e131mod->patchChannel(4, "ripple", 255);
    //     for (JsonObject childVar: mdl->findVar("e131Tbl")["n"].as<JsonArray>()) {
    //       ui->callVarFun(childVar, UINT8_MAX, f_UIFun);
    //     }
    //   }

    // #endif
  }
}; //GEQ

class FunkyPlank: public Effect {
  const char * name() {return "Funky Plank";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "♫💡💫";}

  void setup(Leds &leds) {
    leds.fill_solid(CRGB::Black, true); //no blend
  }

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t num_bands = leds.sharedData.read<uint8_t>();

    //binding of loop persistent values (pointers) tbd: aux0,1,step etc can be renamed to meaningful names
    uint8_t *aux0 = leds.sharedData.readWrite<uint8_t>();

    uint8_t secondHand = (speed < 255) ? (micros()/(256-speed)/500 % 16) : 0;
    if ((speed > 254) || (*aux0 != secondHand)) {   // WLEDMM allow run run at full speed
      *aux0 = secondHand;

      //evenly distribute see also GEQ/By ewowi/From AXI
      float bandwidth = (float)leds.size.x / num_bands;
      float remaining = bandwidth;
      uint8_t band = 0;
      for (int posx=0; posx < leds.size.x; posx++) {
        if (remaining < 1) {band++; remaining += bandwidth;} //increase remaining but keep the current remaining
        remaining--; //consume remaining

        int hue = wledAudioMod->fftResults[map(band, 0, num_bands-1, 0, 15)];
        int v = map(hue, 0, 255, 10, 255);
        leds.setPixelColor(leds.XY(posx, 0), CHSV(hue, 255, v));
      }

      // drip down:
      for (int i = (leds.size.y - 1); i > 0; i--) {
        for (int j = (leds.size.x - 1); j >= 0; j--) {
          leds.setPixelColor(leds.XY(j, i), leds.getPixelColor(leds.XY(j, i-1)));
        }
      }
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(255));
    ui->initSlider(parentVar, "bands", leds.sharedData.write<uint8_t>(NUM_GEQ_CHANNELS), 1, NUM_GEQ_CHANNELS);
  }
}; //FunkyPlank


#endif // End Audio Effects


//3D Effects
//==========

class RipplesEffect: public Effect {
  const char * name() {return "Ripples";}
  uint8_t dim() {return _3D;}
  const char * tags() {return "💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();
    uint8_t interval = leds.sharedData.read<uint8_t>();

    float ripple_interval = 1.3f * (interval/128.0f);

    leds.fill_solid(CRGB::Black);

    Coord3D pos = {0,0,0};
    for (pos.z=0; pos.z<leds.size.z; pos.z++) {
      for (pos.x=0; pos.x<leds.size.x; pos.x++) {
        float d = distance(3.5f, 3.5f, 0.0f, (float)pos.y, (float)pos.z, 0.0f) / 9.899495f * leds.size.y;
        uint32_t time_interval = sys->now/(100 - speed)/((256.0f-128.0f)/20.0f);
        pos.y = floor(leds.size.y/2.0f + sinf(d/ripple_interval + time_interval) * leds.size.y/2.0f); //between 0 and leds.size.y

        leds[pos] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(leds.palette,call, bri, LINEARBLEND);
      }
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(50), 0, 99);
    ui->initSlider(parentVar, "interval", leds.sharedData.write<uint8_t>(128));
  }
};

class SphereMoveEffect: public Effect {
  const char * name() {return "SphereMove";}
  uint8_t dim() {return _3D;}
  const char * tags() {return "💫";}

  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t speed = leds.sharedData.read<uint8_t>();

    leds.fill_solid(CRGB::Black);

    uint32_t time_interval = sys->now/(100 - speed)/((256.0f-128.0f)/20.0f);

    Coord3D origin;
    origin.x = 3.5f+sinf(time_interval)*2.5f;
    origin.y = 3.5f+cosf(time_interval)*2.5f;
    origin.z = 3.5f+cosf(time_interval)*2.0f;

    float diameter = 2.0f+sinf(time_interval/3.0f);

    Coord3D pos;
    for (pos.x=0; pos.x<leds.size.x; pos.x++) {
        for (pos.y=0; pos.y<leds.size.y; pos.y++) {
            for (pos.z=0; pos.z<leds.size.z; pos.z++) {
                uint16_t d = distance(pos.x, pos.y, pos.z, origin.x, origin.y, origin.z);

                if (d>diameter && d<diameter+1) {
                  leds[pos] = CHSV( gHue + random8(64), 200, 255);// ColorFromPalette(leds.palette,call, bri, LINEARBLEND);
                }
            }
        }
    }
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "speed", leds.sharedData.write<uint8_t>(50), 0, 99);
  }
}; // SphereMove3DEffect

/*
#define unsigned8 uint8_t
#define unsigned16 uint16_t
#define unsigned32 unsigned
#define forUnsigned8 unsigned
#define forUnsigned16 unsigned
#define stackUnsigned8 uint8_t //unsigned causes some effects to run faulty. tbd to find out
#define stackUnsigned16 unsigned
#define stackUnsigned32 unsigned
*/
class Praxis: public Effect {
public:
  const char * name() {return "Praxis";}
  uint8_t dim() {return _2D;}
  const char * tags() {return "🌌";}

  void setup(Leds &leds) {
    leds.fill_solid(CRGB::Black);
  }
  
  void loop(Leds &leds) {
    //Binding of controls. Keep before binding of vars and keep in same order as in controls()
    uint8_t huespeed = leds.sharedData.read<uint8_t>();
    uint8_t macro_mutator_freq = leds.sharedData.read<uint8_t>(); // 14, -125, -1
    uint8_t macro_mutator_min = leds.sharedData.read<uint8_t>();
    uint8_t macro_mutator_max = leds.sharedData.read<uint8_t>();
    uint8_t micro_mutator_freq = leds.sharedData.read<uint8_t>(); // 2, 550, 900
    uint8_t micro_mutator_min = leds.sharedData.read<uint8_t>();
    uint8_t micro_mutator_max = leds.sharedData.read<uint8_t>();
    uint8_t saturation = leds.sharedData.read<uint8_t>();
    uint16_t macro_mutator = beatsin16(macro_mutator_freq, -macro_mutator_min, -macro_mutator_max, sys->now); // , sys->now
    uint16_t micro_mutator = beatsin16(micro_mutator_freq, micro_mutator_min*3, micro_mutator_max*3, sys->now); // , sys->now

    // Loop persistent values
    uint8_t huebase = sys->now/(255 - huespeed);

    Coord3D pos = {0,0,0};

    for (pos.x = 0; pos.x < (leds.size.x+1)/2; pos.x++){   // Halved for mirroring
      for(pos.y = 0; pos.y < (leds.size.y+1)/2; pos.y++){
        //uint8_t hue = huebase + ((pos.x+pos.y)*(250-macro_mutator)/5) + ((pos.x+pos.y*macro_mutator*pos.x)/micro_mutator);
        uint8_t hue = ((pos.x+pos.y)*(250-macro_mutator)/5) + ((pos.x+pos.y*macro_mutator*pos.x)/micro_mutator);
        CRGB colour = ColorFromPalette(leds.palette, hue, saturation, LINEARBLEND);
        colour = blend(leds.getPixelColor(leds.XY(pos.x, pos.y)), colour, 4);
        leds[leds.XY(pos.x, pos.y)] = colour;
        leds[leds.XY(pos.x, leds.size.y - 1 - pos.y)] = colour;
        leds[leds.XY(leds.size.x - 1 - pos.x, pos.y)] = colour;
        leds[leds.XY(leds.size.x - 1 - pos.x, leds.size.y - 1 - pos.y)] = colour;
        matrix.drawPixelRGB888(pos.x, pos.y, huebase*pos.x, huebase*pos.y, 0);
      }
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    Effect::controls(leds, parentVar);
    ui->initSlider(parentVar, "Hue Speed", leds.sharedData.write<uint8_t>(3), 1, 254); // (14), 1, 255)
    ui->initSlider(parentVar, "Macro Mutator Freq", leds.sharedData.write<uint8_t>(14), 1, 255); // (14), 1, 255)
    ui->initSlider(parentVar, "Macro Mutator Min", leds.sharedData.write<uint8_t>(125), 0, 255); // (125), 1, 2500)
    ui->initSlider(parentVar, "Macro Mutator Max", leds.sharedData.write<uint8_t>(255), 0, 255); // (1), 1, 2500)
    ui->initSlider(parentVar, "Micro Mutator Freq", leds.sharedData.write<uint8_t>(2), 0, 255); // (128), 1, 255)
    ui->initSlider(parentVar, "Micro Mutator Min", leds.sharedData.write<uint8_t>(100), 0, 255); // (550), 0, 2500)
    ui->initSlider(parentVar, "Micro Mutator Max", leds.sharedData.write<uint8_t>(200), 0, 255); // (900), 0, 2500)
    ui->initSlider(parentVar, "Saturation", leds.sharedData.write<uint8_t>(255), 0, 255);
  }
}; // Praxis

class Effects {
public:
  std::vector<Effect *> effects;

  Effects() {
    //create effects before fx.chFun is called

    //1D Basis
    effects.push_back(new SolidEffect);
    // 1D FastLed
    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);
    //1D StarLeds
    effects.push_back(new RunningEffect);
    effects.push_back(new RingRandomFlow);
    // 1D WLED
    effects.push_back(new BouncingBalls);
    effects.push_back(new RainEffect);
    effects.push_back(new DripEffect);
    effects.push_back(new HeartBeatEffect);

    #ifdef STARLEDS_USERMOD_WLEDAUDIO
      //1D Volume
      effects.push_back(new FreqMatrix);
      effects.push_back(new PopCorn);
      effects.push_back(new NoiseMeter);
      //1D frequency
      effects.push_back(new AudioRings);
      effects.push_back(new DJLight);
    #endif

    //2D StarLeds
    effects.push_back(new Lines);
    effects.push_back(new Praxis);
    //2D WLED
    effects.push_back(new BlackHole);
    effects.push_back(new DNA);
    effects.push_back(new DistortionWaves);
    effects.push_back(new Octopus);
    effects.push_back(new Lissajous);
    effects.push_back(new Frizzles);
    effects.push_back(new ScrollingText);
    effects.push_back(new Noise2D);
    effects.push_back(new GameOfLife);
    #ifdef STARLEDS_USERMOD_WLEDAUDIO
      //2D WLED
      effects.push_back(new Waverly);
      effects.push_back(new GEQEffect);
      effects.push_back(new FunkyPlank);
    #endif
    //3D
    effects.push_back(new RipplesEffect);
    effects.push_back(new SphereMoveEffect);
}

  void setup() {
    //check of no local variables (should be only 4 bytes): tbd: can we loop over effects (sizeof(effect does not work))
    // for (Effect *effect:effects) {
    //     ppf("Size of %s is %d\n", effect->name(), sizeof(*effect));
    // }
    // ppf("Size of %s is %d\n", "RainbowEffect", sizeof(RainbowEffect));
    // ppf("Size of %s is %d\n", "RainbowWithGlitterEffect", sizeof(RainbowWithGlitterEffect));
    // ppf("Size of %s is %d\n", "SinelonEffect", sizeof(SinelonEffect));
    // ppf("Size of %s is %d\n", "RunningEffect", sizeof(RunningEffect));
    // ppf("Size of %s is %d\n", "ConfettiEffect", sizeof(ConfettiEffect));
    // ppf("Size of %s is %d\n", "BPMEffect", sizeof(BPMEffect));
    // ppf("Size of %s is %d\n", "JuggleEffect", sizeof(JuggleEffect));
    // ppf("Size of %s is %d\n", "RipplesEffect", sizeof(RipplesEffect));
    // ppf("Size of %s is %d\n", "SphereMoveEffect", sizeof(SphereMoveEffect));
    // ppf("Size of %s is %d\n", "Frizzles", sizeof(Frizzles));
    // ppf("Size of %s is %d\n", "Lines", sizeof(Lines));
    // ppf("Size of %s is %d\n", "DistortionWaves", sizeof(DistortionWaves));
    // ppf("Size of %s is %d\n", "Octopus", sizeof(Octopus));
    // ppf("Size of %s is %d\n", "Lissajous", sizeof(Lissajous));
    // ppf("Size of %s is %d\n", "BouncingBalls", sizeof(BouncingBalls));
    // ppf("Size of %s is %d\n", "RingRandomFlow", sizeof(RingRandomFlow));
    // #ifdef STARLEDS_USERMOD_WLEDAUDIO
    //   ppf("Size of %s is %d\n", "GEQEffect", sizeof(GEQEffect));
    //   ppf("Size of %s is %d\n", "AudioRings", sizeof(AudioRings));
    // #endif
  }

  void loop(Leds &leds) {
    leds.sharedData.loop(); //sets the sharedData pointer back to 0 so loop effect can go through it
    effects[leds.fx%effects.size()]->loop(leds);

    call++;

    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  }

};