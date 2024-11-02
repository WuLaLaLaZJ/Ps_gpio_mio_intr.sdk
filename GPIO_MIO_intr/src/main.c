/*
 * main.c
 *
 *  Created on: 2024年10月30日
 *      Author: 呜啦啦啦
 */

#include <stdio.h>
#include "sleep.h"
#include "xil_printf.h"
#include "xparameters.h"	//包含器件参数信息
#include "xgpiops.h"		//包含有关GPIO操作函数
#include "xscugic.h"		//包含中断响应函数（GIC是中断管理器的意思）
#include "xil_exception.h"
#include "xplatform_info.h"

//设备实例化
XGpioPs GPIO; 					//GPIO实例   XGpioPs结构体，里面有各种配置参数，其包括XGpioPs_Config
XGpioPs_Config *ConfigPtr;		//GPIO器件配置信息  是一个结构体指针，用于配置GPIO基本信息

XScuGic Intc;					//中断实例 	 XScuGic是结构体
XScuGic_Config *IntcConfig; 	//中断控制器配置信息


//定义ID
#define GPIO_ID       		XPAR_XGPIOPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define GPIO_INTERRUPT_ID	XPAR_XGPIOPS_0_INTR 		//GPIO的中断ID

//定义LED与Key
#define PS_LED1 0
#define PS_KEY1 9

//控制LED的状态
int flag = 0;

//GPIO的中断编号是52

/******************************************************************************************
 * GPIO初始化函数
 * 功能：初始化GPIO，配置对应GPIO的输入输出
 * 参数1：XGpioPs的地址
 * 参数2：设备ID
 * ***************************************************************************************/
void Gpio_Config(XGpioPs *Gpio_ps, u16 DeviceId)
{
	//第一步，初始化GPIO的驱动
	//通过设备ID查找器件的配置信息，并返回给ConfigPtr这个结构体指针
	ConfigPtr = XGpioPs_LookupConfig(DeviceId);

	//初始化GPIO的配置，设置一些默认值 比如最大BANK 最大引脚，器件ID等，关闭了GPIO的中断
	//参数1：设备指针（XGpioPs结构体地址）   参数2：器件配置信息（XGpioPs_Config结构体地址）
	XGpioPs_CfgInitialize(Gpio_ps, ConfigPtr, ConfigPtr->BaseAddr);

	//第二步：设置GPIO的方向，1为输出，0为输入
	XGpioPs_SetDirectionPin(Gpio_ps, PS_LED1, 1);
	XGpioPs_SetDirectionPin(Gpio_ps, PS_KEY1, 0);


	//输出使能 	1为使能 	0为不使能
	XGpioPs_SetOutputEnablePin(Gpio_ps, PS_LED1, 1);
}

/******************************************************************************************
 *中断服务函数
 *注意：一定要屏蔽中断和清除中断标志位！且在最后要重新打开中断使能
 *相较于STM32，PS端的中断关闭要操作两次，即屏蔽和清除
 * 关闭后要打开中断使能
 * ***************************************************************************************/
void MIO_INTR_PROCESS()
{
	printf("intr_test\n");
	flag = ~flag;
	//屏蔽中断
	XGpioPs_IntrDisablePin(&GPIO,PS_KEY1);
	//清除中断标志位
	XGpioPs_IntrClearPin(&GPIO,PS_KEY1);
	//重新打开中断使能
	XGpioPs_IntrEnablePin(&GPIO,PS_KEY1);
}

/******************************************************************************************
 * 设置中断系统
 * 功能：初始化中断控制器，关联中断处理函数，设置为上升沿触发，最后打开异常处理（打开中断）
 * 参数1：中断实例的地址
 * 参数2：GPIO实例
 * 参数3：GPIO的中断ID
 * ***************************************************************************************/
void SetupInterruptSystem(XScuGic *GicInstancePtr, XGpioPs *Gpio,u16 GpioIntrId)
{
		
//step1：根据器件ID查找配置信息，并初始化

		//查找中断配置信息，返回配置参数，初始化中断控制器
		IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
		//初始化中断控制器
		XScuGic_CfgInitialize(GicInstancePtr, IntcConfig,IntcConfig->CpuBaseAddress);

//step2：初始化中断服务函数，开启异常处理功能

		//初始化异常（中断请求也是异常的一种）
		Xil_ExceptionInit();
		//给中断请求注册一个处理函数，即生成一个中断服务函数
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XScuGic_InterruptHandler,GicInstancePtr);
		//使能处理器的异常处理功能（打开中断）
		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

//step3：关联中断处理函数

		//关联中断处理函数，第二个参数(Xil_ExceptionHandler)后的函数就是中断处理函数，可以任意改名
		XScuGic_Connect(GicInstancePtr, GpioIntrId,(Xil_ExceptionHandler)MIO_INTR_PROCESS,(void *)Gpio);

//step4：设置中断触发模式，使能中断引脚和GIC中断管理器的中断

		//设置PS_KEY1的中断为上升沿触发
		XGpioPs_SetIntrTypePin(Gpio,PS_KEY1,XGPIOPS_IRQ_TYPE_EDGE_RISING);
		//引脚中断使能
		XGpioPs_IntrEnablePin(Gpio,PS_KEY1);
		//打开GPIO设备中断（打开GIC中断管理器到GPIO的通道）
		XScuGic_Enable(GicInstancePtr, GpioIntrId);		
}

int main()
{
	printf("MIO TEST");
	Gpio_Config(&GPIO,GPIO_ID);//注：传入的第一个参数是地址
	SetupInterruptSystem(&Intc,&GPIO, GPIO_INTERRUPT_ID);
	while(1)
	{
		if(XGpioPs_ReadPin(&GPIO,PS_KEY1) == 0)
		{	//按键使用了滤波电容，滤掉了抖动		
			XGpioPs_WritePin(&GPIO,PS_LED1,flag);
		}
	}
}
