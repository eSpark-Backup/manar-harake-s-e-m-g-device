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

#define WINDOW_SIZE 200  // Fixed window size for averaging

typedef struct {
    int values[WINDOW_SIZE];
    long sum; // Use long if values are int, or double for floating-point precision
    int index;
} moving_avg_t;

static moving_avg_t emg_avg_buffers[NUM_ADC_CHANNELS] = {0};  // Buffer for each channel

void adc_queue_handle_task(void *pvParameters);

QueueHandle_t adc_queue;

typedef char* (*adc_formatter_t)(const adc_ctrl_t *adc_channels, size_t num_channels, char *output, size_t max_output_length);

adc_ctrl_t adc_channels[NUM_ADC_CHANNELS] = {
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_5, .type = ADC_UNIT_1, .offset = OFFSET_0},
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_0, .type = ADC_UNIT_1, .offset = OFFSET_100},
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_1, .type = ADC_UNIT_1, .offset = OFFSET_200},
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_2, .type = ADC_UNIT_1, .offset = OFFSET_300},
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_3, .type = ADC_UNIT_1, .offset = OFFSET_400},
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_4, .type = ADC_UNIT_1, .offset = OFFSET_500},
    { .adc_ctl_channel_t.adc2_channel = ADC2_EXAMPLE_CHANNEL_0, .type = ADC_UNIT_2, .offset = OFFSET_600},
    { .adc_ctl_channel_t.adc2_channel = ADC2_EXAMPLE_CHANNEL_1, .type = ADC_UNIT_2, .offset = OFFSET_700},
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
    xTaskCreate(adc_queue_handle_task, "adc_queue_handle_task", 2048, NULL, 10, NULL);

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
#define SINE_WAVE_FREQ 50
// Timer frequency (200Hz)
#define TIMER_FREQ 200
// Timer interval in microseconds
#define TIMER_INTERVAL_US (1000000 / TIMER_FREQ)

// Sine wave generator function
double generate_sine_wave() {
    static double amplitude = 4095.0; // Amplitude of the sine wave
    static double time = 0.0; // Time variable for sine wave calculation
    // Generate the sine wave value at the current time
    double value = amplitude * sin(2 * M_PI * SINE_WAVE_FREQ * time);
    
    // Update time by the period of the timer
    time += 1.0 / TIMER_FREQ; // Increment time by 1 sample period (1/200Hz)
    
    return value;
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
    // __read_adc(adc_channels, NUM_ADC_CHANNELS, width);
    __read_adc_mock(adc_channels, NUM_ADC_CHANNELS, width);
    __send_adc_batch_to_queue(adc_channels);
}

// Interrupt handler for the timer
void IRAM_ATTR timer_isr_handler(void *param) {
    // Clear the interrupt flag
    timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_INDEX);

    // Re-enable the alarm
    timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER_INDEX);

    read_adc();
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
    if (xQueueSend(adc_queue, &adc_batch, portMAX_DELAY) != pdPASS)
    {
        // Handle queue send error (e.g., queue full)
        // printf("Failed to send ADC batch to the queue\n");
    }
}

// Function to add a new sample and compute the mean
float compute_mean(moving_avg_t *buffer, int new_value)
{
    // Ensure sum uses a type that can handle large values
    buffer->sum -= buffer->values[buffer->index];    // Remove old value
    buffer->values[buffer->index] = new_value;       // Add new value
    buffer->sum += new_value;                        // Update sum
    buffer->index = (buffer->index + 1) % WINDOW_SIZE; // Update index

    // Calculate and return the mean
    return (float)((float)buffer->sum / (float)WINDOW_SIZE);
}

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

float calculate_alpha(float sampling_rate, float cutoff_frequency) {
    float rc = 1.0f / (2.0f * M_PI * cutoff_frequency);
    float t = 1.0f / sampling_rate;
    return rc / (rc + t);
}

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

float calculate_beta(float sampling_rate, float cutoff_frequency)
{
    float beta = 1 / (1 + (sampling_rate / (2 * M_PI * cutoff_frequency)));
    return beta;
}

void lowpass_filter_emg_signal(adc_batch_t *batch, size_t num_channels, float sampling_rate, float cutoff_frequency) {
    // float beta = calculate_alpha(sampling_rate, cutoff_frequency);
    float beta = calculate_beta(sampling_rate, cutoff_frequency);

    // Iterate over each channel in the batch
    for (size_t ch = 0; ch < num_channels; ch++) {
        adc_ctrl_t *channel = &batch->adc_channels[ch];

        // Apply high-pass filter
        float input = (float)channel->filtered_value; // Current input sample
        float output = beta * input + (1 - beta) * channel->prev_lpf_output;

        // Store the filtered value and update the previous input/output
        channel->filtered_value = (int)output;
        channel->prev_lpf_input = input;
        channel->prev_lpf_output = output;
    }
}


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
            // highpass_filter_emg_signal(&received_batch, NUM_ADC_CHANNELS, 200.0f, 20.0f); // Example: 1000 Hz sampling, 10 Hz cutoff
            // lowpass_filter_emg_signal(&received_batch, NUM_ADC_CHANNELS, 200.0f, 40.0f); // Example: 1000 Hz sampling, 10 Hz cutoff
            // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, serial_plotter_formatter_voltage);
            char *adc_values_string = __get_adc_values_string((adc_ctrl_t*)&received_batch, NUM_ADC_CHANNELS, serial_plotter_mean_formatter);
            // char *adc_values_string = __get_adc_values_string((adc_ctrl_t*)&received_batch, NUM_ADC_CHANNELS, serial_plotter_formatter);
            // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, json_formatter);
            // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, csv_formatter);


            printf("%s", adc_values_string);
            gpio_set_level(13, 0);
        }
        else
        {
            // Handle the case where receiving from the queue failed
            printf("Failed to receive ADC batch from the queue\n");
        }
        vTaskDelay(1);
    }
}

