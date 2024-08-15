#include <Settingator.h>
#include <MiscDef.h>
#include <ESPNowCommunicator.h>
#include <HardwareSerialCommunicator.h>
#include <FastLED.h>
#include <Arduino.h>

#define RED         13
#define GREEN       12
#define YELLOW      14
#define BLUE        27

#define DATA_PIN    26
#define NUM_LEDS    52

CRGB leds[NUM_LEDS];


#define NO_ANIMATION        0
#define BLUE_LOADING        1
#define RED_ACCEL_LOADING   2
#define GREEN_LOADING       3
#define BLUE_FROZEN         4
#define RED_FROZEN          5
#define GREEN_GOOD          6
#define RED_BAD             7

uint8_t animation = NO_ANIMATION;

uint8_t answerPow = 3;
char*   answerPowLabel = (char*)malloc(18);
uint8_t refPower = 0;

int state = LOW;
char* digitalLabel = (char*)malloc(5);
uint8_t refDigitalLabel = 0;

uint16_t voltage = 512;
char* analogLabel = (char*)malloc(5);
uint8_t refAnalogLabel = 0;

#define STATE_PIN 4
#define LASER_NOTIF 0x05

ulong stateTimer = millis();

void UpdateState()
{
    int newState = digitalRead(STATE_PIN);
    if (newState == HIGH && state == LOW)
    {
        STR.SendDirectSettingUpdate(0x02);
        STR.SendNotif(LASER_NOTIF);
    }

    if (state != newState)
    {
        STR.UpdateSetting(refAnalogLabel, (byte*)(newState ? "HIGH" : "LOW "), 4);
        state = newState;    
    }
}

void NoAnimation()
{
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = CRGB(0, 0, 0);
    
}

void BlueLoading()
{
    ulong speed = 500;
    ulong t = millis() % speed;
    uint8_t mid = NUM_LEDS / 2;

    uint8_t val;
    uint8_t offset = ((float)t / (float)speed * (float)(mid + 9));
    for (int i = 0; i < mid; i++)
    {
        val = (uint8_t)(255.0 * pow((1.0 - ((float)(i + offset) / (float)(mid + 9 - 1))), answerPow));
        leds[i] = leds[i + mid] = CRGB(0, 0, val);
    }
}

void RedAccelLoading()
{
    ulong speed = 250;
    ulong t = millis() % speed;
    uint8_t mid = NUM_LEDS / 2;

    uint8_t val;
    uint8_t offset = ((float)t / (float)speed * ((float)mid + 9.0));
    for (int i = 0; i < mid; i++)
    {
        val = (uint8_t)(255.0 * pow((1.0 - ((float)(i + offset) / (float)(mid + 9 - 1))), answerPow));
        leds[i] = leds[i + mid] = CRGB(val, 0, 0);
    }
}

void GreenLoading()
{
    ulong speed = 500;
    ulong t = millis() % speed;
    uint8_t mid = NUM_LEDS / 2;

    uint8_t val;
    uint8_t offset = ((float)t / (float)speed * ((float)mid + 9.0));
    for (int i = 0; i < mid; i++)
    {
        val = (uint8_t)(255.0 * pow((1.0 - ((float)(i + offset) / (float)(mid + 9 - 1))), answerPow));
        leds[i] = leds[i + mid] = CRGB(0, val, 0);
    }
}

void BlueFrozen()
{
}

void RedFrozen()
{
}

void GreenGood()
{
    uint8_t mid = NUM_LEDS / 2;
    uint8_t val = 0;

    for (int i = 0; i < mid; i++)
    {
        val = (uint8_t)(255.0 * pow((1.0 - ((float)i / (float)(mid - 1))), answerPow));
        leds[i] = CRGB(0, val, 0);
        leds[NUM_LEDS - 1 - i] = CRGB(0, val, 0);
        Serial.println(val);
    }

}

void RedBad()
{
    uint8_t mid = NUM_LEDS / 2;
    uint8_t val = 0;

    for (int i = 0; i < mid; i++)
    {
        val = (uint8_t)(255.0 * pow((1.0 - ((float)i / (float)(mid - 1))), answerPow));
        leds[i] = CRGB(val, 0, 0);
        leds[NUM_LEDS - 1 - i] = CRGB(val, 0, 0);
    }

}

void   (*AnimArray[8])() = {&NoAnimation,
                            &BlueLoading,
                            &RedAccelLoading, 
                            &GreenLoading,
                            &BlueFrozen,
                            &RedFrozen,
                            &GreenGood,
                            &RedBad};

void UpdateAnimation()
{
    AnimArray[animation]();
}

void ConfigureButtonPin(uint8_t pin)
{
    pinMode(pin, INPUT_PULLDOWN);
    attachInterruptArg(pin, [](void* pin){ STR.SendNotif(*(uint8_t*)pin); }, &pin, RISING);
}

void setup()
{
    pinMode(STATE_PIN, INPUT);

    ConfigureButtonPin(RED);
    ConfigureButtonPin(BLUE);
    ConfigureButtonPin(GREEN);
    ConfigureButtonPin(YELLOW);

    
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    STR.SetCommunicator(ESPNowCTR::CreateInstanceDiscoverableWithSSID("Desk"));
    //STR.SetCommunicator(HardwareSerialCTR::CreateInstance(9600));

    refDigitalLabel = STR.AddSetting(Setting::Type::Label, digitalLabel, sizeof(digitalLabel), "State");
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "NO ANIMATION", [](){ animation = NO_ANIMATION; });
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "BLUE LOADING", [](){ animation = BLUE_LOADING; });
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "RED ACCEL LOADING", [](){ animation = RED_ACCEL_LOADING; });
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "GREEN LOADING", [](){ animation = GREEN_LOADING; });
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "BLUE FROZEN", [](){ animation = BLUE_FROZEN; });
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "RED FROZEN", [](){ animation = RED_FROZEN; });
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "GREEN GOOD", [](){ animation = GREEN_GOOD; });
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "RED BAD", [](){ animation = RED_BAD; });
    refPower = STR.AddSetting(Setting::Type::Label, answerPowLabel, sizeof(answerPowLabel), "Arabe Power");
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "Answer Pow +1", [](){ DEBUG_PRINT_LN("Callback");answerPow += 1;
                                                                                answerPowLabel = itoa(answerPow, answerPowLabel, 10);
                                                                                STR.UpdateSetting(refPower, (unsigned char*)answerPowLabel, sizeof(answerPowLabel));});
    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "Answer Pow -1", [](){ answerPow -= 1;
                                                                                answerPowLabel = itoa(answerPow, answerPowLabel, 10);
                                                                                STR.UpdateSetting(refPower, (unsigned char*)answerPowLabel, sizeof(answerPowLabel));});


    answerPowLabel = itoa(answerPow, answerPowLabel, 10);
}

void loop()
{
    STR.Update();
    UpdateAnimation();
    FastLED.show();
    UpdateState();
}