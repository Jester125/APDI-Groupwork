
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
            {   "Param 0",  Parameter::METER, 0.0, 0.2, 0.0, AUTO_SIZE  },
            {   "Param 1",  Parameter::METER, 0.0, 1.0, 0.0, AUTO_SIZE  },
            {   "Gate Threshold (dB)",  Parameter::ROTARY, -100, 0, -100, AUTO_SIZE },
            {   "Hysteresis (dB)",  Parameter::ROTARY, -20, 0, 0.0, AUTO_SIZE },
            {   "Attack (ms)",  Parameter::ROTARY, 0, 100, 0.0, AUTO_SIZE },
            {   "Release (ms)",  Parameter::ROTARY, 0, 100, 0.0, AUTO_SIZE },
            {   "Lookahead",  Parameter::ROTARY, -20, 0, 0.0, AUTO_SIZE },
            {   "Reduction (dB)",  Parameter::ROTARY, -100, 0, 0.0, AUTO_SIZE },
            {   "Filter Type",  Parameter::MENU, {"BandPass", "LowPass", "HighPass"}, AUTO_SIZE },
            {   "LFP Cutoff (Hz)",  Parameter::ROTARY, 20, 20000, 0.0, AUTO_SIZE },
            {   "HPF Cutoff (Hz)",  Parameter::ROTARY, 20, 20000, 0.0, AUTO_SIZE },
            {   "BandPass High Cutoff (Hz)",  Parameter::ROTARY, 200, 20000, 0.0, AUTO_SIZE },
            {   "BandPass Low Cutoff (Hz)",  Parameter::ROTARY, 100, 10000, 0.0, AUTO_SIZE },

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
    bool bHighPass;
    bool bBandPass;
    bool bLowPass;
    
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
    
    //here im trying to reference the options in the dropdown menu for parameters[8] but have no clue, have emailed chris
    //iOptionMenu = parameters[8];
    //iItem = parameters[8];
    
    //if (iItem = "BandPass") {
    //    bHighPass = true;
     //   bBandPass = false;
     //   bLowPass = false;
   // }
  //  if (iItem = )
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
    
    float fAval0;
    //float fAval1;
    float fMix;
    float fFilteredMix;
    float fThresh(parameters[2]);
    float fThreshRec;
    //float fThreshCubed;
    float fGateRebound(parameters[3]);
    float fGateReboundRec;
    bool gState;
    float fReductionAmount(parameters[7]);
    float fReductionAmountRec;
    float fAttack(parameters[4]);
    float fRelease(parameters[5]);
    //float fGateReboundCubed;
    //bool bGateRebound = true;

    LPF filterlpf;
    HPF filterhpf;
    BPF filterbpf; //gonna try to make a menu where you can select the type of filtering that gets passed to the aval, with controls for the frequency cutoff, and for the bandpass with controls for the center frequency and bandwidth
    
     
    //float fGain = parameters[0];
    
    while(numSamples--)
    {
        // Get sample from input
        fIn0 = *pfInBuffer0++;
        fIn1 = *pfInBuffer1++;
        
        // Add your effect processing here
        
        fMix = fIn0 + fIn1 * 0.5;
        
        filterlpf.setCutoff(200);
        
        fFilteredMix = filterlpf.tick(fMix);
        
        fThreshRec = (fThresh + 100) / 100; //takes -100 - 1 and converts to a range of 0 - 1
        
        //fGateReboundRec = (fGateRebound + 20) / 100;
        
        fGateReboundRec = fabs(fGateRebound) / 100; //converts -20 - 0 to a range of 0.2 - 0
        
        //parameters[0] = fGateReboundRec;
        
        fReductionAmountRec = (fReductionAmount + 100) / 100; //rectifies -100 - 0 to 1 - 0
        
        fAval0 = fabs(fFilteredMix);
        
        fAval0 = 20 * log10(fAval0) + 100; //converts the aval envelope follower to log scale
        
        fAval0 = fAval0 / 100; // converts log scale to a range of 0 - 1
       
        
        if (fAval0 > fMax0 ) {
            
            fMax0 = fAval0;
            
        }
       
        iMeasuredItems++;
        
        if (iMeasuredItems == iMeasuredLength){
            
            if (fMax0 > (fThreshRec) ){
                //removed hysteresis code to the closing of the gate after reading online it gets considered when gate closes not opens
            
                fOutMultiplier = (fOutMultiplier + rampUp(fAttack)); //see the function in .h
                
                if (fOutMultiplier >= 1){
                    fOutMultiplier = 1; // this limits the mult so i dont go deaf
                    
                    //gState = true;  i was gonna add a led that shows if the gate is on or off using this variable but the nasher didnt include one in the parameters
                }
            }
            
            else if (fMax0 < fThreshRec - fGateReboundRec){ //this means that the level must fall below the threshold - the hysteresis e.g. threshold at -20 db and hysteresis - 3 db means the level has to fall below -23db for the gate to close
                
                fOutMultiplier = (fOutMultiplier - rampDown(fRelease)); // same as above but subtracting instead
                
                if (fOutMultiplier <= fReductionAmountRec){
                    fOutMultiplier = fReductionAmountRec;
                    
                    gState = false;
                }
                //fOutMultiplier = 0.1;
                //bGateRebound = false;
                
            }
            
            
            fMax0 = 0.f;
            iMeasuredItems = 0;
            fOutMultiplierOld = fOutMultiplier;
        }
        
        fMix = fMix * fOutMultiplier;
        
        
        fOut0 = fMix;
        fOut1 = fMix;
        
        // Copy result to output
        *pfOutBuffer0++ = fOut0;
        *pfOutBuffer1++ = fOut1;
    }
}


