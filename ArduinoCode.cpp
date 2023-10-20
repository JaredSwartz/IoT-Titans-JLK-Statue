#include <Adafruit_NeoPixel.h>
#include <stdint.h>
#include <string.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

//Global variiables
#define SIG1                38          // using pin 38 for signal 1 (mux array 1)
#define SIG2                51          // using pin 51 for data signal 2 (mux array 2)
#define BRIGHTNESS          128         // level of brightness of all LEDs (8 bit value --> [0, 255])const int rings = 17;
const uint8_t rings = 17;
const uint8_t crystals = 22;

const uint8_t pixel = 24;
const uint8_t muxX = 16;
const uint8_t muxY = 16;


//-------------------------INSTANTIATE LED RING OBJECT-------------------------//
// Parameter 1 = number of pixels in ring
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
Adafruit_NeoPixel muxA = Adafruit_NeoPixel(24, SIG1, NEO_KHZ800 + NEO_GRB);
Adafruit_NeoPixel muxB = Adafruit_NeoPixel(24, SIG2, NEO_KHZ800 + NEO_GRB);

//Color class used for arrays
class Color{
private:
    uint8_t red = 0;
    uint8_t blue = 0;
    uint8_t green = 0;

public:
    Color();
    void setColor(Color c);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    bool equals(Color c);
    bool notEquals(Color c);
    uint8_t getRed();
    uint8_t getGreen();
    uint8_t getBlue();
    uint32_t getMuxColor();
};

Color::Color(){
    red = 0;
    green = 0;
    blue = 0;
}

// Define colors
Color off;
Color blue;
Color cyan;
Color green;
Color red;
Color purple;
Color orange;
Color yellow;
Color white;
Color pink;

Color muxArrayA[muxX][muxY];
Color muxArrayB[muxX][muxY];
Color muxArrayAOld[muxX][muxY];
Color muxArrayBOld[muxX][muxY];

//Color statueArray[rings_table][crystals_table];
Color* sortedArray[rings][crystals];
Color emptyColor;

//Setter
void Color::setColor(Color c){
    red = c.red;
    green = c.green;
    blue = c.blue;
}
void Color::setColor(uint8_t r, uint8_t g, uint8_t b){
    if((r < 256 && r >= 0) && (g < 256 && g >= 0) && (b < 256 && b >= 0)){
        red = r;
        green = g;
        blue = b;
    } else{
        red = 0;
        green = 0;
        blue = 0;
    }
}

//Getter
uint8_t Color::getRed(){ return red; }
uint8_t Color::getBlue(){ return blue; }
uint8_t Color::getGreen(){ return green; }

bool Color::equals(Color c){
    return (red == c.red) && (green == c.green) && (blue == c.blue);
}
bool Color::notEquals(Color c){
    return (red != c.red) || (green != c.green) || (blue != c.blue);
}


uint32_t Color::getMuxColor(){ return muxA.Color(red, green, blue); }

// Define and instantiate demux select switch pin arrays (Colored wires that are the output of Arduino to control board)
uint8_t innerMux1[4] = {22, 23, 24, 25};
uint8_t outerMux1[4] = {26, 27, 28, 29};
uint8_t innerMux2[4] = {30, 31, 32, 33};
uint8_t outerMux2[4] = {34, 35, 36, 37};

//Function prototypes
void defaultStatue();
//void transferStatueToMux();
void setupSortedArray();

// Set Statue Functions
void setRow(int row, int value);
void setColumn(int column, int value);
void setCrystal(int row, int column, int value);

// Chase Functions
void chaseRow(int value);
void chaseCrystal(int value);
void chaseStatue(int value);

// Display Functions
void displayMuxA();
void displayMuxB();
void displayStatue();
void display();

// Test Functions
void setRowTest(int row, int value);
void setColumnTest(int column, int value);
void chaseRowTest(int value);
void chaseCrystalTest(int value);
void chaseStatueTest(int value);
void checkCrystals(int ring, int crystal);
void firstCrystal();


// Just for Fun Just For Fun Just for Fun
void colormStatue();
void fadeWheel();

//----------------------------------------------SETUP-----------------------------------------------//
void setup(){
    for(int i = 3; i >= 0; i--){
        pinMode(innerMux1[i], OUTPUT);
        pinMode(outerMux1[i], OUTPUT);
        pinMode(innerMux2[i], OUTPUT);
        pinMode(outerMux2[i], OUTPUT);

        digitalWrite(innerMux1[i], LOW);
        digitalWrite(outerMux1[i], LOW);
        digitalWrite(innerMux2[i], LOW);
        digitalWrite(outerMux2[i], LOW);
    }

    muxA.begin();                      // Start higher LED rings
    muxB.begin();                      // Start lower LED rings
    muxA.setBrightness(BRIGHTNESS);    // Set higher LEDs brightness
    muxB.setBrightness(BRIGHTNESS);    // Set higher LEDs brightness
    muxA.show();                       // Initialize all pixels to 'off'
    muxB.show();                       // Initialize all pixels to 'off'

    setupSortedArray();

    off.setColor(0, 0, 0);
    blue.setColor(0, 0, 255);
    cyan.setColor(0, 100, 255);
    green.setColor(0, 255, 0);
    red.setColor(255, 0, 0);
    purple.setColor(255, 0, 255);
    orange.setColor(255, 30, 0);
    yellow.setColor(255, 210, 0);
    white.setColor(255, 255, 255);
    pink.setColor(244, 41, 65);

    defaultStatue();

    // Sets up serial connection
    Serial.begin(9600);
    Serial.setTimeout(1);
}

Color mergeColors(Color c1, Color c2, float percent){
    float p = percent;
    if(percent < 0){
        p = 0.0;
    }
    if(percent > 1){
        p = 1;
    }
    Color out;
    uint8_t red = c1.getRed() * p + c2.getRed() * (1 - p);
    uint8_t green = c1.getGreen() * p + c2.getGreen() * (1 - p);
    uint8_t blue = c1.getBlue() * p + c2.getBlue() * (1 - p);
    out.setColor(red, green, blue);
    return out;

}

//----------------------------------------------LOOP-----------------------------------------------//
float offset = -1;
float offset_two = 5;
float diff = 0;
float diff_two = 0;
Color c;
uint8_t mode = 1;
String incoming = "";
void loop(){
    if(mode == 0){
        setStatue(green);
        for(int i = 11 - 1; i >= 0; i--){
            if(i % 2 == 0){
                setRing(i, red);
            } else{
                setRing(i, green);
            }
        }
    } else if(mode == 1){
        setStatue(orange);
        for(int i = 11 - 1; i >= 0; i--){
            c = mergeColors(orange, white, abs(i - offset));
            setRing(i, c);
        }
        offset += 0.1;
        offset_two += 0.1;
        if(offset > 11){
            offset = -1;
        }
    } else if(mode == 2){
        setStatue(off);
    } else if (mode == 3){
        setStatue(pink);
        
        
//        for(int i = 11 - 1; i >= 0; i--){
//            diff = abs(i - offset);
//            diff_two = abs(i - offset_two);
//            if(diff_two < 2){
//                c = mergeColors(pink, yellow, diff_two);
//            }else{
//                c = mergeColors(pink, cyan, diff);
//            }
//            setRing(i, c);
//        }
//        offset += 0.2;
//        offset_two += 0.2;
//        if(offset > 11){
//            offset = -1;
//        }
//        if(offset_two > 11){
//          offset_two = -1;
//        }
        setRing(9, yellow);
        setRing(10, cyan);
    }



    display();
    if(Serial.available() > 0){
        incoming += Serial.readStringUntil("\n");
        if(incoming.endsWith("\n")){
            if(incoming == "christmas\n"){
                mode = 0;
                Serial.write("CHRISTMAS\n");
            } else if(incoming == "normal\n"){
                mode = 1;
                Serial.write("NORMAL\n");
            } else if(incoming == "off\n"){
                mode = 2;
                offset = -1;
                Serial.write("OFF\n");
            }else if(incoming == "bocchi\n"){
                mode = 3;
                offset = -1;
                offset_two = 5;
                Serial.write("BOCCHI\n");
            }else{
                Serial.write(String("UNKNOWN("+incoming).c_str());  
            }
            incoming = "";
        }
    }
}

// --------------------------------------------- FUNCTIONS -----------------------------------------//
//Fill Functions
void defaultStatue(){
    for(int i = 0; i < muxY; i++){
        for(int j = 0; j < muxX; j++){
            muxArrayA[i][j].setColor(0, 0, 0);
            muxArrayB[i][j].setColor(0, 0, 0);
        }
    }
}

//Set Statue Functions
void setStatue(Color color){
    for(int i = 0; i < muxY; i++){
        for(int j = 0; j < muxX; j++){
            muxArrayA[i][j].setColor(color);
            muxArrayB[i][j].setColor(color);
        }
    }
}

void setStatue(uint8_t r, uint8_t g, uint8_t b){
    for(int i = 0; i < muxY; i++){
        for(int j = 0; j < muxX; j++){
            muxArrayA[i][j].setColor(r, g, b);
            muxArrayB[i][j].setColor(r, g, b);
        }
    }
}

void setRing(int ring, Color color){
    for(int j = 0; j < crystals; j++){
        sortedArray[ring][j]->setColor(color);
    }
}

void setRing(int ring, uint8_t r, uint8_t g, uint8_t b){
    for(int j = 0; j < crystals; j++){
        sortedArray[ring][j]->setColor(r, g, b);
    }
}

void setCrystal(int row, int column, Color color){
    sortedArray[row][column]->setColor(color);;
    //transferStatueToMux();
    //display();
}

void setCrystal(int row, int column, int r, int g, int b){
    sortedArray[row][column]->setColor(r, g, b);
}

//Chase Functions
void chaseCrystal(Color value){
    for(int row = 0; row < rings; row++){
        for(int column = 0; column < crystals; column++){
            //defaultStatue();
            setCrystal(row, column, value);
            //transferStatueToMux();
            display();
            //delay(100);
        }
    }
}

//Display Functions
void display(){
    displayMux(innerMux1, outerMux1, &muxA, &muxArrayA, &muxArrayAOld);
    displayMux(innerMux2, outerMux2, &muxB, &muxArrayB, &muxArrayBOld);
}

void displayNew(int bit){
    displayMuxNew(innerMux1, outerMux1, &muxA, bit);
    displayMuxNew(innerMux2, outerMux2, &muxB, bit);
}

void displayMux(uint8_t innerMux[4], uint8_t outerMux[4], Adafruit_NeoPixel* mux, Color(*muxArray)[muxX][muxY], Color(*muxArrayOld)[muxX][muxY]){
    for(int i = 0; i < muxX; i++){
        digitalWrite(innerMux[0], bitRead(i, 0));
        digitalWrite(innerMux[1], bitRead(i, 1));
        digitalWrite(innerMux[2], bitRead(i, 2));
        digitalWrite(innerMux[3], bitRead(i, 3));

        for(int j = 0; j < muxY; j++){
            if((*muxArray)[i][j].getMuxColor()!=(*muxArrayOld)[i][j].getMuxColor()){
                digitalWrite(outerMux[0], bitRead(j, 0));
                digitalWrite(outerMux[1], bitRead(j, 1));
                digitalWrite(outerMux[2], bitRead(j, 2));
                digitalWrite(outerMux[3], bitRead(j, 3));

                mux->fill((*muxArray)[i][j].getMuxColor());
                (*muxArrayOld)[i][j].setColor((*muxArray)[i][j]);

                mux->show();
            }
        }
    }
}

void displayMuxNew(uint8_t innerMux[4], uint8_t outerMux[4], Adafruit_NeoPixel* mux, int bit){
    Color c;
    for(int i = 0; i < muxX; i++){
        digitalWrite(innerMux[0], bitRead(i, 0));
        digitalWrite(innerMux[1], bitRead(i, 1));
        digitalWrite(innerMux[2], bitRead(i, 2));
        digitalWrite(innerMux[3], bitRead(i, 3));

        for(int j = 0; j < muxY; j++){
            digitalWrite(outerMux[0], bitRead(j, 0));
            digitalWrite(outerMux[1], bitRead(j, 1));
            digitalWrite(outerMux[2], bitRead(j, 2));
            digitalWrite(outerMux[3], bitRead(j, 3));

            if(bit < 4){
                c.setColor(bitRead(i, bit), bitRead(i, bit), bitRead(i, bit));
            } else{
                c.setColor(bitRead(j, bit), bitRead(j, bit), bitRead(j, bit));
            }

            mux->fill(c.getMuxColor());

            mux->show();
        }
    }
}

void testDisplay(){
    for(int a = 0; a < 16; a++){
        bool bit0 = bitRead(a, 0);
        bool bit1 = bitRead(a, 1);
        bool bit2 = bitRead(a, 2);
        bool bit3 = bitRead(a, 3);

        digitalWrite(innerMux1[0], bit0);
        digitalWrite(innerMux1[1], bit1);
        digitalWrite(innerMux1[2], bit2);
        digitalWrite(innerMux1[3], bit3);

        for(int b = 0; b < 16; b++){
            bool bit4 = bitRead(b, 0);
            bool bit5 = bitRead(b, 1);
            bool bit6 = bitRead(b, 2);
            bool bit7 = bitRead(b, 3);

            digitalWrite(outerMux1[0], bit4);
            digitalWrite(outerMux1[1], bit5);
            digitalWrite(outerMux1[2], bit6);
            digitalWrite(outerMux1[3], bit7);


            for(int c = 0; c < 16; c++){
                bool bit8 = bitRead(c, 0);
                bool bit9 = bitRead(c, 1);
                bool bit10 = bitRead(c, 2);
                bool bit11 = bitRead(c, 3);

                digitalWrite(innerMux2[0], bit8);
                digitalWrite(innerMux2[1], bit9);
                digitalWrite(innerMux2[2], bit10);
                digitalWrite(innerMux2[3], bit11);

                for(int d = 0; d < 16; d++){
                    bool bit12 = bitRead(d, 0);
                    bool bit13 = bitRead(d, 1);
                    bool bit14 = bitRead(d, 2);
                    bool bit15 = bitRead(d, 3);

                    digitalWrite(outerMux2[0], bit12);
                    digitalWrite(outerMux2[1], bit13);
                    digitalWrite(outerMux2[2], bit14);
                    digitalWrite(outerMux2[3], bit15);

                    muxA.fill(green.getMuxColor());
                    muxA.show();

                    muxB.fill(green.getMuxColor());
                    muxB.show();

                }
            }
        }
    }
}

void displayStatueColor(Color color){
    setStatue(color);
    display();
}
// Test Functions
void checkCrystals(int ring, int crystal){
    //setStatue(off);
    firstCrystal();
    //setBottomHalf(white);

    if(ring < 17 && crystal < 22 && ring >= 0 && crystal >= 0){
        setCrystal(ring, crystal, white);
        crystal++;
        if(crystal >= 22){
            crystal = 0;
            ring++;
        }
        setCrystal(ring, crystal, red);
        crystal++;
        if(crystal >= 22){
            crystal = 0;
            ring++;
        }
        setCrystal(ring, crystal, green);
        crystal++;
        if(crystal >= 22){
            crystal = 0;
            ring++;
        }
        setCrystal(ring, crystal, blue);
        crystal++;
        if(crystal >= 22){
            crystal = 0;
            ring++;
        }
        setCrystal(ring, crystal, yellow);
        crystal++;
        if(crystal >= 22){
            crystal = 0;
            ring++;
        }
        setCrystal(ring, crystal, purple);
        crystal++;
        if(crystal >= 22){
            crystal = 0;
            ring++;
        }
        setCrystal(ring, crystal, orange);
    } else{
        setStatue(off);
        return;
    }

    //transferStatueToMux();
    display();
}
// **************************** Set Index ******************
void firstCrystal(){
    setCrystal(0, 0, red);
    setCrystal(2, 15, red);
    setCrystal(3, 7, red);
    setCrystal(4, 17, red);
    setCrystal(6, 6, red);
    setCrystal(6, 20, red);
    setCrystal(7, 18, red);
    setCrystal(9, 1, red);
    setCrystal(11, 0, red);
    setCrystal(9, 19, red);
    setCrystal(10, 15, red);
    setCrystal(13, 16, red);
    return;
}

//---------JUST FOR FUN JUST FOR FUN JUST FOR FUN JUST FOR FUN---------//
void randomStatue(){
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    short sum = 765;
    Color color;
    color.setColor(int(random(0, 255)), int(random(0, 255)), int(random(0, 255)));

    for(uint8_t i = 0; i < 17; i++){
        for(uint8_t j = 0; j < 22; j++){
            r = int(random(0, 255));
            if(r < 100) r = 0;
            g = int(random(0, 255));
            if(g < 100) g = 0;
            b = int(random(0, 255));
            if(b < 100) b = 0;
            sum = r + g + b;
            color.setColor(r, g, b);
            setCrystal(i, j, color);
        }
    }
    //transferStatueToMux();
    display();
}

void fadeWheel(){
    Color fadeColor;

    uint8_t speed = 5; // Max of 255

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    while(true){
        // Off
        if(r == 0 && g == 0 && b == 0){
            for(b = 0; b < 255; b += speed){
                fadeColor.setColor(r, g, b);
                setStatue(fadeColor);
                //transferStatueToMux();
                display();
            }
            r = 0;
            g = 0;
            b = 255;
        }
        // Blue
        if(r == 0 && g == 0 && b == 255){
            for(g = 0; g < 255; g += speed){
                fadeColor.setColor(r, g, b);
                setStatue(fadeColor);
                //transferStatueToMux();
                display();
            }
            r = 0;
            g = 255;
            b = 255;
        }
        // Teal
        if(r == 0 && g == 255 && b == 255){
            for(b = 255; b > 0; b -= speed){
                fadeColor.setColor(r, g, b);
                setStatue(fadeColor);
                //transferStatueToMux();
                display();
            }
            r = 0;
            g = 255;
            b = 0;
        }
        // Green
        if(r == 0 && g == 255 && b == 0){
            for(r = 0; r < 255; r += speed){
                fadeColor.setColor(r, g, b);
                setStatue(fadeColor);
                //transferStatueToMux();
                display();
            }
            r = 255;
            g = 255;
            b = 0;
        }
        // Yellow
        if(r == 255 && g == 255 && b == 0){
            for(g = 255; g > 0; g -= speed){
                fadeColor.setColor(r, g, b);
                setStatue(fadeColor);
                //transferStatueToMux();
                display();
            }
            r = 255;
            g = 0;
            b = 0;
        }
        // Red
        if(r == 255 && g == 0 && b == 0){
            for(b = 0; b < 255; b += speed){
                fadeColor.setColor(r, g, b);
                setStatue(fadeColor);
                //transferStatueToMux();
                display();
            }
            r = 255;
            g = 0;
            b = 255;
        }
        // Pink
        if(r == 255 && g == 0 && b == 255){
            for(r = 255; r > 0; r -= speed){
                fadeColor.setColor(r, g, b);
                setStatue(fadeColor);
                //transferStatueToMux();
                display();
            }
            r = 0;
            g = 0;
            b = 255;
        }
    }

}


void setupSortedArray(){
    short conversionArr[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15,128,136,132,140,130,-1,138,134,142,129,137,133,141,131,139,135,143,64,72,68,76,66,74,70,78,65,-1,-1,73,69,77,67,75,71,79,192,200,196,204,194,202,198,206,193,201,197,205,195,203,-1,172,162,170,166,174,161,169,165,173,163,171,167,175,96,104,100,108,98,106,102,110,-1,199,207,32,40,36,44,34,42,38,46,33,41,37,45,35,43,39,47,160,168,164,-1,105,101,109,99,107,103,111,224,232,228,236,226,234,230,238,225,233,229,237,227,97,-1,152,148,156,146,154,150,158,145,153,149,157,147,155,151,159,-1,88,84,92,82,-1,-1,90,86,94,81,89,85,93,83,91,87,95,208,216,212,220,210,218,214,222,209,-1,-1,217,213,221,211,219,215,223,48,56,52,60,50,58,54,62,49,57,53,61,51,-1,-1,59,55,63,176,184,180,188,178,186,182,190,177,185,181,189,179,187,183,191,112,-1,-1,242,120,116,124,114,122,118,126,113,121,117,125,115,123,119,127,240,248,244,252,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,269,-1,267,-1,271,-1,-1,-1,-1,-1,-1,268,-1,-1,-1,-1,-1,265,253,-1,-1,-1,235,259,-1,-1,24,-1,28,-1,26,-1,-1,-1,-1,-1,-1,27,23,-1,-1,-1,-1,-1,302,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    short value;
    uint8_t y;
    uint8_t x;
    int array_location;
    for(int i = 0; i < rings; i++){
        for(int j = 0; j < crystals; j++){
            value = conversionArr[i * crystals + j];

            if(value == -1){
                sortedArray[i][j] = &emptyColor;
                continue;
            }

            array_location = value;

            x = array_location / muxX;
            y = array_location % muxX;
            if(x < muxX){
                sortedArray[i][j] = &muxArrayA[x][y];
            } else{
                sortedArray[i][j] = &muxArrayB[x - muxX][y];
            }
        }
    }
}
