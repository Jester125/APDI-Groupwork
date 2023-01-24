
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
            {   "L Meter",  Parameter::METER, 0.0, 1.0, 0.0, { 130,350,50,150 }  }, //0
            {   "R Meter",  Parameter::METER, 0.0, 1.0, 0.0, { 210,350,50,150 }  }, //1
            {   "Gate Threshold (dB)",  Parameter::ROTARY, -100, 0, -100, { 80,20,90,90 } }, //2
            {   "Hysteresis (dB)",  Parameter::ROTARY, -20, 0, 0.0, { 10,30,70,70 } }, //3
            {   "Attack (ms)",  Parameter::ROTARY, 0, 100, 0.0, { 10,120,70,70 } },//4
            {   "Hold (ms)",  Parameter::ROTARY, 0, 1000, 0.0, { 90,130,70,70 } },//5
            {   "Release (ms)",  Parameter::ROTARY, 0, 100, 0.0, { 170,120,70,70 } },//6
            {   "Reduction (dB)",  Parameter::ROTARY, -100, 0, -100, { 170,30,70,70 } },//7
            {   "Filter Type",  Parameter::MENU, {"BandPass", "LowPass", "HighPass"}, { 270,20,100,20 } },//8
            {   "LFP Cutoff (Hz)",  Parameter::ROTARY, 200, 1000, 200, { 260,70,50,50 } },//9
            {   "HPF Cutoff (Hz)",  Parameter::ROTARY, 1000, 20000, 1000, { 330,70,50,50 } },//10
            {   "BP Center (Hz)",  Parameter::ROTARY, 200, 20000, 200, { 260,140,50,50 } },//11
            {   "BP Width (Hz)",  Parameter::ROTARY, 50, 10000, 100, { 330,140,50,50 } },//12
            {   "Delay Feedback",  Parameter::ROTARY, 0, 1.0, 0.0, { 40,230,70,70 }  },//13
            {   "Delay Time (ms)",  Parameter::ROTARY, 10, 1000, 0.0, { 120,230,70,70 }  },//14
            {   "Delay Output %",  Parameter::ROTARY, 0, 200, 100, { 200,230,70,70 }  },//15
            {   "Gated Output %",  Parameter::ROTARY, 0, 100, 100, { 280,230,70,70 }  },//16
        };

        const Presets PRESETS = {
            { "Preset 1", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
            { "Preset 2", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
            { "Preset 3", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
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
    
    bBandPass = true;
    
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
    // logic for filter selection
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

}

void MyEffect::buttonPressed(int iButton)
{
    // A button, with index iButton, has been pressed
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
    
    
    float fAval;
    
    float fMix;
    float fMixSidechain;
    float fFilteredMix = 0;
    float fThresh(parameters[2]);
    float fThreshRec;
    float fGateHyst(parameters[3]);
    float fGateHystRec;
    float fReductionAmount(parameters[7]);
    float fReductionAmountRec;
    float fAttack(parameters[4]);
    float fRelease(parameters[6]);
    float fHold(parameters[5]);

    LPF filterlpf;
    HPF filterhpf;
    BPF filterbpf;
    
    //for the delay
    float fAval0;
    float fAval1;
    float fSR = getSampleRate();
    int iBufferReadPos;
    float fDelaySignal;
    float fDelayAmount;
    float fDelayTime = (parameters[14] / 1000); // converts to ms
    float fDelayTimeBPM = (60000 / parameters[17]);
    float fOutDel;
    float fDry(parameters[16] / 100);
    float fWet(parameters[15] / 100);
     
    //float fGain = parameters[0];
    
    while(numSamples--)
    {
        // Get sample from input
        fIn0 = *pfInBuffer0++;
        fIn1 = *pfInBuffer1++;
 
        //creates a mix for the gating and a mix for the delay buffer
        fMix = fIn0 + fIn1 * 0.5;
        fMixSidechain = fIn0 + fIn1 * 0.5;
        
        //assigns parameters to the filters
        filterlpf.setCutoff(parameters[9]);
        filterhpf.setCutoff(parameters[10]);
        filterbpf.set(parameters[11], parameters[12]);
        //logic for filter switching
        if (bLowPass == true) {
            fFilteredMix = filterlpf.tick(fMix);
        }
        if (bHighPass == true) {
            fFilteredMix = filterhpf.tick(fMix);
        }
        if (bBandPass == true) {
            fFilteredMix = filterbpf.tick(fMix);
        }
        //rectifies and scales threshold, hysteresis and reduction amount
        fThreshRec = (fThresh + 100) / 100;
        fGateHystRec = fabs(fGateHyst) / 100;
        fReductionAmountRec = (fReductionAmount + 100) / 100;
        
        //peak detection for the noise gate, converts detected peaks into a log
        fAval = fabs(fFilteredMix);
        fAval = 20 * log10(fAval) + 100;
        fAval = fAval / 100;
        
        if (fAval > fMax ) {
            
            fMax = fAval;
            
        }
        //peak detction for the metering
        
        fAval0 = fabs(fIn0);
        fAval1 = fabs(fIn1);
        
        if (fAval0 > fMax0 ) {
            
            fMax0 = fAval0;
            
        }
        if (fAval1 > fMax1 ) {
            
            fMax1 = fAval1;
            
        }
       //
        iMeasuredItems++; //steps through the measured items
        
        if (iMeasuredItems == iMeasuredLength){
            
            if (fHoldCounter <= 0){
                
                fHoldCounter = 0; //keeps the hold counter at 0 (explained below)
                
            }
            
            if (fMax > (fThreshRec) ){
                
                fHoldCounter = 1.0; //sets the hold counter to 1 every time the gate is open
            
                fOutMultiplier = (fOutMultiplier + secToValue(fAttack)); // ramps up the multiplier according to user input
                
                if (fOutMultiplier >= 1){
                    fOutMultiplier = 1; // this limits the mult so the user doesnt go deaf
                    
                }
            }
            
            fHoldCounter = (fHoldCounter - secToValue(fHold)); // always counts down at a rate set by the user until it reaches 0
            
            if (fMax < (fThreshRec - fGateHystRec) && fHoldCounter <= 0){
                    
                    fOutMultiplier = (fOutMultiplier - secToValue(fRelease)); //subtracts the multiplier at a rate set by the user
                
                    if (fOutMultiplier <= fReductionAmountRec){
                        
                        fOutMultiplier = fReductionAmountRec; //keeps the lowest value of the multiplier at the reduction amount set by the user
                        
                    }
                
            }
            
            //Code for making the metering look nice
            fMax0 = fMax0 * fOutMultiplier;
            fMax1 = fMax1 * fOutMultiplier; //not sure if this step is needed / could be re written to use the actual output from the gate

            fMax0 = (fMax0 * 39 + 1);
            fMax1 = (fMax1 * 39 + 1);

            fMax0 = log10(fMax0);
            fMax1 = log10(fMax1);

            fMax0 = (fMax0 / log10(40));
            fMax1 = (fMax1 / log10(40)); //this and above scales and offsets the values into a useable range

            //Making the slow decay
            if (fMax0 < fMaxOldL){
                fMax0 = (fMax0 * 0.01 + fMaxOldL * 0.99);
            }
            if (fMax1 < fMaxOldR){
                fMax1 = (fMax1 * 0.01 + fMaxOldR * 0.99);
            }

            parameters[0] = fMax0;
            parameters[1] = fMax1; //output to the meters

            fMaxOldL = fMax0;
            fMaxOldR = fMax1;

            fMax0 = 0;
            fMax1 = 0;
            fMax = 0;
            iMeasuredItems = 0;
            fOutMultiplierOld = fOutMultiplier; //resetting variables for the next loop
        }
        
        //Code for the delay
        iBufferReadPos = iBufferWritePos - (fSR * fDelayTime); //sets the read point behind the write point according to user spec, since sample rate is per second, just need to multiply by the ms value
                
        if (iBufferReadPos < 0 ){
                            
            iBufferReadPos  += iBufferSize;
                            
        }
        
        fDelaySignal = pfCircularBuffer[iBufferReadPos]; //reads the buffer output
                        
        fDelayAmount = parameters[13];
                        
        fOutDel = (fDelaySignal * fDelayAmount); //multiplies the buffer output by the delay feedback value
            
        //adding wet and dry
        fMix = (fMix * fOutMultiplier) * fDry; //creates an output for the gate using the multiplier and the gain
            
        fMixSidechain = (fMixSidechain + fOutDel) * 0.5; // copys the delay back into itself
            
        pfCircularBuffer[iBufferWritePos] = fMixSidechain; // writes the feedback signal back into the buffer
                        
        iBufferWritePos++;
                        
        if (iBufferWritePos == iBufferSize - 1){
                            
            iBufferWritePos = 0;
                            
        } // resets the delay buffer back to the start
            
        fOut0 = (fMix + (fOutDel * fWet)) * 0.5;
        fOut1 = (fMix + (fOutDel * fWet)) * 0.5; //mixes the delay signal and the gate signal
        
        // Copy result to output
        *pfOutBuffer0++ = fOut0;
        *pfOutBuffer1++ = fOut1;
    }
}
