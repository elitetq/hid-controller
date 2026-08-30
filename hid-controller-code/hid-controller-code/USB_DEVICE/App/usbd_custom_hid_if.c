/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if.c
  * @version        : v2.0_Cube
  * @brief          : USB Device Custom HID interface file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_custom_hid_if.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @addtogroup USBD_CUSTOM_HID
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions USBD_CUSTOM_HID_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Defines USBD_CUSTOM_HID_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Macros USBD_CUSTOM_HID_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Variables USBD_CUSTOM_HID_Private_Variables
  * @brief Private variables.
  * @{
  */

/** Usb HID report descriptor. */
__ALIGN_BEGIN static uint8_t CUSTOM_HID_ReportDesc_FS[USBD_CUSTOM_HID_REPORT_DESC_SIZE] __ALIGN_END =
{
  /* USER CODE BEGIN 0 */
  /* Generic dual-stick gamepad. 6-byte input report:
   *   byte 0 : X   (left  stick, ADC1_IN1 / PA1)
   *   byte 1 : Y   (left  stick, ADC1_IN2 / PA2)
   *   byte 2 : Z   (right stick, ADC1_IN4 / PA4)
   *   byte 3 : Rz  (right stick, ADC1_IN5 / PA5)
   *   byte 4 : bits 0..3 hat switch, bits 4..7 padding
   *   byte 5 : buttons 1..8, bit 0 = button 1
   * X/Y + Z/Rz is the usual DirectInput pairing for two sticks, so Windows
   * and anything built on SDL treat this as an ordinary gamepad.
   */
  0x05, 0x01,        /* Usage Page (Generic Desktop)         */
  0x09, 0x05,        /* Usage (Game Pad)                     */
  0xA1, 0x01,        /* Collection (Application)             */
  0xA1, 0x00,        /*   Collection (Physical)              */
  /* ---- four 8-bit analog axes ---- */
  0x05, 0x01,        /*     Usage Page (Generic Desktop)     */
  0x09, 0x30,        /*     Usage (X)                        */
  0x09, 0x31,        /*     Usage (Y)                        */
  0x09, 0x32,        /*     Usage (Z)                        */
  0x09, 0x35,        /*     Usage (Rz)                       */
  0x15, 0x00,        /*     Logical Minimum (0)              */
  0x26, 0xFF, 0x00,  /*     Logical Maximum (255)            */
  0x35, 0x00,        /*     Physical Minimum (0)             */
  0x46, 0xFF, 0x00,  /*     Physical Maximum (255)           */
  0x75, 0x08,        /*     Report Size (8)                  */
  0x95, 0x04,        /*     Report Count (4)                 */
  0x81, 0x02,        /*     Input (Data,Var,Abs)             */
  /* ---- d-pad as a proper hat switch ---- */
  0x09, 0x39,        /*     Usage (Hat Switch)               */
  0x15, 0x00,        /*     Logical Minimum (0)              */
  0x25, 0x07,        /*     Logical Maximum (7)              */
  0x35, 0x00,        /*     Physical Minimum (0)             */
  0x46, 0x3B, 0x01,  /*     Physical Maximum (315 degrees)   */
  0x65, 0x14,        /*     Unit (Eng Rotation: Degrees)     */
  0x75, 0x04,        /*     Report Size (4)                  */
  0x95, 0x01,        /*     Report Count (1)                 */
  0x81, 0x42,        /*     Input (Data,Var,Abs,Null State)  */
  0x65, 0x00,        /*     Unit (None)                      */
  0x75, 0x04,        /*     Report Size (4)                  */
  0x95, 0x01,        /*     Report Count (1)                 */
  0x81, 0x03,        /*     Input (Const,Var,Abs) - padding  */
  /* ---- eight buttons ---- */
  0x05, 0x09,        /*     Usage Page (Button)              */
  0x19, 0x01,        /*     Usage Minimum (Button 1)         */
  0x29, 0x08,        /*     Usage Maximum (Button 8)         */
  0x15, 0x00,        /*     Logical Minimum (0)              */
  0x25, 0x01,        /*     Logical Maximum (1)              */
  0x35, 0x00,        /*     Physical Minimum (0)             */
  0x45, 0x01,        /*     Physical Maximum (1)             */
  0x75, 0x01,        /*     Report Size (1)                  */
  0x95, 0x08,        /*     Report Count (8)                 */
  0x81, 0x02,        /*     Input (Data,Var,Abs)             */
  0xC0,              /*   End Collection                     */
  /* USER CODE END 0 */
  0xC0    /*     END_COLLECTION	             */
};

/* USER CODE BEGIN PRIVATE_VARIABLES */
/* CubeMX regenerates usbd_conf.h and resets USBD_CUSTOM_HID_REPORT_DESC_SIZE to
 * its default of 2, which silently truncates the descriptor above. Fail the
 * build instead of shipping a 2-byte report descriptor. */
_Static_assert(sizeof(CUSTOM_HID_ReportDesc_FS) == 83,
               "USBD_CUSTOM_HID_REPORT_DESC_SIZE must be 83 - check usbd_conf.h");
/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
  * @brief Public variables.
  * @{
  */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_Init_FS(void);
static int8_t CUSTOM_HID_DeInit_FS(void);
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_fops_FS =
{
  CUSTOM_HID_ReportDesc_FS,
  CUSTOM_HID_Init_FS,
  CUSTOM_HID_DeInit_FS,
  CUSTOM_HID_OutEvent_FS
};

/** @defgroup USBD_CUSTOM_HID_Private_Functions USBD_CUSTOM_HID_Private_Functions
  * @brief Private functions.
  * @{
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_Init_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_DeInit_FS(void)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Manage the CUSTOM HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state)
{
  /* USER CODE BEGIN 6 */
  return (USBD_OK);
  /* USER CODE END 6 */
}

/* USER CODE BEGIN 7 */
/**
  * @brief  Send the report to the Host
  * @param  report: The report to be sent
  * @param  len: The report length
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t USBD_CUSTOM_HID_SendReport_FS(uint8_t *report, uint16_t len)
{
  return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, len);
}
/* USER CODE END 7 */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

