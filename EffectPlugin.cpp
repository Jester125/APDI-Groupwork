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
            {   "Param 0",  Parameter::METER, 0.0, 1.0, 0.0, AUTO_SIZE  }, //0
            {   "Param 1",  Parameter::METER, 0.0, 1.0, 0.0, AUTO_SIZE  }, //1
            {   "Gate Threshold (dB)",  Parameter::ROTARY, -100, 0, -100, AUTO_SIZE }, //2
            {   "Hysteresis (dB)",  Parameter::ROTARY, -20, 0, 0.0, AUTO_SIZE }, //3
            {   "Attack (ms)",  Parameter::ROTARY, 0, 100, 0.0, AUTO_SIZE },//4
            {   "Release (ms)",  Parameter::ROTARY, 0, 100, 0.0, AUTO_SIZE },//5
            {   "Lookahead",  Parameter::ROTARY, -20, 0, 0.0, AUTO_SIZE },//6
            {   "Reduction (dB)",  Parameter::ROTARY, -100, 0, -100, AUTO_SIZE },//7
            {   "Filter Type",  Parameter::MENU, {"BandPass", "LowPass", "HighPass"}, AUTO_SIZE },//8
            {   "LFP Cutoff (Hz)",  Parameter::ROTARY, 200, 1000, 200, AUTO_SIZE },//9
            {   "HPF Cutoff (Hz)",  Parameter::ROTARY, 1000, 20000, 1000, AUTO_SIZE },//10
            {   "BandPass Center (Hz)",  Parameter::ROTARY, 200, 20000, 200, AUTO_SIZE },//11
            {   "BandPass Bandwidth (Hz)",  Parameter::ROTARY, 100, 10000, 100, AUTO_SIZE },//12
            {   "Delay Feedback",  Parameter::ROTARY, 0, 0.7, 0.0, AUTO_SIZE  },//13
            {   "Delay Time",  Parameter::ROTARY, 0.1, 0.8, 0.0, AUTO_SIZE  },//14
            {   "Delay Dry %",  Parameter::ROTARY, 0, 100, 50, AUTO_SIZE  },//15
            {   "Delay Wet %",  Parameter::ROTARY, 0, 200, 100, AUTO_SIZE  },//16
            //{   "Only when gate is closed",  Parameter::BUTTON, 0, 1, 0, AUTO_SIZE  },//17

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
    
    iBufferSize = 2 * getSampleRate();
            
    pfCircularBuffer = new float[iBufferSize];
    for(int x = 0; x < iBufferSize; x++)
        pfCircularBuffer[x] = 0;
            
    iBufferWritePos = 0;
    
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
       iOptionMenu = parameters[8];
       if (iItem == 0){
           bBandPass = true;
           bLowPass = false;
           bHighPass = false;
       }
       if (iItem == 1){
           bBandPass = false;
           bLowPass = true;
           bHighPass = false;
       }
       if (iItem == 2){
           bBandPass = false;
           bLowPass = false;
           bHighPass = true;
       }

    // An option menu, with index iOptionMenu, has been changed to the entry, iItem
}

void MyEffect::buttonPressed(int iButton)
{
    // A button, with index iButton, has been pressed
//    iButton = parameters[16];
//    if (bDelayisOn) {
//        bDelayisOn = false;
//    }
//    else {
//        bDelayisOn = true;
//    }
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
    float fAval1;
    float fAval;
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
    BPF filterbpf;
    
    //for the delay
    float fSR = getSampleRate();
    int iBufferReadPos;
    float fDelaySignal;
    float fDelayAmount;
    float fDelayTime = (parameters[14] * 0.5);
    float fOut;
    float fDry(parameters[15] / 100);
    float fWet(parameters[16] / 100);
    

     
    //float fGain = parameters[0];
    
    while(numSamples--)
    {
        // Get sample from input
        fIn0 = *pfInBuffer0++;
        fIn1 = *pfInBuffer1++;
        
        // Add your effect processing here
        
        fMix = fIn0 + fIn1 * 0.5;
        
        filterlpf.setCutoff(parameters[9]);
        
        filterhpf.setCutoff(parameters[10]);
        
        filterbpf.set(parameters[11], parameters[12]);
            
        if (bLowPass == true) {
            fFilteredMix = filterlpf.tick(fMix);
        }
        if (bHighPass == true) {
            fFilteredMix = filterhpf.tick(fMix);
        }
        if (bBandPass == true) {
            fFilteredMix = filterbpf.tick(fMix);
        }
        
        fThreshRec = (fThresh + 100) / 100;
        
        //fGateReboundRec = (fGateRebound + 20) / 100;
        
        fGateReboundRec = fabs(fGateRebound) / 100;
        
        parameters[0] = fGateReboundRec;
        
        fReductionAmountRec = (fReductionAmount + 100) / 100;
        
        fAval = fabs(fFilteredMix);
        fAval0 = fabs(fIn0);
        fAval1 = fabs(fIn1);
        
        fAval = 20 * log10(fAval) + 100;
        
        fAval = fAval / 100;
       
        //peak detection for the noise gate
        if (fAval > fMax ) {
            
            fMax = fAval;
            
        }
        
        //peak detction for the metering
        if (fAval0 > fMax0 ) {
            
            fMax0 = fAval0;
            
        }
        if (fAval1 > fMax1 ) {
            
            fMax1 = fAval1;
            
        }
       
        iMeasuredItems++;
        
        if (iMeasuredItems == iMeasuredLength){
            
            if (fMax > (fThreshRec) ){
                //bGateRebound = true; //not sure if i need the bool gate rebound or not
            
                fOutMultiplier = (fOutMultiplier + rampUp(fAttack)); // here ive added a little +0.05 just so that the multiplier doesnt remain on 0 when the code loops back round
                
                if (fOutMultiplier >= 1){
                    fOutMultiplier = 1; // this limits the mult so i dont go deaf
                    
                    gState = true;
                }
            }
            
            else if (fMax < fThreshRec - fGateReboundRec){
                
                fOutMultiplier = (fOutMultiplier - rampDown(fRelease));
                
                if (fOutMultiplier <= fReductionAmountRec){
                    fOutMultiplier = fReductionAmountRec;
                    
                    gState = false;
                }
                //fOutMultiplier = 0.1;
                //bGateRebound = false;
                
            }
            
            //Code for making the metering look nice
            fMax0 = fMax0 * fOutMultiplier;
            fMax1 = fMax1 * fOutMultiplier;


            fMax0 = (fMax0 * 39 + 1);
            fMax1 = (fMax1 * 39 + 1);

            fMax0 = log10(fMax0);
            fMax1 = log10(fMax1);

            fMax0 = (fMax0 / log10(40));
            fMax1 = (fMax1 / log10(40));

            //Making the slow decay
            if (fMax0 < fMaxOldL){
                fMax0 = (fMax0 * 0.01 + fMaxOldL * 0.99);

            }
            if (fMax1 < fMaxOldR){
                fMax1 = (fMax1 * 0.01 + fMaxOldR * 0.99);
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
        
        //Code for the delay
        fMix = fIn0 + fIn1 * 0.5;
                
        iBufferReadPos = iBufferWritePos - (fSR * fDelayTime);
                
        if (iBufferReadPos < 0 ){
                            
            iBufferReadPos  += iBufferSize;
                            
        }
        fDelaySignal = pfCircularBuffer[iBufferReadPos];
                        
        fDelayAmount = parameters[13];
                        
        fDelaySignal = fDelaySignal * fDelayAmount;
        
        //adding wet and dry
        fOut = (fMix * fDry) + (fDelaySignal * fWet);
                        
        pfCircularBuffer[iBufferWritePos] = fOut;
                        
        iBufferWritePos++;
                        
        if (iBufferWritePos == iBufferSize - 1){
                            
            iBufferWritePos = 0;
                            
        }
        
        fOut0 = ((fIn1 * fOutMultiplier) + fOut) / 2;
        fOut1 = ((fIn1 * fOutMultiplier) + fOut) / 2;
        
        // Copy result to output
        *pfOutBuffer0++ = fOut0;
        *pfOutBuffer1++ = fOut1;
    }
}
