//
//  EffectPlugin.h
//  MyEffect Plugin Header File
//
//  Used to declare objects and data structures used by the plugin.
//

#pragma once

#include "apdi/Plugin.h"
#include "apdi/Helpers.h"
using namespace APDI;

#include "EffectExtra.h"

class MyEffect : public APDI::Effect
{
public:
    MyEffect(const Parameters& parameters, const Presets& presets); // constructor (initialise variables, etc.)
    ~MyEffect();                                                    // destructor (clean up, free memory, etc.)

    void setSampleRate(float sampleRate){ stk::Stk::setSampleRate(sampleRate); }
    float getSampleRate() const { return stk::Stk::sampleRate(); };
    
    void process(const float** inputBuffers, float** outputBuffers, int numSamples);
    
    void presetLoaded(int iPresetNum, const char *sPresetName);
    void optionChanged(int iOptionMenu, int iItem);
    void buttonPressed(int iButton);
    float fAval0;
    float fAval1;
    float fOutMultiplier;
    float fOutMultiplierOld;
    float fMax;
    float rampUp(float attackTime) {
        float attackValue;
        attackValue = ((getSampleRate() / attackTime) / 1000) / iMeasuredLength; // function calculates the amount of samples per s in the first brackets, then divides by 1000 to convert to ms, then divides my the length of the buffer to get the value to be added to the multiplier each time the code loops
        
        return attackValue;
        
    }

    float rampDown(float releaseTime) {
        float releaseValue;
        releaseValue = ((getSampleRate() / releaseTime) / 1000) / iMeasuredLength; // same as above
        
        return releaseValue;
        
    }
    

private:
    // Declare shared member variables here
    float *pfCircularBuffer;
    float fSR;
    int iMeasuredLength, iMeasuredItems;
    float fMax0, fMax1, fMaxOldL, fMaxOldR;
    int iBufferSize, iBufferWritePos;
    
};



