/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include "string.h"

#define USART_SERIAL                     CONF_UART
#define USART_SERIAL_BAUDRATE            CONF_UART_BAUDRATE
#define USART_SERIAL_CHAR_LENGTH         CONF_UART_CHAR_LENGTH
#define USART_SERIAL_PARITY              CONF_UART_PARITY
#define USART_SERIAL_STOP_BIT            CONF_UART_STOP_BITS

/* ADC global definitions */
/** All interrupt mask. */
#define ALL_INTERRUPT_MASK  0xffffffff

#define NUM_CHANNELS 8
/** ADC convention done mask. */
#define ADC_DONE_MASK   ( (1<<NUM_CHANNELS) - 1 )
/** The maximal digital value */
#define MAX_DIGITAL     (4096)
/** Reference voltage for ADC, in mv. */
#define VOLT_REF        (3300)
/** ADC sample data */
struct {
	uint8_t uc_ch_num[NUM_CHANNELS];
	uint16_t us_value[NUM_CHANNELS];
	uint16_t us_done;
} g_adc_sample_data;

enum adc_channel_num_t ch_list[NUM_CHANNELS] = {
	ADC_CHANNEL_0,
	ADC_CHANNEL_1,
	ADC_CHANNEL_2,
	ADC_CHANNEL_3,
	ADC_CHANNEL_4,
	ADC_CHANNEL_5,
	ADC_CHANNEL_6,
	ADC_CHANNEL_7
};

#define JSON_TEMPLATE "{\"ADC_Values\":{\"Value_1\":%d,\"Value_2\":%d,\"Value_3\":%d,\"Value_4\":%d,\"Value_5\":%d,\"Value_6\":%d,\"Value_7\":%d,\"Value_8\":%d}}\r\n"


/* UART Data buffer */
char buf[256];

/* Setup ADC Configurations */
void adc_setup(void)
{
	pmc_enable_periph_clk(ID_ADC);
	adc_init(ADC, sysclk_get_cpu_hz(), 6400000, ADC_STARTUP_TIME_4);
	memset((void *)&g_adc_sample_data, 0, sizeof(g_adc_sample_data));

	adc_configure_timing(ADC, 0, ADC_SETTLING_TIME_3, 1);
	
	adc_enable_tag(ADC);
	adc_configure_sequence(ADC, ch_list, NUM_CHANNELS);

	/* Enable sequencer. */
	adc_start_sequencer(ADC);

	/* Enable channels. */
	for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
		adc_enable_channel(ADC, (enum adc_channel_num_t)i);
		// 		adc_set_channel_input_gain(ADC, (enum adc_channel_num_t)i, ADC_GAINVALUE_1);
		// 		adc_disable_channel_differential_input(ADC, (enum adc_channel_num_t)i);
		/* Update channel number. */
		g_adc_sample_data.uc_ch_num[i] = ch_list[i];
	}
	
	adc_set_resolution(ADC, ADC_MR_LOWRES_BITS_12);

	adc_enable_interrupt(ADC, ADC_IER_DRDY);
	NVIC_EnableIRQ(ADC_IRQn);
	
	adc_configure_trigger(ADC, ADC_TRIG_SW, ADC_MR_FREERUN_ON);
	
}

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	sysclk_init();
	
	/* Initialize board */
	board_init();
	
	/* Initialize ADC */
	adc_setup();
	
	/* UART config */	
	static usart_serial_options_t usart_options = {
		.baudrate = USART_SERIAL_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT
	};
	usart_serial_init(USART_SERIAL, &usart_options);

	/* Insert application code here, after the board has been initialized. */
	
	while(true)
	{
		//const char* testString = "{\"ADC_Values\":{\"Value_1\":592,\"Value_2\":601,\"Value_3\":607,\"Value_4\":627,\"Value_5\":631,\"Value_6\":644,\"Value_7\":665,\"Value_8\":689}}\r\n";
		//char* testString = "Test hello world 1 2 3 4 5 6 7";
/*		usart_serial_write_packet(USART_SERIAL, (uint8_t*)testString, strlen(testString));*/
// 		usart_serial_putchar(USART_SERIAL, '\r');
// 		usart_serial_putchar(USART_SERIAL, '\n');
		delay_ms(1000);
		
		adc_start(ADC);
		/* Check if ADC sample is done. */
		if (g_adc_sample_data.us_done == ADC_DONE_MASK) {
			// 			for (int i = 0; i < NUM_CHANNELS; i++) {
			// 				sprintf(buf, "CH%02d voltage %0d mv\n",
			// 				(int)g_adc_sample_data.uc_ch_num[i],
			// 				(int)(g_adc_sample_data.us_value[i] * VOLT_REF / MAX_DIGITAL));
			// 				println(buf);
			// 			}
			sprintf(buf, JSON_TEMPLATE,
			g_adc_sample_data.us_value[0], g_adc_sample_data.us_value[1], g_adc_sample_data.us_value[2], g_adc_sample_data.us_value[3],
			g_adc_sample_data.us_value[4], g_adc_sample_data.us_value[5], g_adc_sample_data.us_value[6], g_adc_sample_data.us_value[7]);
			g_adc_sample_data.us_done = 0;
		}
		
		/* Send Packet to UART */
		usart_serial_write_packet(USART_SERIAL, buf, strlen(buf));
	}
}

/************************************************************************/
/*							ADC HANDLER									*/
/************************************************************************/
void ADC_Handler(void)
{
	uint32_t i;
	uint32_t ul_temp;
	uint8_t uc_ch_num;
	// Check the ADC conversion status
	// check= !check;
	if ((adc_get_status(ADC) & ADC_ISR_DRDY) == ADC_ISR_DRDY)
	{
		ul_temp = adc_get_latest_value(ADC);
		for (i = 0; i < NUM_CHANNELS; i++) {
			uc_ch_num = (ul_temp & ADC_LCDR_CHNB_Msk) >>
			ADC_LCDR_CHNB_Pos;
			if (g_adc_sample_data.uc_ch_num[i] == uc_ch_num) {
				g_adc_sample_data.us_value[i] =
				ul_temp &
				ADC_LCDR_LDATA_Msk;
				g_adc_sample_data.us_done |= 1 << i;
			}
		}
		//result = adc_get_latest_value(ADC);
	}
	
}
