/*
   @title     StarLight
   @file      LedModFixtureGen.h
   @date      20240720
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

//GenFix: class to provide fixture write functions and save to json file
// {
//   "name": "F_Hexagon",
//   "nrOfLeds": 216,
//   "width": 6554,
//   "height": 6554,
//   "depth": 1,
//   "outputs": [{"pin": 2,"leds": [[720,360,0],[1073,1066,0]]}], {"pin": 3,"leds": [[720,360,0],[1073,1066,0]]}]
// }
class GenFix {

public:
  char name[32] = "";

  // unsigned16 distance = 1; //cm, not used yet (to display multiple fixture, also from other devices)

  char pinSep[2]="";
  char pixelSep[2]="";

  Coord3D fixSize = {0,0,0};
  unsigned16 nrOfLeds=0;
  uint16_t ledSize = 5; //mm
  uint8_t shape = 0; //0 = sphere, 1 = TetrahedronGeometry
  
  File f;

  GenFix() {
    ppf("GenFix constructor\n");
  }

  ~GenFix() {
    ppf("GenFix destructor\n");
  }

  void openHeader(const char * format, ...) {
    va_list args;
    va_start(args, format);

    vsnprintf(name, sizeof(name)-1, format, args);

    va_end(args);

    f = files->open("/temp.json", "w");
    if (!f)
      ppf("GenFix could not open temp file for writing\n");

    f.print(",\"outputs\":[");
    strcpy(pinSep, "");
  }

  void closeHeader() {
    f.print("]"); //outputs

    ppf("closeHeader %d-%d-%d %d\n", fixSize.x, fixSize.y, fixSize.z, nrOfLeds);
    f.close();
    f = files->open("/temp.json", "r");

    File g;

    char fileName[32] = "/";
    print->fFormat(fileName, sizeof(fileName)-1, "/%s.json", name);

    //create g by merging in f (better solution?)
    g = files->open(fileName, "w");

    g.print("{");
    g.printf("\"name\":\"%s\"", name);
    g.printf(",\"nrOfLeds\":%d", nrOfLeds);
    g.printf(",\"width\":%d", (fixSize.x+9)/10+1); //effects run on 1 led is 1 cm mode.
    g.printf(",\"height\":%d", (fixSize.y+9)/10+1); //e.g. (110+9)/10 +1 = 11+1 = 12, (111+9)/10 +1 = 12+1 = 13
    g.printf(",\"depth\":%d", (fixSize.z+9)/10+1);
    g.printf(",\"ledSize\":%d", ledSize);
    g.printf(",\"shape\":%d", shape);

    byte character;
    f.read(&character, sizeof(byte));
    while (f.available()) {
      g.write(character);
      f.read(&character, sizeof(byte));
    }
    g.write(character);

    g.print("}");
    g.close();
    f.close();

    files->remove("/temp.json");
  }

  void openPin(unsigned8 pin) {
    f.printf("%s{\"pin\":%d,\"leds\":[", pinSep, pin);
    strcpy(pinSep, ",");
    strcpy(pixelSep, "");
  }
  void closePin() {
    f.printf("]}");
  }

  void write3D(Coord3D pixel) {
    write3D(pixel.x, pixel.y, pixel.z);
  }

  void write3D(unsigned16 x, unsigned16 y, unsigned16 z) {
    // if (x>UINT16_MAX/2 || y>UINT16_MAX/2 || z>UINT16_MAX/2) 
    if (x>1000 || y>1000 || z>1000) 
      ppf("write3D coord too high %d,%d,%d\n", x, y, z);
    else
    {
      f.printf("%s[%d,%d,%d]", pixelSep, x, y, z);
      strcpy(pixelSep, ",");
      fixSize.x = max((unsigned16)fixSize.x, x);
      fixSize.y = max((unsigned16)fixSize.y, y);
      fixSize.z = max((unsigned16)fixSize.z, z);
      nrOfLeds++;
    }

  }

  //utility
  unsigned u_abs(int x) {
    if (x < 0) return -x;
    else return x;
  }

  void matrix(Coord3D first, Coord3D rowEnd, Coord3D colEnd, unsigned8 ip, unsigned8 pin, uint16_t tilt = 0, uint16_t pan = 0, uint16_t roll = 0) {

    openPin(pin);

    //advance from pixel to rowEnd
    //if rowEnd: is it serpentine or not?
    //  no: set advancing dimension back to first
    //  yes: set non advancing dimension 1 step closer to colEnd
    //determine new rowEnd
    //if rowEnd > colEnd
    //  yes: done
    //  no: go back to first step

    // Coord3D pixel = first;

    //middle is (max(rowEnd, colEnd) + first) / 2
    Coord3D middle = rowEnd;
    if (colEnd.x > middle.x) middle.x = colEnd.x;
    if (colEnd.y > middle.y) middle.y = colEnd.y;
    if (colEnd.z > middle.z) middle.z = colEnd.z;
    middle = (middle + first) / 2;

    ppf("middle %d,%d,%d", middle.x, middle.y, middle.z);

    stackUnsigned8 rowDimension; //in what dimension the row will advance (x=0, y=1, z=2), now only one should differ
    if (first.x != rowEnd.x) rowDimension = 0;
    if (first.y != rowEnd.y) rowDimension = 1;
    if (first.z != rowEnd.z) rowDimension = 2;

    stackUnsigned8 colDimension; //in what dimension the col will advance, not the rowDimension
    if (first.x != colEnd.x && rowDimension != 0) colDimension = 0;
    if (first.y != colEnd.y && rowDimension != 1) colDimension = 1;
    if (first.z != colEnd.z && rowDimension != 2) colDimension = 2;

    bool rowValueSame = (rowDimension == 0)? first.x == colEnd.x : (rowDimension == 1)? first.y == colEnd.y : first.z == colEnd.z; 
    bool serpentine = rowValueSame;
    
    unsigned16 nrOfColumns = (colDimension == 0)? u_abs(colEnd.x - first.x) + 1 : (colDimension == 1)? u_abs(colEnd.y - first.y) + 1 : u_abs(colEnd.z - first.z) + 1;
    if (nrOfColumns % 2 == 1 && rowValueSame) //if odd nrOfCols and rowValueSame then adjust the endpoint col value to the rowEnd col value
    {
      if (rowDimension == 0)
        colEnd.x = rowEnd.x;
      else if (rowDimension == 1)
        colEnd.y = rowEnd.y;
      else if (rowDimension == 2)
        colEnd.z = rowEnd.z;
    }

    Trigo trigo;

    Coord3D colPixel = Coord3D{(rowDimension==0)?colEnd.x:first.x, (rowDimension==1)?colEnd.y:first.y, (rowDimension==2)?colEnd.z:first.z};
    stackUnsigned8 colNr = 0;
    while (true) {
      // colPixel is not advancing over the dimension of the row but advances over it's own dimension towards the colEnd

      Coord3D cRowStart = Coord3D{(colDimension==0)?colPixel.x:first.x, (colDimension==1)?colPixel.y:first.y, (colDimension==2)?colPixel.z:first.z};
      Coord3D cRowEnd = Coord3D{(colDimension==0)?colPixel.x:rowEnd.x, (colDimension==1)?colPixel.y:rowEnd.y, (colDimension==2)?colPixel.z:rowEnd.z};

      if (serpentine && colNr%2 != 0) {
        Coord3D temp = cRowStart;
        cRowStart = cRowEnd;
        cRowEnd = temp;
      }

      Coord3D rowPixel = cRowStart;
      while (true) {
        ppf(" %d,%d,%d", rowPixel.x, rowPixel.y, rowPixel.z);
        // write3D(rowPixel.x*10, rowPixel.y*10, rowPixel.z*10);
        write3D(trigo.rotate(rowPixel, middle, tilt, pan, roll, 360));

        
        if (rowPixel == cRowEnd) break; //end condition row
        rowPixel.advance(cRowEnd, 10);
      }

      ppf("\n");

      if (colPixel == colEnd) break; //end condition columns
      colPixel.advance(colEnd, 10);
      colNr++;
    }

    closePin();
  }

  void ring(Coord3D middle, unsigned16 ledCount, unsigned8 ip, unsigned8 pin, uint16_t tilt = 0, uint16_t pan = 0, uint16_t roll = 0, bool preRadius = false) {

    openPin(pin);

    uint8_t radius = 10 * ledCount / M_TWOPI; //in mm;
    if (preRadius) {
      //check for rings241 rings which have a specific radius
      switch (ledCount) {
        case 1: radius = 0; break;
        case 8: radius = 13; break;
        case 12: radius = 23; break;
        case 16: radius = 33; break;
        case 24: radius = 43; break;
        case 32: radius = 53; break;
        case 40: radius = 63; break;
        case 48: radius = 73; break;
        case 60: radius = 83; break;
        default: radius = 10 * ledCount / M_TWOPI; //in mm
      }
    }

    Trigo trigo;

    for (int i=0; i<ledCount; i++) {
      Coord3D pixel;
      trigo.period = ledCount; //rotate uses period 360 so set to ledCount
      pixel.x = middle.x + trigo.sin(radius, i);
      pixel.y = middle.y + trigo.cos(radius, i);
      pixel.z = middle.z;

      write3D(trigo.rotate(pixel, middle, tilt, pan, roll, 360));
    }

    closePin();
  }

  void rings241(Coord3D middle, unsigned8 nrOfRings, bool in2out, unsigned8 ip, unsigned8 pin, uint16_t tilt = 0, uint16_t pan = 0, uint16_t roll = 0) {
    stackUnsigned8 ringsNrOfLeds[9] = {1, 8, 12, 16, 24, 32, 40, 48, 60};
    // stackUnsigned8 radiuss[9] = {0, 13, 23, 33, 43, 53, 63, 73, 83}; //in mm
    //  {0, 0},     //0 Center Point -> 1
    //   {1, 8},     //1 -> 8
    //   {9, 20},   //2 -> 12
    //   {21, 36},   //3 -> 16
    //   {37, 60},   //4 -> 24
    //   {61, 92},   //5 -> 32
    //   {93, 132},  //6 -> 40
    //   {133, 180}, //7 -> 48
    //   {181, 240}, //8 Outer Ring -> 60   -> 241

    // stackUnsigned16 size = radiuss[nrOfRings-1]; //size if the biggest ring
    for (int j=0; j<nrOfRings; j++) {
      stackUnsigned8 ringNrOfLeds = in2out?ringsNrOfLeds[j]:ringsNrOfLeds[nrOfRings - 1 - j];
      ring(middle, ringNrOfLeds, ip, pin, tilt, pan, roll, true); //use predefined radius
    }
  }

  void spiral(Coord3D middle, unsigned16 ledCount, unsigned16 radius, unsigned8 ip, unsigned8 pin) {

    openPin(pin);

    for (int i=0; i<ledCount; i++) {
      float radians = i*360/48 * DEG_TO_RAD; //48 leds per round
      stackUnsigned16 x = radius * sinf(radians);
      stackUnsigned16 y = 10 * i/12; //12 leds is 1 cm ?
      stackUnsigned16 z = radius * cosf(radians);
      write3D(x + middle.x, y + middle.y, z + middle.z);
    }

    closePin();
  }

  void helix(Coord3D middle, uint16_t ledCount, uint16_t radius, uint16_t pitch, uint16_t deltaLed, unsigned8 ip, unsigned8 pin, uint16_t tilt = 0, uint16_t pan = 0, uint16_t roll = 0) {

    openPin(pin);

    float circumference = radius * M_TWOPI; //376
    float ledsPerRound = circumference / deltaLed; //38
    float degreesPerLed = 360 / ledsPerRound; //9.5

    // ppf("helix lc:%d r:%d p:%d dl:%d c:%f lr%f dl:%f\n", ledCount, radius, pitch, deltaLed, circumference, ledsPerRound, degreesPerLed);

    Trigo trigo;

    for (int i=0; i<ledCount; i++) {
      Coord3D pixel;
      // trigo.period = ledCount; //rotate uses period 360 so set to ledCount
      pixel.x = middle.x + trigo.sin(radius, i * degreesPerLed);
      pixel.y = middle.y + pitch * i / ledsPerRound;
      pixel.z = middle.z + trigo.cos(radius, i * degreesPerLed);

      // ppf("l-%d: %d,%d,%d %f %d\n",i, pixel.x, pixel.y, pixel.z, i * degreesPerLed, trigo.sin(radius, i * degreesPerLed));

      write3D(trigo.rotate(pixel, middle, tilt, pan, roll, 360));
    }

    closePin();
  }

  void wheel(Coord3D middle, unsigned16 nrOfSpokes, unsigned16 ledsPerSpoke, unsigned8 ip, unsigned8 pin) {

    openPin(pin);

    for (int i=0; i<nrOfSpokes; i++) {
      float radians = i*360/nrOfSpokes * DEG_TO_RAD;
      for (int j=0;j<ledsPerSpoke;j++) {
        float radius = 50 + 10 * j; //in mm
        stackUnsigned16 x = radius * sinf(radians);
        stackUnsigned16 y = radius * cosf(radians);
        write3D(x+middle.x,y+middle.y, middle.z);
      }
    }
    closePin();
  }

  //https://stackoverflow.com/questions/71816702/coordinates-of-dot-on-an-hexagon-path
  void hexagon(Coord3D middle, unsigned16 ledsPerSide, unsigned8 ip, unsigned8 pin) {

    openPin(pin);

    float radius = ledsPerSide; //or float if it needs to be tuned

    const float y = sqrtf(3)/2; // = sin(60°)
    float hexaX[7] = {1.0, 0.5, -0.5, -1.0, -0.5, 0.5, 1.0};
    float hexaY[7] = {0.0, y, y, 0, -y, -y, 0.0};

    for (forUnsigned16 i = 0; i < ledsPerSide * 6; i++) {
      float offset = 6.0f * (float)i / (float)(ledsPerSide*6);
      stackUnsigned8 edgenum = floor(offset);  // On which edge is this dot?
      offset = offset - (float)edgenum; // Retain fractional part only: offset on that edge

      // Use interpolation to get coordinates of that point on that edge
      float x = (float)middle.x + radius*10.0f * (hexaX[edgenum] + offset * (hexaX[edgenum + 1] - hexaX[edgenum]));
      float y = (float)middle.y + radius*10.0f * (hexaY[edgenum] + offset * (hexaY[edgenum + 1] - hexaY[edgenum]));
      // ppf(" %d %f: %f,%f", edgenum, offset, x, y);

      write3D(x, y, middle.z);
    }

    closePin();
  }

  void cone(Coord3D middle, unsigned8 nrOfRings, unsigned8 ip, unsigned8 pin) {

    for (int j=0; j<nrOfRings; j++) {
      stackUnsigned8 ringNrOfLeds = (j+1) * 3;
      middle.y += 10;

      ppf("Cone %d %d %d %d,%d,%d\n", j, nrOfRings, ringNrOfLeds, middle.x, middle.y, middle.z);
      ring(middle, ringNrOfLeds, ip, pin, 90, 0, 0); //tilt 90
    }

  }

  void cloud5416(Coord3D first, unsigned8 ip, unsigned8 pin) {
    //Small RL Alt Test

    stackUnsigned8 y;

    //tbd: different pins for each section!!!

    //first pin (red)
    openPin(pin);
    y = 150; for (int x = 530; x >= 0; x-=10) write3D(x+first.x, y+first.y, first.z);
    y = 110; for (int x = 90; x <= 510; x+=10) write3D(x+first.x, y+first.y, first.z);
    y = 70; for (int x = 400; x >= 110; x-=10) write3D(x+first.x, y+first.y, first.z);
    closePin();
    //second pin (green)
    openPin(pin);
    y = 140; for (int x = 530; x >= 0; x-=10) write3D(x+first.x, y+first.y, first.z);
    y = 100; for (int x = 90; x <= 510; x+=10) write3D(x+first.x, y+first.y, first.z);
    y = 60; for (int x = 390; x >= 120; x-=10) write3D(x+first.x, y+first.y, first.z);
    closePin();
    //third pin (blue)
    openPin(pin);
    y = 130; for (int x = 520; x >= 10; x-=10) write3D(x+first.x, y+first.y, first.z);
    y = 90; for (int x = 100; x <= 500; x+=10) write3D(x+first.x, y+first.y, first.z);
    y = 50; for (int x = 390; x >= 140; x-=10) write3D(x+first.x, y+first.y, first.z);
    closePin();
    //fourth pin (yellow)
    openPin(pin);
    y = 120; for (int x = 520; x >= 30; x-=10) write3D(x+first.x, y+first.y, first.z);
    y = 80; for (int x = 100; x <= 480; x+=10) write3D(x+first.x, y+first.y, first.z);
    y = 40; for (int x = 380; x >= 240; x-=10) write3D(x+first.x, y+first.y, first.z);
    y = 30; for (int x = 250; x <= 370; x+=10) write3D(x+first.x, y+first.y, first.z);
    y = 20; for (int x = 360; x >= 260; x-=10) write3D(x+first.x, y+first.y, first.z);
    y = 10; for (int x = 270; x <= 350; x+=10) write3D(x+first.x, y+first.y, first.z);
    y = 00; for (int x = 330; x >= 290; x-=10) write3D(x+first.x, y+first.y, first.z);
    closePin();
  }

  void globe(Coord3D middle, unsigned8 width, unsigned8 ip, unsigned8 pin) {

    // loop over latitudes -> sin 0 to pi
    for (uint8_t lat = 0; lat <= width; lat++) {
      float factor = 128.0f / width; //16 => 8
      float radius = (sin8(lat * factor)-128) / factor / 2.0f; // 0 .. 128 => 0 .. 128 .. 0 => 0 .. 16 .. 0 in cm

      uint8_t ledCount = radius * M_TWOPI; 

      middle.y += 10;

      // Coord3D start = first;
      // start.y = first.y + lat;
      ppf("Globe %d %f %d %d,%d,%d\n", lat, radius, ledCount, middle.x, middle.y, middle.z);
      ring(middle, ledCount, ip, pin, 90, 0, 0); //tilt 90
    }
  }

  // https://stackoverflow.com/questions/17705621/algorithm-for-a-geodesic-sphere
  //https://opengl.org.ru/docs/pg/0208.html
  void geodesicDome(Coord3D first, unsigned16 radius, unsigned8 ip, unsigned8 pin) {

    //radius unused (tbd)
 
    stackUnsigned8 tindices[20][3] = {    {0,40,10}, {0,90,40}, {90,50,40}, {40,50,80}, {40,80,10},       {80,100,10}, {80,30,100}, {50,30,80}, {50,20,30}, {20,70,30},       {70,100,30}, {70,60,100}, {70,110,60}, {110,0,60}, {0,10,60},    {60,10,100}, {90,0,110}, {90,110,20}, {90,20,50}, {70,20,110} };

    openPin(pin);

    for (int i=0; i<20; i++) {
      write3D(tindices[i][0] + first.x, tindices[i][1] + first.y, tindices[i][2] + first.z);
    }

    closePin();
  }

};

class LedModFixtureGen:public SysModule {

public:

  LedModFixtureGen() :SysModule("Fixture Generator") {};

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 6302); //created as a usermod, not an appmod to have it in the usermods tab
    parentVar["s"] = true; //setup

    JsonObject currentVar = ui->initSelect(parentVar, "fixtureVar", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        ui->setLabel(var, "Fixture");
        ui->setComment(var, "Predefined fixture");
        JsonArray options1 = ui->setOptions(var); //See enum Fixtures for order of options

        //create 3 level select options
        char jsonString[512] = "";
        strcat(jsonString, "[ {\"Strips\": [\"Spiral\", \"Helix\"]}");
        strcat(jsonString, ", {\"Matrices\": [\"Panel\", \"Panel2x2\", \"Panel4x1\", \"Sticks\"]}");
        strcat(jsonString, ", {\"Cubes\": [\"Human Sized Cube\", \"CubeBox\", \"Cube3D\"]}");
        strcat(jsonString, ", {\"Rings\": [\"Ring\", \"Audi\", \"Olympic\"]}");
        strcat(jsonString, ", {\"Shapes\": [\"Rings241\", {\"Hexagon\":[\"Hexagon\", \"HexaWall\"]}, \"Cone\", \"Cloud5416\"]}");
        strcat(jsonString, ", {\"Combinations\": [\"Wall\", \"6Rings\", \"SpaceStation\", \"Star\", \"Wheel\", \"Human\", \"Curtain\"]}");
        strcat(jsonString, ", {\"Spheres\": [\"Globe\", \"LeGlorb\", \"GeodesicDome WIP\"]}");
        strcat(jsonString, "]");

        // char jsonString[1024] = "";
        // strcat(jsonString, "[ {\"Strips\": [\"Spiral 🧊\", \"Helix 🧊\"]}");
        // strcat(jsonString, ", {\"Matrices\": [\"Panel ▦\", \"Panel2x2 ▦\", \"Panel4x1 ▦\", \"Human Sized Cube 🧊\", \"CubeBox 🧊\", \"Cube3D 🧊\", \"Sticks ▦\"]}");
        // strcat(jsonString, ", {\"Rings\": [\"Ring ▦\", \"Audi ▦\", \"Olympic ▦\"]}");
        // strcat(jsonString, ", {\"Shapes\": [\"Rings241 ▦\", {\"Hexagon\":[\"Hexagon\", \"HexaWall\"]}, \"Cone 🧊\", \"Cloud5416 ▦\"]}");
        // strcat(jsonString, ", {\"Combinations\": [\"Wall ▦\", \"6Rings 🧊\", \"SpaceStation 🧊\", \"Star ▦\", \"Wheel ▦\", \"Human ▦\", \"Curtain ▦\"]}");
        // strcat(jsonString, ", {\"Spheres\": [\"Globe 🧊\", \"LeGlorb 🧊\", \"GeodesicDome WIP 🧊\"]}");
        // strcat(jsonString, "]");

        DeserializationError error = deserializeJson(options1, jsonString);
        if (error)
          ppf("Error %s %s\n", jsonString, error.c_str());
        // else
        //   print->printJson("OPTIONS", options1);
        return true; }
      case onChange:
        this->fixtureVarOnChange();
        return true;
      default: return false; 
    }}); //fixtureVar

  } //setup

  void loop() {
    // SysModule::loop();
  }

  void rebuildMatrix(const char * fgText) {

    uint8_t width = mdl->getValue("width");
    uint8_t height = mdl->getValue("height");
    //recalc the columns
    stackUnsigned8 fixNr = 0;

    mdl->findVar("fixFirst").remove("value");
    mdl->findVar("mrxRowEnd").remove("value");
    mdl->findVar("mrxColEnd").remove("value");
    mdl->findVar("fixIP").remove("value");
    mdl->findVar("fixPin").remove("value");

    if (strstr(fgText, "Panel2x2") != nullptr) {
      fixNr = 0;
      for (int panelY = 0; panelY < 2; panelY++) {
        for (int panelX = 0; panelX < 2; panelX++) {
          mdl->setValue("fixFirst", Coord3D{width * panelX, height * panelY, 0}, fixNr);
          mdl->setValue("mrxRowEnd", Coord3D{width * panelX, height * (panelY+1)-1, 0}, fixNr);
          mdl->setValue("mrxColEnd", Coord3D{width * (panelX+1)-1, height * (panelY+1)-1, 0}, fixNr);
          mdl->setValue("fixPin", 2, fixNr);
          fixNr++;
        }
      }
    }
    else if (strstr(fgText, "Panel4x1") != nullptr) {
      fixNr = 0;
      for (int panelY = 3; panelY >= 0; panelY--) {
        mdl->setValue("fixFirst", Coord3D{width - 1, height*(panelY+1) - 1, 0}, fixNr);
        mdl->setValue("mrxRowEnd", Coord3D{width - 1, height*panelY, 0}, fixNr);
        mdl->setValue("mrxColEnd", Coord3D{0, height*(panelY+1) - 1, 0}, fixNr);
        mdl->setValue("fixPin", 2, fixNr);
        fixNr++;
      }
    }
    else if (strstr(fgText, "Panel") != nullptr) { //after panel2x2 / 4x1
      fixNr = 0; mdl->setValue("fixFirst", Coord3D{0,0,0}, fixNr++); 
      fixNr = 0; mdl->setValue("mrxRowEnd", Coord3D{0,height-1,0}, fixNr++);
      fixNr = 0; mdl->setValue("mrxColEnd", Coord3D{width-1,height-1,0}, fixNr++);
      fixNr = 0; mdl->setValue("fixPin", 2, fixNr++); // default per board...
    }
    else if (strstr(fgText, "Sticks") != nullptr) {
      for (forUnsigned8 fixNr = 0; fixNr < width; fixNr++) {
        mdl->setValue("fixFirst", Coord3D{(unsigned16)(fixNr*5), height, 0}, fixNr);
        mdl->setValue("mrxRowEnd", Coord3D{(unsigned16)(fixNr*5), height, 0}, fixNr);
        mdl->setValue("mrxColEnd", Coord3D{(unsigned16)(fixNr*5), 0, 0}, fixNr);
        mdl->setValue("fixPin", 2, fixNr);
      }
    }
  }

  void rebuildCube(const char * fgText) {
    uint8_t length = mdl->getValue("length");
    //recalc the columns
    stackUnsigned8 fixNr = 0;

    mdl->findVar("fixFirst").remove("value");
    mdl->findVar("mrxRowEnd").remove("value");
    mdl->findVar("mrxColEnd").remove("value");
    mdl->findVar("fixIP").remove("value");
    mdl->findVar("fixPin").remove("value");

    if (strstr(fgText, "Human Sized Cube") != nullptr) {
      stackUnsigned8 size = length + 1;
      fixNr = 0; mdl->setValue("fixFirst", Coord3D{1,1,size}, fixNr++); mdl->setValue("fixFirst", Coord3D{0,1,1}, fixNr++); mdl->setValue("fixFirst", Coord3D{1,1,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{size,1,1}, fixNr++); mdl->setValue("fixFirst", Coord3D{1,0,1}, fixNr++);
      fixNr = 0; mdl->setValue("mrxRowEnd", Coord3D{1,length,size}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{0,length,1}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{1,length,0}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{size,length,1}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{1,0,length}, fixNr++);
      fixNr = 0; mdl->setValue("mrxColEnd", Coord3D{length,length,size}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{0,length,length}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{length,length,0}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{size,length,length}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{length,0,length}, fixNr++);
      fixNr = 0; mdl->setValue("fixPin", 16, fixNr++); mdl->setValue("fixPin", 14, fixNr++); mdl->setValue("fixPin", 32, fixNr++); mdl->setValue("fixPin", 3, fixNr++); mdl->setValue("fixPin", 15, fixNr++);
    }
    else if (strstr(fgText, "CubeBox") != nullptr) {
      stackUnsigned8 size = length + 1;
      fixNr = 0; mdl->setValue("fixFirst", Coord3D{1,1,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{length, size, length}, fixNr++); mdl->setValue("fixFirst", Coord3D{size, 1, 1}, fixNr++); mdl->setValue("fixFirst", Coord3D{0, length, length}, fixNr++); mdl->setValue("fixFirst", Coord3D{length, 1, size}, fixNr++); mdl->setValue("fixFirst", Coord3D{1, 0, length}, fixNr++);
      fixNr = 0; mdl->setValue("mrxRowEnd", Coord3D{1,length,0}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{1, size, length}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{size, 1, length}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{0, 1, length}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{1, 1, size}, fixNr++); mdl->setValue("mrxRowEnd", Coord3D{length, 0, length}, fixNr++);
      fixNr = 0; mdl->setValue("mrxColEnd", Coord3D{length,length,0}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{1, size, 1}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{size, length, length}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{0, 1, 1}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{1, length, size}, fixNr++); mdl->setValue("mrxColEnd", Coord3D{length, 0, 1}, fixNr++);
      fixNr = 0; mdl->setValue("fixPin", 12, fixNr++); mdl->setValue("fixPin", 12, fixNr++); mdl->setValue("fixPin", 13, fixNr++); mdl->setValue("fixPin", 13, fixNr++); mdl->setValue("fixPin", 14, fixNr++); mdl->setValue("fixPin", 14, fixNr++);
    }
    else if (strstr(fgText, "Cube3D") != nullptr) {
      stackUnsigned8 size = length -1;
      for (forUnsigned8 fixNr = 0; fixNr < length; fixNr++) {
        mdl->setValue("fixFirst", Coord3D{0,0,(unsigned16)fixNr}, fixNr);
        mdl->setValue("mrxRowEnd", Coord3D{0, length -1,(unsigned16)fixNr}, fixNr);
        mdl->setValue("mrxColEnd", Coord3D{length -1, length -1,(unsigned16)fixNr}, fixNr);
        mdl->setValue("fixPin", 12, fixNr);
      }
    }
  }

  //generate dynamic html for fixture controls
  void fixtureVarOnChange() {

    JsonObject fixtureVar = mdl->findVar("fixtureVar");

    // JsonObject parentVar = mdl->findVar(var["id"]); //local parentVar
    stackUnsigned8 fgValue = fixtureVar["value"];

    //find option group and text
    char fgGroup[32];
    char fgText[32];
    ui->findOptionsText(fixtureVar, fgValue, fgGroup, fgText);

    //remove all the variables
    fixtureVar.remove("n"); //tbd: we should also remove the varFun !!

    //part 0: group variables
    if (strcmp(fgGroup, "Matrices") == 0 || strcmp(fgGroup, "Cubes") == 0) {

      if (strcmp(fgGroup, "Matrices") == 0) {
        uint8_t width = 16;
        uint8_t height = 16;
        if (strstr(fgText, "Panel4x1") != nullptr) {
          width = 32; height = 8;
        }
        else if (strstr(fgText, "Sticks") != nullptr) {
          width = 10; height = 54;
        }

        ui->initNumber(fixtureVar, "width", width, 1, NUM_LEDS_Max, false, [this,fgText](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onChange:
            rebuildMatrix(fgText);
            return true;
          default: return false; 
        }});
        ui->initNumber(fixtureVar, "height", height, 1, NUM_LEDS_Max, false, [this,fgText](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onChange:
            rebuildMatrix(fgText);
            return true;
          default: return false; 
        }});
      } else { //Cubes
        uint8_t length = 16;
        if (strstr(fgText, "Human") != nullptr)
          length = 20;
        else if (strstr(fgText, "Cube3D") != nullptr)
          length = 8;
        else if (strstr(fgText, "CubeBox") != nullptr)
          length = 8;

        ui->initNumber(fixtureVar, "length", length, 1, NUM_LEDS_Max, false, [this,fgText](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onChange:
            rebuildCube(fgText);
            return true;
          default: return false; 
        }});
      }
    }

    ui->initButton(fixtureVar, "generate", false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "Create F_ixture.json");
        return true;
      case onChange: {

        char fileName[32]; 
        generateOnChange(var, fileName);

        //set fixture in fixture module
        ui->callVarFun("fixture", UINT8_MAX, onUI); //rebuild options

        uint8_t value = ui->selectOptionToValue("fixture", fileName);
        if (value != UINT8_MAX)
          mdl->setValue("fixture", value);

        return true; }
      default: return false;
    }});

    // mdl->varPreDetails(fixtureVar);

    bool showTable = true;
    JsonObject parentVar = fixtureVar;

    //default table variables - part 1
    if (showTable) {

      parentVar = ui->initTable(fixtureVar, "fixTbl", nullptr, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onUI:
          ui->setLabel(var, "Fixtures");
          ui->setComment(var, "Multiple parts");
          return true;
        case onAddRow:
          web->getResponseObject()["addRow"]["rowNr"] = rowNr;
          return true;
        case onDeleteRow:
          // web->getResponseObject()["addRow"]["rowNr"] = rowNr;
          return true;
        default: return false;
      }});

      ui->initCoord3D(parentVar, "fixFirst", {0,0,0}, 0, NUM_LEDS_Max, false, [fgGroup](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onUI:
          //show Top Left for all fixture except Matrix as it has its own
          if (strcmp(fgGroup, "Matrices") == 0 || strcmp(fgGroup, "Cubes") == 0)
            ui->setLabel(var, "First LED");
          else 
            ui->setLabel(var, "Top left");
          return true;
        default: return false;
      }});

    } //if showTable

    //custom variables
    if (strcmp(fgGroup, "Strips") == 0) {
      if (strstr(fgText, "Spiral") != nullptr) {
        ui->initNumber(parentVar, "fixLeds", 64, 1, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onUI:
            ui->setLabel(var, "Leds");
            return true;
          default: return false; 
        }});
        ui->initNumber(parentVar, "radius", 100, 1, 1000);
      }
      else if (strstr(fgText, "Helix") != nullptr) {
        ui->initNumber(parentVar, "fixLeds", 100, 1, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onUI:
            ui->setLabel(var, "Leds");
            return true;
          default: return false; 
        }});
        ui->initNumber(parentVar, "radius", 60, 1, 600);
        ui->initNumber(parentVar, "pitch", 30, 1, 100);
        ui->initNumber(parentVar, "deltaLed", 30, 1, 100);
      }
    }
    else if (strcmp(fgGroup, "Matrices") == 0 || strcmp(fgGroup, "Cubes") == 0) {

      ui->initCoord3D(parentVar, "mrxRowEnd", {7,0,0}, 0, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onUI:
          ui->setLabel(var, "Row end");
          ui->setComment(var, "-> Orientation");
          return true;
        default: return false;
      }});

      ui->initCoord3D(parentVar, "mrxColEnd", {7,7,0}, 0, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onUI:
          ui->setLabel(var, "Column end");
          ui->setComment(var, "Last LED -> nrOfLeds, Serpentine");
          return true;
        default: return false;
      }});
    }
    else if (strcmp(fgGroup, "Rings") == 0) {
      ui->initNumber(parentVar, "fixLeds", 24, 1, NUM_LEDS_Max, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onUI:
          ui->setLabel(var, "Leds");
          return true;
        default: return false; 
      }});
    }
    else if (strcmp(fgGroup, "Shapes") == 0) {
      if (strstr(fgText, "Rings241") != nullptr) {
        ui->initNumber(parentVar, "nrOfRings", 9, 1, 9);
        ui->initCheckBox(parentVar, "in2out", true);
      }
      else if (strstr(fgText, "Hexa") != nullptr) {
        ui->initNumber(parentVar, "ledsPerSide", 12, 1, 255);
      }
      else if (strstr(fgText, "Cone") != nullptr) {
        ui->initNumber(parentVar, "nrOfRings", 24, 1, 360);
      }
    }
    else if (strcmp(fgGroup, "Combinations") == 0) {
      if (strstr(fgText, "Wheel") != nullptr) {
        ui->initNumber(parentVar, "nrOfSpokes", 36, 1, 360);
        ui->initNumber(parentVar, "ledsPerSpoke", 24, 1, 360);
      }
      else if (strstr(fgText, "Human") != nullptr) {
      }
      else if (strstr(fgText, "Curtain") != nullptr) {
        ui->initNumber(parentVar, "width", 20, 1, 100);
        ui->initNumber(parentVar, "height", 20, 1, 100);
      }
    }
    else if (strcmp(fgGroup, "Spheres") == 0) {
      if (strstr(fgText, "Globe") != nullptr) {
        ui->initNumber(parentVar, "width", 24, 1, 48);
      }
      else if (strstr(fgText, "GeodesicDome") != nullptr) {
        ui->initNumber(parentVar, "radius", 100, 1, 1000);
      }
    }


    //default variables - part 2
    if (strcmp(fgGroup, "Matrices") == 0 || strcmp(fgGroup, "Cubes") == 0 || strstr(fgText, "Rings241") != nullptr || strstr(fgText, "Helix") != nullptr) { //tbd: the rest
      ui->initCoord3D(parentVar, "fixRotate", {0,0,0}, 0, 359, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onUI:
          ui->setLabel(var, "Rotate");
          ui->setComment(var, "Tilt, Pan, Roll");
          return true;
        default: return false;
      }});
    }

    ui->initNumber(parentVar, "fixIP", WiFi.localIP()[3], 1, 256, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "IP");
        ui->setComment(var, "Super-Sync WIP");
        return true;
      default: return false; 
    }});

    ui->initPin(parentVar, "fixPin", 2, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        ui->setLabel(var, "Pin");
        return true; }
      case onChange: {
        //set remaining rows to same pin
        JsonArray valArray = mdl->varValArray(var);

        stackUnsigned8 thisVal = var["value"];
        stackUnsigned8 rowNrL = 0;
        for (JsonVariant val: valArray) {
          if (rowNrL > rowNr)
            mdl->setValue(var, valArray[rowNr].as<unsigned8>(), rowNrL);
          rowNrL++;
        }
        return true; }
      default: return false; 
    }});


    // predefined options
    stackUnsigned8 fixNr = 0;

    if (strcmp(fgGroup, "Matrices") == 0)
      rebuildMatrix(fgText); 
    else if (strcmp(fgGroup, "Cubes") == 0) 
      rebuildCube(fgText);      
    else if (strcmp(fgGroup, "Rings") == 0) {
      if (strstr(fgText, "Olympic") != nullptr) {
        fixNr = 0; mdl->setValue("fixFirst", Coord3D{0,0,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{10,0,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{20,0,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{5,3,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{15,3,0}, fixNr++); 
        fixNr = 0; mdl->setValue("fixLeds", 24, fixNr++); mdl->setValue("fixLeds", 24, fixNr++); mdl->setValue("fixLeds", 24, fixNr++); mdl->setValue("fixLeds", 24, fixNr++); mdl->setValue("fixLeds", 24, fixNr++); 
        fixNr = 0; mdl->setValue("fixPin", 2, fixNr++); mdl->setValue("fixPin", 2, fixNr++); mdl->setValue("fixPin", 2, fixNr++); mdl->setValue("fixPin", 2, fixNr++); mdl->setValue("fixPin", 2, fixNr++); // default per board...
      }
      else if (strstr(fgText, "Audi") != nullptr) {
        fixNr = 0; mdl->setValue("fixFirst", Coord3D{0,0,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{6,0,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{12,0,0}, fixNr++); mdl->setValue("fixFirst", Coord3D{18  ,0,0}, fixNr++);
        fixNr = 0; mdl->setValue("fixLeds", 24, fixNr++); mdl->setValue("fixLeds", 24, fixNr++); mdl->setValue("fixLeds", 24, fixNr++); mdl->setValue("fixLeds", 24, fixNr++); 
        fixNr = 0; mdl->setValue("fixPin", 2, fixNr++); mdl->setValue("fixPin", 2, fixNr++); mdl->setValue("fixPin", 2, fixNr++); mdl->setValue("fixPin", 2, fixNr++); // default per board...
      }

    }
    else if (strcmp(fgGroup, "Shapes") == 0) {
      if (strstr(fgText, "HexaWall") != nullptr) {
        fixNr = 0; 
        mdl->setValue("fixFirst", Coord3D{0,0,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{10,6,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{10,18,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{20,0,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{30,6,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{40,12,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{40,24,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{50,6,0}, fixNr++); 
        mdl->setValue("fixFirst", Coord3D{60,0,0}, fixNr++); 

        for (forUnsigned8 fixNr = 0; fixNr < 9; fixNr++) {
          mdl->setValue("ledsPerSide", 6, fixNr);
          mdl->setValue("fixPin", 2, fixNr);
        }
      }
    }

    mdl->varPostDetails(fixtureVar, UINT8_MAX);
  }

  //tbd: move to utility functions
  char *removeSpaces(char *str) 
  { 
      int i = 0, j = 0; 
      while (str[i]) 
      { 
          if (str[i] != ' ') 
          str[j++] = str[i]; 
          i++; 
      } 
      str[j] = '\0'; 
      return str; 
  } 

  void getFixtures(const char * fixtureName, std::function<void(GenFix *, unsigned8, Coord3D, unsigned8, unsigned8)> genFun) {
    GenFix genFix;

    char fileName[32];
    print->fFormat(fileName, sizeof(fileName)-1, "F_%s", fixtureName);
    removeSpaces(fileName);

    if (strstr(fixtureName, "Sized")!=nullptr) genFix.ledSize = 2; //hack to make the hcs leds smaller

    genFix.openHeader(fileName);

    JsonVariant fixFirstValue = mdl->findVar("fixFirst")["value"];

    if (fixFirstValue.is<JsonArray>()) { //multiple rows
      stackUnsigned8 rowNr = 0;
      for (JsonVariant firstValueRow: fixFirstValue.as<JsonArray>()) {
        genFun(&genFix, rowNr, mdl->getValue("fixFirst", rowNr), mdl->getValue("fixIP", rowNr), mdl->getValue("fixPin", rowNr));
        rowNr++;
      }
    } else {
      genFun(&genFix, UINT8_MAX, mdl->getValue("fixFirst"), mdl->getValue("fixIP"), mdl->getValue("fixPin"));
    }

    genFix.closeHeader();
  }
 
  //generate the F-ixture.json file
  void generateOnChange(JsonObject var, char * fileName) {

    JsonObject fixtureVar = mdl->findVar("fixtureVar");
    stackUnsigned8 fgValue = fixtureVar["value"];

    //find option group and text
    char fgGroup[32];
    char fgText[32];
    ui->findOptionsText(fixtureVar, fgValue, fgGroup, fgText);


    if (strcmp(fgGroup, "Matrices") == 0 || strcmp(fgGroup, "Cubes") == 0) {

      if (strcmp(fgGroup, "Matrices") == 0)
        print->fFormat(fileName, 31, "%s-%dx%d", fgText, mdl->getValue("width").as<unsigned8>(), mdl->getValue("height").as<unsigned8>());
      else //Cubes
        print->fFormat(fileName, 31, "%s-%d", fgText, mdl->getValue("length").as<unsigned8>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        Coord3D fixRotate = mdl->getValue("fixRotate", rowNr);
        Coord3D mrxRowEnd = mdl->getValue("mrxRowEnd", rowNr);
        Coord3D mrxColEnd = mdl->getValue("mrxColEnd", rowNr);
        genFix->matrix(fixFirst * 10, mrxRowEnd * 10, mrxColEnd * 10, fixIP, fixPin, fixRotate.x, fixRotate.y, fixRotate.z);
      });

    } else if (strcmp(fgGroup, "Rings") == 0) {

      print->fFormat(fileName, 31, "%s%d", fgText, mdl->getValue("fixLeds").as<unsigned16>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        uint16_t ledCount = mdl->getValue("fixLeds", rowNr);
        //first to middle (in mm)
        Coord3D middle = fixFirst;
        uint8_t radius = 10 * ledCount / M_TWOPI + 10; //in mm
        middle.x = middle.x*10 + radius;
        middle.y = middle.y*10 + radius;
        
        genFix->ring(middle, ledCount, fixIP, fixPin);
      });

    } else if (strstr(fgText, "Rings241") != nullptr) {

      print->fFormat(fileName, 31, "%s-%d", fgText, mdl->getValue("nrOfRings").as<unsigned8>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        uint16_t nrOfLeds = mdl->getValue("fixLeds", rowNr);
        Coord3D fixRotate = mdl->getValue("fixRotate", rowNr);
        //first to middle (in mm)
        Coord3D middle = fixFirst;
        uint8_t radius = 10 * 60 / M_TWOPI; //in mm
        middle.x = middle.x*10 + radius;
        middle.y = middle.y*10 + radius;
        genFix->rings241(middle, mdl->getValue("nrOfRings", rowNr), mdl->getValue("in2out", rowNr), fixIP, fixPin, fixRotate.x, fixRotate.y, fixRotate.z);
      });

    } else if (strstr(fgText, "Spiral") != nullptr) {

      print->fFormat(fileName, 31, "%s%d", fgText, mdl->getValue("fixLeds").as<unsigned16>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        uint16_t fixLeds = mdl->getValue("fixLeds", rowNr);
        uint16_t radius = mdl->getValue("radius", rowNr);
        //first to middle (in mm)
        Coord3D middle;
        middle.x = fixFirst.x*10 + radius;
        middle.y = fixFirst.y*10;
        middle.z = fixFirst.z*10 + radius;

        genFix->spiral(middle, fixLeds, radius, fixIP, fixPin);
      });

    } else if (strstr(fgText, "Helix") != nullptr) {

      print->fFormat(fileName, 31, "%s%d", fgText, mdl->getValue("fixLeds").as<unsigned16>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        Coord3D fixRotate = mdl->getValue("fixRotate", rowNr);
        uint16_t fixLeds = mdl->getValue("fixLeds", rowNr);
        uint16_t radius = mdl->getValue("radius", rowNr);
        uint16_t pitch = mdl->getValue("pitch", rowNr);
        uint16_t deltaLed = mdl->getValue("deltaLed", rowNr);

        //first to middle (in mm)
        Coord3D middle;
        middle.x = fixFirst.x*10 + radius;
        middle.y = fixFirst.y*10;
        middle.z = fixFirst.z*10 + radius;

        genFix->helix(middle, fixLeds, radius, pitch, deltaLed, fixIP, fixPin, fixRotate.x, fixRotate.y, fixRotate.z);
      });

    } else if (strstr(fgText, "Wheel") != nullptr) {

      print->fFormat(fileName, 31, "%s%d%d", fgText, mdl->getValue("nrOfSpokes").as<unsigned8>(), mdl->getValue("ledsPerSpoke").as<unsigned8>());
      
      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        uint8_t ledsPerSpoke = mdl->getValue("ledsPerSpoke", rowNr);
          //first to middle (in mm)
        float size = 50 + 10 * ledsPerSpoke;
        Coord3D middle;
        middle.x = fixFirst.x*10 + size;
        middle.y = fixFirst.y*10 + size;
        middle.z = fixFirst.z*10;

        genFix->wheel(middle, mdl->getValue("nrOfSpokes", rowNr), ledsPerSpoke, fixIP, fixPin);
      });

    } else if (strstr(fgText, "Hexa") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        uint8_t ledsPerSide = mdl->getValue("ledsPerSide", rowNr);
        //first to middle (in mm)
        Coord3D middle = (fixFirst + Coord3D{ledsPerSide, ledsPerSide, 0})*10; //in mm
        genFix->hexagon(middle, ledsPerSide, fixIP, fixPin);
      });

    } else if (strstr(fgText, "Cone") != nullptr) {

      print->fFormat(fileName, 31, "%s%d", fgText, mdl->getValue("nrOfRings").as<unsigned8>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        uint8_t nrOfRings = mdl->getValue("nrOfRings", rowNr);

        //first to middle (in mm)
        float width = nrOfRings * 1.5f / M_PI + 1;
        Coord3D middle;
        middle.x = fixFirst.x*10 + width*10;
        middle.y = fixFirst.y*10;
        middle.z = fixFirst.z*10 + width*10;

        genFix->cone(middle, nrOfRings, fixIP, fixPin);
      });

    } else if (strstr(fgText, "Cloud5416") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        genFix->cloud5416(fixFirst*10, fixIP, fixPin);
      });

    } else if (strstr(fgText, "Wall") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        genFix->rings241(Coord3D{110,110,0}, 9, true, fixIP, fixPin);

        genFix->matrix(Coord3D{190,0,0}, Coord3D{190,80,0}, Coord3D{270,0,0}, fixIP, fixPin);
        genFix->matrix(Coord3D{0,190,0}, Coord3D{0,250,0}, Coord3D{500,190,0}, fixIP, fixPin);

        genFix->ring(Coord3D{190+80,80+80,0}, 48, fixIP, fixPin);

        // genFix.spiral(240, 0, 0, 48);
      });

    } else if (strstr(fgText, "Star") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {

        uint8_t ringsCentre = 170;
        uint8_t sateliteRadius = 130;

        genFix->rings241(Coord3D{ringsCentre,ringsCentre,0}, 9, true, fixIP, fixPin); //middle in mm

        Trigo trigo;

        for (int degrees = 0; degrees < 360; degrees+=45) {
          Coord3D middle;
          middle.x = ringsCentre + trigo.sin(sateliteRadius, degrees);
          middle.y = ringsCentre + trigo.cos(sateliteRadius, degrees);
          middle.z = 0;
          genFix->matrix(middle + Coord3D{-35,-35,0}, middle + Coord3D{-35,35,0}, middle + Coord3D{+35,-35,0}, fixIP, fixPin, 0, 0, degrees);
        }

      });

    } else if (strstr(fgText, "6Rings") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        genFix->rings241(Coord3D{110,110,0}, 9, true, fixIP, fixPin);
        genFix->rings241(Coord3D{0,110,110}, 9, true, fixIP, fixPin, 0, 90); //pan 90
        genFix->rings241(Coord3D{110,110,220}, 9, true, fixIP, fixPin);
        genFix->rings241(Coord3D{220,110,110}, 9, true, fixIP, fixPin, 0, 90); // pan 90
        genFix->rings241(Coord3D{110,0,110}, 9, true, fixIP, fixPin, 90); // tilt 90
        genFix->rings241(Coord3D{110,220,110}, 9, true, fixIP, fixPin, 90); // tilt 90
      });

    } else if (strstr(fgText, "SpaceStation") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        Trigo trigo;
        for (int i=0; i<360; i+=20) {
          uint8_t ringRadius = 50;
          uint8_t issRadius = 150;
          Coord3D middle = Coord3D{ringRadius + issRadius + trigo.sin(issRadius, i), ringRadius + issRadius + trigo.cos(issRadius, i), ringRadius};
          genFix->ring(middle, 12, fixIP, fixPin, 90, 0, 360-i); // tilt 90 then roll a bit
        }
      });

    } else if (strstr(fgText, "Human") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {

        //total dimensions width: 10 + 18 + 10
        // height 18 + 5 + 
        Coord3D headSize = {180, 180, 10};
        Coord3D armSize = {120, 50, 10};
        Coord3D legSize = {50, 200, 10};

        // head
        genFix->rings241({190,110,0}, 9, true, fixIP, fixPin);

        //arms
        genFix->matrix(Coord3D{0,headSize.y + 20,0}, Coord3D{0,headSize.y + 20 + armSize.y,0}, Coord3D{armSize.x,headSize.y + 20,0}, fixIP, fixPin);
        genFix->matrix(Coord3D{armSize.x + 140,headSize.y + 20,0}, Coord3D{armSize.x + 140,headSize.y + 20 + armSize.y,0}, Coord3D{380,headSize.y + 20,0}, fixIP, fixPin);

        //legs
        genFix->matrix(Coord3D{100, 280, 0}, Coord3D{100, 500, 0}, Coord3D{150,280,0}, fixIP, fixPin);
        genFix->matrix(Coord3D{230, 280, 0}, Coord3D{230, 500, 0}, Coord3D{280,280,0}, fixIP, fixPin);


      });

    } else if (strstr(fgText, "Globe") != nullptr) {

      print->fFormat(fileName, 31, "%s%d", fgText, mdl->getValue("width").as<unsigned16>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        uint16_t width = mdl->getValue("width", rowNr);
        //first to middle (in mm)
        Coord3D middle;
        middle.x = fixFirst.x*10 + 10 * width / 2;
        middle.y = fixFirst.y*10;
        middle.z = fixFirst.z*10 + 10 * width / 2;

        genFix->globe(middle, width, fixIP, fixPin);
      });

    } else if (strstr(fgText, "LeGlorb") != nullptr) {
      strcpy(fileName, fgText);

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {

        genFix->openPin(fixPin);

        genFix->ledSize = 40;
        genFix->shape = 1;

        char ledsString[] = "[[136,12,159],[96,12,146],[96,12,104],[136,12,91],[160,12,125],[184,49,193],[147,32,192],[118,49,215],[78,49,202],[68,32,167],[37,49,146],[37,49,104],[68,32,83],[78,49,48],[118,49,35],[147,32,58],[184,49,57],[208,49,90],[196,32,125],[208,49,160],[219,106,193],[191,84,214],[160,103,234],[124,84,236],[89,106,236],[60,84,215],[32,103,192],[19,84,159],[8,106,125],[19,84,91],[32,103,58],[60,84,35],[89,106,14],[124,84,14],[160,103,16],[191,84,36],[219,106,57],[230,84,90],[240,103,125],[230,84,160],[218,147,192],[190,166,215],[161,144,236],[126,166,236],[90,147,234],[59,166,214],[31,144,193],[20,166,160],[10,147,125],[20,166,90],[31,144,57],[59,166,36],[90,147,16],[126,166,14],[161,144,14],[190,166,35],[218,147,58],[231,166,91],[242,144,125],[231,166,159],[182,218,167],[172,201,202],[132,201,215],[103,218,192],[66,201,193],[42,201,160],[54,218,125],[42,201,90],[66,201,57],[103,218,58],[132,201,35],[172,201,48],[182,218,83],[213,201,104],[213,201,146],[154,238,146],[114,238,159],[90,238,125],[114,238,91],[154,238,104]]";
        JsonDocument leds;
        deserializeJson(leds, ledsString);

        for (JsonArray pixel:leds.as<JsonArray>()) {
          Coord3D pix = {pixel[0], pixel[1], pixel[2]};
          genFix->write3D(pix + fixFirst);
          ppf("pixel %d %d %d\n", pixel[0], pixel[1], pixel[2]);
        }

        genFix->closePin();

      });

    } else if (strstr(fgText, "GeodesicDome") != nullptr) {

      print->fFormat(fileName, 31, "%s%d", fgText, mdl->getValue("radius").as<unsigned16>());

      getFixtures(fileName, [](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        genFix->geodesicDome(fixFirst, mdl->getValue("radius", rowNr), fixIP, fixPin);
      });

    } else if (strstr(fgText, "Curtain") != nullptr) {

      uint16_t width = mdl->getValue("width");
      uint16_t height = mdl->getValue("height");
      print->fFormat(fileName, 31, "%s%dx%d", fgText, width, height);

      getFixtures(fileName, [width, height](GenFix * genFix, unsigned8 rowNr, Coord3D fixFirst, unsigned8 fixIP, unsigned8 fixPin) {
        genFix->openPin(fixPin);

        uint8_t offX;
        for (int y=0; y<width; y++) {
          for (int x=0; x<height; x++) {
            offX = (y%2 == 1)?5:0;
            genFix->write3D(x*10 + offX + fixFirst.x, y*10 + fixFirst.y, fixFirst.z);
          }
        }

        genFix->closePin();
      });
    }

    files->filesChanged = true;
  }

  // File openFile(const char * name) {
  //   char fileName[30] = "/";
  //   strncat(fileName, name, sizeof(fileName)-1);
  //   strncat(fileName, ".json", sizeof(fileName)-1);

  //   File f = files->open(fileName, "w");
  //   if (!f)
  //     ppf("fixtureVar Could not open file %s for writing\n", fileName);

  //   return f;
  // }

};

extern LedModFixtureGen *lfg;