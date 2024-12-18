#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "adc_controller.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "adc_data_formatter.h"
#include "math.h"

static const char *TAG = "ADC_CONTROLLER_READER";

#define WINDOW_SIZE 100  // Fixed window size for averaging

typedef struct {
    float values[WINDOW_SIZE];    // Store float values since we are working with normalized values
    float sum;                    // Use float to store the sum for precision
    int index;                    // Index for the circular buffer
} moving_avg_t;

static moving_avg_t emg_avg_buffers[NUM_ADC_CHANNELS] = {0};  // Buffer for each channel

void adc_queue_handle_task(void *pvParameters);

QueueHandle_t adc_queue;

typedef char* (*adc_formatter_t)(const adc_ctrl_t *adc_channels, size_t num_channels, char *output, size_t max_output_length);

adc_ctrl_t adc_channels[NUM_ADC_CHANNELS] = {
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_5, .type = ADC_UNIT_1, .offset = OFFSET_0, .prev_lpf_output = 1.0f,},    
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_0, .type = ADC_UNIT_1, .offset = OFFSET_100, .prev_lpf_output = 1.0f,},  
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_1, .type = ADC_UNIT_1, .offset = OFFSET_200, .prev_lpf_output = 1.0f,},  
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_2, .type = ADC_UNIT_1, .offset = OFFSET_300, .prev_lpf_output = 1.0f,},  
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_3, .type = ADC_UNIT_1, .offset = OFFSET_400, .prev_lpf_output = 1.0f,},  
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_4, .type = ADC_UNIT_1, .offset = OFFSET_500, .prev_lpf_output = 1.0f,},  
    { .adc_ctl_channel_t.adc2_channel = ADC2_EXAMPLE_CHANNEL_0, .type = ADC_UNIT_2, .offset = OFFSET_600, .prev_lpf_output = 1.0f,},  
    { .adc_ctl_channel_t.adc2_channel = ADC2_EXAMPLE_CHANNEL_1, .type = ADC_UNIT_2, .offset = OFFSET_700, .prev_lpf_output = 1.0f,},  
};

esp_err_t __initialize_adc(adc_ctrl_t *adc_channels, size_t num_channels)
{
    gpio_num_t adc_gpio_num;
    esp_err_t r = ESP_FAIL;

    for (size_t i = 0; i < num_channels; i++)
    {
        switch (adc_channels[i].type)
        {
        case ADC_UNIT_1:
            {
                adc1_channel_t channel = adc_channels[i].adc_ctl_channel_t.adc1_channel;

                // Get GPIO for ADC1 channel
                r = adc1_pad_get_io_num(channel, &adc_gpio_num);
                assert(r == ESP_OK);

                printf("ADC1 channel %d @ GPIO %d\n", channel, adc_gpio_num);

                // Configure attenuation for ADC1
                r = adc1_config_channel_atten(channel, ADC_CON_ATTEN);
                assert(r == ESP_OK);

                // Allocate and initialize calibration characteristics for ADC1
                adc_channels[i].adc_cal_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
                if (adc_channels[i].adc_cal_chars == NULL) {
                    return ESP_ERR_NO_MEM;  // Allocation failure
                }

                // Calibrate ADC1
                r = esp_adc_cal_characterize(adc_channels[i].type, ADC_CON_ATTEN, width, DEFAULT_VREF, adc_channels[i].adc_cal_chars);
                assert(r == ESP_OK);
                break;
            }
        case ADC_UNIT_2:
            {
                adc2_channel_t channel = adc_channels[i].adc_ctl_channel_t.adc2_channel;

                // Get GPIO for ADC2 channel
                r = adc2_pad_get_io_num(channel, &adc_gpio_num);
                assert(r == ESP_OK);

                printf("ADC2 channel %d @ GPIO %d\n", channel, adc_gpio_num);

                // Configure attenuation for ADC2
                r = adc2_config_channel_atten(channel, ADC_CON_ATTEN);
                assert(r == ESP_OK);

                // Allocate and initialize calibration characteristics for ADC2
                adc_channels[i].adc_cal_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
                if (adc_channels[i].adc_cal_chars == NULL) {
                    return ESP_ERR_NO_MEM;  // Allocation failure
                }

                // Calibrate ADC2
                r = esp_adc_cal_characterize(adc_channels[i].type, ADC_CON_ATTEN, width, DEFAULT_VREF, adc_channels[i].adc_cal_chars);
                assert(r == ESP_OK);
                break;
            }
        
        default:
            break;
        }

        vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    return r;
}

esp_err_t initialize_adc(void)
{
    esp_err_t r = ESP_FAIL;

    r = __initialize_adc(adc_channels, NUM_ADC_CHANNELS);
    assert( r == ESP_OK );

    adc_queue = xQueueCreate(100, sizeof(adc_batch_t));  // 10 is the max number of batches in the queue
    if (adc_queue == NULL) {
        // Handle queue creation failure
        printf("Failed to create ADC queue\n");
        return;
    }

    // Start the ADC processing task
    xTaskCreatePinnedToCore(adc_queue_handle_task, "adc_queue_handle_task", 1024*5, NULL, 1, NULL, 0);
    // xTaskCreate(adc_queue_handle_task, "adc_queue_handle_task", 2048, NULL, 1, NULL);

    return r;
}

void __read_adc(adc_ctrl_t *adc_channels, size_t num_channels, adc_bits_width_t width)
{
    for (size_t i = 0; i < num_channels; i++)
    {
        int read_raw = 0;
        esp_err_t r;

        switch (adc_channels[i].type)
        {
            case ADC_UNIT_1:
            {
                adc1_channel_t channel = adc_channels[i].adc_ctl_channel_t.adc1_channel;
                read_raw = adc1_get_raw(channel);
                adc_channels[i].raw_value = read_raw; // Save the raw value
                break;
            }

            case ADC_UNIT_2:
            {
                adc2_channel_t channel = adc_channels[i].adc_ctl_channel_t.adc2_channel;
                r = adc2_get_raw(channel, width, &read_raw);
                if (r == ESP_OK)
                {
                    adc_channels[i].raw_value = read_raw; // Save the raw value
                }
                else
                {
                    adc_channels[i].raw_value = -1; // Indicate error with -1
                }
                break;
            }

            default:
                adc_channels[i].raw_value = -1; // Invalid or uninitialized type
                break;
        }

    }
}

char* __get_adc_values_string(const adc_ctrl_t *adc_channels, size_t num_channels, adc_formatter_t formatter)
{
    static char output[MAX_ADC_OUTPUT_LENGTH];  // Static buffer to hold the output string

    // Call the provided formatter to format the ADC values into the output buffer
    return formatter(adc_channels, num_channels, output, sizeof(output));
}

void print_adc_values()
{
    char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, serial_plotter_formatter);
    // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, json_formatter);
    // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, csv_formatter);
    printf("%s", adc_values_string);
}

// Frequency of the sine wave in Hz
#define SINE_WAVE_FREQ 10
// Timer frequency (200Hz)
#define TIMER_FREQ (1/TIMER_INTERVAL_SEC)
// Timer interval in microseconds
#define TIMER_INTERVAL_US (1000000 / TIMER_FREQ)

// Sine wave generator function
double generate_sine_wave() {
    static double amplitude = 2048.0; // Amplitude of the sine wave
    static double time = 0.0; // Time variable for sine wave calculation
    // Generate the sine wave value at the current time
    double value = amplitude * sin(2 * M_PI * SINE_WAVE_FREQ * time) + amplitude;
    
    // Update time by the period of the timer
    time += 1.0 / TIMER_FREQ; // Increment time by 1 sample period (1/200Hz)
    
    return value;
}

// Normalize the ADC value (example for 12-bit ADC with a range of 0-4095)
float normalize_adc_value(int raw_value) {
    return (float)raw_value / 4095.0f; // Normalize to range [0, 1]
}

void __read_adc_mock(adc_ctrl_t *adc_channels, size_t num_channels, adc_bits_width_t width)
{
    double adc_value = generate_sine_wave();
    for (size_t i = 0; i < num_channels; i++)
    {
        adc_channels[i].raw_value = (int)adc_value;
    }
}

void read_adc(void)
{
    __read_adc(adc_channels, NUM_ADC_CHANNELS, width);
    // __read_adc_mock(adc_channels, NUM_ADC_CHANNELS, width);
    __send_adc_batch_to_queue(adc_channels);
}

// Interrupt handler for the timer
void IRAM_ATTR timer_isr_handler(void *param) {
    // Clear the interrupt flag
    // gpio_set_level(13, 1);
    timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_INDEX);

    read_adc();

    // Re-enable the alarm
    timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER_INDEX);
    // gpio_set_level(13, 0);
}

void adc_timer_init() {
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true
    };

    timer_init(TIMER_GROUP, TIMER_INDEX, &config);
    timer_set_counter_value(TIMER_GROUP, TIMER_INDEX, 0x00000000);
    timer_set_alarm_value(TIMER_GROUP, TIMER_INDEX, TIMER_INTERVAL_SEC * (TIMER_BASE_CLK / TIMER_DIVIDER));
    timer_enable_intr(TIMER_GROUP, TIMER_INDEX);
    printf("ADC INIT %d\n", TIMER_BASE_CLK);
    timer_isr_register(TIMER_GROUP, TIMER_INDEX, timer_isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP, TIMER_INDEX);

    gpio_config_t debug_gpio = {
        (1ULL << 13),
        GPIO_MODE_OUTPUT,
    };
    gpio_config(&debug_gpio);
}

void adc_sampling_start()
{
    adc_timer_init();
}


void __send_adc_batch_to_queue(adc_ctrl_t *adc_channels)
{
    adc_batch_t adc_batch;  // Structure to hold all ADC readings

    // Copy the adc_channels array to adc_batch
    memcpy(adc_batch.adc_channels, adc_channels, sizeof(adc_batch.adc_channels));

    // Send the entire batch of ADC readings to the queue
    if (xQueueSend(adc_queue, (adc_batch_t*)&adc_batch, pdMS_TO_TICKS(1)) != pdPASS)
    // if (xQueueSend(adc_queue, (adc_batch_t*)&adc_batch, portMAX_DELAY) != pdPASS)
    {
        // Handle queue send error (e.g., queue full)
        // printf("Failed to send ADC batch to the queue\n");
    }
}

// Function to compute the moving average
float compute_mean(moving_avg_t *buffer, float new_value)
{
    // Remove the oldest value from the sum
    buffer->sum -= buffer->values[buffer->index];

    // Add the new value to the sum
    buffer->values[buffer->index] = new_value;
    buffer->sum += new_value;

    // Update the index (circular buffer)
    buffer->index = (buffer->index + 1) % WINDOW_SIZE;

    // Calculate and return the mean
    return buffer->sum / (float)WINDOW_SIZE;  // Return the average as a float
}

#if 0
void center_emg_signal(adc_batch_t *batch, size_t num_channels)
{
    static moving_avg_t emg_avg_buffers[NUM_ADC_CHANNELS] = {0}; // Circular buffers for each channel

    // Iterate over each channel in the batch
    for (size_t ch = 0; ch < NUM_ADC_CHANNELS; ch++)
    {
        moving_avg_t *buffer = &emg_avg_buffers[ch]; // Get the buffer for the channel

        int raw_value = (batch)->adc_channels[ch].raw_value; // Get the raw sample
        (batch)->adc_channels[ch].mean_value = compute_mean(buffer, raw_value);      // Compute mean using moving average

        // Subtract the mean from the raw value to center it around zero
        batch->adc_channels[ch].centered_value = (int)((float)raw_value - (float)((batch)->adc_channels[ch].mean_value));
    }
}
#else
void center_emg_signal(adc_batch_t *batch, size_t num_channels) {
    static moving_avg_t emg_avg_buffers[NUM_ADC_CHANNELS] = {0};  // Circular buffers for each channel

    // Iterate over each channel in the batch
    for (size_t ch = 0; ch < num_channels; ch++) {
        moving_avg_t *buffer = &emg_avg_buffers[ch];  // Get the buffer for the channel

        int raw_value = batch->adc_channels[ch].raw_value;  // Get the raw sample
        batch->adc_channels[ch].normalized_value = normalize_adc_value(raw_value);  // Normalize the value

        // Compute the mean using the moving average of the normalized value
        float mean_value = compute_mean(buffer, batch->adc_channels[ch].normalized_value);

        // Subtract the mean from the normalized value to center it around zero
        batch->adc_channels[ch].centered_value = (float)(batch->adc_channels[ch].normalized_value - (float)mean_value);
        batch->adc_channels[ch].filtered_value = batch->adc_channels[ch].centered_value;
    }
}
#endif

float calculate_alpha(float sampling_rate, float cutoff_frequency) {
    float rc = 1.0f / (2.0f * M_PI * cutoff_frequency);
    float t = 1.0f / sampling_rate;
    return rc / (rc + t);
}

#if 0
void highpass_filter_emg_signal(adc_batch_t *batch, size_t num_channels, float sampling_rate, float cutoff_frequency) {
    float alpha = calculate_alpha(sampling_rate, cutoff_frequency);

    // Iterate over each channel in the batch
    for (size_t ch = 0; ch < num_channels; ch++) {
        adc_ctrl_t *channel = &batch->adc_channels[ch];

        // Apply high-pass filter
        float input = (float)channel->centered_value; // Current input sample
        float output = alpha * (channel->prev_hpf_output + input - channel->prev_hpf_input);

        // Store the filtered value and update the previous input/output
        channel->filtered_value = (int)output;
        channel->prev_hpf_input = input;
        channel->prev_hpf_output = output;
    }
}
#else
void highpass_filter_emg_signal(adc_batch_t *batch, size_t num_channels, float sampling_rate, float cutoff_frequency) {
    float alpha = calculate_alpha(sampling_rate, cutoff_frequency);

    // Iterate over each channel in the batch
    for (size_t ch = 0; ch < num_channels; ch++) {
        adc_ctrl_t *channel = &batch->adc_channels[ch];

        // Apply high-pass filter on the centered value (which has been DC removed)
        float input = (float)channel->centered_value;  // Current input sample
        float output = alpha * (channel->prev_hpf_output + input - channel->prev_hpf_input);

        // Store the filtered value and update the previous input/output
        channel->filtered_value = output;  // Store as float for higher accuracy
        channel->prev_hpf_input = input;
        channel->prev_hpf_output = output;
    }
}
#endif

float calculate_beta(float sampling_rate, float cutoff_frequency) {
    float rc = 1.0f / (2.0f * M_PI * cutoff_frequency);
    float dt = 1.0f / sampling_rate;
    return dt / (rc + dt);
}

#if 0
void lowpass_filter_emg_signal(adc_batch_t *batch, size_t num_channels, float sampling_rate, float cutoff_frequency) {
    // Calculate the smoothing factor (alpha)
    float alpha = calculate_beta(sampling_rate, cutoff_frequency);

    // Iterate over each channel in the batch
    for (size_t ch = 0; ch < num_channels; ch++) {
        adc_ctrl_t *channel = &batch->adc_channels[ch];

        // Apply the low-pass filter
        float input = (float)channel->filtered_value; // Current input sample
        float output = alpha * input + (1 - alpha) * channel->prev_lpf_output;

        // Store the filtered value and update the previous output
        channel->filtered_value = (int)output; // Cast to integer if needed
        channel->prev_lpf_output = output; // Update the filter state
    }
}
#else
void lowpass_filter_emg_signal(adc_batch_t *batch, size_t num_channels, float sampling_rate, float cutoff_frequency) {
    // Calculate the smoothing factor (alpha)
    float alpha = calculate_beta(sampling_rate, cutoff_frequency);

    // Iterate over each channel in the batch
    for (size_t ch = 0; ch < num_channels; ch++) {
        adc_ctrl_t *channel = &batch->adc_channels[ch];

        // Apply the low-pass filter on the filtered value (after high-pass or other filtering steps)
        float input = (float)channel->filtered_value; // Current input sample
        float output = alpha * input + (1 - alpha) * channel->prev_lpf_output;

        // Store the filtered value and update the previous output
        channel->filtered_value = output;  // Keep as float for accuracy
        channel->prev_lpf_output = output; // Update the filter state
    }
}
#endif

#if 0
void adc_queue_handle_task(void *pvParameters)
{
    adc_batch_t received_batch;  // Structure to hold the received batch of ADC data

    while (1)
    {
        // Wait for the batch of ADC data to arrive in the queue
        if (xQueueReceive(adc_queue, &received_batch, portMAX_DELAY) == pdPASS)
        {
            gpio_set_level(13, 1);
            // Process the received ADC batch

            // Compute the mean for each channel
            center_emg_signal(&received_batch, NUM_ADC_CHANNELS);
            highpass_filter_emg_signal(&received_batch, NUM_ADC_CHANNELS, 281.0f, 30.0f); // Example: 1000 Hz sampling, 10 Hz cutoff
            lowpass_filter_emg_signal(&received_batch, NUM_ADC_CHANNELS, 281.0f, 500.0f); // Example: 1000 Hz sampling, 10 Hz cutoff
            
            // Log the data out
            // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, serial_plotter_formatter_voltage);
            char *adc_values_string = __get_adc_values_string((adc_ctrl_t*)&received_batch, NUM_ADC_CHANNELS, serial_plotter_mean_formatter);
            // char *adc_values_string = __get_adc_values_string((adc_ctrl_t*)&received_batch, NUM_ADC_CHANNELS, serial_plotter_formatter);
            // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, json_formatter);
            // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, csv_formatter);


            printf("%s", adc_values_string);
            gpio_set_level(13, 0);
        }
    }
}
#else
#define SIGNAL_BUFFER 40
adc_batch_t batch_buffer[SIGNAL_BUFFER];  // Array to hold batches from the queue
void adc_queue_handle_task(void *pvParameters)
{
    size_t batch_count = 0;                // Number of batches collected in the buffer

    while (1)
    {
        batch_count = 0;

        // Collect all messages from the queue
        while (xQueueReceive(adc_queue, &batch_buffer[batch_count], 0) == pdPASS)
        {
            gpio_set_level(13, 1);
            batch_count++;
            if (batch_count >= SIGNAL_BUFFER)
            {
                break;  // Stop collecting if the buffer is full
            }
            gpio_set_level(13, 0);
        }

        if (batch_count > 0)
        {

            // Process each batch in the buffer
            for (size_t i = 0; i < batch_count; i++)
            {
                center_emg_signal(&batch_buffer[i], NUM_ADC_CHANNELS);
                highpass_filter_emg_signal(&batch_buffer[i], NUM_ADC_CHANNELS, 281.0f, 30.0f);
                lowpass_filter_emg_signal(&batch_buffer[i], NUM_ADC_CHANNELS, 281.0f, 500.0f);

                // Log the processed data for all batches
                gpio_set_level(13, 1);
                char *adc_values_string = __get_adc_values_string(
                    (adc_ctrl_t*)&batch_buffer[i], NUM_ADC_CHANNELS, serial_plotter_mean_formatter);
                printf("%s", adc_values_string);
                gpio_set_level(13, 0);
            }
        }

        // Optional: Add a delay if you want to prevent task hogging
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif
