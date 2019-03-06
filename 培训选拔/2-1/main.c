#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/fpu.h"

uint32_t ui32Period;
void UARTSend(const uint8_t *pucBuffer, uint32_t ulCount);
void InitADC(uint64_t ui64ADC);

int main(void)
{
    FPUEnable();
    FPULazyStackingEnable();
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,GPIO_PIN_1);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE,GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE,SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    InitADC(ADC_CTL_TS);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE,TIMER_CFG_PERIODIC);

    ui32Period = (SysCtlClockGet()/2);
    TimerLoadSet(TIMER0_BASE,TIMER_A,ui32Period -1);
    TimerLoadSet(TIMER0_BASE,TIMER_B,5000);

    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0B);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    IntMasterEnable();


    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE,UART_INT_RX | UART_INT_RT);
    TimerEnable(TIMER0_BASE,TIMER_A);

    while(1){
    }

}
void Timer0IntHandler(void)
{
    TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);

    uint32_t ui32ADC0Value[4];
    volatile uint32_t ui32TempAvg;
    volatile double ui32TempValueC;
    uint8_t ui8Temp[4];

    ADCIntClear(ADC0_BASE,1);
    TimerIntClear(TIMER0_BASE,TIMER_TIMB_TIMEOUT);
    TimerControlTrigger(ADC0_BASE,TIMER_B,ADC0_BASE);

    while (!ADCIntStatus(ADC0_BASE,1,false)){

    }

    ADCSequenceDataGet(ADC0_BASE,1,ui32ADC0Value);
    ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] +2)/4;

    ui32TempValueC = (1475 - (2475*ui32TempAvg)/4096)/10.0;

    ui8Temp[0] = ui32TempValueC/10;
    ui8Temp[1] = (ui32TempValueC-ui8Temp[0]*10)/1;
    ui8Temp[2] = (ui32TempValueC*10-ui8Temp[0]*100-ui8Temp[1]*10)/1;
    ui8Temp[3] = (ui32TempValueC*100-ui8Temp[0]*1000-ui8Temp[1]*100-ui8Temp[2]*10)/1;

    ui8Temp[0] = ui8Temp[0]+48;
    ui8Temp[1] = ui8Temp[1]+48;
    ui8Temp[2] = ui8Temp[2]+48;
    ui8Temp[3] = ui8Temp[3]+48;

    UARTSend(ui8Temp, 4);
    UARTCharPut(UART0_BASE,'\r');
    UARTCharPut(UART0_BASE,'\n');
}

void InitADC(uint64_t ui64ADC){
    ADCReferenceSet(ADC0_BASE,ADC_REF_EXT_3V);
    ADCSequenceConfigure(ADC0_BASE,1,ADC_TRIGGER_PROCESSOR,0);
    ADCSequenceStepConfigure(ADC0_BASE,1,0,ui64ADC);
    ADCSequenceStepConfigure(ADC0_BASE,1,1,ui64ADC);
    ADCSequenceStepConfigure(ADC0_BASE,1,2,ui64ADC);
    ADCSequenceStepConfigure(ADC0_BASE,1,3,ui64ADC|ADC_CTL_IE|ADC_CTL_END);

    IntEnable(INT_ADC0SS1);
    ADCIntEnable(ADC0_BASE,0);
    IntPrioritySet(INT_ADC0SS1,0);
    ADCSequenceEnable(ADC0_BASE,1);
}

void UARTSend(const uint8_t *pucBuffer, uint32_t ulCount){
    while(ulCount--){
        UARTCharPut(UART0_BASE,*pucBuffer++);
    }

}
