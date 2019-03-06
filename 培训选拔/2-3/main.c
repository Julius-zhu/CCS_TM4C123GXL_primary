#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"

uint32_t ui32duty = 0,ui32Period;
int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,GPIO_PIN_1);

    GPIOPinTypeADC(GPIO_PORTD_BASE,GPIO_PIN_0);
    ADCReferenceSet(ADC0_BASE,ADC_REF_EXT_3V);
    ADCSequenceConfigure(ADC0_BASE,0,ADC_TRIGGER_PROCESSOR,0);
    ADCSequenceStepConfigure(ADC0_BASE,0,0,ADC_CTL_CH5);
    ADCSequenceStepConfigure(ADC0_BASE,0,1,ADC_CTL_CH5|ADC_CTL_IE|ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE,0);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);

    ui32Period = (SysCtlClockGet()/2000);
    TimerLoadSet(TIMER0_BASE,TIMER_A,ui32Period -1);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE,TIMER_A);

    while(1){
    }

}

void Timer0IntHandler(void)
    {
        TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);

        uint32_t ui32ADC0Value[2];
        volatile uint32_t ui32little;
        volatile double ui32TempValueC,ui32Vavg;

        ADCIntClear(ADC0_BASE,0);
        ADCProcessorTrigger(ADC0_BASE,0);

        while (!ADCIntStatus(ADC0_BASE,0,false)){

        }

        ADCSequenceDataGet(ADC0_BASE,0,ui32ADC0Value);
        ui32Vavg = (ui32ADC0Value[0] + ui32ADC0Value[1] +1)/2*3.3/4096;


        ui32duty = ui32Vavg/3.3*1000;
        ui32little = ui32Period * ui32duty / 1000;

        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1,2);
        SysCtlDelay(ui32little);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1,0);

        TimerLoadSet(TIMER0_BASE,TIMER_A,ui32Period - 1 -ui32little);
    }
