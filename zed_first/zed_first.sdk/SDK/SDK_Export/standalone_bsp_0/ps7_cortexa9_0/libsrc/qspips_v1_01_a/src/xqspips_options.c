/* $Id: xqspips_options.c,v 1.1.2.2 2011/03/09 10:41:30 sadanan Exp $ */
/******************************************************************************
*
* (c) Copyright 2010 Xilinx, Inc. All rights reserved.
*
* This file contains confidential and proprietary information of Xilinx, Inc.
* and is protected under U.S. and international copyright and other
* intellectual property laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any rights to the
* materials distributed herewith. Except as otherwise provided in a valid
* license issued to you by Xilinx, and to the maximum extent permitted by
* applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
* FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
* IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
* MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
* and (2) Xilinx shall not be liable (whether in contract or tort, including
* negligence, or under any other theory of liability) for any loss or damage
* of any kind or nature related to, arising under or in connection with these
* materials, including for any direct, or any indirect, special, incidental,
* or consequential loss or damage (including loss of data, profits, goodwill,
* or any type of loss or damage suffered as a result of any action brought by
* a third party) even if such damage or loss was reasonably foreseeable or
* Xilinx had been advised of the possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-safe, or for use in
* any application requiring fail-safe performance, such as life-support or
* safety devices or systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any other applications
* that could lead to death, personal injury, or severe property or
* environmental damage (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and liability of any use of
* Xilinx products in Critical Applications, subject only to applicable laws
* and regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
* AT ALL TIMES.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xqspips_options.c
*
* Contains functions for the configuration of the XQspiPs driver component.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00  sdm 11/25/10 First release
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xqspips.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/*
 * Create the table of options which are processed to get/set the device
 * options. These options are table driven to allow easy maintenance and
 * expansion of the options.
 */
typedef struct {
	u32 Option;
	u32 Mask;
} OptionsMap;

static OptionsMap OptionsTable[] = {
	{XQSPIPS_MASTER_OPTION, XQSPIPS_CR_MSTREN_MASK},
	{XQSPIPS_CLK_ACTIVE_LOW_OPTION, XQSPIPS_CR_CPOL_MASK},
	{XQSPIPS_CLK_PHASE_1_OPTION, XQSPIPS_CR_CPHA_MASK},
	{XQSPIPS_DECODE_SSELECT_OPTION, XQSPIPS_CR_SSDECEN_MASK},
	{XQSPIPS_FORCE_SSELECT_OPTION, XQSPIPS_CR_SSFORCE_MASK},
	{XQSPIPS_MANUAL_START_OPTION, XQSPIPS_CR_MANSTRTEN_MASK},
	{XQSPIPS_FLASH_MODE_OPTION, XQSPIPS_CR_IFMODE_MASK},
};

#define XQSPIPS_NUM_OPTIONS	(sizeof(OptionsTable) / sizeof(OptionsMap))

/*****************************************************************************/
/**
*
* This function sets the options for the QSPI device driver. The options control
* how the device behaves relative to the QSPI bus. The device must be idle
* rather than busy transferring data before setting these device options.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
* @param	Options contains the specified options to be set. This is a bit
*		mask where a 1 means to turn the option on, and a 0 means to
*		turn the option off. One or more bit values may be contained in
*		the mask. See the bit definitions named XQSPIPS_*_OPTIONS in
*		the file xqspips.h.
*
* @return
*		- XST_SUCCESS if options are successfully set.
*		- XST_DEVICE_BUSY if the device is currently transferring data.
*		The transfer must complete or be aborted before setting options.
*
* @note
* This function is not thread-safe.
*
******************************************************************************/
int XQspiPs_SetOptions(XQspiPs *InstancePtr, u32 Options)
{
	u32 ControlReg;
	unsigned int Index;
	u32 QspiOptions;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Do not allow to modify the Control Register while a transfer is in
	 * progress. Not thread-safe.
	 */
	if (InstancePtr->IsBusy) {
		return XST_DEVICE_BUSY;
	}

	QspiOptions = Options & XQSPIPS_LQSPI_MODE_OPTION;
	Options &= ~XQSPIPS_LQSPI_MODE_OPTION;

	ControlReg = XQspiPs_ReadReg(InstancePtr->Config.BaseAddress,
				      XQSPIPS_CR_OFFSET);
	/*
	 * Loop through the options table, turning the option on or off
	 * depending on whether the bit is set in the incoming options flag.
	 */
	for (Index = 0; Index < XQSPIPS_NUM_OPTIONS; Index++) {
		if (Options & OptionsTable[Index].Option) {
			ControlReg |= OptionsTable[Index].Mask; /* turn it on */
		}
		else {
			/* Turn it off */
			ControlReg &= ~(OptionsTable[Index].Mask);
		}
	}

	/*
	 * Now write the control register. Leave it to the upper layers
	 * to restart the device.
	 */
	XQspiPs_WriteReg(InstancePtr->Config.BaseAddress,
			  XQSPIPS_CR_OFFSET, ControlReg);

	/*
	 * Check for the LQSPI configuration options.
	 */
	ControlReg = XQspiPs_ReadReg(InstancePtr->Config.BaseAddress,
				      XQSPIPS_LQSPI_CR_OFFSET);

	/*
	 * If LQSPI enable option is set, reset the QSPI controller and LQSPI
	 * controller and enable LQSPI.
	 */
	if (QspiOptions & XQSPIPS_LQSPI_MODE_OPTION) {
		XQspiPs_Reset(InstancePtr);
		XQspiPs_WriteReg(InstancePtr->Config.BaseAddress,
				  XQSPIPS_LQSPI_CR_OFFSET,
				  XQSPIPS_LQSPI_CR_RST_STATE);
		XQspiPs_SetSlaveSelect(InstancePtr, InstancePtr->SlaveSelect);
		XQspiPs_Enable(InstancePtr->Config.BaseAddress);
	} else {
		ControlReg &= ~XQSPIPS_LQSPI_CR_LINEAR_MASK;
		XQspiPs_WriteReg(InstancePtr->Config.BaseAddress,
				  XQSPIPS_LQSPI_CR_OFFSET, ControlReg);
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function gets the options for the QSPI device. The options control how
* the device behaves relative to the QSPI bus.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
*
* @return
*
* Options contains the specified options currently set. This is a bit value
* where a 1 means the option is on, and a 0 means the option is off.
* See the bit definitions named XQSPIPS_*_OPTIONS in file xqspips.h.
*
* @note		None.
*
******************************************************************************/
u32 XQspiPs_GetOptions(XQspiPs *InstancePtr)
{
	u32 OptionsFlag = 0;
	u32 ControlReg;
	unsigned int Index;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Get the current options from QSPI control register.
	 */
	ControlReg = XQspiPs_ReadReg(InstancePtr->Config.BaseAddress,
				      XQSPIPS_CR_OFFSET);

	/*
	 * Loop through the options table to grab options
	 */
	for (Index = 0; Index < XQSPIPS_NUM_OPTIONS; Index++) {
		if (ControlReg & OptionsTable[Index].Mask) {
			OptionsFlag |= OptionsTable[Index].Option;
		}
	}

	/*
	 * Check for the LQSPI configuration options.
	 */
	ControlReg = XQspiPs_ReadReg(InstancePtr->Config.BaseAddress,
				      XQSPIPS_LQSPI_CR_OFFSET);

	if ((ControlReg & XQSPIPS_LQSPI_CR_LINEAR_MASK) != 0) {
		OptionsFlag |= XQSPIPS_LQSPI_MODE_OPTION;
	}

	return OptionsFlag;
}

/*****************************************************************************/
/**
*
* This function sets the clock prescaler for an QSPI device. The device
* must be idle rather than busy transferring data before setting these device
* options.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
* @param	Prescaler is the value that determine how much the clock should
*		be divided by. Use the XQSPIPS_CLK_PRESCALE_* constants defined
*		in xqspips.h for this setting.
*
* @return
*		- XST_SUCCESS if options are successfully set.
*		- XST_DEVICE_BUSY if the device is currently transferring data.
*		The transfer must complete or be aborted before setting options.
*
* @note
* This function is not thread-safe.
*
******************************************************************************/
int XQspiPs_SetClkPrescaler(XQspiPs *InstancePtr, u8 Prescaler)
{
	u32 ControlReg;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertNonvoid(Prescaler <= XQSPIPS_CR_PRESC_MAXIMUM);

	/*
	 * Do not allow the slave select to change while a transfer is in
	 * progress. Not thread-safe.
	 */
	if (InstancePtr->IsBusy) {
		return XST_DEVICE_BUSY;
	}

	/*
	 * Read the control register, mask out the interesting bits, and set
	 * them with the shifted value passed into the function. Write the
	 * results back to the control register.
	 */
	ControlReg = XQspiPs_ReadReg(InstancePtr->Config.BaseAddress,
				      XQSPIPS_CR_OFFSET);

	ControlReg &= ~XQSPIPS_CR_PRESC_MASK;
	ControlReg |= (u32) (Prescaler & XQSPIPS_CR_PRESC_MAXIMUM) <<
			    XQSPIPS_CR_PRESC_SHIFT;

	XQspiPs_WriteReg(InstancePtr->Config.BaseAddress,
			  XQSPIPS_CR_OFFSET,
			  ControlReg);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function gets the clock prescaler of an QSPI device.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
*
* @return	The prescaler value.
*
* @note		None.
*
*
******************************************************************************/
u8 XQspiPs_GetClkPrescaler(XQspiPs *InstancePtr)
{
	u32 ControlReg;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	ControlReg = XQspiPs_ReadReg(InstancePtr->Config.BaseAddress,
				      XQSPIPS_CR_OFFSET);

	ControlReg &= XQSPIPS_CR_PRESC_MASK;

	return (u8)(ControlReg >> XQSPIPS_CR_PRESC_SHIFT);
}

/*****************************************************************************/
/**
*
* This function sets the delay register for the QSPI device driver.
* The delay register controls the Delay Between Transfers, Delay After
* Transfers, and the Delay Initially. The default value is 0x0. The range of
* each delay value is 0-255.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
* @param	DelayBtwn is the delay between one Slave Select being
*		de-activated and the activation of another slave. The dealy is
*		the number of master clock periods given by DelayBtwn + 2.
* @param	DelayAfter define the delay between the last bit of the current
*		byte transfer and the first bit of the next byte transfer.
*		The delay in number of master clock periods is given as:
*		CHPA=0:DelayInit+DelayAfter+3
*		CHPA=1:DelayAfter+1
* @param	DelayInit is the delay between asserting the slave select signal
*		and the first bit transfer. The delay int number of master clock
*		periods is DelayInit+1.
*
* @return
*		- XST_SUCCESS if delays are successfully set.
*		- XST_DEVICE_BUSY if the device is currently transferring data.
*		The transfer must complete or be aborted before setting options.
*
* @note		None.
*
******************************************************************************/
int XQspiPs_SetDelays(XQspiPs *InstancePtr, u8 DelayBtwn, u8 DelayAfter,
		       u8 DelayInit)
{
	u32 DelayRegister;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Do not allow the delays to change while a transfer is in
	 * progress. Not thread-safe.
	 */
	if (InstancePtr->IsBusy) {
		return XST_DEVICE_BUSY;
	}

	/* Shift, Mask and OR the values to build the register settings */
	DelayRegister = (u32) DelayBtwn << XQSPIPS_DR_BTWN_SHIFT;
	DelayRegister |= (u32) DelayAfter << XQSPIPS_DR_AFTER_SHIFT;
	DelayRegister |= (u32) DelayInit;

	XQspiPs_WriteReg(InstancePtr->Config.BaseAddress,
			  XQSPIPS_DR_OFFSET, DelayRegister);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function gets the delay settings for an QSPI device.
* The delay register controls the Delay Between Transfers, Delay After
* Transfers, and the Delay Initially. The default value is 0x0.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
* @param	DelayBtwn is a pointer to the Delay Between transfers value.
*		This is a return parameter.
* @param	DelayAfter is a pointer to the Delay After transfer value.
*		This is a return parameter.
* @param	DelayInit is a pointer to the Delay Initially value. This is
*		a return parameter.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XQspiPs_GetDelays(XQspiPs *InstancePtr, u8 *DelayBtwn, u8 *DelayAfter,
			u8 *DelayInit)
{
	u32 DelayRegister;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	DelayRegister = XQspiPs_ReadReg(InstancePtr->Config.BaseAddress,
					 XQSPIPS_DR_OFFSET);

	*DelayInit = (u8)(DelayRegister & XQSPIPS_DR_INIT_MASK);

	*DelayAfter = (u8)((DelayRegister & XQSPIPS_DR_AFTER_MASK) >>
			   XQSPIPS_DR_AFTER_SHIFT);

	*DelayBtwn = (u8)((DelayRegister & XQSPIPS_DR_BTWN_MASK) >>
			  XQSPIPS_DR_BTWN_SHIFT);
}