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

//DOM-IGNORE-BEGIN
#ifndef _USB_HOST_HID_H_
#define _USB_HOST_HID_H_
//DOM-IGNORE-END

#include <stdint.h>
#include <stdbool.h>
#include "usb_hid.h"
#include "usb_host_hid_parser.h"

// *****************************************************************************
// *****************************************************************************
// Section: Configuration
// *****************************************************************************
// *****************************************************************************

#ifndef USB_ENABLE_TRANSFER_EVENT
    #error USB_ENABLE_TRANSFER_EVENT must be defined
#endif

// *****************************************************************************
// *****************************************************************************
// Section: Constants
// *****************************************************************************
// *****************************************************************************

#define USB_HID_TRANSFER_IN             1
#define USB_HID_TRANSFER_OUT            0

// *****************************************************************************
// Section: HID Class Error Codes
// *****************************************************************************

#define USB_HID_CLASS_ERROR              USB_ERROR_CLASS_DEFINED

typedef enum
{
    USB_HID_COMMAND_PASSED          = USB_SUCCESS,          // Command was successful.
    USB_HID_COMMAND_FAILED          = USB_HID_CLASS_ERROR,  // Command failed at the device.
    USB_HID_PHASE_ERROR,                                    // Command had a phase error at the device.
    USB_HID_DEVICE_NOT_FOUND,                               // Device with the specified address is not available.
    USB_HID_DEVICE_BUSY,                                    // A transfer is currently in progress.
    USB_HID_NO_REPORT_DESCRIPTOR,                           // No report descriptor found
    USB_HID_INTERFACE_ERROR,                                // The interface layer cannot support the device.
    USB_HID_REPORT_DESCRIPTOR_BAD,                          // Report Descriptor for not proper
    USB_HID_RESET_ERROR,                                    // An error occurred while resetting the device.
    USB_HID_ILLEGAL_REQUEST,                                // Cannot perform requested operation.

    USB_HID_DEVICE_DETACHED,
    USB_HID_INITIALIZING,
    USB_PROCESSING_REPORT_DESCRIPTOR,
    USB_HID_NORMAL_RUNNING,
    USB_HID_DEVICE_HOLDING,
    USB_HID_RESETTING_DEVICE
    
} USB_HOST_HID_RETURN_CODES;



// *****************************************************************************
// Section: Interface and Protocol Constants
// *****************************************************************************

#define DEVICE_CLASS_HID             0x03   // HID Interface Class Code 

#define DSC_HID                      0x21   // HID Descriptor Code 
#define DSC_RPT_wValue               0x2200 // Report Descriptor Code, used for USBHostIssueDeviceRequest
#define DSC_PHY                      0x23   // Physical Descriptor Code 

// *****************************************************************************
// Section: HID Event Definition
// *****************************************************************************

// If the application has not defined an offset for HID events, set it to 0.
#ifndef EVENT_HID_OFFSET
    #define EVENT_HID_OFFSET    0
#endif

    // No event occured (NULL event)
#define EVENT_HID_NONE                      EVENT_HID_BASE + EVENT_HID_OFFSET + 0   
    // A Report Descriptor has been parsed.  The returned data pointer is NULL.
    // The application must collect details, or simply return true if the 
    // application is already aware of the data format.
#define EVENT_HID_RPT_DESC_PARSED           EVENT_HID_BASE + EVENT_HID_OFFSET + 1   
//#define EVENT_HID_TRANSFER        EVENT_HID_BASE + EVENT_HID_OFFSET + 3   // Unused - value retained for legacy.
    // A HID Read transfer has completed.  The returned data pointer points to a 
    // HID_TRANSFER_DATA structure, with information about the transfer.  
#define EVENT_HID_READ_DONE                 EVENT_HID_BASE + EVENT_HID_OFFSET + 4   
    // A HID Write transfer has completed.  The returned data pointer points to a 
    // HID_TRANSFER_DATA structure, with information about the transfer.  
#define EVENT_HID_WRITE_DONE                EVENT_HID_BASE + EVENT_HID_OFFSET + 5 
    // HID reset complete.  The returned data pointer is NULL.
#define EVENT_HID_RESET                     EVENT_HID_BASE + EVENT_HID_OFFSET + 6   
    // A HID device has attached.  The returned data pointer points to a
    // USB_HID_DEVICE_ID structure.
#define EVENT_HID_ATTACH                    EVENT_HID_BASE + EVENT_HID_OFFSET + 7   
    // A HID device has detached.  The returned data pointer points to a
    // byte with the previous address of the detached device.
#define EVENT_HID_DETACH                    EVENT_HID_BASE + EVENT_HID_OFFSET + 8  
    // There was a problem parsing the report descriptor of the attached device.
    // Communication with the device is not allowed, and the device should be
    // detached. 
#define EVENT_HID_BAD_REPORT_DESCRIPTOR     EVENT_HID_BASE + EVENT_HID_OFFSET + 9   
    // An error occurred while trying to do a HID reset.  The returned data pointer 
    // is NULL.
#define EVENT_HID_RESET_ERROR               EVENT_HID_BASE + EVENT_HID_OFFSET + 10   



// *****************************************************************************
// *****************************************************************************
// Section: Data Structures
// *****************************************************************************
// *****************************************************************************

//******************************************************************************
/* HID Data Details

This structure defines the objects used by the application to access required
report. Application must use parser interface functions to fill these details.
e.g. USBHostHID_ApiFindValue
*/
typedef struct _HID_DATA_DETAILS
{
    uint16_t reportLength;                // reportLength - the expected length of the parent report.
    uint16_t reportID;                    // reportID - report ID - the first byte of the parent report.
    uint8_t bitOffset;                   // BitOffset - bit offset within the report.
    uint8_t bitLength;                   // bitlength - length of the data in bits.
    uint8_t count;                       // count - what's left of the message after this data.
    uint8_t signExtend;                  // extend - sign extend the data.
    uint8_t interfaceNum;                // interfaceNum - informs HID layer about interface number.
}   HID_DATA_DETAILS;
// Note: One would ordinarily get these values from a parser
//       find. (e.g. USBHostHID_ApiFindValue)


// *****************************************************************************
/* HID User Data Size

This defines the data type required to hold the maximum field size data.
*/
#if((HID_MAX_DATA_FIELD_SIZE >= 1) && (HID_MAX_DATA_FIELD_SIZE <= 8))  /* Maximum size of data field within a report */
    typedef unsigned char    HID_USER_DATA_SIZE;
#elif((HID_MAX_DATA_FIELD_SIZE >= 9) && (HID_MAX_DATA_FIELD_SIZE <= 16))
    typedef unsigned int     HID_USER_DATA_SIZE;
#elif((HID_MAX_DATA_FIELD_SIZE >= 17) && (HID_MAX_DATA_FIELD_SIZE <= 32))
    typedef unsigned long    HID_USER_DATA_SIZE;
#else
    typedef unsigned char    HID_USER_DATA_SIZE;
#endif


// *****************************************************************************
/* HID Device ID Information

This structure contains identification information about an attached device.
*/
typedef struct _USB_HID_DEVICE_ID
{
    uint16_t                           vid;                    // Vendor ID of the device
    uint16_t                           pid;                    // Product ID of the device
    uint8_t                            deviceAddress;          // Address of the device on the USB
    uint8_t                            clientDriverID;         // Client driver ID for device requests
} USB_HID_DEVICE_ID;


// *****************************************************************************
/* HID Transfer Information

This structure is used when the event handler is used to notify the upper layer
of transfer completion (EVENT_HID_READ_DONE or EVENT_HID_WRITE_DONE).
*/

typedef struct _HID_TRANSFER_DATA
{
   uint32_t                dataCount;          // Count of bytes transferred.
   uint8_t                 bErrorCode;         // Transfer error code.
} HID_TRANSFER_DATA;


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes 
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
    uint8_t deviceAddress  - Address of the attached device.

  Return Values:
    uint8_t deviceAddress  - Address of the newly attached device or 0 if there
 are no newly attached devices.

  Remarks:
    None
*******************************************************************************/
uint8_t USBHostHIDDeviceDetect
(
    void    
);


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
uint8_t    USBHostHIDDeviceStatus
(
    uint8_t deviceAddress 
);

/*******************************************************************************
  Function:
    uint8_t USBHostHIDRead( uint8_t deviceAddress,uint8_t reportid, uint8_t interface, 
                uint8_t size, uint8_t *data)

  Summary:
     This function starts a Get report transfer reuest from the device,
     utilizing the function USBHostHIDTransfer();

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
uint8_t USBHostHIDRead
( 
    uint8_t deviceAddress, 
    uint8_t reportid, 
    uint8_t interface, 
    uint8_t size, 
    uint8_t *data 
);

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
    true    - Transfer is complete, errorCode is valid
    false   - Transfer is not complete, errorCode is not valid
*******************************************************************************/
bool USBHostHIDReadIsComplete
( 
    uint8_t deviceAddress, 
    uint8_t *errorCode, 
    uint8_t *byteCount 
);

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
uint8_t USBHostHIDReadTerminate
( 
    uint8_t deviceAddress, 
    uint8_t interfaceNum 
);

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
uint8_t    USBHostHIDResetDevice
( 
    uint8_t deviceAddress 
);

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
    None - None

  Returns:
    None

  Remarks:
    None
*******************************************************************************/
void    USBHostHIDTasks( void );

/*******************************************************************************
  Function:
    uint8_t USBHostHIDWrite( uint8_t deviceAddress,uint8_t reportid, uint8_t interface,
                uint8_t size, uint8_t *data)

  Summary:
    This function starts a Set report transfer request to the device,
    utilizing the function USBHostHIDTransfer();

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
uint8_t USBHostHIDWrite
( 
    uint8_t deviceAddress, 
    uint8_t reportid, 
    uint8_t interface,
    uint8_t size, 
    uint8_t *data 
);


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
    true    - Transfer is complete, errorCode is valid
    false   - Transfer is not complete, errorCode is not valid
*******************************************************************************/
bool USBHostHIDWriteIsComplete
( 
    uint8_t deviceAddress, 
    uint8_t *errorCode, 
    uint8_t *byteCount 
);


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
uint8_t USBHostHIDWriteTerminate
( 
    uint8_t deviceAddress, 
    uint8_t interfaceNum 
);

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
bool USBHostHID_ApiFindBit
(
    uint16_t usagePage,
    uint16_t usage,
    HIDReportTypeEnum type,
    uint8_t* Report_ID,
    uint8_t* Report_Length, 
    uint8_t* Start_Bit
);


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
bool USBHostHID_ApiFindValue
(
    uint16_t usagePage,
    uint16_t usage,
    HIDReportTypeEnum type,
    uint8_t* Report_ID,
    uint8_t* Report_Length,
    uint8_t* Start_Bit, 
    uint8_t* Bit_Length
);


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
uint8_t USBHostHID_ApiGetCurrentInterfaceNum(void);


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
bool USBHostHID_ApiImportData
(
    uint8_t *report,
    uint16_t reportLength,
    HID_USER_DATA_SIZE *buffer, 
    HID_DATA_DETAILS *pDataDetails
);


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
#define USBHostHID_GetCurrentReportInfo() (&deviceRptInfo)


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
#define USBHostHID_GetItemListPointers() (&itemListPtrs)



// *****************************************************************************
// *****************************************************************************
// Section: USB Host Callback Function Prototypes
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    bool USBHostHIDInitialize( uint8_t address, uint16_t flags, uint8_t clientDriverID )

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
bool USBHostHIDInitialize
(
    uint8_t address, 
    uint32_t flags, 
    uint8_t clientDriverID
);

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
bool USBHostHIDEventHandler
( 
    uint8_t address, 
    USB_EVENT event, 
    void *data, 
    uint32_t size 
);

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
bool USBHostHIDAppDataEventHandler( uint8_t address, USB_EVENT event, void *data, uint32_t size );

#endif
