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


#include <stdlib.h>
#include <string.h>
#include "usb_config.h"
#include "usb.h"
#include "usb_host_hid.h"
#include "usb_host_hid_parser.h"



// *****************************************************************************
// *****************************************************************************
// Configuration
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Max Number of Supported Devices

This value represents the maximum number of attached devices this class driver
can support.  If the user does not define a value, it will be set to 1.
Currently this must be set to 1, due to limitations in the USB Host layer.
*/
#ifndef USB_MAX_HID_DEVICES
    #define USB_MAX_HID_DEVICES        1
#endif

// *****************************************************************************
// *****************************************************************************
// Constants
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// State Machine Constants
// *****************************************************************************

//------------------------------------------------------------------------------
// Constants for the overall HID device state machine

#define STATE_DETACHED                      0x0000 //

#define STATE_INITIALIZE_DEVICE             0x0001 //
#define STATE_WAIT_FOR_REPORT_DSC           0x0002 //
#define STATE_PARSING_COMPLETE              0x0003 //

#define STATE_RUNNING                       0x0004 //

#define STATE_WAIT_FOR_RESET                0x0007 //
#define STATE_RESET_COMPLETE                0x0008 //

#define STATE_HOLDING                       0x0009 //

//------------------------------------------------------------------------------
// Constants for the the individual HID request (read or write)

#define STATE_TRANSFER_WAITING                  0
#define STATE_TRANSFER_REQUESTED                1
#define STATE_TRANSFER_COMPLETE                 2

// *****************************************************************************
// Other Constants
// *****************************************************************************

#define USB_HID_RESET           (0xFF)  // Device Request code to reset the device.
#define MARK_RESET_RECOVERY     (0x0E)  // Maintain with USB_MSD_DEVICE_INFO

/* Class-Specific Requests */
#define USB_HID_GET_REPORT      (0x01)  //
#define USB_HID_GET_IDLE        (0x02)  //
#define USB_HID_GET_PROTOCOL    (0x03)  //
#define USB_HID_SET_REPORT      (0x09)  //
#define USB_HID_SET_IDLE        (0x0A)  //
#define USB_HID_SET_PROTOCOL    (0x0B)  //

#define USB_HID_INPUT_REPORT    (0x01)  //
#define USB_HID_OUTPUT_REPORT   (0x02)  //
#define USB_HID_FEATURE_REPORT  (0x03)  //


//******************************************************************************
//******************************************************************************
// Data Structures
//******************************************************************************

//------------------------------------------------------------------------------
/*  USB HID Interface Information

   This structure is used to hold interface specific information.
*/
typedef struct _USB_HID_INTERFACE_DETAILS
{
    struct _USB_HID_INTERFACE_DETAILS   *next;                // Pointer to next interface in the list.
    uint16_t                            sizeOfRptDescriptor;  // Size of report descriptor of a particular interface.
    uint16_t                            endpointMaxDataSize;  // Max data size for a interface.
    uint8_t                             endpointIN;           // HID IN endpoint for corresponding interface.
    uint8_t                             endpointOUT;          // HID OUT endpoint for corresponding interface.
    uint8_t                             interfaceNumber;      // Interface number.
    uint8_t                             endpointPollInterval; // Polling rate of corresponding interface.
}   USB_HID_INTERFACE_DETAILS;

//------------------------------------------------------------------------------
/*  USB HID Transfer Information

   This structure is used to hold information about a current read or write transfer.
*/
typedef struct _USB_HID_TRANSFER_INFO
{
    uint8_t                             state;                 // State of the endpoint.
    uint16_t                            reportId;              // Report ID of the current transfer.
    uint8_t                             *userData;              // Data pointer to application buffer.
    uint8_t                             bytesTransferred;      // Number of bytes transferred to/from the user's data buffer.
    uint8_t                             endpoint;              // Endpoint to use for the transfer.
    uint8_t                             reportSize;            // Size of report currently requested for transfer.
    uint8_t                             interface;             // Interface number of current transfer.
} USB_HID_TRANSFER_INFO;


//------------------------------------------------------------------------------
/*  USB HID Device Information

   This structure is used to hold information about the entire device.
*/
typedef struct _USB_HID_DEVICE_INFO
{
    USB_HID_DEVICE_ID                   ID;                    // Information about the device.
    uint16_t                            reportId;              // Report ID of the current transfer.
    uint8_t*                            userData;              // Data pointer to application buffer.
    uint8_t*                            rptDescriptor;         // Common pointer to report descriptor for all the interfaces.
    USB_HID_RPT_DESC_ERROR              HIDparserError;        // Error code incase report descriptor is not in proper format.
    union
    {
        struct
        {
            uint8_t                     bfDirection           : 1;   // Direction of current transfer (0=OUT, 1=IN).
            uint8_t                     bfReset               : 1;   // Flag indicating to perform HID Reset.
            uint8_t                     bfClearDataIN         : 1;   // Flag indicating to clear the IN endpoint.
            uint8_t                     bfClearDataOUT        : 1;   // Flag indicating to clear the OUT endpoint.
            uint8_t                     bfReportDataCollected : 1;   // Flag indicating report data is collected
            uint8_t                     bfAddressReported     : 1;   // Flag indicating if the device address has been reported to the user yet.
        };
        uint8_t                         val;
    }                                   flags;
    uint8_t                             driverSupported;        // If HID driver supports requested Class,Subclass & Protocol.
    uint8_t                             errorCode;              // Error code of last error.
    uint8_t                             state;                  // State machine state of the device.
    uint8_t                             returnState;            // State to return to after performing error handling.
    uint8_t                             noOfInterfaces;         // Total number of interfaces in the device.

    USB_HID_TRANSFER_INFO               transferIN;             // IN transfer information
    USB_HID_TRANSFER_INFO               transferOUT;            // OUT transfer information
} USB_HID_DEVICE_INFO;


//******************************************************************************
//******************************************************************************
// Section: Local Prototypes
//******************************************************************************
//******************************************************************************

static void _USBHostHID_FreeRptDecriptorDataMem(uint8_t deviceAddress);
static void _USBHostHID_ResetStateJump( uint8_t i );


//******************************************************************************
//******************************************************************************
// Macros
//******************************************************************************
//******************************************************************************

#ifndef USB_MALLOC
    #define USB_MALLOC(size) malloc(size)
#endif

#ifndef USB_FREE
    #define USB_FREE(ptr) free(ptr)
#endif

#define USB_FREE_AND_CLEAR(ptr) {USB_FREE(ptr); ptr = NULL;}

#define _USBHostHID_LockDevice(x)                   {                                                   \
                                                        deviceInfoHID[i].errorCode  = x;                \
                                                        deviceInfoHID[i].state      = STATE_HOLDING;    \
                                                    }

#ifdef USB_HID_ENABLE_TRANSFER_EVENT
    #define _USBHostHID_TerminateReadTransfer( error )  {                                                                                   \
                                                            deviceInfoHID[i].errorCode          = error;                                    \
                                                            deviceInfoHID[i].state              = STATE_RUNNING;                            \
                                                            deviceInfoHID[i].transferIN.state   = STATE_TRANSFER_WAITING;                   \
                                                            USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress,                  \
                                                                    EVENT_HID_READ_DONE, &transferEventData, sizeof(HID_TRANSFER_DATA) );   \
                                                        }
    #define _USBHostHID_TerminateWriteTransfer( error ) {                                                                                   \
                                                            deviceInfoHID[i].errorCode          = error;                                    \
                                                            deviceInfoHID[i].state              = STATE_RUNNING;                            \
                                                            deviceInfoHID[i].transferOUT.state  = STATE_TRANSFER_WAITING;                   \
                                                            USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress,                  \
                                                                    EVENT_HID_WRITE_DONE, &transferEventData, sizeof(HID_TRANSFER_DATA) );  \
                                                        }
#else
    #define _USBHostHID_TerminateReadTransfer( error )  {                                                                   \
                                                            deviceInfoHID[i].errorCode          = error;                    \
                                                            deviceInfoHID[i].state              = STATE_RUNNING;            \
                                                            deviceInfoHID[i].transferIN.state   = STATE_TRANSFER_WAITING;   \
                                                        }
    #define _USBHostHID_TerminateWriteTransfer( error ) {                                                                   \
                                                            deviceInfoHID[i].errorCode          = error;                    \
                                                            deviceInfoHID[i].state              = STATE_RUNNING;            \
                                                            deviceInfoHID[i].transferOUT.state  = STATE_TRANSFER_WAITING;   \
                                                        }
#endif

//******************************************************************************
//******************************************************************************
// Section: HID Host Global Variables
//******************************************************************************
//******************************************************************************

static USB_HID_DEVICE_INFO          deviceInfoHID[USB_MAX_HID_DEVICES] __attribute__ ((aligned));
static USB_HID_INTERFACE_DETAILS*   pInterfaceDetails = NULL;
static USB_HID_INTERFACE_DETAILS*   pCurrInterfaceDetails = NULL;

#ifdef USB_HID_ENABLE_TRANSFER_EVENT
    static HID_TRANSFER_DATA            transferEventData;
#endif

//******************************************************************************
//******************************************************************************
// Section: HID Host External Variables
//******************************************************************************
//******************************************************************************

extern uint8_t* parsedDataMem;

extern USB_HID_RPT_DESC_ERROR _USBHostHID_Parse_Report(uint8_t*, uint16_t, uint16_t, uint8_t);

// *****************************************************************************
// *****************************************************************************
// Application Callable Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    bool USBHostHIDDeviceDetect( uint8_t deviceAddress )

  Description:
    This function determines if a HID device is attached and ready to use.

  Precondition:
    None

  Parameters:
    none

  Return Values:
    uint8_t deviceAddress  - Address of the newly attached device or 0 if there
 are no newly attached devices.

  Remarks:
    This function replaces the USBHostHID_ApiDeviceDetect() function.
*******************************************************************************/
uint8_t USBHostHIDDeviceDetect( void )
{
    uint8_t    i;
    
    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].flags.bfAddressReported == 1); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return false;
    }
    
    if ((USBHostHIDDeviceStatus(deviceInfoHID[i].ID.deviceAddress) == USB_HID_NORMAL_RUNNING) &&
        (deviceInfoHID[i].ID.deviceAddress != 0))
    {
        deviceInfoHID[i].flags.bfAddressReported = 1;
        return deviceInfoHID[i].ID.deviceAddress;
    }
    
    return 0;
}


/*******************************************************************************
  Function:
    uint8_t    USBHostHIDDeviceStatus( uint8_t deviceAddress )

  Summary:

  Description:
    This function determines the status of a HID device.

  Preconditions:  None

  Parameters:
    uint8_t deviceAddress - address of device to query

  Return Values:
    USB_HID_DEVICE_NOT_FOUND           -  Illegal device address, or the
                                          device is not an HID
    USB_HID_INITIALIZING               -  HID is attached and in the
                                          process of initializing
    USB_PROCESSING_REPORT_DESCRIPTOR   -  HID device is detected and report 
                                          descriptor is being parsed
    USB_HID_NORMAL_RUNNING             -  HID Device is running normal, 
                                          ready to send and receive reports 
    USB_HID_DEVICE_HOLDING             -  Driver has encountered error and
                                          could not recover
    USB_HID_DEVICE_DETACHED            -  HID detached.

  Remarks:
    None
*******************************************************************************/
uint8_t    USBHostHIDDeviceStatus( uint8_t deviceAddress )
{
    uint8_t    i;
    uint8_t    status;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    status = USBHostDeviceStatus( deviceAddress );
    if (status != USB_DEVICE_ATTACHED)
    {
        return status;
    }
    else
    {
        // The device is attached and done enumerating.  We can get more specific now.
        switch (deviceInfoHID[i].state)
        {
            case STATE_INITIALIZE_DEVICE:
            case STATE_PARSING_COMPLETE:
                return USB_HID_INITIALIZING;
                break;

            case STATE_RUNNING:
                return USB_HID_NORMAL_RUNNING;
                break;

            case STATE_HOLDING:
                return USB_HID_DEVICE_HOLDING;
                break;

            case STATE_WAIT_FOR_RESET:
            case STATE_RESET_COMPLETE:
                return USB_HID_RESETTING_DEVICE;
                break;

            default:
                return USB_HID_DEVICE_DETACHED;
                break;
        }
    }
}

/*******************************************************************************
  Function:
    uint8_t USBHostHIDRead( uint8_t deviceAddress,uint8_t reportid, uint8_t interface,
                uint8_t size, uint8_t *data)

  Summary:
     This function starts a Get report transfer request from the device.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress      - Device address
    uint8_t reportid           - Report ID of the requested report
    uint8_t interface          - Interface number
    uint8_t size               - Byte size of the data buffer
    uint8_t *data              - Pointer to the data buffer

  Return Values:
    USB_SUCCESS                 - Request started successfully
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    USB_HID_DEVICE_BUSY         - Device not in proper state for
                                  performing a transfer
    Others                      - Return values from USBHostRead()

  Remarks:
    None
*******************************************************************************/
uint8_t USBHostHIDRead( uint8_t deviceAddress, uint8_t reportid, uint8_t interface,
                uint8_t size, uint8_t *data )
{
    uint8_t    i;
    uint8_t    errorCode;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    pCurrInterfaceDetails = pInterfaceDetails;
    while((pCurrInterfaceDetails != NULL) && (pCurrInterfaceDetails->interfaceNumber != interface))
    {
        pCurrInterfaceDetails = pCurrInterfaceDetails->next;
    }
    // Make sure the device is in a state ready to read/write.
    if ( ! ( ( deviceInfoHID[i].state == STATE_RUNNING ) &&
             ( deviceInfoHID[i].transferIN.state == STATE_TRANSFER_WAITING ) ) )
    {
        return USB_HID_DEVICE_BUSY;
    }

    // Initialize the transfer information.
    deviceInfoHID[i].transferIN.bytesTransferred  = 0;
    deviceInfoHID[i].transferIN.userData          = data;
    deviceInfoHID[i].transferIN.endpoint          = pCurrInterfaceDetails->endpointIN;
    deviceInfoHID[i].transferIN.reportSize        = size;
    deviceInfoHID[i].transferIN.reportId          = reportid;
    deviceInfoHID[i].transferIN.interface         = interface;
    deviceInfoHID[i].errorCode                    = USB_SUCCESS;

    errorCode = USBHostRead( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].transferIN.endpoint,
                             deviceInfoHID[i].transferIN.userData, deviceInfoHID[i].transferIN.reportSize );
    if (!errorCode)
    {
        deviceInfoHID[i].transferIN.state  = STATE_TRANSFER_REQUESTED;
    }
    else
    {
        deviceInfoHID[i].errorCode          = errorCode;
    }

    return errorCode;
}

/*******************************************************************************
  Function:
    bool USBHostHIDReadIsComplete( uint8_t deviceAddress, uint8_t *errorCode, uint32_t *byteCount )

  Summary:
    This function indicates whether or not the last read request is complete.

  Description:
    This function indicates whether or not the last read request is complete.
    If the functions returns true, the returned byte count and error
    code are valid. Since only one read request can be performed at once
    and only one endpoint can be used, we only need to know the
    device address.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress  - Device address
    uint8_t *errorCode     - Error code from last transfer
    uint32_t *byteCount    - Number of bytes transferred

  Return Values:
    true    - Transfer is complete, errorCode and byteCount are valid
    false   - Transfer is not complete, errorCode and byteCount are not valid
*******************************************************************************/
bool USBHostHIDReadIsComplete ( uint8_t deviceAddress, uint8_t *errorCode, uint8_t *byteCount )
{
    uint8_t    i;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if ((i == USB_MAX_HID_DEVICES) || (deviceInfoHID[i].state == STATE_DETACHED))
    {
        *errorCode = USB_HID_DEVICE_NOT_FOUND;
        *byteCount = 0;
        return true;
    }

    *errorCode = deviceInfoHID[i].errorCode;
    *byteCount = deviceInfoHID[i].transferIN.bytesTransferred;

    if(deviceInfoHID[i].transferIN.state == STATE_TRANSFER_WAITING)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*******************************************************************************
  Function:
     uint8_t USBHostHIDReadTerminate( uint8_t deviceAddress, uint8_t interfaceNum )

  Summary:
    This function terminates a read request that is in progress.

  Description:
    This function terminates a read request that is in progress.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress  - Device address
    uint8_t interfaceNum   - Interface number
                            
  Return Values:
    USB_SUCCESS                 - read request terminated
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    
  Remarks:
    None
*******************************************************************************/
uint8_t USBHostHIDReadTerminate ( uint8_t deviceAddress, uint8_t interfaceNum )
{
    uint8_t    i;
    uint8_t    endpoint;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    pCurrInterfaceDetails = pInterfaceDetails;
    while((pCurrInterfaceDetails != NULL) && (pCurrInterfaceDetails->interfaceNumber != interfaceNum))
    {
        pCurrInterfaceDetails = pCurrInterfaceDetails->next;
    }

    endpoint = pCurrInterfaceDetails->endpointIN;

    USBHostTerminateTransfer( deviceAddress, endpoint );

    deviceInfoHID[i].transferIN.state = STATE_TRANSFER_WAITING;

    return USB_SUCCESS;
}

/*******************************************************************************
  Function:
    uint8_t USBHostHIDResetDevice( uint8_t deviceAddress )

  Summary:
    This function starts a HID  reset.

  Description:
    This function starts a HID reset.  A reset can be
    issued only if the device is attached and not being initialized.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress - Device address

  Return Values:
    USB_SUCCESS                 - Reset started
    USB_MSD_DEVICE_NOT_FOUND    - No device with specified address
    USB_MSD_ILLEGAL_REQUEST     - Device is in an illegal state for reset

  Remarks:
    None
*******************************************************************************/
uint8_t USBHostHIDResetDevice( uint8_t deviceAddress )
{
    uint8_t    i;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    if ((deviceInfoHID[i].state != STATE_DETACHED) &&
        (deviceInfoHID[i].state != STATE_INITIALIZE_DEVICE))
    {
        deviceInfoHID[i].flags.val |= MARK_RESET_RECOVERY;
        deviceInfoHID[i].flags.bfReset = 1;
        deviceInfoHID[i].returnState = STATE_RUNNING;

        _USBHostHID_ResetStateJump( i );
        return USB_SUCCESS;
    }
    
    return USB_HID_ILLEGAL_REQUEST;
}


/****************************************************************************
  Function:
    bool USBHostHIDResetDeviceWithWait( uint8_t deviceAddress  )

  Description:
    This function resets a HID device, and waits until the reset is complete.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress  - Address of the device to reset.

  Return Values:
    USB_SUCCESS                 - Reset successful
    USB_HID_RESET_ERROR         - Error while resetting device
    Others                      - See return values for USBHostHIDResetDevice()
                                    and error codes that can be returned
                                    in the errorCode parameter of
                                    USBHostHIDWriteIsComplete();
                                    
  Remarks:
    None
  ***************************************************************************/

uint8_t USBHostHIDResetDeviceWithWait( uint8_t deviceAddress  )
{
    uint8_t    byteCount;
    uint8_t    errorCode;

    errorCode = USBHostHIDResetDevice( deviceAddress );
    if (errorCode)
    {
        return errorCode;
    }

    do
    {
        USBHostTasks();
        errorCode = USBHostHIDDeviceStatus( deviceAddress );
    } while (errorCode == USB_HID_RESETTING_DEVICE);


    if (USBHostHIDWriteIsComplete( deviceAddress, &errorCode, (uint8_t*)&byteCount ))
    {
        return errorCode;
    }

    return USB_HID_RESET_ERROR;
}

/*******************************************************************************
  Function:
     void USBHostHIDTasks( void )

  Summary:
    This function performs the maintenance tasks required by HID class

  Description:
    This function performs the maintenance tasks required by the HID
    class.  If transfer events from the host layer are not being used, then
    it should be called on a regular basis by the application.  If transfer
    events from the host layer are being used, this function is compiled out,
    and does not need to be called.

  Precondition:
    USBHostHIDInitialize() has been called.

  Parameters:
    None

  Returns:
    None

  Remarks:
    None
*******************************************************************************/
void USBHostHIDTasks( void )
{
    // This function isn't used when USB_ENABLE_TRANSFER_EVENT is defined.
}

/*******************************************************************************
  Function:
    uint8_t USBHostHIDWrite( uint8_t deviceAddress,uint8_t reportid, uint8_t interface,
                uint8_t size, uint8_t *data)

  Summary:
    This function starts a Set report transfer request to the device.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress      - Device address
    uint8_t reportid           - Report ID of the requested report
    uint8_t interface          - Interface number
    uint8_t size               - Byte size of the data buffer
    uint8_t *data              - Pointer to the data buffer

  Return Values:
    USB_SUCCESS                 - Request started successfully
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    USB_HID_DEVICE_BUSY         - Device not in proper state for
                                  performing a transfer
    Others                      - Return values from USBHostIssueDeviceRequest(),
                                    and USBHostWrite()
                                    
  Remarks:
    None
*******************************************************************************/
uint8_t USBHostHIDWrite( uint8_t deviceAddress, uint8_t reportid, uint8_t interface,
                uint8_t size, uint8_t *data )
{
    uint8_t    i;
    uint8_t    errorCode;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    pCurrInterfaceDetails = pInterfaceDetails;
    while((pCurrInterfaceDetails != NULL) && (pCurrInterfaceDetails->interfaceNumber != interface))
    {
        pCurrInterfaceDetails = pCurrInterfaceDetails->next;
    }

    // Make sure the device is in a state ready to read/write.
    if ( ! ( ( deviceInfoHID[i].state == STATE_RUNNING ) &&
             ( deviceInfoHID[i].transferOUT.state == STATE_TRANSFER_WAITING ) ) )
    {
        return USB_HID_DEVICE_BUSY;
    }

    // Initialize the transfer information.
    deviceInfoHID[i].transferOUT.bytesTransferred  = 0;
    deviceInfoHID[i].transferOUT.userData          = data;
    deviceInfoHID[i].transferOUT.endpoint          = pCurrInterfaceDetails->endpointOUT;
    deviceInfoHID[i].transferOUT.reportSize        = size;
    deviceInfoHID[i].transferOUT.reportId          = (reportid |((uint16_t)USB_HID_OUTPUT_REPORT<<8));
    deviceInfoHID[i].transferOUT.interface         = interface;
    deviceInfoHID[i].errorCode                     = USB_SUCCESS;

    if(deviceInfoHID[i].transferOUT.endpoint == 0x00)// if endpoint 0 then use control transfer
    {
        errorCode = USBHostIssueDeviceRequest( deviceInfoHID[i].ID.deviceAddress, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE,
                        USB_HID_SET_REPORT, deviceInfoHID[i].transferOUT.reportId, deviceInfoHID[i].transferOUT.interface,deviceInfoHID[i].transferOUT.reportSize,
                        deviceInfoHID[i].transferOUT.userData, USB_DEVICE_REQUEST_SET, deviceInfoHID[i].ID.clientDriverID );
    }
    else
    {
        errorCode = USBHostWrite( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].transferOUT.endpoint,
                                  deviceInfoHID[i].transferOUT.userData, deviceInfoHID[i].transferOUT.reportSize );
    }

    if (!errorCode)
    {
        deviceInfoHID[i].transferOUT.state  = STATE_TRANSFER_REQUESTED;
    }
    else
    {
        deviceInfoHID[i].errorCode          = errorCode;
    }

    return errorCode;
}

/*******************************************************************************
  Function:
    bool USBHostHIDWriteIsComplete( uint8_t deviceAddress, uint8_t *errorCode, uint32_t *byteCount )

  Summary:
    This function indicates whether or not the last write request is complete.

  Description:
    This function indicates whether or not the last write request is complete.
    If the functions returns true, the returned byte count and error
    code are valid. Since only one write can be performed at once
    and only one endpoint can be used, we only need to know the
    device address.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress  - Device address
    uint8_t *errorCode     - Error code from last transfer
    uint32_t *byteCount    - Number of bytes transferred

  Return Values:
    true    - Transfer is complete, errorCode and byteCount are valid
    false   - Transfer is not complete, errorCode and byteCount are not valid
*******************************************************************************/
bool USBHostHIDWriteIsComplete ( uint8_t deviceAddress, uint8_t *errorCode, uint8_t *byteCount )
{
    uint8_t    i;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if ((i == USB_MAX_HID_DEVICES) || (deviceInfoHID[i].state == STATE_DETACHED))
    {
        *errorCode = USB_HID_DEVICE_NOT_FOUND;
        *byteCount = 0;
        return true;
    }

    *errorCode = deviceInfoHID[i].errorCode;
    *byteCount = deviceInfoHID[i].transferOUT.bytesTransferred;

    if(deviceInfoHID[i].transferOUT.state == STATE_TRANSFER_WAITING)
    {
        return true;
    }
    else
    {
        return false;
    }
}


/*******************************************************************************
  Function:
     uint8_t USBHostHIDWriteTerminate( uint8_t deviceAddress, uint8_t interfaceNum )

  Summary:
    This function terminates a write request that is in progress.

  Description:
    This function terminates a write request that is in progress.

  Precondition:
    None

  Parameters:
    uint8_t deviceAddress  - Device address
    uint8_t interfaceNum   - Interface number
                            
  Return Values:
    USB_SUCCESS                 - write request terminated
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    
  Remarks:
    None
*******************************************************************************/
uint8_t USBHostHIDWriteTerminate ( uint8_t deviceAddress, uint8_t interfaceNum )
{
    uint8_t    i;
    uint8_t    endpoint;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    pCurrInterfaceDetails = pInterfaceDetails;
    while((pCurrInterfaceDetails != NULL) && (pCurrInterfaceDetails->interfaceNumber != interfaceNum))
    {
        pCurrInterfaceDetails = pCurrInterfaceDetails->next;
    }

    endpoint = pCurrInterfaceDetails->endpointOUT;

    USBHostTerminateTransfer( deviceAddress, endpoint );

    deviceInfoHID[i].transferOUT.state = STATE_TRANSFER_WAITING;

    return USB_SUCCESS;
}

// *****************************************************************************
// *****************************************************************************
// Section: USB Host HID Report Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    bool USBHostHID_ApiFindBit(uint16_t usagePage,uint16_t usage,HIDReportTypeEnum type,
                          uint8_t* Report_ID, uint8_t* Report_Length, uint8_t* Start_Bit)

  Description:
    This function is used to locate a specific button or indicator.
    Once the report descriptor is parsed by the HID layer without any error,
    data from the report descriptor is stored in pre defined dat structures.
    This function traverses these data structure and exract data required
    by application

  Precondition:
    None

  Parameters:
    uint16_t usagePage         - usage page supported by application
    uint16_t usage             - usage supported by application
    HIDReportTypeEnum type - report type Input/Output for the particular
                             usage
    uint8_t* Report_ID        - returns the report ID of the required usage
    uint8_t* Report_Length    - returns the report length of the required usage
    uint8_t* Start_Bit        - returns  the start bit of the usage in a
                             particular report

  Return Values:
    true    - If the required usage is located in the report descriptor
    false   - If the application required usage is not supported by the
              device(i.e report descriptor).

  Remarks:
    Application event handler with event 'EVENT_HID_RPT_DESC_PARSED' is called.
    Application is suppose to fill in data details in structure 'HID_DATA_DETAILS'.
    This function can be used to the get the details of the required usages.
*******************************************************************************/
bool USBHostHID_ApiFindBit(uint16_t usagePage,uint16_t usage,HIDReportTypeEnum type,uint8_t* Report_ID,
                    uint8_t* Report_Length, uint8_t* Start_Bit)
{
    uint16_t iR;
    uint16_t index;
    uint16_t reportIndex;
    HID_REPORTITEM *reportItem;
    uint8_t* count;

//  Disallow Null Pointers

    if((Report_ID == NULL)|(Report_Length == NULL)|(Start_Bit == NULL))
        return false;

//  Search through all the report items

    for (iR=0; iR < deviceRptInfo.reportItems; iR++)
        {
            reportItem = &itemListPtrs.reportItemList[iR];

//      Search only reports of the proper type

            if ((reportItem->reportType==type))// && (reportItem->globals.reportsize == 1))
                {
                    if (USBHostHID_HasUsage(reportItem,usagePage,usage,(uint16_t*)&index,(uint8_t*)&count))
                        {
                             reportIndex = reportItem->globals.reportIndex;
                             *Report_ID = itemListPtrs.reportList[reportIndex].reportID;
                             *Start_Bit = reportItem->startBit + index;
                             if (type == hidReportInput)
                                 *Report_Length = (itemListPtrs.reportList[reportIndex].inputBits + 7)/8;
                             else if (type == hidReportOutput)
                                 *Report_Length = (itemListPtrs.reportList[reportIndex].outputBits + 7)/8;
                             else
                                 *Report_Length = (itemListPtrs.reportList[reportIndex].featureBits + 7)/8;
                             return true;
                         }
                }
        }
    return false;
}

/*******************************************************************************
  Function:
    bool USBHostHID_ApiFindValue(uint16_t usagePage,uint16_t usage,
                HIDReportTypeEnum type,uint8_t* Report_ID,uint8_t* Report_Length,uint8_t*
                Start_Bit, uint8_t* Bit_Length)

  Description:
    Find a specific Usage Value. Once the report descriptor is parsed by the HID
    layer without any error, data from the report descriptor is stored in
    pre defined dat structures. This function traverses these data structure and
    exract data required by application.

  Precondition:
    None

  Parameters:
    uint16_t usagePage         - usage page supported by application
    uint16_t usage             - usage supported by application
    HIDReportTypeEnum type - report type Input/Output for the particular
                             usage
    uint8_t* Report_ID        - returns the report ID of the required usage
    uint8_t* Report_Length    - returns the report length of the required usage
    uint8_t* Start_Bit        - returns  the start bit of the usage in a
                             particular report
    uint8_t* Bit_Length       - returns size of requested usage type data in bits

  Return Values:
    true    - If the required usage is located in the report descriptor
    false   - If the application required usage is not supported by the
              device(i.e report descriptor).

  Remarks:
    Application event handler with event 'EVENT_HID_RPT_DESC_PARSED' is called.
    Application is suppose to fill in data details structure 'HID_DATA_DETAILS'
    This function can be used to the get the details of the required usages.
*******************************************************************************/
bool USBHostHID_ApiFindValue(uint16_t usagePage,uint16_t usage,HIDReportTypeEnum type,uint8_t* Report_ID,
                    uint8_t* Report_Length,uint8_t* Start_Bit, uint8_t* Bit_Length)
{
    uint16_t index;
    uint16_t reportIndex;
    uint8_t iR;
    uint8_t count;
    HID_REPORTITEM *reportItem;

//  Disallow Null Pointers

     if((Report_ID == NULL)|(Report_Length == NULL)|(Start_Bit == NULL)|(Bit_Length == NULL))
        return false;

//  Search through all the report items

    for (iR=0; iR < deviceRptInfo.reportItems; iR++)
    {
        reportItem = &itemListPtrs.reportItemList[iR];

//      Search only reports of the proper type

        if ((reportItem->reportType==type)&& ((reportItem->dataModes & HIDData_ArrayBit) != HIDData_Array)
             && (reportItem->globals.reportsize != 1))
        {
            if (USBHostHID_HasUsage(reportItem,usagePage,usage,&index,&count))
            {
                 reportIndex = reportItem->globals.reportIndex;
                 *Report_ID = itemListPtrs.reportList[reportIndex].reportID;
                 *Bit_Length = reportItem->globals.reportsize;
                 *Start_Bit = reportItem->startBit + index * (reportItem->globals.reportsize);
                 if (type == hidReportInput)
                     *Report_Length = (itemListPtrs.reportList[reportIndex].inputBits + 7)/8;
                 else if (type == hidReportOutput)
                     *Report_Length = (itemListPtrs.reportList[reportIndex].outputBits + 7)/8;
                 else
                     *Report_Length = (itemListPtrs.reportList[reportIndex].featureBits + 7)/8;
                 return true;
             }
        }
    }
    return false;
}


/*******************************************************************************
  Function:
    bool USBHostHID_ApiImportData(uint8_t *report, uint16_t reportLength,
                     HID_USER_DATA_SIZE *buffer,HID_DATA_DETAILS *pDataDetails)
  Description:
    This function can be used by application to extract data from the input 
    reports. On receiving the input report from the device application can call
    the function with required inputs 'HID_DATA_DETAILS'.

  Precondition:
    None

  Parameters:
    uint8_t *report                    - Input report received from device
    uint16_t reportLength               - Length of input report report
    HID_USER_DATA_SIZE *buffer      - Buffer into which data needs to be
                                      populated
    HID_DATA_DETAILS *pDataDetails  - data details extracted from report
                                      descriptor
  Return Values:
    true    - If the required data is retrieved from the report
    false   - If required data is not found.

  Remarks:
    None
*******************************************************************************/
bool USBHostHID_ApiImportData(uint8_t *report, uint16_t reportLength, HID_USER_DATA_SIZE *buffer, HID_DATA_DETAILS *pDataDetails)
{
    uint16_t data;
    uint16_t signBit;
    uint16_t mask;
    uint16_t extendMask;
    uint16_t start;
    uint16_t startByte;
    uint16_t startBit;
    uint16_t lastByte;
    uint16_t i;

//  Report must be ok

    if (report == NULL) return false;

//  Must be the right report

    if ((pDataDetails->reportID != 0) && (pDataDetails->reportID != report[0])) return false;

//  Length must be ok

    if (pDataDetails->reportLength != reportLength) return false;
    lastByte = (pDataDetails->bitOffset + (pDataDetails->bitLength * pDataDetails->count) - 1)/8;
    if (lastByte > reportLength) return false;

//  Extract data one count at a time

    start = pDataDetails->bitOffset;
    for (i=0; i<pDataDetails->count; i++) {
        startByte = start/8;
        startBit = start&7;
        lastByte = (start + pDataDetails->bitLength - 1)/8;

//      Pick up the data bytes backwards

        data = 0;
        do {
            data <<= 8;
            data |= (int) report[lastByte];
        }
        while (lastByte-- > startByte);

//      Shift to the right to byte align the least significant bit

        if (startBit > 0) data >>= startBit;

//      Done if 16 bits long

        if (pDataDetails->bitLength < 16) {

//          Mask off the other bits

            mask = 1 << pDataDetails->bitLength;
            mask--;
            data &= mask;

//          Sign extend the report item

            if (pDataDetails->signExtend) {
                signBit = 1;
                if (pDataDetails->bitLength > 1) signBit <<= (pDataDetails->bitLength-1);
                extendMask = (signBit << 1) - 1;
                if ((data & signBit)==0) data &= extendMask;
                else data |= ~extendMask;
            }
        }

//      Save the value

        *buffer++ = data;

//      Next one

        start += pDataDetails->bitLength;
    }
    return true;
}


/*******************************************************************************
  Function:
    uint8_t USBHostHID_ApiGetCurrentInterfaceNum(void)

  Description:
    This function reurns the interface number of the cuurent report descriptor
    parsed. This function must be called to fill data interface detail data
    structure and passed as parameter when requesinf for report transfers.

  Precondition:
    None

  Parameters:
    None

  Return Values:
    true    - Transfer is complete, errorCode is valid
    false   - Transfer is not complete, errorCode is not valid

  Remarks:
    None
*******************************************************************************/
uint8_t USBHostHID_ApiGetCurrentInterfaceNum(void)
{
    return(deviceRptInfo.interfaceNumber);
}

/****************************************************************************
  Function:
    uint8_t* USBHostHID_GetCurrentReportInfo(void)

  Description:
    This function returns a pointer to the current report info structure.

  Precondition:
    None

  Parameters:
    None

  Returns:
    uint8_t * - Pointer to the report Info structure.

  Remarks:
    None
  ***************************************************************************/
 // Implemented as a macro. See usb_host_hid.h


/****************************************************************************
  Function:
    uint8_t* USBHostHID_GetItemListPointers()

  Description:
    This function returns a pointer to list of item pointers stored in a
    structure.

  Precondition:
    None

  Parameters:
    None

  Returns:
    uint8_t * - Pointer to list of item pointers structure.

  Remarks:
    None
  ***************************************************************************/
 // Implemented as a macro. See usb_host_hid.h


// *****************************************************************************
// *****************************************************************************
// Section: USB Host Stack Interface Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    bool USBHostHIDEventHandler( uint8_t address, USB_EVENT event,
                        void *data, uint32_t size )

  Precondition:
    The device has been initialized.

  Summary:
    This function is the event handler for this client driver.

  Description:
    This function is the event handler for this client driver.  It is called
    by the host layer when various events occur.

  Parameters:
    uint8_t address    - Address of the device
    USB_EVENT event - Event that has occurred
    void *data      - Pointer to data pertinent to the event
    uint32_t size       - Size of the data

  Return Values:
    true   - Event was handled
    false  - Event was not handled

  Remarks:
    None
*******************************************************************************/
bool USBHostHIDEventHandler( uint8_t address, USB_EVENT event, void *data, uint32_t size )
{
    uint8_t    i;
    switch (event)
    {
        case EVENT_NONE:             // No event occured (NULL event)
            return true;
            break;

        case EVENT_DETACH:           // USB cable has been detached (data: uint8_t, address of device)
            #ifdef DEBUG_MODE
                UART2PrintString( "HID: Detach\r\n" );
            #endif
            // Find the device in the table.  If found, clear the important fields.
            for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != address); i++);
            if (i < USB_MAX_HID_DEVICES)
            {
                // Notify that application that the device has been detached.
                USB_HOST_APP_EVENT_HANDLER( address, EVENT_HID_DETACH,
                    &deviceInfoHID[i].ID.deviceAddress, sizeof(uint8_t) );

                /* Free the memory used by the HID device */
                _USBHostHID_FreeRptDecriptorDataMem(address);
                deviceInfoHID[i].ID.deviceAddress   = 0;
                deviceInfoHID[i].state              = STATE_DETACHED;
            }

            return true;
            break;

        case EVENT_TRANSFER:         // A USB transfer has completed
            #ifdef DEBUG_MODE
                UART2PrintString( "HID: transfer event\r\n" );
            #endif

            for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != address); i++) {}
            if (i == USB_MAX_HID_DEVICES)
            {
                #ifdef DEBUG_MODE
                    UART2PrintString( "HID: Unknown device\r\n" );
                #endif
                return false;
            }

            switch (deviceInfoHID[i].state)
            {
                case STATE_WAIT_FOR_REPORT_DSC:
                    #ifdef DEBUG_MODE
                        UART2PrintString( "HID: Got Report Descriptor\r\n" );
                    #endif
                    deviceInfoHID[i].transferIN.bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount;
                    if ((!((HOST_TRANSFER_DATA *)data)->bErrorCode) && (deviceInfoHID[i].transferIN.bytesTransferred == pCurrInterfaceDetails->sizeOfRptDescriptor ))
                    {
                        /* Invoke HID Parser ,, validate for all the errors in report Descriptor */
//                             deviceInfoHID[i].bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount;
                         deviceInfoHID[i].HIDparserError = _USBHostHID_Parse_Report((uint8_t*)deviceInfoHID[i].rptDescriptor , (uint16_t)pCurrInterfaceDetails->sizeOfRptDescriptor,
                                                                                (uint16_t)pCurrInterfaceDetails->endpointPollInterval, pCurrInterfaceDetails->interfaceNumber);

                        if(deviceInfoHID[i].HIDparserError)
                        {
                            /* Report Descriptor is flawed , flag error and free memory ,
                               retry by requesting again */
                            #ifdef DEBUG_MODE
                                UART2PrintString("\r\nHID Error Reported :  ");
                                UART2PutHex(deviceInfoHID[i].HIDparserError);
                            #endif

                            _USBHostHID_FreeRptDecriptorDataMem(deviceInfoHID[i].ID.deviceAddress);
                            _USBHostHID_LockDevice( USB_HID_REPORT_DESCRIPTOR_BAD );
                            USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress, EVENT_HID_BAD_REPORT_DESCRIPTOR, NULL, 0 );
                        }
                        else
                        {
                            /* Inform Application layer of new device attached */
                            #ifdef DEBUG_MODE
                                UART2PrintString( "HID: Sending Report Descriptor Parsed event\r\n" );
                            #endif
                            if (USB_HOST_APP_EVENT_HANDLER(deviceInfoHID[i].ID.deviceAddress, EVENT_HID_RPT_DESC_PARSED, NULL, 0 ))
                            {
                                deviceInfoHID[i].flags.bfReportDataCollected = 1;
                            }
                            else
                            {
                                if ((pCurrInterfaceDetails->interfaceNumber == (deviceInfoHID[i].noOfInterfaces-1)) &&
                                    (deviceInfoHID[i].flags.bfReportDataCollected == 0))
                                {
                                    #ifdef DEBUG_MODE
                                        UART2PrintString( "HID: Error parsing descriptor\r\n" );
                                    #endif
                                    _USBHostHID_FreeRptDecriptorDataMem(deviceInfoHID[i].ID.deviceAddress);
                                    _USBHostHID_LockDevice( USB_HID_REPORT_DESCRIPTOR_BAD );
                                    USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress, EVENT_HID_BAD_REPORT_DESCRIPTOR, NULL, 0 );
                                }
                            }
                            USB_FREE_AND_CLEAR(deviceInfoHID[i].rptDescriptor);
                            pCurrInterfaceDetails = pCurrInterfaceDetails->next;

                            if(pCurrInterfaceDetails != NULL)
                            {
                                if(pCurrInterfaceDetails->sizeOfRptDescriptor !=0)
                                {
                                    if((deviceInfoHID[i].rptDescriptor = (uint8_t *)USB_MALLOC(pCurrInterfaceDetails->sizeOfRptDescriptor)) == NULL)
                                    {
                                        #ifdef DEBUG_MODE
                                            UART2PrintString( "HID: Out of memory\r\n" );
                                        #endif
                                        _USBHostHID_LockDevice( USB_MEMORY_ALLOCATION_ERROR );
                                        break;
                                    }
                                }
                                if(USBHostIssueDeviceRequest( deviceInfoHID[i].ID.deviceAddress, USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE,
                                        USB_REQUEST_GET_DESCRIPTOR, DSC_RPT_wValue, pCurrInterfaceDetails->interfaceNumber, pCurrInterfaceDetails->sizeOfRptDescriptor, deviceInfoHID[i].rptDescriptor,
                                        USB_DEVICE_REQUEST_GET, deviceInfoHID[i].ID.clientDriverID ))
                                {
                                    #ifdef DEBUG_MODE
                                        UART2PrintString( "HID: Error getting descriptor\r\n" );
                                    #endif
                                    USB_FREE_AND_CLEAR(deviceInfoHID[i].rptDescriptor);
                                    break;
                                }
                                deviceInfoHID[i].state = STATE_WAIT_FOR_REPORT_DSC;
                            }
                            else
                            {
                                if(deviceInfoHID[i].flags.bfReportDataCollected == 0)
                                {
                                    #ifdef DEBUG_MODE
                                        UART2PrintString( "HID: Problem collecting report data\r\n" );
                                    #endif
                                    _USBHostHID_FreeRptDecriptorDataMem(deviceInfoHID[i].ID.deviceAddress);
                                    _USBHostHID_LockDevice( USB_HID_REPORT_DESCRIPTOR_BAD );
                                    USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress, EVENT_HID_BAD_REPORT_DESCRIPTOR, NULL, 0 );
                                }
                                else
                                {
                                    #ifdef DEBUG_MODE
                                        UART2PrintString( "HID: Proceeding to run state\r\n" );
                                    #endif
                                    deviceInfoHID[i].state = STATE_RUNNING;

                                    // Tell the application layer that we have a device.
                                    USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress, EVENT_HID_ATTACH, &(deviceInfoHID[i].ID), sizeof(USB_HID_DEVICE_ID) );
                                }
                            }
                        }
                    }
                    else
                    {
                        // Assuming only a STALL here.  Since it is EP0, we do not have to clear the stall.
                        USBHostClearEndpointErrors( deviceInfoHID[i].ID.deviceAddress, 0 );
                    }
                    break;

                case STATE_RUNNING:
                    if ( ( deviceInfoHID[i].transferIN.state == STATE_TRANSFER_REQUESTED ) &&
                         ( ((HOST_TRANSFER_DATA *)data)->bEndpointAddress == deviceInfoHID[i].transferIN.endpoint ) )
                    {
                        #ifdef DEBUG_MODE
                            UART2PrintString( "HID: Read Event\r\n" );
                        #endif
                        if (((HOST_TRANSFER_DATA *)data)->bErrorCode)
                        {
                            if (USB_ENDPOINT_STALLED == ((HOST_TRANSFER_DATA *)data)->bErrorCode)
                            {
                                 USBHostClearEndpointErrors( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].transferIN.endpoint );
                                 deviceInfoHID[i].returnState = STATE_RUNNING;
                                 deviceInfoHID[i].flags.bfReset = 1;
                                 _USBHostHID_ResetStateJump( i );
                            }
                            else
                            {
                                /* Set proper error code as per HID guideline */
                                #ifdef USB_HID_ENABLE_TRANSFER_EVENT
                                    transferEventData.dataCount         = ((HOST_TRANSFER_DATA *)data)->dataCount;
                                    transferEventData.bErrorCode        = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
                                #endif
                                _USBHostHID_TerminateReadTransfer(((HOST_TRANSFER_DATA *)data)->bErrorCode);
                            }
                        }
                        else
                        {
                            USBHostClearEndpointErrors( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].transferIN.endpoint );
                            deviceInfoHID[i].transferIN.bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount; /* Can compare with report size and flag error ???*/
                            #ifdef USB_HID_ENABLE_TRANSFER_EVENT
                                transferEventData.dataCount         = ((HOST_TRANSFER_DATA *)data)->dataCount;
                                transferEventData.bErrorCode        = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
                            #endif
                            _USBHostHID_TerminateReadTransfer(USB_SUCCESS);
                            return true;
                        }
                    }
                    else if ( (deviceInfoHID[i].transferOUT.state == STATE_TRANSFER_REQUESTED ) &&
                              ( ((HOST_TRANSFER_DATA *)data)->bEndpointAddress == deviceInfoHID[i].transferOUT.endpoint ) )
                    {
                        #ifdef DEBUG_MODE
                            UART2PrintString( "HID: Write Event\r\n" );
                        #endif
                        if (((HOST_TRANSFER_DATA *)data)->bErrorCode)
                        {
                            if (USB_ENDPOINT_STALLED == ((HOST_TRANSFER_DATA *)data)->bErrorCode)
                            {
                                 USBHostClearEndpointErrors( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].transferOUT.endpoint );
                                 deviceInfoHID[i].returnState = STATE_RUNNING ;
                                 deviceInfoHID[i].flags.bfReset = 1;
                                 _USBHostHID_ResetStateJump( i );
                            }
                            else
                            {
                                /* Set proper error code as per HID guideline */
                                #ifdef USB_HID_ENABLE_TRANSFER_EVENT
                                    transferEventData.dataCount         = ((HOST_TRANSFER_DATA *)data)->dataCount;
                                    transferEventData.bErrorCode        = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
                                #endif
                                _USBHostHID_TerminateWriteTransfer(((HOST_TRANSFER_DATA *)data)->bErrorCode);
                            }
                        }
                        else
                        {
                            USBHostClearEndpointErrors( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].transferOUT.endpoint );
                            deviceInfoHID[i].transferOUT.bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount; /* Can compare with report size and flag error ???*/
                            #ifdef USB_HID_ENABLE_TRANSFER_EVENT
                                transferEventData.dataCount         = ((HOST_TRANSFER_DATA *)data)->dataCount;
                                transferEventData.bErrorCode        = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
                            #endif
                            _USBHostHID_TerminateWriteTransfer(USB_SUCCESS);
                            return true;
                        }
                    }
                    // These will be events from application issued requests.  Just pass them up.
                    #ifdef USB_HID_ENABLE_TRANSFER_EVENT
                    else
                    {
                        transferEventData.dataCount         = ((HOST_TRANSFER_DATA *)data)->dataCount;
                        transferEventData.bErrorCode        = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
                        if (((HOST_TRANSFER_DATA *)data)->bEndpointAddress & 0x80)
                        {
                            USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress, EVENT_HID_READ_DONE, &transferEventData, sizeof(HID_TRANSFER_DATA) );
                        }
                        else
                        {
                            USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress, EVENT_HID_WRITE_DONE, &transferEventData, sizeof(HID_TRANSFER_DATA) );
                        }
                    }
                    #endif
                    break;

                case STATE_WAIT_FOR_RESET:
                    #ifdef DEBUG_MODE
                        UART2PrintString( "HID: Reset Event\r\n" );
                    #endif
                    deviceInfoHID[i].errorCode = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
                    deviceInfoHID[i].flags.bfReset = 0;
                    _USBHostHID_ResetStateJump( i );
                    return true;
                    break;

                case STATE_HOLDING:
                    return true;
                    break;

                default:
                    return false;
            }

// This addition has not been fully tested yet.  It may be included in the future.
//        case EVENT_BUS_ERROR:
//            // We will get this error if we have a problem, like too many NAK's 
//            // on a write, so we know the write failed.
//            switch (deviceInfoHID[i].state)
//            {
//                case STATE_READ_REQ_WAIT:
//                    USBHostClearEndpointErrors( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].endpointDATA );
//                    deviceInfoHID[i].bytesTransferred = 0;
//                    #ifdef USB_HID_ENABLE_TRANSFER_EVENT    
//                        transferEventData.dataCount         = 0;
//                        transferEventData.bErrorCode        = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
//                    #endif
//                    _USBHostHID_TerminateReadTransfer( ((HOST_TRANSFER_DATA *)data)->bErrorCode );
//                    return true;
//                    break;
//                    
//                case STATE_WRITE_REQ_WAIT:
//                    USBHostClearEndpointErrors( deviceInfoHID[i].ID.deviceAddress, deviceInfoHID[i].endpointDATA );
//                    deviceInfoHID[i].bytesTransferred = 0;
//                    #ifdef USB_HID_ENABLE_TRANSFER_EVENT    
//                        transferEventData.dataCount         = 0;
//                        transferEventData.bErrorCode        = ((HOST_TRANSFER_DATA *)data)->bErrorCode;
//                    #endif
//                    _USBHostHID_TerminateWriteTransfer( ((HOST_TRANSFER_DATA *)data)->bErrorCode );
//                    return true;
//                    break;
//            }            
//            break;
            
        case EVENT_SOF:              // Start of frame - NOT NEEDED
        case EVENT_RESUME:           // Device-mode resume received
        case EVENT_SUSPEND:          // Device-mode suspend/idle event received
        case EVENT_RESET:            // Device-mode bus reset received
        case EVENT_STALL:            // A stall has occured
            return true;
            break;

        default:
            return false;
            break;
    }
    return false;
}
/*******************************************************************************
  Function:
    bool USBHostHIDAppDataEventHandler( uint8_t address, USB_EVENT event,
                        void *data, uint32_t size )

  Precondition:
    The device has been initialized.

  Summary:
    This function is the data event handler for this client driver.

  Description:
    This function is the data event handler for this client driver.  It is called
    by the host layer when various events occur.

  Parameters:
    uint8_t address    - Address of the device
    USB_EVENT event - Event that has occurred
    void *data      - Pointer to data pertinent to the event
    uint32_t size       - Size of the data

  Return Values:
    true   - Event was handled
    false  - Event was not handled

  Remarks:
    None
*******************************************************************************/
bool USBHostHIDAppDataEventHandler( uint8_t address, USB_EVENT event, void *data, uint32_t size )
{
    return true;
}

/*******************************************************************************
  Function:
    bool USBHostHIDInitialize( uint8_t address, uint32_t flags, uint8_t clientDriverID )

  Summary:
    This function is the initialization routine for this client driver.

  Description:
    This function is the initialization routine for this client driver.  It
    is called by the host layer when the USB device is being enumerated.For a 
    HID device we need to look into HID descriptor, interface descriptor and 
    endpoint descriptor.

  Precondition:
    None

  Parameters:
    uint8_t address        - Address of the new device
    uint32_t flags          - Initialization flags
    uint8_t clientDriverID - Client driver identification for device requests

  Return Values:
    true   - We can support the device.
    false  - We cannot support the device.

  Remarks:
    None
*******************************************************************************/
bool USBHostHIDInitialize( uint8_t address, uint32_t flags, uint8_t clientDriverID )
{
    uint8_t                     *descriptor;
    bool                        validConfiguration      = false;
    bool                        rptDescriptorfound      = false;
    uint16_t                    i;
    uint8_t                     endpointIN              = 0;
    uint8_t                     endpointOUT             = 0;
    uint8_t                     device;
    uint8_t                     numofinterfaces         = 0;
    uint8_t                     temp_i                  = 0;
    USB_HID_INTERFACE_DETAILS   *pNewInterfaceDetails   = NULL;

    // Find the device in the table.  If it's there, we have already initialized this device.
    for (device = 0; (device < USB_MAX_HID_DEVICES) ; device++)
    {
        if(deviceInfoHID[device].ID.deviceAddress == address)
            return true;
    }

    // See if we have room for another device
    for (device = 0; (device < USB_MAX_HID_DEVICES) && (deviceInfoHID[device].ID.deviceAddress != 0); device++);
    if (device == USB_MAX_HID_DEVICES)
    {
        return false;
    }

    // Fill in the VID, PID, and client driver ID. They are not not valid unless deviceAddress is non-zero.
    descriptor = USBHostGetDeviceDescriptor( address );
    deviceInfoHID[device].flags.bfAddressReported = 0;
    deviceInfoHID[device].ID.vid            = ((USB_DEVICE_DESCRIPTOR *)descriptor)->idVendor;
    deviceInfoHID[device].ID.pid            = ((USB_DEVICE_DESCRIPTOR *)descriptor)->idProduct;
    deviceInfoHID[device].ID.clientDriverID = clientDriverID;

    // Get ready to parse the configuration descriptor.
    descriptor = USBHostGetCurrentConfigurationDescriptor( address );

//    pCurrInterfaceDetails = pInterfaceDetails;
    i = 0;

    // Total no of interfaces
    deviceInfoHID[device].noOfInterfaces = descriptor[i+4] ;

    // Set current configuration to this configuration.  We can change it later.

    // MCHP - Check power requirement

    // Find the next interface descriptor.
    while (i < ((USB_CONFIGURATION_DESCRIPTOR *)descriptor)->wTotalLength)
    {
        // See if we are pointing to an interface descriptor.
        if (descriptor[i+1] == USB_DESCRIPTOR_INTERFACE)
        {
            // See if the interface is a HID interface.
            if ((descriptor[i+5] == DEVICE_CLASS_HID)||(descriptor[i+5] == 0))
            {
                deviceInfoHID[device].driverSupported = 1;
                if (deviceInfoHID[device].driverSupported)
                {
                    if ((pNewInterfaceDetails = (USB_HID_INTERFACE_DETAILS*)USB_MALLOC(sizeof(USB_HID_INTERFACE_DETAILS))) == NULL)
                    {
                        return false;
                    }
                    numofinterfaces ++ ;

                    // Create new entry into interface list
                    if(pInterfaceDetails == NULL)
                    {
                        pInterfaceDetails       = pNewInterfaceDetails;
                        pCurrInterfaceDetails   = pNewInterfaceDetails;
                        pInterfaceDetails->next = NULL;
                    }
                    else
                    {
                        pCurrInterfaceDetails->next             = pNewInterfaceDetails;
                        pCurrInterfaceDetails                   = pNewInterfaceDetails;
                        pCurrInterfaceDetails->next             = NULL;
                    }

//                       USB_FREE_AND_CLEAR( pNewInterfaceDetails );
                    pCurrInterfaceDetails->interfaceNumber   = descriptor[i+2];

                    // Scan for hid descriptors.
                    i += descriptor[i];
                    if(descriptor[i+1] == DSC_HID)
                    {
                        if(descriptor[i+5] == 0)
                        {
                            // At least one report descriptor must be present - flag error
                            rptDescriptorfound = false;
                        }
                        else
                        {
                            rptDescriptorfound = true;
                            pCurrInterfaceDetails->sizeOfRptDescriptor = ((descriptor[i+7]) |
                                                                          ((descriptor[i+8]) << 8));

                            // Look for IN and OUT endpoints.
                            // MCHP - what if there are no endpoints?
                            endpointIN  = 0;
                            endpointOUT = 0;
                            temp_i = 0;
                            // Scan for endpoint descriptors.
                            i += descriptor[i];
                            while(descriptor[i+1] == USB_DESCRIPTOR_ENDPOINT)
                            {
                                if (descriptor[i+3] == 0x03) // Interrupt
                                {
                                    if (((descriptor[i+2] & 0x80) == 0x80) && (endpointIN == 0))
                                    {
                                        endpointIN = descriptor[i+2];
                                    }
                                    if (((descriptor[i+2] & 0x80) == 0x00) && (endpointOUT == 0))
                                    {
                                        endpointOUT = descriptor[i+2];
                                    }
                                }
                                temp_i = descriptor[i];
                                i += descriptor[i];
                            }

                            i -= temp_i;
//                                if ((endpointIN != 0) || (endpointOUT != 0))
                            // Some devices use EP0 as the OUT endpoint
                            if (endpointIN != 0)
                            {
                                // Initialize the remaining device information.
                                deviceInfoHID[device].ID.deviceAddress        = address;
//
                                pCurrInterfaceDetails->endpointIN           = endpointIN;
                                pCurrInterfaceDetails->endpointOUT          = endpointOUT;
                                pCurrInterfaceDetails->endpointMaxDataSize  = ((descriptor[i+4]) |
                                                                           (descriptor[i+5] << 8));
                                pCurrInterfaceDetails->endpointPollInterval = descriptor[i+6];
                                validConfiguration = true;

                                /* By default NAK time out is disabled for HID class */
                                /* If timeout is required then pass '1' instead '0' in below function call */
//                                    USBHostSetNAKTimeout( address, endpointIN,  0, USB_NUM_INTERRUPT_NAKS );
//                                    USBHostSetNAKTimeout( address, endpointOUT, 0, USB_NUM_INTERRUPT_NAKS );
                            }
//                            i -= temp_i;

                        }
                    }
                    else
                    {
                        /* HID Descriptor not found */
                    }
                }
            }
        }

        // Jump to the next descriptor in this configuration.
        i += descriptor[i];
    }

//    This check doesn't account for the fact that some of the interface descriptors are for
//    alternate settings of the same interface.  It's not really necessary - sometimes
//    descriptors have errors in them.
//    if(numofinterfaces != deviceInfoHID[device].noOfInterfaces)
//    {
//        #ifdef DEBUG_MODE
//            UART2PrintString("HID: Interface count mismatch\r\n" );
//        #endif
//        validConfiguration = false;
//    }

    if(validConfiguration)
    {
        pCurrInterfaceDetails = pInterfaceDetails;
        if (pCurrInterfaceDetails->sizeOfRptDescriptor !=0)
        {
            if (deviceInfoHID[device].rptDescriptor != NULL)
            {
                USB_FREE_AND_CLEAR( deviceInfoHID[device].rptDescriptor );
            }

            if((deviceInfoHID[device].rptDescriptor = (uint8_t *)USB_MALLOC(pCurrInterfaceDetails->sizeOfRptDescriptor)) == NULL)
            {
                return false;
            }
        }
        if (USBHostIssueDeviceRequest( deviceInfoHID[device].ID.deviceAddress, USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE,
             USB_REQUEST_GET_DESCRIPTOR, DSC_RPT_wValue, pCurrInterfaceDetails->interfaceNumber, pCurrInterfaceDetails->sizeOfRptDescriptor, deviceInfoHID[device].rptDescriptor,
             USB_DEVICE_REQUEST_GET, deviceInfoHID[device].ID.clientDriverID ))
        {
            USB_FREE_AND_CLEAR(deviceInfoHID[device].rptDescriptor);
            return false;
        }
        deviceInfoHID[device].state             = STATE_WAIT_FOR_REPORT_DSC;

        deviceInfoHID[device].transferIN.state  = STATE_TRANSFER_WAITING;
        deviceInfoHID[device].transferOUT.state = STATE_TRANSFER_WAITING;

        return true;
    }
    else
    {
       return false;
    }
}


// *****************************************************************************
// *****************************************************************************
// Internal Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    void _USBHostHID_FreeRptDecriptorDataMem(uint8_t deviceAddress)

  Summary:


  Description:
    This function is called in case of error is encountered . This function
    frees the memory allocated for report descriptor and interface
    descriptors.

  Precondition:
    None.

  Parameters:
    uint8_t address        - Address of the device

  Returns:
    None

  Remarks:
    None
***************************************************************************/
static void _USBHostHID_FreeRptDecriptorDataMem(uint8_t deviceAddress)
{
    USB_HID_INTERFACE_DETAILS*           ptempInterface;
    uint8_t    i;

    // Find the device in the table.  If found, clear the important fields.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfoHID[i].ID.deviceAddress != deviceAddress); i++);
    if (i < USB_MAX_HID_DEVICES)
    {
        if(deviceInfoHID[i].rptDescriptor != NULL)
        {
            USB_FREE_AND_CLEAR(deviceInfoHID[i].rptDescriptor);
        }

    /* free memory allocated to report descriptor in deviceInfoHID */
       while(pInterfaceDetails != NULL)
        {
            ptempInterface = pInterfaceDetails->next;
            USB_FREE_AND_CLEAR(pInterfaceDetails);
            pInterfaceDetails = ptempInterface;
        }
    }
}

/*******************************************************************************
  Function:
    void _USBHostHID_ResetStateJump( uint8_t i )

  Summary:

  Description:
    This function determines which portion of the reset processing needs to
    be executed next and jumps to that state.

Precondition:
    The device information must be in the deviceInfoHID array.

  Parameters:
    uint8_t i  - Index into the deviceInfoHIDMSD structure for the device to reset.

  Returns:
    None

  Remarks:
    None
*******************************************************************************/
static void _USBHostHID_ResetStateJump( uint8_t i )
{
    uint8_t    errorCode;

    if (deviceInfoHID[i].flags.bfReset)
    {
        deviceInfoHID[i].transferIN.state   = STATE_TRANSFER_WAITING;
        deviceInfoHID[i].transferOUT.state  = STATE_TRANSFER_WAITING;
                
        errorCode = USBHostIssueDeviceRequest( deviceInfoHID[i].ID.deviceAddress, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE,
                        USB_HID_RESET, 0, deviceInfoHID[i].transferOUT.interface, 0, NULL, USB_DEVICE_REQUEST_SET, deviceInfoHID[i].ID.clientDriverID );

        if (errorCode)
        {
            deviceInfoHID[i].errorCode    = USB_HID_RESET_ERROR;
            deviceInfoHID[i].state        = STATE_RUNNING;
            USB_HOST_APP_EVENT_HANDLER( deviceInfoHID[i].ID.deviceAddress, EVENT_HID_RESET_ERROR, NULL, 0 );
        }
        else
        {
            deviceInfoHID[i].state = STATE_WAIT_FOR_RESET;
        }
    }
    else
    {
        if (!deviceInfoHID[i].errorCode)
        {
            USB_HOST_APP_EVENT_HANDLER(deviceInfoHID[i].ID.deviceAddress, EVENT_HID_RESET, NULL, 0 );
        }
        else
        {
            USB_HOST_APP_EVENT_HANDLER(deviceInfoHID[i].ID.deviceAddress, EVENT_HID_RESET_ERROR, NULL, 0 );
        }

        deviceInfoHID[i].state = deviceInfoHID[i].returnState;
    }
}


