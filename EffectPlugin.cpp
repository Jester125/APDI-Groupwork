//
//  EffectPlugin.cpp
//  MyEffect Plugin Source Code
//
//  Used to define the bodies of functions used by the plugin, as declared in EffectPlugin.h.
//

#include "EffectPlugin.h"

////////////////////////////////////////////////////////////////////////////
// EFFECT - represents the whole effect plugin
////////////////////////////////////////////////////////////////////////////

// Called to create the effect (used to add your effect to the host plugin)
extern "C" {
    CREATE_FUNCTION createEffect(float sampleRate) {
        ::stk::Stk::setSampleRate(sampleRate);
        
        //==========================================================================
        // CONTROLS - Use this array to completely specify your UI
        // - tells the system what parameters you want, and how they are controlled
        // - add or remove parameters by adding or removing entries from the list
        // - each control should have an expressive label / caption
        // - controls can be of different types: ROTARY, BUTTON, TOGGLE, SLIDER, or MENU (see definitions)
        // - for rotary and linear sliders, you can set the range of values (make sure the initial value is inside the range)
        // - for menus, replace the three numeric values with a single array of option strings: e.g. { "one", "two", "three" }
        // - by default, the controls are laid out in a grid, but you can also move and size them manually
        //   i.e. replace AUTO_SIZE with { 50,50,100,100 } to place a 100x100 control at (50,50)
        
        const Parameters CONTROLS = {
            //  name,       type,              min, max, initial, size
            {   "Param 0",  Parameter::METER, 0.0, 1.0, 0.0, AUTO_SIZE  },
            {   "Param 1",  Parameter::METER, 0.0, 1.0, 0.0, AUTO_SIZE  },
            {   "Gate Threshold dB",  Parameter::ROTARY, 0, 1, 0.0, AUTO_SIZE },
            {   "Reduction dB",  Parameter::ROTARY, 0, 100, 0.0, AUTO_SIZE  },
        };

        const Presets PRESETS = {
            { "Preset 1", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
            { "Preset 2", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
            { "Preset 3", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
        };

        return (APDI::Effect*)new MyEffect(CONTROLS, PRESETS);
    }
}

// Constructor: called when the effect is first created / loaded
MyEffect::MyEffect(const Parameters& parameters, const Presets& presets)
: Effect(parameters, presets)
{
    // Initialise member variables, etc.
    iMeasuredLength = iMeasuredItems = 0;
    fMax0 = fMax1 = fMaxOldL = fMaxOldR = 0;
    fOutMultiplier = 0;
    fOutMultiplierOld = 0;
    
}

// Destructor: called when the effect is terminated / unloaded
MyEffect::~MyEffect()
{
    // Put your own additional clean up code here (e.g. free memory)
}

// EVENT HANDLERS: handle different user input (button presses, preset selection, drop menus)

void MyEffect::presetLoaded(int iPresetNum, const char *sPresetName)
{
    // A preset has been loaded, so you could perform setup, such as retrieving parameter values
    // using getParameter and use them to set state variables in the plugin
}

void MyEffect::optionChanged(int iOptionMenu, int iItem)
{
    // An option menu, with index iOptionMenu, has been changed to the entry, iItem
}

void MyEffect::buttonPressed(int iButton)
{
    // A button, with index iButton, has been pressed
}

// Applies audio processing to a buffer of audio
// (inputBuffer contains the input audio, and processed samples should be stored in outputBuffer)
void MyEffect::process(const float** inputBuffers, float** outputBuffers, int numSamples)
{
    iMeasuredLength = (0.001 * getSampleRate());
    float fIn0, fIn1, fOut0 = 0, fOut1 = 0;
    const float *pfInBuffer0 = inputBuffers[0], *pfInBuffer1 = inputBuffers[1];
    float *pfOutBuffer0 = outputBuffers[0], *pfOutBuffer1 = outputBuffers[1];
    
    float fAval;
    float fAval0;
    float fAval1;
    float fMix;
    float fFilteredMix;
    float fThreshDb(parameters[2]);
    float fThresh;
    float fThreshCubed;
    float fGateRebound;
    float fGateReboundCubed;
    float fReduction(parameters[3] / 100);
    bool bGateRebound = true;

    LPF filter1;
     
    //float fGain = parameters[0];
    
    while(numSamples--)
    {
        // Get sample from input
        fIn0 = *pfInBuffer0++;
        fIn1 = *pfInBuffer1++;
        
        // Add your effect processing here
        
        fMix = fIn0 + fIn1 * 0.5;
        
        filter1.setCutoff(200);
        
        fFilteredMix = filter1.tick(fMix);
        
        fThresh =  fThreshDb; //pow(10, (fThreshDb / 100));
        //fReduction = 1 / (5*fReductionDb + 1);
        //std::cout << (fThresh);
        //std::cout << (fThreshDB);
        //fGateRebound = 1 - fSustain;
        
        fThreshCubed = (fThresh * fThresh * fThresh);
        
        //fGateReboundCubed = (fGateRebound * fGateRebound * fGateRebound);
        
        fAval = fabs(fFilteredMix);
        fAval0 = fabs(fIn0);
        fAval1 = fabs(fIn1);
       
        
        if (fAval > fMax ) {
            
            fMax = fAval;
            
        }
        
        if (fAval0 > fMax0) {
            fMax0 = fAval0;
        }
        
        if (fAval1 > fMax1) {
            fMax1 = fAval1;
        }
        
       
        iMeasuredItems++;
        
        if (iMeasuredItems == iMeasuredLength){
            
            // if beyond the threshold...
            if (fMax > fThreshCubed){
                //bGateRebound = true; //not sure if i need the bool gate rebound or not
            
                fOutMultiplier = 1; //(fOutMultiplier + 0.05 + fOutMultiplierOld * 0.95); // here ive added a little +0.05 just so that the multiplier doesnt remain on 0 when the code loops
                //std::cout << (fOutMultiplier);
                
//                if (fOutMultiplier >= 1){
//                    fOutMultiplier = 1; // this limits the mult so i dont go deaf
//                }
            }
            
            else if (fMax < fThreshCubed){
                
                fOutMultiplier = 1 - fReduction; //(fOutMultiplier - fOutMultiplierOld * 0.03);
                
                //if (fOutMultiplier <= 0.05){
                //    fOutMultiplier = 0.05;
                //}
                //fOutMultiplier = 0.1;
                //bGateRebound = false;
                
            }

            fMax0 = fMax0 * fOutMultiplier;
            fMax1 = fMax1 * fOutMultiplier;


            fMax0 = (fMax0 * 39 + 1);
            fMax1 = (fMax1 * 39 + 1);

            fMax0 = log10(fMax0);
            fMax1 = log10(fMax1);

            fMax0 = (fMax0 / log10(40));
            fMax1 = (fMax1 / log10(40));


            if (fMax0 < fMaxOldL){
                fMax0 = (fMax0 * 0.1 + fMaxOldL * 0.9);

            }
            if (fMax1 < fMaxOldR){
                fMax1 = (fMax1 * 0.1 + fMaxOldR * 0.9);
            }


            parameters[0] = fMax0;
            parameters[1] = fMax1;

            fMaxOldL = fMax0;
            fMaxOldR = fMax1;

            fMax0 = 0.f;
            fMax1 = 0.f;
            fMax = 0;
            iMeasuredItems = 0;
            fOutMultiplierOld = fOutMultiplier;
        }
        
        
        fOut0 = fIn0 * fOutMultiplier;
        fOut1 = fIn1 * fOutMultiplier;
        
        
        
        // Copy result to output
        *pfOutBuffer0++ = fOut0;
        *pfOutBuffer1++ = fOut1;
    }
}
