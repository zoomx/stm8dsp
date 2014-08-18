/********************************************************************************
 *								I N C L U D E S									*
 ********************************************************************************/
#include <stm8l15x.h>

/********************************************************************************
 *								D E F I N E S									*
 ********************************************************************************/
#define	FIRST_BUFFER	0
#define	SECOND_BUFFER	1
#define	BUF_LEN			128

/********************************************************************************
 *								T Y P E D E F S									*
 ********************************************************************************/
typedef	uint8_t		BYTE;
typedef	uint16_t	WORD;
typedef	uint32_t	DWORD;

/********************************************************************************
 *						S T A T I C   V A R I A B L E S							*
 ********************************************************************************/

/********************************************************************************
 *						G L O B A L   V A R I A B L E S							*
 ********************************************************************************/
volatile WORD	Buffer[BUF_LEN];
BYTE			BuffNum = 0;
BYTE			Cnt;
/********************************************************************************
 *						S T A T I C   F U N C T I O N S							*
 ********************************************************************************/
static void Init_ADC_DAC (void)
{
	// CLK
	CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_DAC, ENABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_DMA1, ENABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM1, ENABLE);
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
	
	// ADC
	GPIO_Init(GPIOD, GPIO_Pin_4, GPIO_Mode_In_FL_No_IT);				// Configure PD4 (pin 33) to Floating In
	ADC_SchmittTriggerConfig(ADC1, ADC_Channel_10, DISABLE);			// Disable Schmitt trigger on PD4
	ADC_Init(ADC1, ADC_ConversionMode_Single, ADC_Resolution_12Bit, ADC_Prescaler_1);	// Init ADC: Single conv., 12 bit, no presc.
	ADC_SamplingTimeConfig(ADC1, ADC_Group_FastChannels, ADC_SamplingTime_48Cycles);	// Set Sampling Time
	ADC_Cmd(ADC1, ENABLE);												// Enable ADC1
	ADC_ChannelCmd(ADC1, ADC_Channel_10, ENABLE);						// Enable ADC Channel 10
	
	// DAC
	GPIO_Init(GPIOF, GPIO_Pin_0, GPIO_Mode_In_FL_No_IT);				// Configure PF0 (pin 32) to Floating In
	ADC_SchmittTriggerConfig(ADC1, ADC_Channel_24, DISABLE);			// Disable Schmitt trigger on PF0
	DAC_DeInit();														// Deinit DAC
	DAC_Init(DAC_Channel_1, DAC_Trigger_T4_TRGO, DAC_OutputBuffer_Enable);	// Init DAC: TIM4 Trigger, Out Buffer Enabled
	DAC_Cmd(DAC_Channel_1, ENABLE);										// Enable DAC Channel 1
	
	// SYSCFG
	SYSCFG_REMAPDMAChannelConfig(REMAP_DMA1Channel_ADC1ToChannel0);
	
	// DMA
	DMA_GlobalDeInit();
	DMA_DeInit(DMA1_Channel0);
	DMA_Init(DMA1_Channel0,
			 (WORD)(&Buffer),
			 ADC1_BASE + 0x04,
			 BUF_LEN,
			 DMA_DIR_PeripheralToMemory,
			 DMA_Mode_Circular,
			 DMA_MemoryIncMode_Inc,
			 DMA_Priority_High,
			 DMA_MemoryDataSize_HalfWord);
	DMA_ITConfig(DMA1_Channel0, DMA_ITx_TC, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_Init(DMA1_Channel3,
			 (WORD)(&Buffer),
			 DAC_BASE + 0x08,
			 BUF_LEN,
			 DMA_DIR_MemoryToPeripheral,
			 DMA_Mode_Circular,
			 DMA_MemoryIncMode_Inc,
			 DMA_Priority_Medium,
			 DMA_MemoryDataSize_HalfWord);
	DMA_ITConfig(DMA1_Channel3, DMA_ITx_TC, ENABLE);
	DMA_GlobalCmd(ENABLE);
	
	ADC_DMACmd(ADC1, ENABLE);											// Enable ADC DMA
	DAC_DMACmd(DAC_Channel_1, ENABLE);									// Disable DAC DMA
	ADC_ExternalTrigConfig(ADC1, ADC_ExtEventSelection_Trigger2,
						   ADC_ExtTRGSensitivity_Rising);
	TIM1_TimeBaseInit(1,
					  TIM1_CounterMode_Up,
					  10,
					  10);
	TIM1_SelectOutputTrigger(TIM1_TRGOSource_Update);
	TIM1_Cmd(ENABLE);
	
	ADC_ExternalTrigConfig(ADC1, ADC_ExtEventSelection_Trigger2, ADC_ExtTRGSensitivity_Rising);
	
	// TIM4
	TIM4_DeInit();
	TIM4_TimeBaseInit(TIM4_Prescaler_1, 10);
	TIM4_SelectOutputTrigger(TIM4_TRGOSource_Update);
	TIM4_Cmd(ENABLE);
}
/********************************************************************************/
/*static void DAC_Out(WORD data)
{
	DAC_SetChannel1Data(DAC_Align_12b_R, data);
}*/
/********************************************************************************/
/*static WORD Read_ADC (ADC_Channel_TypeDef Channel)
{
	WORD data;
	
	ADC_ChannelCmd(ADC1, Channel, ENABLE);
	if (Channel == ADC_Channel_Vrefint)
		ADC_VrefintCmd(ENABLE);
	ADC_SoftwareStartConv(ADC1);
	while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
	data  = ADC_GetConversionValue(ADC1);
	if (Channel == ADC_Channel_Vrefint)
		ADC_VrefintCmd(DISABLE);
	ADC_ChannelCmd(ADC1, Channel, DISABLE);
	
	return data;
}*/

/********************************************************************************
 *						G L O B A L   F U N C T I O N S							*
 ********************************************************************************/

/********************************************************************************
 *							M A I N   F U N C T I O N							*
 ********************************************************************************/
int main (void)
{
	Init_ADC_DAC();
	GPIO_Init(GPIOC, GPIO_Pin_7, GPIO_Mode_Out_PP_Low_Slow);
	
	DMA_Cmd(DMA1_Channel0, ENABLE);
	DMA_Cmd(DMA1_Channel3, ENABLE);
	
	while (1)
	{
		while (!DMA_GetITStatus(DMA1_IT_TC0));
		DMA_ClearITPendingBit(DMA1_IT_TC0);
		while (!DMA_GetITStatus(DMA1_IT_TC3));
		DMA_ClearITPendingBit(DMA1_IT_TC3);
	}
}
/********************************************************************************
 *									E O F										*
 ********************************************************************************/
