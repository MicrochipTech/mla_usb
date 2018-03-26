/*******************************************************************************

    USB Host Generic Client Driver (Header File)

Description:
    This is the Generic client driver file for a USB Embedded Host device.  This
    driver should be used in a project with usb_host.c to provided the USB
    hardware interface.

    To interface with USB Embedded Host layer, the routine USBHostGenericInit()
    should be specified as the Initialize() function, and
    USBHostGenericEventHandler() should be specified as the EventHandler()
    function in the usbClientDrvTable[] array declared in usb_config.c.

    This driver can be configured to either use transfer events from usb_host.c
    or use a polling mechanism.  If USB_ENABLE_TRANSFER_EVENT is defined, this
    driver will utilize transfer events.  Otherwise, this driver will utilize
    polling.

    Since the generic class is performed with interrupt transfers,
    USB_SUPPORT_INTERRUPT_TRANSFERS must be defined.

Summary:
    This is the Generic client driver file for a USB Embedded Host device.

*******************************************************************************/
//DOM-IGNORE-BEGIN
/******************************************************************************

* FileName:        usb_client_generic.h
* Dependencies:    None
* Processor:       PIC24/dsPIC30/dsPIC33/PIC32MX
* Compiler:        C30 v2.01/C32 v0.00.18
* Company:         Microchip Technology, Inc.

Software License Agreement

The software supplied herewith by Microchip Technology Incorporated
(the �Company�) for its PICmicro� Microcontroller is intended and
supplied to you, the Company�s customer, for use solely and
exclusively on Microchip PICmicro Microcontroller products. The
software is owned by the Company and/or its supplier, and is
protected under applicable copyright laws. All rights are reserved.
Any use in violation of the foregoing restrictions may subject the
user to criminal sanctions under applicable laws, as well as to
civil liability for the breach of the terms and conditions of this
license.

THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

Author          Date    Comments
--------------------------------------------------------------------------------
BC/KO       25-Dec-2007 First release

*******************************************************************************/
#ifndef __USBHOSTGENERIC_H__
#define __USBHOSTGENERIC_H__
//DOM-IGNORE-END

#include <stdint.h>
#include <stdbool.h>

// *****************************************************************************
// *****************************************************************************
// Section: Constants
// *****************************************************************************
// *****************************************************************************

// This is the default Generic Client Driver endpoint number.
#ifndef USB_GENERIC_EP
    #define USB_GENERIC_EP       1
#endif

// *****************************************************************************
// *****************************************************************************
// Section: USB Generic Client Events
// *****************************************************************************
// *****************************************************************************

        // This is an optional offset for the values of the generated events.
        // If necessary, the application can use a non-zero offset for the
        // generic events to resolve conflicts in event number.
#ifndef EVENT_GENERIC_OFFSET
#define EVENT_GENERIC_OFFSET 0
#endif

        // This event indicates that a Generic device has been attached.
        // When USB_HOST_APP_EVENT_HANDLER is called with this event, *data
        // points to a GENERIC_DEVICE_ID structure, and size is the size of the
        // GENERIC_DEVICE_ID structure.
#define EVENT_GENERIC_ATTACH  (EVENT_GENERIC_BASE+EVENT_GENERIC_OFFSET+0)

        // This event indicates that the specified device has been detached
        // from the USB.  When USB_HOST_APP_EVENT_HANDLER is called with this
        // event, *data points to a uint8_t that contains the device address, and
        // size is the size of a uint8_t.
#define EVENT_GENERIC_DETACH  (EVENT_GENERIC_BASE+EVENT_GENERIC_OFFSET+1)

        // This event indicates that a previous write request has completed.
        // These events are enabled if USB Embedded Host transfer events are
        // enabled (USB_ENABLE_TRANSFER_EVENT is defined).  When
        // USB_HOST_APP_EVENT_HANDLER is called with this event, *data points
        // to the buffer that completed transmission, and size is the actual
        // number of bytes that were written to the device.
#define EVENT_GENERIC_TX_DONE (EVENT_GENERIC_BASE+EVENT_GENERIC_OFFSET+2)

        // This event indicates that a previous read request has completed.
        // These events are enabled if USB Embedded Host transfer events are
        // enabled (USB_ENABLE_TRANSFER_EVENT is defined).  When
        // USB_HOST_APP_EVENT_HANDLER is called with this event, *data points
        // to the receive buffer, and size is the actual number of bytes read
        // from the device.
#define EVENT_GENERIC_RX_DONE (EVENT_GENERIC_BASE+EVENT_GENERIC_OFFSET+3)


// *****************************************************************************
// *****************************************************************************
// Section: USB Data Structures
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Generic Device ID Information

This structure contains identification information about an attached device.
*/
typedef struct _GENERIC_DEVICE_ID
{
    uint16_t        vid;                    // Vendor ID of the device
    uint16_t        pid;                    // Product ID of the device
    #ifdef USB_GENERIC_SUPPORT_SERIAL_NUMBERS
        uint16_t   *serialNumber;           // Pointer to the Unicode serial number string
        uint8_t    serialNumberLength;     // Length of the serial number string (in Unicode characters)
    #endif
    uint8_t        deviceAddress;          // Address of the device on the USB
} GENERIC_DEVICE_ID;


// *****************************************************************************
/* Generic Device Information

This structure contains information about an attached device, including
status flags and device identification.
*/
typedef struct _GENERIC_DEVICE
{
    GENERIC_DEVICE_ID   ID;             // Identification information about the device
    uint32_t               rxLength;       // Number of bytes received in the last IN transfer
    uint8_t                clientDriverID; // ID to send when issuing a Device Request
    
    #ifndef USB_ENABLE_TRANSFER_EVENT
        uint8_t            rxErrorCode;    // Error code of last IN transfer
        uint8_t            txErrorCode;    // Error code of last OUT transfer
    #endif

    union
    {
        uint8_t val;                       // uint8_t representation of device status flags
        struct
        {
            uint8_t initialized    : 1;    // Driver has been initialized
            uint8_t txBusy         : 1;    // Driver busy transmitting data
            uint8_t rxBusy         : 1;    // Driver busy receiving data
            #ifdef USB_GENERIC_SUPPORT_SERIAL_NUMBERS
                uint8_t serialNumberValid    : 1;    // Serial number is valid
            #endif
        };
    } flags;                            // Generic client driver status flags

} GENERIC_DEVICE;


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************

extern GENERIC_DEVICE   gc_DevData; // Information about the attached device.

// *****************************************************************************
// *****************************************************************************
// Section: Host Stack Interface Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    bool USBHostGenericInit ( uint8_t address, uint32_t flags, uint8_t clientDriverID )

  Summary:
    This function is called by the USB Embedded Host layer when a "generic"
    device attaches.

  Description:
    This routine is a call out from the USB Embedded Host layer to the USB
    generic client driver.  It is called when a "generic" device has been
    connected to the host.  Its purpose is to initialize and activate the USB
    Generic client driver.

  Preconditions:
    The device has been configured.

  Parameters:
    uint8_t address    - Device's address on the bus
    uint32_t flags     - Initialization flags
    uint8_t clientDriverID - ID to send when issuing a Device Request via
                            USBHostIssueDeviceRequest(), USBHostSetDeviceConfiguration(),
                            or USBHostSetDeviceInterface().  

  Return Values:
    true    - Initialization was successful
    false   - Initialization failed

  Remarks:
    Multiple client drivers may be used in a single application.  The USB
    Embedded Host layer will call the initialize routine required for the
    attached device.
  ***************************************************************************/

bool USBHostGenericInitialize ( uint8_t address, uint32_t flags, uint8_t clientDriverID );


/****************************************************************************
  Function:
    bool USBHostGenericEventHandler ( uint8_t address, USB_EVENT event,
                            void *data, uint32_t size )

  Summary:
    This routine is called by the Host layer to notify the general client of
    events that occur.

  Description:
    This routine is called by the Host layer to notify the general client of
    events that occur.  If the event is recognized, it is handled and the
    routine returns true.  Otherwise, it is ignored and the routine returns
    false.

  Preconditions:
    None

  Parameters:
    uint8_t address    - Address of device with the event
    USB_EVENT event - The bus event that occured
    void *data      - Pointer to event-specific data
    uint32_t size      - Size of the event-specific data

  Return Values:
    true    - The event was handled
    false   - The event was not handled

  Remarks:
    None
  ***************************************************************************/

bool USBHostGenericEventHandler ( uint8_t address, USB_EVENT event, void *data, uint32_t size );


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes and Macro Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    bool API_VALID( uint8_t address )

  Description:
    This function is used internally to ensure that the requested device is
    attached and initialized before performing an operation.

  Preconditions:
    None

  Parameters:
    uint8_t address    - USB address of the device

  Returns:
    true    - A device with the requested address is attached and initialized.
    false   - A device with the requested address is not available, or it
                has not been initialized.

  Remarks:
    None
  ***************************************************************************/

#define API_VALID(a) ( (((a)==gc_DevData.ID.deviceAddress) && gc_DevData.flags.initialized == 1) ? true : false )


/****************************************************************************
  Function:
    bool USBHostGenericDeviceDetached( uint8_t deviceAddress )

  Description:
    This interface is used to check if the devich has been detached from the
    bus.

  Preconditions:
    None

  Parameters:
    uint8_t deviceAddress	- USB Address of the device.

  Return Values:
    true    - The device has been detached, or an invalid deviceAddress is given.
    false   - The device is attached

  Example:
    <code>
    if (USBHostGenericDeviceDetached( deviceAddress ))
    {
        // Handle detach
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

#define USBHostGenericDeviceDetached(a) ( (((a)==gc_DevData.ID.deviceAddress) && gc_DevData.flags.initialized == 1) ? false : true )
//bool USBHostGenericDeviceDetached( uint8_t deviceAddress );


/****************************************************************************
  Function:
    bool USBHostGenericGetDeviceAddress(GENERIC_DEVICE_ID *pDevID)

  Description:
    This interface is used get the address of a specific generic device on
    the USB.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    GENERIC_DEVICE_ID* pDevID  - Pointer to a structure containing the Device ID Info (VID,
                    		 PID, serial number, and device address).

  Return Values:
    true    - The device is connected
    false   - The device is not connected.

  Example:
    <code>
    GENERIC_DEVICE_ID   deviceID;
    uint16_t                serialNumber[] = { '1', '2', '3', '4', '5', '6' };
    uint8_t                deviceAddress;

    deviceID.vid          = 0x1234;
    deviceID.pid          = 0x5678;
    deviceID.serialNumber = &serialNumber;

    if (USBHostGenericGetDeviceAddress(&deviceID))
    {
        deviceAddress = deviceID.deviceAddress;
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

bool USBHostGenericGetDeviceAddress(GENERIC_DEVICE_ID *pDevID);


/****************************************************************************
  Function:
    uint32_t USBHostGenericGetRxLength( uint8_t deviceAddress )

  Description:
    This function retrieves the number of bytes copied to user's buffer by
    the most recent call to the USBHostGenericRead() function.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    uint8_t deviceAddress	- USB Address of the device

  Returns:
    Returns the number of bytes most recently received from the Generic
    device with address deviceAddress.

  Remarks:
    This function can only be called once per transfer.  Subsequent calls will
    return zero until new data has been received.
  ***************************************************************************/

#define USBHostGenericGetRxLength(a) ( (API_VALID(a)) ? gc_DevData.rxLength : 0 )
//uint32_t USBHostGenericGetRxLength( uint8_t deviceAddress );


/****************************************************************************
  Function:
    void USBHostGenericRead( uint8_t deviceAddress, uint8_t *buffer, uint32_t length )

  Description:
    Use this routine to receive from the device and store it into memory.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    uint8_t deviceAddress  - USB Address of the device.
    uint8_t *buffer        - Pointer to the data buffer
    uint32_t length        - Number of bytes to be transferred

  Return Values:
    USB_SUCCESS         - The Read was started successfully
    (USB error code)    - The Read was not started.  See USBHostRead() for
                            a list of errors.

  Example:
    <code>
    if (!USBHostGenericRxIsBusy( deviceAddress ))
    {
        USBHostGenericRead( deviceAddress, &buffer, sizeof(buffer) );
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

uint8_t USBHostGenericRead( uint8_t deviceAddress, void *buffer, uint32_t length);
/* Macro Implementation:
#define USBHostGenericRead(a,b,l)                                                 \
        ( API_VALID(deviceAddress) ? USBHostRead((a),USB_GENERIC_EP,(uint8_t *)(b),(l)) : \
                              USB_INVALID_STATE )
*/


/****************************************************************************
  Function:
    bool USBHostGenericRxIsBusy( uint8_t deviceAddress )

  Summary:
    This interface is used to check if the client driver is currently busy
    receiving data from the device.

  Description:
    This interface is used to check if the client driver is currently busy
    receiving data from the device.  This function is intended for use with
    transfer events.  With polling, the function USBHostGenericRxIsComplete()
    should be used.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    uint8_t deviceAddress     - USB Address of the device

  Return Values:
    true    - The device is receiving data or an invalid deviceAddress is
                given.
    false   - The device is not receiving data

  Example:
    <code>
    if (!USBHostGenericRxIsBusy( deviceAddress ))
    {
        USBHostGenericRead( deviceAddress, &buffer, sizeof( buffer ) );
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

#define USBHostGenericRxIsBusy(a) ( (API_VALID(a)) ? ((gc_DevData.flags.rxBusy == 1) ? true : false) : true )
//bool USBHostGenericRxIsBusy( uint8_t deviceAddress );


/****************************************************************************
  Function:
    bool USBHostGenericRxIsComplete( uint8_t deviceAddress, uint8_t *errorCode,
                uint32_t *byteCount )

  Summary:
    This routine indicates whether or not the last IN transfer is complete.

  Description:
    This routine indicates whether or not the last IN transfer is complete.
    If it is, then the returned errorCode and byteCount are valid, and
    reflect the error code and the number of bytes received.

    This function is intended for use with polling.  With transfer events,
    the function USBHostGenericRxIsBusy() should be used.

  Preconditions:
    None

  Parameters:
    uint8_t deviceAddress  - Address of the attached peripheral
    uint8_t *errorCode     - Error code of the last transfer, if complete
    uint32_t *byteCount    - Bytes transferred during the last transfer, if
                            complete

  Return Values:
    true    - The IN transfer is complete.  errorCode and byteCount are valid.
    false   - The IN transfer is not complete.  errorCode and byteCount are
                invalid.

  Remarks:
    None
  ***************************************************************************/

bool USBHostGenericRxIsComplete( uint8_t deviceAddress,
                                    uint8_t *errorCode, uint32_t *byteCount );


/****************************************************************************
  Function:
    void USBHostGenericTasks( void )

  Summary:
    This routine is used if transfer events are not utilized. It monitors the
    host status and updates the transmit and receive flags.

  Description:
    This routine is used if transfer events are not utilized. It monitors the
    host status and updates the transmit and receive flags.  If serial
    numbers are supported, then this routine handles the reception of the
    serial number.

  Preconditions:
    None

  Parameters:
    None

  Returns:
    None

  Remarks:
    This function is compiled only if USB_ENABLE_TRANSFER_EVENT is not
    defined.
  ***************************************************************************/

#ifndef USB_ENABLE_TRANSFER_EVENT
void USBHostGenericTasks( void );
#endif


/****************************************************************************
  Function:
    bool USBHostGenericTxIsBusy( uint8_t deviceAddress )

  Summary:
    This interface is used to check if the client driver is currently busy
    transmitting data to the device.

  Description:
    This interface is used to check if the client driver is currently busy
    transmitting data to the device.  This function is intended for use with
    transfer events.  With polling, the function USBHostGenericTxIsComplete()
    should be used.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    uint8_t deviceAddress	- USB Address of the device

  Return Values:
    true    - The device is transmitting data or an invalid deviceAddress
                is given.
    false   - The device is not transmitting data

  Example:
    <code>
    if (!USBHostGenericTxIsBusy( deviceAddress ) )
    {
        USBHostGenericWrite( deviceAddress, &buffer, sizeof( buffer ) );
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

#define USBHostGenericTxIsBusy(a) ( (API_VALID(a)) ? ((gc_DevData.flags.txBusy == 1) ? true : false) : true )
//bool USBHostGenericTxIsBusy( uint8_t deviceAddress );


/****************************************************************************
  Function:
    bool USBHostGenericTxIsComplete( uint8_t deviceAddress, uint8_t *errorCode )

  Summary:
    This routine indicates whether or not the last OUT transfer is complete.

  Description:
    This routine indicates whether or not the last OUT transfer is complete.
    If it is, then the returned errorCode is valid, and reflect the error
    code of the transfer.

    This function is intended for use with polling.  With transfer events,
    the function USBHostGenericTxIsBusy() should be used.

  Preconditions:
    None

  Parameters:
    uint8_t deviceAddress  - Address of the attached peripheral
    uint8_t *errorCode     - Error code of the last transfer, if complete

  Return Values:
    true    - The OUT transfer is complete.  errorCode is valid.
    false   - The OUT transfer is not complete.  errorCode is invalid.

  Remarks:
    None
  ***************************************************************************/

bool USBHostGenericTxIsComplete( uint8_t deviceAddress, uint8_t *errorCode );


/****************************************************************************
  Function:
    void USBHostGenericWrite( uint8_t deviceAddress, uint8_t *buffer, uint32_t length )

  Description:
    Use this routine to transmit data from memory to the device.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    uint8_t deviceAddress   - USB Address of the device.
    uint8_t *buffer         - Pointer to the data buffer
    uint32_t length         - Number of bytes to be transferred

  Return Values:
    USB_SUCCESS         - The Write was started successfully
    (USB error code)    - The Write was not started.  See USBHostWrite() for
                            a list of errors.

  Example:
    <code>
    if (!USBHostGenericTxIsBusy( deviceAddress ))
    {
        USBHostGenericWrite( deviceAddress, &buffer, sizeof(buffer) );
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

uint8_t USBHostGenericWrite( uint8_t deviceAddress, void *buffer, uint32_t length);
/* Macro Implementation:
#define USBHostGenericWrite(a,b,l)                                                 \
        ( API_VALID(deviceAddress) ? USBHostWrite((a),USB_GENERIC_EP,(uint8_t *)(b),(l)) : \
                              USB_INVALID_STATE )
*/


/*************************************************************************
 * EOF usb_client_generic.h
 */

#endif
