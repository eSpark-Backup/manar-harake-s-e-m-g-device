#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "adc_controller.h"

typedef enum {
    ADC_TYPE_NONE = 0, // No ADC type assigned
    ADC_TYPE_ADC1,     // ADC1 type
    ADC_TYPE_ADC2      // ADC2 type
} adc_type_t;

typedef struct adc_ctrl_t {
    union {
        adc1_channel_t adc1_channel;
        adc2_channel_t adc2_channel;
    } adc_ctl_channel_t;
    int raw_value;     // Store the raw ADC reading
    adc_type_t type;   // Type of ADC (ADC1 or ADC2)
} adc_ctrl_t;

#define MAX_ADC_OUTPUT_LENGTH 1024*2  // Adjust size as needed

typedef char* (*adc_formatter_t)(const adc_ctrl_t *adc_channels, size_t num_channels, char *output, size_t max_output_length);

adc_ctrl_t adc_channels[NUM_ADC_CHANNELS] = {
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_5, .type = ADC_TYPE_ADC1 },
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_0, .type = ADC_TYPE_ADC1 },
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_1, .type = ADC_TYPE_ADC1 },
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_2, .type = ADC_TYPE_ADC1 },
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_3, .type = ADC_TYPE_ADC1 },
    { .adc_ctl_channel_t.adc1_channel = ADC1_EXAMPLE_CHANNEL_4, .type = ADC_TYPE_ADC1 },
    { .adc_ctl_channel_t.adc2_channel = ADC2_EXAMPLE_CHANNEL_0, .type = ADC_TYPE_ADC2 },
    { .adc_ctl_channel_t.adc2_channel = ADC2_EXAMPLE_CHANNEL_1, .type = ADC_TYPE_ADC2 },
};

esp_err_t __initialize_adc(adc_ctrl_t *adc_channels, size_t num_channels)
{
    gpio_num_t adc_gpio_num;
    esp_err_t r = ESP_FAIL;

    for (size_t i = 0; i < num_channels; i++)
    {
        switch (adc_channels[i].type)
        {
        case ADC_TYPE_ADC1:
            {
                adc1_channel_t channel = adc_channels[i].adc_ctl_channel_t.adc1_channel;

                // Get GPIO for ADC1 channel
                r = adc1_pad_get_io_num(channel, &adc_gpio_num);
                assert(r == ESP_OK);

                printf("ADC1 channel %d @ GPIO %d\n", channel, adc_gpio_num);

                // Configure attenuation for ADC1
                r = adc1_config_channel_atten(channel, ADC_CON_ATTEN);
                assert(r == ESP_OK);
                break;
            }
        case ADC_TYPE_ADC2:
        {
            adc2_channel_t channel = adc_channels[i].adc_ctl_channel_t.adc2_channel;

            // Get GPIO for ADC2 channel
            r = adc2_pad_get_io_num(channel, &adc_gpio_num);
            assert(r == ESP_OK);

            printf("ADC2 channel %d @ GPIO %d\n", channel, adc_gpio_num);

            // Configure attenuation for ADC2
            r = adc2_config_channel_atten(channel, ADC_CON_ATTEN);
            assert(r == ESP_OK);
            break;
        }
        
        default:
            break;
        }
        vTaskDelay(2 * portTICK_PERIOD_MS);
    }

    return r;
}

esp_err_t initializing_adc(void)
{
    esp_err_t r = ESP_FAIL;

    r = __initialize_adc(adc_channels, NUM_ADC_CHANNELS);
    assert( r == ESP_OK );

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
            case ADC_TYPE_ADC1:
            {
                adc1_channel_t channel = adc_channels[i].adc_ctl_channel_t.adc1_channel;
                read_raw = adc1_get_raw(channel);
                adc_channels[i].raw_value = read_raw; // Save the raw value
                break;
            }

            case ADC_TYPE_ADC2:
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

            case ADC_TYPE_NONE:
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

char* serial_plotter_formatter(const adc_ctrl_t *adc_channels, size_t num_channels, char *output, size_t max_output_length)
{
    size_t offset = 0;

    // Loop through the ADC channels and format each channel's data for the Serial Plotter
    for (size_t i = 0; i < num_channels; i++)
    {
        int written = 0;  // Number of characters written in this iteration
        
        // Start with an empty line, formatting each ADC reading with its corresponding label
        switch (adc_channels[i].type)
        {
            case ADC_TYPE_ADC1:
                written = snprintf(output + offset, max_output_length - offset,
                                   "%d", adc_channels[i].raw_value);
                break;

            case ADC_TYPE_ADC2:
                written = snprintf(output + offset, max_output_length - offset,
                                   "%d", adc_channels[i].raw_value);
                break;

            case ADC_TYPE_NONE:
            default:
                written = snprintf(output + offset, max_output_length - offset,
                                   "No ADC data available");
                break;
        }

        // If written was negative or we are about to overflow, stop writing more data
        if (written < 0 || written >= max_output_length - offset)
        {
            break;
        }

        // Move the offset ahead by the number of characters written
        offset += written;

        // If this is not the last channel, add a comma to separate the values for the plotter
        if (i < num_channels - 1)
        {
            written = snprintf(output + offset, max_output_length - offset, ",");
            offset += written;
        }
    }

    // After all values are added for this sample, add a newline to separate the sample from the next one
    snprintf(output + offset, max_output_length - offset, "\n");

    return output;
}

char* json_formatter(const adc_ctrl_t *adc_channels, size_t num_channels, char *output, size_t max_output_length)
{
    size_t offset = 0;
    offset += snprintf(output + offset, max_output_length - offset, "{\n");

    for (size_t i = 0; i < num_channels; i++)
    {
        switch (adc_channels[i].type)
        {
            case ADC_TYPE_ADC1:
                offset += snprintf(output + offset, max_output_length - offset,
                                   "  \"ADC1_Channel_%d\": %d,\n",
                                   adc_channels[i].adc_ctl_channel_t.adc1_channel,
                                   adc_channels[i].raw_value);
                break;

            case ADC_TYPE_ADC2:
                offset += snprintf(output + offset, max_output_length - offset,
                                   "  \"ADC2_Channel_%d\": %d,\n",
                                   adc_channels[i].adc_ctl_channel_t.adc2_channel,
                                   adc_channels[i].raw_value);
                break;

            case ADC_TYPE_NONE:
            default:
                offset += snprintf(output + offset, max_output_length - offset,
                                   "  \"Channel_%zu\": \"No ADC data available\",\n", i);
                break;
        }
    }

    // Remove the last comma and add closing bracket
    if (offset > 2)
        output[offset - 2] = '\0';  // Remove last comma

    offset += snprintf(output + offset, max_output_length - offset, "\n}\n");

    return output;
}

char* csv_formatter(const adc_ctrl_t *adc_channels, size_t num_channels, char *output, size_t max_output_length)
{
    size_t offset = 0;
    offset += snprintf(output + offset, max_output_length - offset, "Channel,Type,Raw Value\n");

    for (size_t i = 0; i < num_channels; i++)
    {
        switch (adc_channels[i].type)
        {
            case ADC_TYPE_ADC1:
                offset += snprintf(output + offset, max_output_length - offset,
                                   "ADC1_Channel_%d,ADC1,%d\n",
                                   adc_channels[i].adc_ctl_channel_t.adc1_channel,
                                   adc_channels[i].raw_value);
                break;

            case ADC_TYPE_ADC2:
                offset += snprintf(output + offset, max_output_length - offset,
                                   "ADC2_Channel_%d,ADC2,%d\n",
                                   adc_channels[i].adc_ctl_channel_t.adc2_channel,
                                   adc_channels[i].raw_value);
                break;

            case ADC_TYPE_NONE:
            default:
                offset += snprintf(output + offset, max_output_length - offset,
                                   "Channel_%zu,Unknown,No ADC data available\n", i);
                break;
        }
    }

    return output;
}


void print_adc_values()
{
    char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, serial_plotter_formatter);
    // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, json_formatter);
    // char *adc_values_string = __get_adc_values_string(adc_channels, NUM_ADC_CHANNELS, csv_formatter);
    printf("%s", adc_values_string);
}


void read_adc(void)
{
    __read_adc(adc_channels, NUM_ADC_CHANNELS, width);
}