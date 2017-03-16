// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright 2015 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license),
please contact mla_licensing@microchip.com
*******************************************************************************/
//DOM-IGNORE-END

/*******************************************************************************
 Change History:
 Revision     Description
 v2.7         Modified the code to allow connection of USB-RS232 dongles that do
              not fully comply with CDC specifications
              Modified API USBHostCDC_Api_Send_OUT_Data to allow data transfers
              more than 256 bytes
********************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "usb_config.h"
#include "usb.h"
#include "usb_host_cdc.h"
#include "usb_host_cdc_interface.h"


// *****************************************************************************
// *****************************************************************************
// Constants
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// State Machine Constants
// *****************************************************************************
//******************************************************************************
//******************************************************************************
// Data Structures
//******************************************************************************
//******************************************************************************

//******************************************************************************
//******************************************************************************
// Section: Local Prototypes
//******************************************************************************
//******************************************************************************
//******************************************************************************
//******************************************************************************
// Macros
//******************************************************************************
//******************************************************************************


//******************************************************************************
//******************************************************************************
// Section: CDC Host Global Variables
//******************************************************************************
//******************************************************************************

//******************************************************************************
//******************************************************************************
// Section: CDC Host External Variables
//******************************************************************************
//******************************************************************************
extern USB_CDC_DEVICE_INFO                  deviceInfoCDC[] __attribute__ ((aligned));
extern uint8_t CDCdeviceAddress ;

//******************************************************************************
//******************************************************************************
// Section: CDC Host Global Variables
//******************************************************************************
//******************************************************************************


/****************************************************************************
  Function:
    bool USBHostCDC_Api_Get_IN_Data(uint8_t no_of_bytes, uint8_t* data)

  Description:
    This function is called by application to receive Input data over DATA
    interface. This function setsup the request to receive data from the device.

  Precondition:
    None

  Parameters:
    uint8_t    no_of_bytes - No. of Bytes expected from the device.
    uint8_t*   data        - Pointer to application receive data buffer.

  Return Values:
    true    -   Transfer request is placed successfully.
    false   -   Transfer request failed.

  Remarks:
    None
***************************************************************************/
bool USBHostCDC_Api_Get_IN_Data(uint8_t no_of_bytes, uint8_t* data)
{
    uint8_t    i;

    for (i=0; (i<USB_MAX_CDC_DEVICES) && (deviceInfoCDC[i].deviceAddress != CDCdeviceAddress); i++);

   if(!USBHostCDCRead_DATA(CDCdeviceAddress, deviceInfoCDC[i].dataInterface.interfaceNum,
                           no_of_bytes,data,deviceInfoCDC[i].dataInterface.endpointIN))
    {
       return true;
    }
    return false;
}


/****************************************************************************
  Function:
    bool USBHostCDC_Api_Send_OUT_Data(uint8_t no_of_bytes, uint8_t* data)

  Description:
    This function is called by application to transmit out data over DATA
    interface. This function setsup the request to transmit data to the
    device.

  Precondition:
    None

  Parameters:
    uint8_t    no_of_bytes - No. of Bytes expected from the device.
    uint8_t*   data        - Pointer to application transmit data buffer.


  Return Values:
    true    -   Transfer request is placed successfully.
    false   -   Transfer request failed.

  Remarks:
    None
***************************************************************************/
bool USBHostCDC_Api_Send_OUT_Data(uint16_t no_of_bytes, uint8_t* data)
{
    uint8_t    i;

    for (i=0; (i<USB_MAX_CDC_DEVICES) && (deviceInfoCDC[i].deviceAddress != CDCdeviceAddress); i++);

   if(!USBHostCDCSend_DATA(CDCdeviceAddress, deviceInfoCDC[i].dataInterface.interfaceNum,
                           no_of_bytes,data,deviceInfoCDC[i].dataInterface.endpointOUT))
    {
       return true;
    }
    return false;
}


/****************************************************************************
  Function:
    bool USBHostCDC_ApiTransferIsComplete(uint8_t* errorCodeDriver,uint8_t* byteCount)

  Description:
    This function is called by application to poll for transfer status. This
    function returns true in the transfer is over. To check whether the transfer
    was successfull or not , application must check the error code returned by
    reference.

  Precondition:
    None

  Parameters:
    uint8_t    *errorCodeDriver - returns.
    uint8_t    *byteCount       - No. of bytes transferred.


  Return Values:
    true    -   Transfer is has completed.
    false   -   Transfer is pending.

  Remarks:
    None
***************************************************************************/
bool USBHostCDC_ApiTransferIsComplete(uint8_t* errorCodeDriver, uint8_t* byteCount )
{
    return(USBHostCDCTransferIsComplete(CDCdeviceAddress,errorCodeDriver,byteCount));
}

/*******************************************************************************
  Function:
    bool USBHostCDC_ApiDeviceDetect( void )

  Description:
    This function determines if a CDC device is attached
    and ready to use.

  Precondition:
    None

  Parameters:
    None

  Return Values:
    true   -  CDC present and ready
    false  -  CDC not present or not ready

  Remarks:
    Since this will often be called in a loop while waiting for
    a device, we'll make sure the tasks are executed.
*******************************************************************************/
bool USBHostCDC_ApiDeviceDetect( void )
{
    USBHostTasks();
    USBHostCDCTasks();

    if ((USBHostCDCDeviceStatus(CDCdeviceAddress) == USB_CDC_NORMAL_RUNNING) &&
        (CDCdeviceAddress != 0))
    {
        return true;
    }

    return false;
}


/*******************************************************************************
  Function:
    uint8_t USBHostCDC_Api_ACM_Request(uint8_t requestType, uint8_t size, uint8_t* data)

  Description:
    This function can be used by application code to dynamically access ACM specific
    requests. This function should be used only if apllication intends to modify for
    example the Baudrate from previouly configured rate. Data transmitted/received
    to/from device is a array of bytes. Application must take extra care of understanding
    the data format before using this function.

  Precondition:
    Device must be enumerated and attached successfully.

  Parameters:
    uint8_t    requestType     - USB_CDC_SEND_ENCAPSULATED_COMMAND
                            - USB_CDC_GET_ENCAPSULATED_REQUEST
                            - USB_CDC_SET_LINE_CODING
                            - USB_CDC_SET_CONTROL_LINE_STATE
                            - USB_CDC_SET_CONTROL_LINE_STATE
                              These are the mandatory CDC request supported by the
                              CDC host stack.
    uint8_t    size            - No of bytes to be transferred.
    uint8_t    *data           - Pointer to data being transferred.

  Return Values:
    USB_SUCCESS                 - Request started successfully
    USB_CDC_DEVICE_NOT_FOUND    - No device with specified address
    USB_CDC_DEVICE_BUSY         - Device not in proper state for
                                  performing a transfer
    USB_CDC_COMMAND_FAILED      - Request is not supported.
    USB_CDC_ILLEGAL_REQUEST     - Requested ID is invalid.

  Remarks:
    None
 *******************************************************************************/
uint8_t USBHostCDC_Api_ACM_Request(uint8_t requestType, uint8_t size, uint8_t* data)
{
    uint8_t    i;
    uint8_t    return_val = USB_CDC_COMMAND_FAILED;

    // Find the correct device.
    for (i=0; (i<USB_MAX_CDC_DEVICES) && (deviceInfoCDC[i].deviceAddress != CDCdeviceAddress); i++);
    if (i == USB_MAX_CDC_DEVICES)
    {
        return USB_CDC_DEVICE_NOT_FOUND;
    }

    switch(requestType)
    {
        case USB_CDC_SEND_ENCAPSULATED_COMMAND :
                               return_val = USBHostCDCTransfer(CDCdeviceAddress, USB_CDC_SEND_ENCAPSULATED_COMMAND,0,
                                                       deviceInfoCDC[i].commInterface.interfaceNum,size,data,
                                                       0);
            break;
        case USB_CDC_GET_ENCAPSULATED_REQUEST :
                               return_val = USBHostCDCTransfer(CDCdeviceAddress,  USB_CDC_GET_ENCAPSULATED_REQUEST,1,
                                                       deviceInfoCDC[i].commInterface.interfaceNum,size,data,
                                                       0);
            break;

        case USB_CDC_SET_COMM_FEATURE :
            break;

        case USB_CDC_GET_COMM_FEATURE :
            break;

        case USB_CDC_SET_LINE_CODING :
                               return_val = USBHostCDCTransfer(CDCdeviceAddress,  USB_CDC_SET_LINE_CODING,0,
                                                       deviceInfoCDC[i].commInterface.interfaceNum,size,data,
                                                       0);
            break;

        case USB_CDC_GET_LINE_CODING :
                               return_val = USBHostCDCTransfer(CDCdeviceAddress,  USB_CDC_GET_LINE_CODING,1,
                                                       deviceInfoCDC[i].commInterface.interfaceNum,size,data,
                                                       0);
            break;

        case USB_CDC_SET_CONTROL_LINE_STATE :
                               return_val = USBHostCDCTransfer(CDCdeviceAddress,  USB_CDC_SET_CONTROL_LINE_STATE,0,
                                                       deviceInfoCDC[i].commInterface.interfaceNum,size,data,
                                                       0);
            break;

        case USB_CDC_SEND_BREAK :
            break;
        default:
                 return USB_CDC_ILLEGAL_REQUEST;
            break;
    }
    return return_val;
}
