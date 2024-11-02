/*
 * main.c
 *
 *  Created on: 2024��10��30��
 *      Author: ������
 */

#include <stdio.h>
#include "sleep.h"
#include "xil_printf.h"
#include "xparameters.h"//��������������Ϣ
#include "xgpiops.h"//�����й�GPIO��������
#include "xscugic.h"//�����ж���Ӧ����
#include "xil_exception.h"
#include "xplatform_info.h"

//�豸ʵ����
XGpioPs GPIO; 					//GPIOʵ��   XGpioPs�ṹ�壬�����и������ò����������XGpioPs_Config
XGpioPs_Config *ConfigPtr;		//GPIO����������Ϣ  ��һ���ṹ��ָ�룬��������GPIO������Ϣ

XScuGic Intc;					//�ж�ʵ��
XScuGic_Config *IntcConfig; 	//�жϿ�����������Ϣ


//����ID
#define GPIO_ID        XPAR_XGPIOPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID


//����LED��Key
#define PS_LED1 0
#define PS_KEY1 9

//����LED��״̬
int flag = 0;

/******************************************************************************************
 * GPIO��ʼ������
 * ���ܣ���ʼ��GPIO�����ö�ӦGPIO���������
 * ����1��XGpioPs�ĵ�ַ
 * ����2���豸ID
 * ***************************************************************************************/
void Gpio_Config(XGpioPs *Gpio_ps, u16 DeviceId)
{
	//��һ������ʼ��GPIO������
	//����������������Ϣ����������ID�ͼĴ�����ַ��XGpioPs_Config����ṹ��
	ConfigPtr = XGpioPs_LookupConfig(DeviceId);

	//��ʼ��GPIO�����ã�����һЩĬ��ֵ �������BANK ������ţ�����ID�ȣ��ر���GPIO���ж�
	//����1���豸ָ�루XGpioPs�ṹ���ַ��   ����2������������Ϣ��XGpioPs_Config�ṹ���ַ��
	XGpioPs_CfgInitialize(Gpio_ps, ConfigPtr, ConfigPtr->BaseAddr);

	//�ڶ���������GPIO�ķ���1Ϊ�����0Ϊ����
	XGpioPs_SetDirectionPin(Gpio_ps, PS_LED1, 1);
	XGpioPs_SetDirectionPin(Gpio_ps, PS_KEY1, 0);

	//���ʹ�� 	1Ϊʹ�� 	0Ϊ��ʹ��
	XGpioPs_SetOutputEnablePin(Gpio_ps, PS_LED1, 1);
}

/******************************************************************************************
 * �����ж�ϵͳ
 * ���ܣ���ʼ��GPIO�����ö�ӦGPIO���������
 * ����1��XGpioPs�ĵ�ַ
 * ����2���豸ID
 * ***************************************************************************************/
void SetupInterruptSystem(XScuGic *GicInstancePtr, XGpioPs *Gpio,u16 GpioIntrId)
{
		//��ʼ���쳣���ж�����Ҳ���쳣��һ�֣�
		Xil_ExceptionInit();

		//�����ж�������Ϣ���������ò�������ʼ���жϿ�����
		IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
		if (NULL == IntcConfig) {
			return XST_FAILURE;
		}

		/*
		 * Connect the interrupt controller interrupt handler to the hardware
		 * interrupt handling logic in the processor.
		 */
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
					(Xil_ExceptionHandler)XScuGic_InterruptHandler,
					GicInstancePtr);

		/*
		 * Connect the device driver handler that will be called when an
		 * interrupt for the device occurs, the handler defined above performs
		 * the specific interrupt processing for the device.
		 */
		Status = XScuGic_Connect(GicInstancePtr, GpioIntrId,
					(Xil_ExceptionHandler)XGpioPs_IntrHandler,
					(void *)Gpio);
		if (Status != XST_SUCCESS) {
			return Status;
		}

		/* Enable falling edge interrupts for all the pins in bank 0. */
		XGpioPs_SetIntrType(Gpio, GPIO_BANK, 0x00, 0xFFFFFFFF, 0x00);

		/* Set the handler for gpio interrupts. */
		XGpioPs_SetCallbackHandler(Gpio, (void *)Gpio, IntrHandler);


		/* Enable the GPIO interrupts of Bank 0. */
		XGpioPs_IntrEnable(Gpio, GPIO_BANK, (1 << Input_Pin));


		/* Enable the interrupt for the GPIO device. */
		XScuGic_Enable(GicInstancePtr, GpioIntrId);


		/* Enable interrupts in the Processor. */
		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
}


SetupInterruptSystem(XScuGic *GicInstancePtr, XGpioPs *Gpio,
				u16 GpioIntrId)


int main()
{
	printf("MIO TEST");
	Gpio_Config(&GPIO,GPIO_ID);//ע������ĵ�һ�������ǵ�ַ
	SetupInterruptSystem(Intc, Gpio, GPIO_INTERRUPT_ID);




}