/*
   @title     StarMod
   @file      AppModLeds.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

// FastLED optional flags to configure drivers, see https://github.com/FastLED/FastLED/blob/master/src/platforms/esp/32
// RMT driver (default)
// #define FASTLED_ESP32_FLASH_LOCK 1    // temporarily disabled FLASH file access while driving LEDs (may prevent random flicker)
// #define FASTLED_RMT_BUILTIN_DRIVER 1  // in case your app needs to use RMT units, too (slower)
// I2S parallel driver
// #define FASTLED_ESP32_I2S true        // to use I2S parallel driver (instead of RMT)
// #define I2S_DEVICE 1                  // I2S driver: allows to still use I2S#0 for audio (only on esp32 and esp32-s3)
// #define FASTLED_I2S_MAX_CONTROLLERS 8 // 8 LED pins should be enough (default = 24)
#include "FastLED.h"

#include "AppFixture.h"
#include "AppEffects.h"

// #ifdef STARMOD_USERMOD_E131
//   #include "../User/UserModE131.h"
// #endif

// #define FASTLED_RGBW

//https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/
inline uint16_t getRGBWsize(uint16_t nleds){
	uint16_t nbytes = nleds * 4;
	if(nbytes % 3 > 0) return nbytes / 3 + 1;
	else return nbytes / 3;
}

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class AppModLeds:public SysModule {

public:
  bool newFrame = false; //for other modules (DDP)

  uint16_t fps = 60;
  unsigned long lastMappingMillis = 0;
  bool doMap = false;
  Effects effects;

  Fixture fixture = Fixture();

  AppModLeds() :SysModule("Leds") {
    Leds leds = Leds();
    leds.rowNr = 0;
    leds.fixture = &fixture;
    leds.fx = 13;
    leds.projectionNr = 2;
    fixture.ledsList.push_back(leds);
    leds = Leds();
    leds.rowNr = 1;
    leds.fixture = &fixture;
    leds.projectionNr = 2;
    leds.fx = 14;
    fixture.ledsList.push_back(leds);
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name);
    if (parentVar["o"] > -1000) parentVar["o"] = -1100; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

    JsonObject currentVar;

    JsonObject tableVar = ui->initTable(parentVar, "fxTbl", nullptr, false, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Effects");
      web->addResponse(var["id"], "comment", "List of effects");
    }, [this](JsonObject var, uint8_t rowNr) { //chFun
      JsonObject responseObject = web->getResponseObject();
      if (responseObject[var["id"].as<const char *>()]["value"] == "addRow") {
        rowNr = fixture.ledsList.size();
        USER_PRINTF("chFun addRow %s %d\n", var["id"].as<const char *>(), rowNr);

        responseObject["addRow"]["id"] = var["id"];
        responseObject["addRow"]["rowNr"] = rowNr;
        // responseObject["addRow"].createNestedArray("value");
        // JsonArray row = responseObject["updRow"]["insTbl"].createNestedArray(rowNrC);
        // addTblRow(responseObject["updRow"]["value"], instance);

        Leds leds = Leds();
        leds.rowNr = rowNr;
        leds.fixture = &fixture;
        leds.projectionNr = 2;
        leds.fx = 14;
        fixture.ledsList.push_back(leds);
      }
      if (responseObject[var["id"].as<const char *>()]["value"] == "delRow") {
        USER_PRINTF("chFun delrow %s %d\n", var["id"].as<const char *>(), rowNr);
        //fade to black
        if (rowNr <fixture.ledsList.size()) {
          fixture.ledsList.erase(fixture.ledsList.begin() + rowNr); //remove from leds
          //remove from columns
          // for (JsonObject columnVar: var["n"].as<JsonArray>())
          //   mdl->setValue(columnVar, fixture.ledsList[rowNr].fx, rowNr);
          //remove from ui
          responseObject["delRow"]["id"] = var["id"];
          responseObject["delRow"]["rowNr"] = rowNr;
        }
      }
    });

    currentVar = ui->initSelect(tableVar, "fx", 0, UINT8_MAX, false, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Effect");
      web->addResponse(var["id"], "comment", "Effect to show");
      JsonArray select = web->addResponseA(var["id"], "options");
      for (Effect *effect:effects.effects) {
        select.add(effect->name());
      }
    }, [this](JsonObject var, uint8_t rowNr) { //chFun
      if (rowNr < fixture.ledsList.size()) {
        doMap = effects.setEffect(fixture.ledsList[rowNr], var, rowNr);

        web->addResponse("details", "var", var);
        web->addResponse("details", "rowNr", rowNr);

      }
    }, nullptr, fixture.ledsList.size(), [this](JsonObject var, uint8_t rowNr) { //valueFun
      mdl->setValue(var, fixture.ledsList[rowNr].fx, rowNr);
    });
    currentVar["stage"] = true;

    currentVar = ui->initSelect(tableVar, "pro", 2, UINT8_MAX, false, [](JsonObject var) { //uiFun.
      web->addResponse(var["id"], "label", "Projection");
      web->addResponse(var["id"], "comment", "How to project fx");
      JsonArray select = web->addResponseA(var["id"], "options");
      select.add("None"); // 0
      select.add("Random"); // 1
      select.add("Distance from point"); //2
      select.add("Distance from center"); //3
      select.add("Mirror"); //4
      select.add("Reverse"); //5
      select.add("Multiply"); //6
      select.add("Kaleidoscope"); //7
      select.add("Fun"); //8

    }, [this](JsonObject var, uint8_t rowNr) { //chFun
      if (rowNr < fixture.ledsList.size()) {
        fixture.ledsList[rowNr].projectionNr = mdl->getValue(var, rowNr);
        USER_PRINTF("pro chFun %d %d %d\n", fixture.ledsList[rowNr].projectionNr, rowNr, fixture.ledsList.size());

        doMap = true;
      }

    }, nullptr, fixture.ledsList.size(), [this](JsonObject var, uint8_t rowNr) { //valueFun
      mdl->setValue(var, fixture.ledsList[rowNr].projectionNr, rowNr);
    });
    currentVar["stage"] = true;

    ui->initCoord3D(tableVar, "fxStart", fixture.ledsList[0].startPos, UINT8_MAX, 0, NUM_LEDS_Max, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Start");
    }, [this](JsonObject var, uint8_t rowNr) { //chFun
      if (rowNr < fixture.ledsList.size()) {
        fixture.ledsList[rowNr].startPos = mdl->getValue(var, rowNr).as<Coord3D>();

        USER_PRINTF("fxStart[%d] chFun %d %d %d\n", rowNr, fixture.ledsList[rowNr].startPos.x, fixture.ledsList[rowNr].startPos.y, fixture.ledsList[rowNr].startPos.z);

        fixture.ledsList[rowNr].fadeToBlackBy();
      }
      else {
        USER_PRINTF("fxStart chfun rownr not in range %d > %d\n", rowNr, fixture.ledsList.size());
      }

      doMap = true;
    }, nullptr, fixture.ledsList.size(), [this](JsonObject var, uint8_t rowNr) { //valueFun
      USER_PRINTF("fxStart[%d] valueFun %d %d %d\n", rowNr, fixture.ledsList[rowNr].startPos.x, fixture.ledsList[rowNr].startPos.y, fixture.ledsList[rowNr].startPos.z);
      mdl->setValue(var, fixture.ledsList[rowNr].startPos, rowNr);
    });

    ui->initCoord3D(tableVar, "fxEnd", fixture.ledsList[0].endPos, UINT8_MAX, 0, NUM_LEDS_Max, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "End");
    }, [this](JsonObject var, uint8_t rowNr) { //chFun
      if (rowNr < fixture.ledsList.size()) {
        fixture.ledsList[rowNr].endPos = mdl->getValue(var, rowNr).as<Coord3D>();

        USER_PRINTF("fxEnd[%d] chFun %d %d %d\n", rowNr, fixture.ledsList[rowNr].endPos.x, fixture.ledsList[rowNr].endPos.y, fixture.ledsList[rowNr].endPos.z);

        fixture.ledsList[rowNr].fadeToBlackBy();
      }
      else {
        USER_PRINTF("fxEnd chfun rownr not in range %d > %d\n", rowNr, fixture.ledsList.size());
      }

      doMap = true;
    }, nullptr, fixture.ledsList.size(), [this](JsonObject var, uint8_t rowNr) { //valueFun
      mdl->setValue(var, fixture.ledsList[rowNr].endPos, rowNr);
      USER_PRINTF("fxEnd[%d] valueFun %d %d %d\n", rowNr, fixture.ledsList[rowNr].endPos.x, fixture.ledsList[rowNr].endPos.y, fixture.ledsList[rowNr].endPos.z);
    });

    ui->initText(tableVar, "fxSize", nullptr, 0, 32, true, [this](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Size");
    });

    ui->initSelect(parentVar, "fxLayout", 0, UINT8_MAX, false, [](JsonObject var) { //uiFun
      web->addResponse(var["id"], "label", "Layout");
      JsonArray select = web->addResponseA(var["id"], "options");
      select.add("□"); //0
      select.add("="); //1
      select.add("||"); //2
      select.add("+"); //3
    }, [this](JsonObject var, uint8_t) { //chFun
    }); //fixtureGen

    #ifdef STARMOD_USERMOD_E131
      // if (e131mod->isEnabled) {
          e131mod->patchChannel(0, "bri", 255); //should be 256??
          e131mod->patchChannel(1, "fx", effects.effects.size());
          e131mod->patchChannel(2, "pal", 8); //tbd: calculate nr of palettes (from select)
          // //add these temporary to test remote changing of this values do not crash the system
          // e131mod->patchChannel(3, "pro", Projections::count);
          // e131mod->patchChannel(4, "fixture", 5); //assuming 5!!!

          // ui->stageVarChanged = true;
          ui->processUiFun("e131Tbl"); //rebuild the table
      // }
      // else
      //   USER_PRINTF("Leds e131 not enabled\n");
    #endif

    effects.setup();

    FastLED.setMaxPowerInVoltsAndMilliamps(5,2000); // 5v, 2000mA
  }

  void loop() {
    // SysModule::loop();

    //set new frame
    if (millis() - frameMillis >= 1000.0/fps) {
      frameMillis = millis();

      newFrame = true;

      //for each programmed effect
      //  run the next frame of the effect
      // vector iteration on classes is faster!!! (22 vs 30 fps !!!!)
      for (std::vector<Leds>::iterator leds=fixture.ledsList.begin(); leds!=fixture.ledsList.end(); ++leds) {
        // USER_PRINTF(" %d %d,%d,%d - %d,%d,%d (%d,%d,%d)", leds->fx, leds->startPos.x, leds->startPos.y, leds->startPos.z, leds->endPos.x, leds->endPos.y, leds->endPos.z, leds->size.x, leds->size.y, leds->size.z );
        effects.loop(*leds);
      }

      FastLED.show();  

      frameCounter++;
    }
    else {
      newFrame = false;
    }

    JsonObject var = mdl->findVar("System");
    if (!var["canvasData"].isNull()) {
      const char * canvasData = var["canvasData"]; //0 - 494 - 140,150,0
      USER_PRINTF("AppModLeds loop canvasData %s\n", canvasData);

      fixture.ledsList[0].fadeToBlackBy();

      char * token = strtok((char *)canvasData, ":");
      bool isStart = strcmp(token, "start") == 0;

      Coord3D *startOrEndPos = isStart? &fixture.ledsList[0].startPos: &fixture.ledsList[0].endPos;

      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->x = atoi(token) / 10; else startOrEndPos->x = 0; //should never happen
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->y = atoi(token) / 10; else startOrEndPos->y = 0;
      token = strtok(NULL, ",");
      if (token != NULL) startOrEndPos->z = atoi(token) / 10; else startOrEndPos->z = 0;

      mdl->setValue(isStart?"fxStart":"fxEnd", *startOrEndPos, 0); //assuming row 0 for the moment

      var.remove("canvasData"); //convasdata has been processed
      doMap = true; //recalc projection
    }

    //update projection
    if (millis() - lastMappingMillis >= 1000 && doMap) { //not more then once per second (for E131)
      lastMappingMillis = millis();
      doMap = false;
      fixture.projectAndMap();

      //https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples

      //allocatePins
      uint8_t pinNr=0;
      for (PinObject pinObject:pins->pinObjects) {
        if (strcmp(pinObject.owner, "Leds")== 0) {
          //dirty trick to decode nrOfLedsPerPin
          char * after = strtok((char *)pinObject.details, "-");
          if (after != NULL ) {
            char * before;
            before = after;
            after = strtok(NULL, " ");
            uint16_t startLed = atoi(before);
            uint16_t nrOfLeds = atoi(after) - atoi(before) + 1;
            USER_PRINTF("FastLED.addLeds new %d: %d-%d\n", pinNr, startLed, nrOfLeds);

            //commented pins: error: static assertion failed: Invalid pin specified
            switch (pinNr) {
              #if CONFIG_IDF_TARGET_ESP32
                case 0: FastLED.addLeds<NEOPIXEL, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 1: FastLED.addLeds<NEOPIXEL, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 2: FastLED.addLeds<NEOPIXEL, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 3: FastLED.addLeds<NEOPIXEL, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 4: FastLED.addLeds<NEOPIXEL, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 5: FastLED.addLeds<NEOPIXEL, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 6: FastLED.addLeds<NEOPIXEL, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 7: FastLED.addLeds<NEOPIXEL, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 8: FastLED.addLeds<NEOPIXEL, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 9: FastLED.addLeds<NEOPIXEL, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 10: FastLED.addLeds<NEOPIXEL, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 11: FastLED.addLeds<NEOPIXEL, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 12: FastLED.addLeds<NEOPIXEL, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 13: FastLED.addLeds<NEOPIXEL, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 14: FastLED.addLeds<NEOPIXEL, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 15: FastLED.addLeds<NEOPIXEL, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #if !defined(BOARD_HAS_PSRAM) && !defined(ARDUINO_ESP32_PICO)
                // 16+17 = reserved for PSRAM, or reserved for FLASH on pico-D4
                case 16: FastLED.addLeds<NEOPIXEL, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 17: FastLED.addLeds<NEOPIXEL, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                case 18: FastLED.addLeds<NEOPIXEL, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 22: FastLED.addLeds<NEOPIXEL, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 23: FastLED.addLeds<NEOPIXEL, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 25: FastLED.addLeds<NEOPIXEL, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 26: FastLED.addLeds<NEOPIXEL, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 27: FastLED.addLeds<NEOPIXEL, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 32: FastLED.addLeds<NEOPIXEL, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 33: FastLED.addLeds<NEOPIXEL, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // 34-39 input-only
                // case 34: FastLED.addLeds<NEOPIXEL, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 35: FastLED.addLeds<NEOPIXEL, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 36: FastLED.addLeds<NEOPIXEL, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 37: FastLED.addLeds<NEOPIXEL, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 38: FastLED.addLeds<NEOPIXEL, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 39: FastLED.addLeds<NEOPIXEL, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif //CONFIG_IDF_TARGET_ESP32

              #if CONFIG_IDF_TARGET_ESP32S2
                case 0: FastLED.addLeds<NEOPIXEL, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 1: FastLED.addLeds<NEOPIXEL, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 2: FastLED.addLeds<NEOPIXEL, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 3: FastLED.addLeds<NEOPIXEL, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 4: FastLED.addLeds<NEOPIXEL, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 5: FastLED.addLeds<NEOPIXEL, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 6: FastLED.addLeds<NEOPIXEL, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 7: FastLED.addLeds<NEOPIXEL, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 8: FastLED.addLeds<NEOPIXEL, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 9: FastLED.addLeds<NEOPIXEL, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 10: FastLED.addLeds<NEOPIXEL, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 11: FastLED.addLeds<NEOPIXEL, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 12: FastLED.addLeds<NEOPIXEL, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 13: FastLED.addLeds<NEOPIXEL, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 14: FastLED.addLeds<NEOPIXEL, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 15: FastLED.addLeds<NEOPIXEL, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 16: FastLED.addLeds<NEOPIXEL, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 17: FastLED.addLeds<NEOPIXEL, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 18: FastLED.addLeds<NEOPIXEL, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #if !ARDUINO_USB_CDC_ON_BOOT
                // 19 + 20 = USB HWCDC. reserved for USB port when ARDUINO_USB_CDC_ON_BOOT=1
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // 22 to 32: not connected, or reserved for SPI FLASH
                // case 22: FastLED.addLeds<NEOPIXEL, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 23: FastLED.addLeds<NEOPIXEL, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 24: FastLED.addLeds<NEOPIXEL, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 25: FastLED.addLeds<NEOPIXEL, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #if !defined(BOARD_HAS_PSRAM)
                // 26-32 = reserved for PSRAM
                case 26: FastLED.addLeds<NEOPIXEL, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 27: FastLED.addLeds<NEOPIXEL, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 28: FastLED.addLeds<NEOPIXEL, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 29: FastLED.addLeds<NEOPIXEL, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 30: FastLED.addLeds<NEOPIXEL, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 31: FastLED.addLeds<NEOPIXEL, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // case 32: FastLED.addLeds<NEOPIXEL, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                case 33: FastLED.addLeds<NEOPIXEL, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 34: FastLED.addLeds<NEOPIXEL, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 35: FastLED.addLeds<NEOPIXEL, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 36: FastLED.addLeds<NEOPIXEL, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 37: FastLED.addLeds<NEOPIXEL, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 38: FastLED.addLeds<NEOPIXEL, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 39: FastLED.addLeds<NEOPIXEL, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 40: FastLED.addLeds<NEOPIXEL, 40>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 41: FastLED.addLeds<NEOPIXEL, 41>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 42: FastLED.addLeds<NEOPIXEL, 42>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 43: FastLED.addLeds<NEOPIXEL, 43>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 44: FastLED.addLeds<NEOPIXEL, 44>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 45: FastLED.addLeds<NEOPIXEL, 45>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // 46 input-only
                // case 46: FastLED.addLeds<NEOPIXEL, 46>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif //CONFIG_IDF_TARGET_ESP32S2

              #if CONFIG_IDF_TARGET_ESP32C3
                case 0: FastLED.addLeds<NEOPIXEL, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 1: FastLED.addLeds<NEOPIXEL, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 2: FastLED.addLeds<NEOPIXEL, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 3: FastLED.addLeds<NEOPIXEL, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 4: FastLED.addLeds<NEOPIXEL, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 5: FastLED.addLeds<NEOPIXEL, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 6: FastLED.addLeds<NEOPIXEL, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 7: FastLED.addLeds<NEOPIXEL, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 8: FastLED.addLeds<NEOPIXEL, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 9: FastLED.addLeds<NEOPIXEL, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 10: FastLED.addLeds<NEOPIXEL, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                // 11-17 reserved for SPI FLASH
                //case 11: FastLED.addLeds<NEOPIXEL, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 12: FastLED.addLeds<NEOPIXEL, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 13: FastLED.addLeds<NEOPIXEL, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 14: FastLED.addLeds<NEOPIXEL, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 15: FastLED.addLeds<NEOPIXEL, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 16: FastLED.addLeds<NEOPIXEL, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                //case 17: FastLED.addLeds<NEOPIXEL, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #if !ARDUINO_USB_CDC_ON_BOOT
                // 18 + 19 = USB HWCDC. reserved for USB port when ARDUINO_USB_CDC_ON_BOOT=1
                case 18: FastLED.addLeds<NEOPIXEL, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 19: FastLED.addLeds<NEOPIXEL, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
            #endif
                // 20+21 = Serial RX+TX --> don't use for LEDS when serial-to-USB is needed
                case 20: FastLED.addLeds<NEOPIXEL, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                case 21: FastLED.addLeds<NEOPIXEL, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif //CONFIG_IDF_TARGET_ESP32S2

              default: USER_PRINTF("FastLedPin assignment: pin not supported %d\n", pinNr);
            }
          }
        }
        pinNr++;
      }
    }
  } //loop

  void loop1s() {
    mdl->setUIValueV("realFps", "%lu /s", frameCounter);
    frameCounter = 0;
  }

private:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

};

static AppModLeds *lds;