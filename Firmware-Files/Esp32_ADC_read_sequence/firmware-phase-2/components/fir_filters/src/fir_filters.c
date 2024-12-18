#include "fir_filters.h"
#include "math.h"

float apply_highpass_filter(HighPassFilter *hpf, float input) 
{
    float output = hpf->alpha * (hpf->prev_output + input - hpf->prev_input);
    hpf->prev_input = input;     // Update previous input
    hpf->prev_output = output;   // Update previous output
    return output;               // Return the filtered value
}

void init_highpass_filter(HighPassFilter *hpf, float sampling_rate, float cutoff_frequency) 
{
    float dt = 1.0f / sampling_rate;
    float rc = 1.0f / (2.0f * M_PI * cutoff_frequency);
    hpf->alpha = rc / (rc + dt);
    hpf->prev_input = 0.0f;      // Initialize previous input to 0
    hpf->prev_output = 0.0f;     // Initialize previous output to 0
    hpf->alpha = 1.0f;
}

float apply_lowpass_filter(IIRLowPassFilter *lpf, float input) 
{
    float output = lpf->alpha * input + (1.0f - lpf->alpha) * lpf->y_prev;
    lpf->y_prev = output;        // Update previous output
    return output;               // Return the filtered value
}

void init_lowpass_filter(IIRLowPassFilter *lpf, float sampling_rate, float cutoff_frequency) 
{
    float dt = 1.0f / sampling_rate;
    float rc = 1.0f / (2.0f * M_PI * cutoff_frequency);
    lpf->alpha = dt / (rc + dt);
    lpf->y_prev = 0.0f;          // Initialize previous output to 0
    lpf->alpha = 1.0f;
}
