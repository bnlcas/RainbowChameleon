#include "FastLED.h"

#define NUM_LEDS 216
#define NUM_PLAYER_LEDS 20

#define DATA_PIN             3
#define CLOCK_PIN            4
#define LED_COLOR_ORDER      BGR
//BGR//GBR

#define BRIGHTNESS 100

#define MAX_COLOR_BLOCK_SIZE 120
#define MAX_SPEED 5

#define InitialStartHue 0
#define InitialHueRange 0
#define InitialHueMatchTolerance 90
#define InitialColorBlockSize 40
#define InitialBlockSpeed 1

#define SOFT_POT_PIN A1

CRGB _ledStrip[NUM_LEDS];
TGradientDirectionCode LED_DIRECTION = FORWARD_HUES;
int Score = 0;

const int HSV_SATURATION = 200;
const int HSV_VALUE = 100;

int _startHue = InitialStartHue;
int _hueRange = InitialHueRange;
int _endHue = InitialStartHue;
int _hueMatchTolerance = InitialHueMatchTolerance;

uint16_t _colorBlockSize = InitialColorBlockSize;
float _colorBlockPosition = NUM_LEDS;
float _speed = InitialBlockSpeed;

bool _gameOn = true;


void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(SOFT_POT_PIN));
  pinMode(SOFT_POT_PIN, INPUT);
  SetupLEDS();
}

void SetupLEDS()
{
  //FastLED.addLeds<WS2812B, DATA_PIN, RGB>(_ledStrip, NUM_LEDS); 
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, LED_COLOR_ORDER>(_ledStrip, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  int softPotADC = analogRead(SOFT_POT_PIN);
  int hue = (((softPotADC) / 4)) % 255;
  FastLED.clear();
  UpdateGame(hue);
  SetPlayerLeds(hue);
  FastLED.show();
}

void SetPlayerLeds(int hue)
{
    fill_solid(_ledStrip, NUM_PLAYER_LEDS, CHSV(hue,HSV_SATURATION,HSV_VALUE));
}

void UpdateGame(int playerHue)
{
    uint16_t startingPosition;
    uint16_t endPosition;
    SetBlockPositions(_colorBlockPosition, _colorBlockSize, &startingPosition, &endPosition);

    if(_gameOn)
    {
        fill_gradient(_ledStrip, startingPosition,
            CHSV(_startHue, HSV_SATURATION, HSV_VALUE),
            endPosition,
            CHSV(_endHue, HSV_SATURATION, HSV_VALUE),
            LED_DIRECTION);
        _colorBlockPosition -= _speed;

        if(_colorBlockPosition <= NUM_PLAYER_LEDS && endPosition > NUM_PLAYER_LEDS)
        {
           _gameOn = CheckHueMatch(playerHue);
           if(_gameOn)
           {
               Score += 1;
           }
        }
        if((int)endPosition <= NUM_PLAYER_LEDS)
        {
            ResetColorBlock();
        }
    }
    else{
        FadeToBlack(playerHue, startingPosition, endPosition);
        ResetGame();
        //Reset Game: Fade to Gray, Pause, Reset Constants
    }
   //_startHue += 1;
  delay(50);
}

void SetBlockPositions(float colorBlockPosition, uint16_t colorBlockSize, uint16_t* startPosition, uint16_t * endPosition)
{
    if(colorBlockPosition < 0)
    {
        *startPosition = 0;
        *endPosition = (uint16_t)(_colorBlockPosition + (float)_colorBlockSize);
    }
    else
    {
        *startPosition = (uint16_t)_colorBlockPosition;
        *endPosition = min(NUM_LEDS, *startPosition + _colorBlockSize);
    }
}

bool CheckHueMatch(int playerHue)
{
    //CRGB currentBlockColor = _ledStrip[NUM_PLAYER_LEDS + 1];
    CHSV currentBlockColorHSV = rgb2hsv_approximate(  _ledStrip[NUM_PLAYER_LEDS + 1] );
    int hue = currentBlockColorHSV.hue;
    //int hue = CalculateHue(_ledStrip[NUM_PLAYER_LEDS + 1]);

    int hueDiff = abs(playerHue - hue);
    
    if(hueDiff > 127)
    {
      hueDiff = 255 - hueDiff;
    }
    //Serial.print( playerHue );
    //Serial.print(',');
    //Serial.print(hue);
    //Serial.print(',');
    //Serial.print(hueDiff);
    //Serial.print('\n');
    bool isMatch = hueDiff < _hueMatchTolerance;
    return isMatch;
    //return true;
}


void FadeToBlack(int playerHue, uint16_t blockStartPosition, uint16_t blockEndPosition)
{
    const int stepDuration = 50;
    const int nSteps = 8;
    const int grayDuration = 500;

    delay(grayDuration);

    for(int i = 0; i < nSteps; i++)
    {
        int saturationStep = (int)( (1.0f - ((float)i)/((float)nSteps)) * (float) HSV_SATURATION);
        
        fill_gradient(_ledStrip, blockStartPosition,
            CHSV(_startHue, saturationStep, HSV_VALUE),
            blockEndPosition,
            CHSV(_endHue, saturationStep, HSV_VALUE),
            LED_DIRECTION);
        
        fill_solid(_ledStrip, NUM_PLAYER_LEDS, CHSV(playerHue,saturationStep,HSV_VALUE));
        
        FastLED.show();     // Show the leds
        delay(stepDuration);
    }
    delay(grayDuration);
    for(int i = 0; i < nSteps; i++)
    {
        int valueStep = (int)( (1.0f - ((float)i)/((float)nSteps)) * (float) HSV_VALUE);
        
        fill_gradient(_ledStrip, blockStartPosition,
            CHSV(_startHue, 0, valueStep),
            blockEndPosition,
            CHSV(_endHue, 0, valueStep),
            LED_DIRECTION);
        
        fill_solid(_ledStrip, NUM_PLAYER_LEDS, CHSV(playerHue,0,valueStep));
        
        FastLED.show();
        delay(stepDuration);
    }
    delay(grayDuration);
}

void ResetColorBlock()
{
    _hueRange = min(245, _hueRange + 12);
    _startHue = random(255 - _hueRange);
    _endHue = (_startHue + _hueRange) % 255;
    if(_endHue < _startHue)
    {
      _endHue = max(0, _startHue - _hueRange);
    }
    _hueMatchTolerance = max(25, _hueMatchTolerance - 5);
    _colorBlockSize = min(MAX_COLOR_BLOCK_SIZE, _colorBlockSize + 10);
    _colorBlockPosition = NUM_LEDS - 1;
    
    if(_hueRange > 100)
    {
        LED_DIRECTION = FORWARD_HUES;
    }
    _speed = min(MAX_SPEED, _speed + 0.2f);// 0.1f;
}

void ResetGame()
{
    _startHue = random(255);
    _hueRange = InitialHueRange;
    _endHue = (_startHue + _hueRange) % 255;
    _hueMatchTolerance = InitialHueMatchTolerance;

    _colorBlockSize = InitialColorBlockSize;
    _colorBlockPosition = NUM_LEDS;
    _speed = InitialBlockSpeed;
    LED_DIRECTION = FORWARD_HUES;
    _gameOn = true;
}
