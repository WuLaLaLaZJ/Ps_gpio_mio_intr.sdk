/*
 * main.c
 *
 *  Created on: 2024��10��30��
 *      Author: ��������
 */

#include <stdio.h>
#include "sleep.h"
#include "xil_printf.h"
#include "xparameters.h"	//��������������Ϣ
#include "xgpiops.h"		//�����й�GPIO��������
#include "xscugic.h"		//�����ж���Ӧ������GIC���жϹ���������˼��
#include "xil_exception.h"
#include "xplatform_info.h"

//�豸ʵ����
XGpioPs GPIO; 					//GPIOʵ��   XGpioPs�ṹ�壬�����и������ò����������XGpioPs_Config
XGpioPs_Config *ConfigPtr;		//GPIO����������Ϣ  ��һ���ṹ��ָ�룬��������GPIO������Ϣ

XScuGic Intc;					//�ж�ʵ�� 	 XScuGic�ǽṹ��
XScuGic_Config *IntcConfig; 	//�жϿ�����������Ϣ


//����ID
#define GPIO_ID       		XPAR_XGPIOPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define GPIO_INTERRUPT_ID	XPAR_XGPIOPS_0_INTR 		//GPIO���ж�ID

//����LED��Key
#define PS_LED1 0
#define PS_KEY1 9

//����LED��״̬
int flag = 0;

//GPIO���жϱ����52

/******************************************************************************************
 * GPIO��ʼ������
 * ���ܣ���ʼ��GPIO�����ö�ӦGPIO���������
 * ����1��XGpioPs�ĵ�ַ
 * ����2���豸ID
 * ***************************************************************************************/
void Gpio_Config(XGpioPs *Gpio_ps, u16 DeviceId)
{
	//��һ������ʼ��GPIO������
	//ͨ���豸ID����������������Ϣ�������ظ�ConfigPtr����ṹ��ָ��
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
 *�жϷ�����
 *ע�⣺һ��Ҫ�����жϺ�����жϱ�־λ���������Ҫ���´��ж�ʹ��
 *�����STM32��PS�˵��жϹر�Ҫ�������Σ������κ����
 * �رպ�Ҫ���ж�ʹ��
 * ***************************************************************************************/
void MIO_INTR_PROCESS()
{
	printf("intr_test\n");
	flag = ~flag;
	//�����ж�
	XGpioPs_IntrDisablePin(&GPIO,PS_KEY1);
	//����жϱ�־λ
	XGpioPs_IntrClearPin(&GPIO,PS_KEY1);
	//���´��ж�ʹ��
	XGpioPs_IntrEnablePin(&GPIO,PS_KEY1);
}

/******************************************************************************************
 * �����ж�ϵͳ
 * ���ܣ���ʼ���жϿ������������жϴ�����������Ϊ�����ش����������쳣�������жϣ�
 * ����1���ж�ʵ���ĵ�ַ
 * ����2��GPIOʵ��
 * ����3��GPIO���ж�ID
 * ***************************************************************************************/
void SetupInterruptSystem(XScuGic *GicInstancePtr, XGpioPs *Gpio,u16 GpioIntrId)
{
		
//step1����������ID����������Ϣ������ʼ��

		//�����ж�������Ϣ���������ò�������ʼ���жϿ�����
		IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
		//��ʼ���жϿ�����
		XScuGic_CfgInitialize(GicInstancePtr, IntcConfig,IntcConfig->CpuBaseAddress);

//step2����ʼ���жϷ������������쳣������

		//��ʼ���쳣���ж�����Ҳ���쳣��һ�֣�
		Xil_ExceptionInit();
		//���ж�����ע��һ����������������һ���жϷ�����
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XScuGic_InterruptHandler,GicInstancePtr);
		//ʹ�ܴ��������쳣�����ܣ����жϣ�
		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

//step3�������жϴ�����

		//�����жϴ��������ڶ�������(Xil_ExceptionHandler)��ĺ��������жϴ������������������
		XScuGic_Connect(GicInstancePtr, GpioIntrId,(Xil_ExceptionHandler)MIO_INTR_PROCESS,(void *)Gpio);

//step4�������жϴ���ģʽ��ʹ���ж����ź�GIC�жϹ��������ж�

		//����PS_KEY1���ж�Ϊ�����ش���
		XGpioPs_SetIntrTypePin(Gpio,PS_KEY1,XGPIOPS_IRQ_TYPE_EDGE_RISING);
		//�����ж�ʹ��
		XGpioPs_IntrEnablePin(Gpio,PS_KEY1);
		//��GPIO�豸�жϣ���GIC�жϹ�������GPIO��ͨ����
		XScuGic_Enable(GicInstancePtr, GpioIntrId);		
}

int main()
{
	printf("MIO TEST");
	Gpio_Config(&GPIO,GPIO_ID);//ע������ĵ�һ�������ǵ�ַ
	SetupInterruptSystem(&Intc,&GPIO, GPIO_INTERRUPT_ID);
	while(1)
	{
		if(XGpioPs_ReadPin(&GPIO,PS_KEY1) == 0)
		{	//����ʹ�����˲����ݣ��˵��˶���		
			XGpioPs_WritePin(&GPIO,PS_LED1,flag);
		}
	}
}
