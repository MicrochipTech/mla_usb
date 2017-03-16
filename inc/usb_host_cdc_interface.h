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

#ifndef _USB_HOST_CDC_INTERFACE_H_
#define _USB_HOST_CDC_INTERFACE_H_

// *****************************************************************************
// *****************************************************************************
// Section: Constants
// *****************************************************************************
// *****************************************************************************

//******************************************************************************
//******************************************************************************
// Data Structures
//******************************************************************************
//******************************************************************************

// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************

/****************************************************************************
  Function:
    bool USBHostCDC_Api_Get_IN_Data(uint8_t no_of_bytes, uint8_t* data)

  Description:
    This function is called by application to receive Input data over DATA
    interface. This function setsup the request to receive data from the device.

  Precondition:
    None

  Parameters:
    uint8_t    no_of_bytes - Number of Bytes expected from the device.
    uint8_t*   data        - Pointer to application receive data buffer.

  Return Values:
    TRUE    -   Transfer request is placed successfully.
    FALSE   -   Transfer request failed.

  Remarks:
    None
***************************************************************************/
bool USBHostCDC_Api_Get_IN_Data(uint8_t no_of_bytes, uint8_t* data);

/****************************************************************************
  Function:
    bool USBHostCDC_Api_Send_OUT_Data(uint16_t no_of_bytes, uint8_t* data)

  Description:
    This function is called by application to transmit out data over DATA
    interface. This function setsup the request to transmit data to the
    device.

  Precondition:
    None

  Parameters:
    uint8_t    no_of_bytes - Number of Bytes expected from the device.
    uint8_t*   data        - Pointer to application transmit data buffer.


  Return Values:
    TRUE    -   Transfer request is placed successfully.
    FALSE   -   Transfer request failed.

  Remarks:
    None
***************************************************************************/
bool USBHostCDC_Api_Send_OUT_Data(uint16_t no_of_bytes, uint8_t* data);

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
    uint8_t    *byteCount       - Number of bytes transferred.


  Return Values:
    TRUE    -   Transfer is has completed.
    FALSE   -   Transfer is pending.

  Remarks:
    None
***************************************************************************/
bool USBHostCDC_ApiTransferIsComplete(uint8_t* errorCodeDriver, uint8_t* byteCount );

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
    TRUE   -  CDC present and ready
    FALSE  -  CDC not present or not ready

  Remarks:
    Since this will often be called in a loop while waiting for
    a device, we'll make sure the tasks are executed.
*******************************************************************************/
bool USBHostCDC_ApiDeviceDetect( void );


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
    uint8_t    requestType     These are the mandatory CDC request supported by the
                            CDC host stack.
                            - USB_CDC_SEND_ENCAPSULATED_COMMAND
                            - USB_CDC_GET_ENCAPSULATED_REQUEST
                            - USB_CDC_SET_LINE_CODING
                            - USB_CDC_SET_CONTROL_LINE_STATE
                            - USB_CDC_SET_CONTROL_LINE_STATE
    uint8_t    size            - Number bytes to be transferred.
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
uint8_t USBHostCDC_Api_ACM_Request(uint8_t requestType, uint8_t size, uint8_t* data);

#endif /* _USB_HOST_CDC_INTERFACE_H_ */
