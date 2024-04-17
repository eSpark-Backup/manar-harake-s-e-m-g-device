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
#include "lsm6dsr_reg.h"
#include "twi.h"

#define USART_SERIAL                     CONF_UART
#define USART_SERIAL_BAUDRATE            CONF_UART_BAUDRATE
#define USART_SERIAL_CHAR_LENGTH         CONF_UART_CHAR_LENGTH
#define USART_SERIAL_PARITY              CONF_UART_PARITY
#define USART_SERIAL_STOP_BIT            CONF_UART_STOP_BITS

/* IMU Related definitions */
#define LSM6DSR_ACC_SENSITIVITY_FS_2G   0.061f
#define LSM6DSR_ACC_SENSITIVITY_FS_4G   0.122f
#define LSM6DSR_ACC_SENSITIVITY_FS_8G   0.244f
#define LSM6DSR_ACC_SENSITIVITY_FS_16G  0.488f

#define LSM6DSR_GYRO_SENSITIVITY_FS_125DPS    4.370f
#define LSM6DSR_GYRO_SENSITIVITY_FS_250DPS    8.750f
#define LSM6DSR_GYRO_SENSITIVITY_FS_500DPS   17.500f
#define LSM6DSR_GYRO_SENSITIVITY_FS_1000DPS  35.000f
#define LSM6DSR_GYRO_SENSITIVITY_FS_2000DPS  70.000f
#define LSM6DSR_GYRO_SENSITIVITY_FS_4000DPS 140.000f

typedef struct
{
	lsm6dsr_odr_xl_t acc_odr;
	lsm6dsr_odr_g_t gyro_odr;
	
	uint8_t acc_is_enabled;
	uint8_t gyro_is_enabled;
}LSM6DSR;

typedef enum
{
	LSM6DSR_OK = 0,
	LSM6DSR_ERROR = 1
} LSM6DSRStatusTypeDef;

/**** TWI Parameters ****/
#define TWI_SPEED			400000
#define BOARD_ID_TWI_IMU    ID_TWI0
#define BOARD_BASE_TWI_IMU  TWI0
uint8_t data_received;
uint8_t data_transmitted;
/**** TWI Parameters ****/

/** LSM6DSR Registers **/
#define IMU_BUS_ADDR	0x6B // 0x6B (based on SDO/SA0 pin)
#define IMU_WHO_AM_I	0x0F
#define STATUS_REG		0x1E
lsm6dsr_ctx_t reg_ctx;
/** LSM6DSR Registers **/

/** TWI & LSM6DSR Functions **/
int32_t LSM6DSR_io_read(void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead);
int32_t LSM6DSR_io_write(void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite);

void twiBegin(uint8_t devAddress);
void imuSetCallback(void);
uint8_t imuBegin(LSM6DSR* LSM);
void imuReadID(uint8_t *Id);

uint8_t imuEnable_G(LSM6DSR* LSM);
uint8_t Get_G_Axes(int32_t *AngularRate);
uint8_t Get_G_Sensitivity(float *Sensitivity);

uint8_t imuEnable_X(LSM6DSR* LSM);
uint8_t Get_X_Sensitivity(float *Sensitivity);
uint8_t Get_X_Axes(int32_t *Acceleration);
/** TWI & LSM6DSR Functions **/

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

//#define JSON_TEMPLATE "{\"ADC_Values\":{\"Value_1\":%d,\"Value_2\":%d,\"Value_3\":%d,\"Value_4\":%d,\"Value_5\":%d,\"Value_6\":%d,\"Value_7\":%d,\"Value_8\":%d}}\r\n"
#define JSON_TEMPLATE "{\"ADC\":{\"Value_1\":%d,\"Value_2\":%d,\"Value_3\":%d,\"Value_4\":%d,\"Value_5\":%d,\"Value_6\":%d,\"Value_7\":%d,\"Value_8\":%d},\"Gyro\":{\"X\":%d,\"Y\":%d,\"Z\":%d},\"Acc\":{\"X\":%d,\"Y\":%d,\"Z\":%d}}\r\n"


/* UART Data buffer */
char buf[300];

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
	/* Start IMU */
	twiBegin( (uint8_t)IMU_BUS_ADDR );
	imuSetCallback();
	imuReadID(&data_received);
		
	LSM6DSR sensor;
	imuBegin(&sensor);
	imuEnable_X(&sensor);
	imuEnable_G(&sensor);
		
	char val[20];
	int32_t accelerometer[3];
	int32_t gyroscope[3];
	
	while(true)
	{

		delay_ms(1000);
		Get_X_Axes(accelerometer);
		Get_G_Axes(gyroscope);
		
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
			g_adc_sample_data.us_value[4], g_adc_sample_data.us_value[5], g_adc_sample_data.us_value[6], g_adc_sample_data.us_value[7],
			(int)gyroscope[0], (int)gyroscope[1], (int)gyroscope[2], (int)accelerometer[0], (int)accelerometer[1], (int)accelerometer[2]);
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

/************************************************************************/
/*							IMU and TWI Func							*/
/************************************************************************/
uint8_t imuEnable_X(LSM6DSR* LSM)
{
	/* Check if the component is already enabled */
	if (LSM->acc_is_enabled == 1U)
	{
		return LSM6DSR_OK;
	}

	/* Output data rate selection. */
	if (lsm6dsr_xl_data_rate_set(&reg_ctx, LSM->acc_odr) != LSM6DSR_OK)
	{
		return LSM6DSR_ERROR;
	}

	LSM->acc_is_enabled = 1;

	return LSM6DSR_OK;
}

uint8_t imuEnable_G(LSM6DSR* LSM)
{
	/* Check if the component is already enabled */
	if (LSM->gyro_is_enabled == 1U)
	{
		//println("gyro is already enabled!");
		return LSM6DSR_OK;
	}

	/* Output data rate selection. */
	if (lsm6dsr_gy_data_rate_set(&reg_ctx, LSM->gyro_odr) != LSM6DSR_OK)
	{
		//println("gyro data-rate not set!");
		return LSM6DSR_ERROR;
	}

	LSM->gyro_is_enabled = 1;

	return LSM6DSR_OK;
}

uint8_t Get_G_Axes(int32_t *AngularRate)
{
	axis3bit16_t data_raw;
	float sensitivity;

	/* Read raw data values. */
	if (lsm6dsr_angular_rate_raw_get(&reg_ctx, data_raw.u8bit) != LSM6DSR_OK)
	{
		return LSM6DSR_ERROR;
	}

	/* Get LSM6DSR actual sensitivity. */
	if (Get_G_Sensitivity(&sensitivity) != LSM6DSR_OK)
	{
		return LSM6DSR_ERROR;
	}

	/* Calculate the data. */
	AngularRate[0] = (int32_t)((float)((float)data_raw.i16bit[0] * sensitivity));
	AngularRate[1] = (int32_t)((float)((float)data_raw.i16bit[1] * sensitivity));
	AngularRate[2] = (int32_t)((float)((float)data_raw.i16bit[2] * sensitivity));

	return LSM6DSR_OK;
}

uint8_t Get_G_Sensitivity(float *Sensitivity)
{
	LSM6DSRStatusTypeDef ret = LSM6DSR_OK;
	lsm6dsr_fs_g_t full_scale;

	/* Read actual full scale selection from sensor. */
	if (lsm6dsr_gy_full_scale_get(&reg_ctx, &full_scale) != LSM6DSR_OK)
	{
		return LSM6DSR_ERROR;
	}

	/* Store the sensitivity based on actual full scale. */
	switch (full_scale)
	{
		case LSM6DSR_125dps:
		*Sensitivity = LSM6DSR_GYRO_SENSITIVITY_FS_125DPS;
		break;

		case LSM6DSR_250dps:
		*Sensitivity = LSM6DSR_GYRO_SENSITIVITY_FS_250DPS;
		break;

		case LSM6DSR_500dps:
		*Sensitivity = LSM6DSR_GYRO_SENSITIVITY_FS_500DPS;
		break;

		case LSM6DSR_1000dps:
		*Sensitivity = LSM6DSR_GYRO_SENSITIVITY_FS_1000DPS;
		break;

		case LSM6DSR_2000dps:
		*Sensitivity = LSM6DSR_GYRO_SENSITIVITY_FS_2000DPS;
		break;

		case LSM6DSR_4000dps:
		*Sensitivity = LSM6DSR_GYRO_SENSITIVITY_FS_4000DPS;
		break;

		default:
		ret = LSM6DSR_ERROR;
		break;
	}

	return ret;
}

uint8_t Get_X_Axes(int32_t *Acceleration)
{
	axis3bit16_t data_raw;
	float sensitivity = 0.0f;

	/* Read raw data values. */
	if (lsm6dsr_acceleration_raw_get(&reg_ctx, data_raw.u8bit) != LSM6DSR_OK)
	{
		return LSM6DSR_ERROR;
	}

	/* Get LSM6DSR actual sensitivity. */
	if (Get_X_Sensitivity(&sensitivity) != LSM6DSR_OK)
	{
		return LSM6DSR_ERROR;
	}
	/* Calculate the data. */
	Acceleration[0] = (int32_t)((float)((float)data_raw.i16bit[0] * sensitivity));
	Acceleration[1] = (int32_t)((float)((float)data_raw.i16bit[1] * sensitivity));
	Acceleration[2] = (int32_t)((float)((float)data_raw.i16bit[2] * sensitivity));
	
	return LSM6DSR_OK;
}

uint8_t Get_X_Sensitivity(float *Sensitivity)
{
	LSM6DSRStatusTypeDef ret = LSM6DSR_OK;
	lsm6dsr_fs_xl_t full_scale;

	/* Read actual full scale selection from sensor. */
	if (lsm6dsr_xl_full_scale_get(&reg_ctx, &full_scale) != LSM6DSR_OK)
	{
		return LSM6DSR_ERROR;
	}

	/* Store the Sensitivity based on actual full scale. */
	switch (full_scale)
	{
		case LSM6DSR_2g:
		*Sensitivity = LSM6DSR_ACC_SENSITIVITY_FS_2G;
		break;

		case LSM6DSR_4g:
		*Sensitivity = LSM6DSR_ACC_SENSITIVITY_FS_4G;
		break;

		case LSM6DSR_8g:
		*Sensitivity = LSM6DSR_ACC_SENSITIVITY_FS_8G;
		break;

		case LSM6DSR_16g:
		*Sensitivity = LSM6DSR_ACC_SENSITIVITY_FS_16G;
		break;

		default:
		ret = LSM6DSR_ERROR;
		break;
	}

	return ret;
}

/* ******* TWI Begin ******** 
	Enables the I2C peripheral.
	
	@para 
		devAddress : The bus address of the I2C Device
		
*/	

void twiBegin(uint8_t devAddress)
{
	pmc_enable_periph_clk(ID_PIOA);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA3);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA4);
	
	twi_options_t opt;
	/* Enable the peripheral clock for TWI */
	pmc_enable_periph_clk(BOARD_ID_TWI_IMU);
	
	/* Configure the options of TWI driver */
	opt.master_clk	= sysclk_get_peripheral_hz();
	opt.chip		= devAddress;
	opt.speed		= TWI_SPEED;
	opt.smbus		= 0;
	
	twi_master_init(BOARD_BASE_TWI_IMU, &opt);
}

void imuSetCallback(void)
{
	reg_ctx.write_reg = LSM6DSR_io_write;
	reg_ctx.read_reg = LSM6DSR_io_read;
	reg_ctx.handle	=	(void *)NULL;	
}

void imuReadID(uint8_t *Id)
{
	if (lsm6dsr_device_id_get(&reg_ctx, Id) == LSM6DSR_OK)
	{
		//println("Reading WHO_AM_I Register");
		//println("packet_recieved : ");
		char temp[10];
		sprintf(temp, "%d", *Id);
		//println(temp);	
	}
}


int32_t LSM6DSR_io_read(void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead)
{
	twi_packet_t packet_received = {
		.chip		  = IMU_BUS_ADDR,
		.addr[0]      = ReadAddr,      // TWI slave memory address data MSB
		.addr_length  = 1,     // TWI slave memory address data sizes
		.buffer       = (void *)pBuffer,        // transfer data destination buffer
		.length       = nBytesToRead   // transfer data size (bytes)
	};
	return  ( twi_master_read(BOARD_BASE_TWI_IMU, &packet_received) );
}

int32_t LSM6DSR_io_write(void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite)
{
	twi_packet_t packet_transmit = {
		.chip		  = IMU_BUS_ADDR,
		.addr[0]      = WriteAddr,      // TWI slave memory address data MSB
		.addr_length  = 1,     // TWI slave memory address data sizes
		.buffer       = (void *)pBuffer,        // transfer data destination buffer
		.length       = nBytesToWrite   // transfer data size (bytes)
	};
	return ( twi_master_write(BOARD_BASE_TWI_IMU, &packet_transmit) );
}

uint8_t imuBegin(LSM6DSR* LSM)
{
	 /* Disable I3C */
  if (lsm6dsr_i3c_disable_set(&reg_ctx, LSM6DSR_I3C_DISABLE) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }

  /* Enable register address automatically incremented during a multiple byte
  access with a serial interface. */
  if (lsm6dsr_auto_increment_set(&reg_ctx, PROPERTY_ENABLE) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }

  /* Enable BDU */
  if (lsm6dsr_block_data_update_set(&reg_ctx, PROPERTY_ENABLE) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }

  /* FIFO mode selection */
  if (lsm6dsr_fifo_mode_set(&reg_ctx, LSM6DSR_BYPASS_MODE) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }

  /* Select default output data rate. */
  LSM->acc_odr = LSM6DSR_XL_ODR_104Hz;

  /* Output data rate selection - power down. */
  if (lsm6dsr_xl_data_rate_set(&reg_ctx, LSM6DSR_XL_ODR_OFF) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }

  /* Full scale selection. */
  if (lsm6dsr_xl_full_scale_set(&reg_ctx, LSM6DSR_2g) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }

  /* Select default output data rate. */
  LSM->gyro_odr = LSM6DSR_GY_ODR_104Hz;

  /* Output data rate selection - power down. */
  if (lsm6dsr_gy_data_rate_set(&reg_ctx, LSM6DSR_GY_ODR_OFF) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }

  /* Full scale selection. */
  if (lsm6dsr_gy_full_scale_set(&reg_ctx, LSM6DSR_2000dps) != LSM6DSR_OK)
  {
    return LSM6DSR_ERROR;
  }
  
  LSM->acc_is_enabled = 0;
  LSM->gyro_is_enabled = 0;

  return LSM6DSR_OK;
}