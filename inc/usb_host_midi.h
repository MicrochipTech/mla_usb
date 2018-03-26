// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright 2017 Microchip Technology Inc. (www.microchip.com)

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

#ifndef __USBHOSTMIDI_H__
#define __USBHOSTMIDI_H__

#include <stdint.h>
#include <stdbool.h>

//DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Constants
// *****************************************************************************
// *****************************************************************************

// The Audio Class, MIDI Subclass, and MIDI Protocol
#define AUDIO_CLASS     1
#define MIDI_SUB_CLASS  3
#define MIDI_PROTOCOL   0

// This is the default MIDI Client Driver endpoint number.
#ifndef USB_MIDI_EP
    #define USB_MIDI_EP       1
#endif

//Each USB-MIDI packet is supposed
//to be exactly 32-bits (4 uint8_ts long)
#define USB_MIDI_PACKET_LENGTH (uint8_t)4

//State machine definitions, when receiving a MIDI command on the MIDI In jack
#define STATE_WAIT_STATUS_uint8_t  0x00
#define STATE_WAITING_uint8_tS     0x01

/* Code Index Number (CIN) values */
/*   Table 4-1 of midi10.pdf      */
#define MIDI_CIN_MISC_FUNCTION_RESERVED         0x0
#define MIDI_CIN_CABLE_EVENTS_RESERVED          0x1
#define MIDI_CIN_2_uint8_t_MESSAGE                 0x2
#define MIDI_CIN_MTC                            0x2
#define MIDI_CIN_SONG_SELECT                    0x2
#define MIDI_CIN_3_uint8_t_MESSAGE                 0x3
#define MIDI_CIN_SSP                            0x3
#define MIDI_CIN_SYSEX_START                    0x4
#define MIDI_CIN_SYSEX_CONTINUE                 0x4
#define MIDI_CIN_1_uint8_t_MESSAGE                 0x5
#define MIDI_CIN_SYSEX_ENDS_1                   0x5
#define MIDI_CIN_SYSEX_ENDS_2                   0x6
#define MIDI_CIN_SYSEX_ENDS_3                   0x7
#define MIDI_CIN_NOTE_OFF                       0x8
#define MIDI_CIN_NOTE_ON                        0x9
#define MIDI_CIN_POLY_KEY_PRESS                 0xA
#define MIDI_CIN_CONTROL_CHANGE                 0xB
#define MIDI_CIN_PROGRAM_CHANGE                 0xC
#define MIDI_CIN_CHANNEL_PREASURE               0xD
#define MIDI_CIN_PITCH_BEND_CHANGE              0xE
#define MIDI_CIN_SINGLE_uint8_t                    0xF

//Possible system command status uint8_ts
#define MIDI_STATUS_SYSEX_START                 0xF0
#define MIDI_STATUS_SYSEX_END                   0xF7
#define MIDI_STATUS_MTC_QFRAME                  0xF1
#define MIDI_STATUS_SONG_POSITION               0xF2
#define MIDI_STATUS_SONG_SELECT                 0xF3
#define MIDI_STATUS_TUNE_REQUEST                0xF6
#define MIDI_STATUS_MIDI_CLOCK                  0xF8
#define MIDI_STATUS_MIDI_TICK                   0xF9
#define MIDI_STATUS_MIDI_START                  0xFA
#define MIDI_STATUS_MIDI_STOP                   0xFC
#define MIDI_STATUS_MIDI_CONTINUE               0xFB
#define MIDI_STATUS_ACTIVE_SENSE                0xFE
#define MIDI_STATUS_RESET                       0xFF

// *****************************************************************************
// *****************************************************************************
// Section: USB MIDI Client Events
// *****************************************************************************
// *****************************************************************************

        // This is an optional offset for the values of the generated events.
        // If necessary, the application can use a non-zero offset for the
        // MIDI events to resolve conflicts in event number.
#ifndef EVENT_MIDI_OFFSET
#define EVENT_MIDI_OFFSET 0
#endif

        // This event indicates that a MIDI device has been attached.
        // When USB_HOST_APP_EVENT_HANDLER is called with this event, *data
        // points to a MIDI_DEVICE_ID structure, and size is the size of the
        // MIDI_DEVICE_ID structure.
#define EVENT_MIDI_ATTACH  (EVENT_AUDIO_BASE+EVENT_MIDI_OFFSET+0)

        // This event indicates that the specified device has been detached
        // from the USB.  When USB_HOST_APP_EVENT_HANDLER is called with this
        // event, *data points to a uint8_t that contains the device address, and
        // size is the size of a uint8_t.
#define EVENT_MIDI_DETACH  (EVENT_AUDIO_BASE+EVENT_MIDI_OFFSET+1)

        // This event indicates that a previous write/read request has completed.
        // These events are enabled if USB Embedded Host transfer events are
        // enabled (USB_ENABLE_TRANSFER_EVENT is defined).  When
        // USB_HOST_APP_EVENT_HANDLER is called with this event, *data points
        // to the buffer that completed transmission, and size is the actual
        // number of uint8_ts that were written to the device.
#define EVENT_MIDI_TRANSFER_DONE (EVENT_AUDIO_BASE+EVENT_MIDI_OFFSET+2)


// *****************************************************************************
// *****************************************************************************
// Section: USB Data Structures
// *****************************************************************************
// *****************************************************************************

typedef enum
{
    OUT = 0,
    IN = 1
}ENDPOINT_DIRECTION;    

typedef struct
{
    bool busy;                  // Driver busy transmitting or receiving data
    uint8_t endpointAddress;       // The endpoint number (1-15, 0 is reserved for control transfers) and direction (0x8X = input, 0x0X = output)
    uint16_t endpointSize;          // Size of the endpoint (we'll take any size <= 64)
} MIDI_ENDPOINT_DATA;           // MIDI endpoints

// *****************************************************************************
/* MIDI Device Information

This structure contains information about an attached device, including
status flags and device identification.
*/
typedef struct
{
    uint8_t deviceAddress;     // Address of the device on the USB
    uint8_t clientDriverID;    // ID to send when issuing a Device Request
    
    uint8_t numEndpoints;               // Number of OUT endpoints for this interface
    MIDI_ENDPOINT_DATA* endpoints;   // List of OUT endpoints
} MIDI_DEVICE;

// *****************************************************************************
/* MIDI Packet information

This structure contains information contained within a USB MIDI packet
*/
typedef union
{
    uint32_t Val;
    uint8_t v[4];
    union
    {
        struct
        {
            uint8_t CIN :4;
            uint8_t CN  :4;
            uint8_t MIDI_0;
            uint8_t MIDI_1;
            uint8_t MIDI_2;
        }; 
        struct
        {
            uint8_t CodeIndexNumber :4;
            uint8_t CableNumber     :4;
            uint8_t DATA_0;
            uint8_t DATA_1;
            uint8_t DATA_2;    
        };
    };
} USB_AUDIO_MIDI_PACKET;


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Host Stack Interface Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    bool USBHostMIDIInit ( uint8_t address, uint32_t flags, uint8_t clientDriverID )

  Summary:
    This function is called by the USB Embedded Host layer when a MIDI
    device attaches.

  Description:
    This routine is a call out from the USB Embedded Host layer to the USB
    MIDI client driver.  It is called when a MIDI device has been connected
    to the host.  Its purpose is to initialize and activate the USB
    MIDI client driver.

  Preconditions:
    The device has been configured.

  Parameters:
    uint8_t address        - Device's address on the bus
    uint32_t flags         - Initialization flags
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

bool USBHostMIDIInit ( uint8_t address, uint32_t flags, uint8_t clientDriverID );


/****************************************************************************
  Function:
    bool USBHostMIDIEventHandler ( uint8_t address, USB_EVENT event,
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

bool USBHostMIDIEventHandler ( uint8_t address, USB_EVENT event, void *data, uint32_t size );


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes and Macro Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    bool USBHostMIDIDeviceDetached( void* handle )

  Description:
    This interface is used to check if the device has been detached from the
    bus.

  Preconditions:
    None

  Parameters:
    void* handle - Pointer to a structure containing the Device Info

  Return Values:
    true    - The device has been detached, or an invalid handle is given.
    false   - The device is attached

  Example:
    <code>
    if (USBHostMIDIDeviceDetached( deviceAddress ))
    {
        // Handle detach
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

#define USBHostMIDIDeviceDetached(a) ( (((a)==NULL) ? false : true )
//bool USBHostMIDIDeviceDetached( void* handle );


/****************************************************************************
  Function:
    MIDI_ENDPOINT_DIRECTION USBHostMIDIEndpointDirection( void* handle, uint8_t endpointIndex )

  Description:
    This function retrieves the endpoint direction of the endpoint at
    endpointIndex for device that's located at handle.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    void* handle       - Pointer to a structure containing the Device Info
    uint8_t endpointIndex - the index of the endpoint whose direction is requested

  Returns:
    MIDI_ENDPOINT_DIRECTION - Returns the direction of the endpoint (IN or OUT)

  Remarks:
    None
  ***************************************************************************/
  
#define USBHostMIDIEndpointDirection(a,b) (((MIDI_DEVICE*)a)->endpoints[b].endpointAddress & 0x80) ? IN : OUT
//MIDI_ENDPOINT_DIRECTION USBHostMIDIEndpointDirection( void* handle, uint8_t endpointIndex );


/****************************************************************************
  Function:
    uint32_t USBHostMIDISizeOfEndpoint( void* handle, uint8_t endpointIndex )

  Description:
    This function retrieves the endpoint size of the endpoint at 
    endpointIndex for device that's located at handle.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    void* handle       - Pointer to a structure containing the Device Info
    uint8_t endpointIndex - the index of the endpoint whose direction is requested

  Returns:
    uint32_t - Returns the number of uint8_ts for the endpoint (4 - 64 uint8_ts per USB spec)

  Remarks:
    None
  ***************************************************************************/

#define USBHostMIDISizeOfEndpoint(a,b) ((MIDI_DEVICE*)a)->endpoints[b].endpointSize
//uint32_t USBHostMIDISizeOfEndpoint( void* handle, uint8_t endpointNumber );


/****************************************************************************
  Function:
    uint8_t USBHostMIDINumberOfEndpoints( void* handle )

  Description:
    This function retrieves the number of endpoints for the device that's
    located at handle.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    void* handle - Pointer to a structure containing the Device Info

  Returns:
    uint8_t - Returns the number of endpoints for the device at handle.

  Remarks:
    None
  ***************************************************************************/
  
#define USBHostMIDINumberOfEndpoints(a) ((MIDI_DEVICE*)a)->numEndpoints
//uint8_t USBHostMIDINumberOfEndpoints( void* handle );


/****************************************************************************
  Function:
    uint8_t USBHostMIDIRead( void* handle, uint8_t endpointIndex, void *buffer, uint16_t length)

  Description:
    This function will attempt to read length number of uint8_ts from the attached MIDI
    device located at handle, and will save the contents to ram located at buffer.

  Preconditions:
    The device must be connected and enumerated. The array at *buffer should have
    at least length number of uint8_ts available.

  Parameters:
    void* handle       - Pointer to a structure containing the Device Info
    uint8_t endpointIndex - the index of the endpoint whose direction is requested
    void* buffer       - Pointer to the data buffer
    uint16_t length        - Number of uint8_ts to be read

  Return Values:
    USB_SUCCESS         - The Read was started successfully
    (USB error code)    - The Read was not started.  See USBHostRead() for
                            a list of errors.

  Example:
    <code>
    if (!USBHostMIDITransferIsBusy( deviceHandle, currentEndpoint )
    {
        USBHostMIDIRead( deviceHandle, currentEndpoint, &buffer, sizeof(buffer));
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

uint8_t USBHostMIDIRead( void* handle, uint8_t endpointIndex, void *buffer, uint16_t length);


/****************************************************************************
  Function:
    bool USBHostMIDITransferIsBusy( void* handle, uint8_t endpointIndex )

  Summary:
    This interface is used to check if the client driver is currently busy
    transferring data over endponitIndex for the device at handle.

  Description:
    This interface is used to check if the client driver is currently busy
    receiving or sending data from the device at the endpoint with number
    endpointIndex.  This function is intended for use with transfer events.
    With polling, the function USBHostMIDITransferIsComplete()
    should be used.

  Preconditions:
    The device must be connected and enumerated.

  Parameters:
    void* handle       - Pointer to a structure containing the Device Info
    uint8_t endpointIndex - the index of the endpoint whose direction is requested

  Return Values:
    true    - The device is receiving data or an invalid handle is
                given.
    false   - The device is not receiving data

  Example:
    <code>
    if (!USBHostMIDITransferIsBusy( handle, endpointIndex ))
    {
        USBHostMIDIRead( handle, endpointIndex, &buffer, sizeof( buffer ) );
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

#define USBHostMIDITransferIsBusy(a,b) ((MIDI_DEVICE*)a)->endpoints[b].busy
//bool USBHostMIDITransferIsBusy( void* handle, uint8_t endpointIndex )


/****************************************************************************
  Function:
    bool USBHostMIDITransferIsComplete( void* handle, uint8_t endpointIndex,
                                        uint8_t *errorCode, uint32_t *uint8_tCount );

  Summary:
    This routine indicates whether or not the last transfer over endpointIndex
    is complete.

  Description:
    This routine indicates whether or not the last transfer over endpointIndex
    is complete. If it is, then the returned errorCode and uint8_tCount are valid,
    and reflect the error code and the number of uint8_ts received.

    This function is intended for use with polling.  With transfer events,
    the function USBHostMIDITransferIsBusy() should be used.

  Preconditions:
    None

  Parameters:
    void* handle        - Pointer to a structure containing the Device Info
    uint8_t endpointIndex  - index of endpoint in endpoints array
    uint8_t *errorCode     - Error code of the last transfer, if complete
    uint32_t *uint8_tCount    - uint8_ts transferred during the last transfer, if
                            complete

  Return Values:
    true    - The IN transfer is complete.  errorCode and uint8_tCount are valid.
    false   - The IN transfer is not complete.  errorCode and uint8_tCount are
                invalid.

  Remarks:
    None
  ***************************************************************************/

#ifndef USB_ENABLE_TRANSFER_EVENT
bool USBHostMIDITransferIsComplete( void* handle, uint8_t endpointIndex, uint8_t *errorCode, uint32_t *uint8_tCount );
#endif


/****************************************************************************
  Function:
    uint8_t USBHostMIDIWrite(void* handle, uint8_t endpointIndex, void *buffer, uint16_t length)

  Description:
    This function will attempt to write length number of uint8_ts from memory at location
    buffer to the attached MIDI device located at handle.

  Preconditions:
    The device must be connected and enumerated. The array at *buffer should have
    at least length number of uint8_ts available.

  Parameters:
    handle          - Pointer to a structure containing the Device Info
    endpointIndex   - Index of the endpoint
    buffer          - Pointer to the data being transferred
    length          - Size of the data being transferred

  Return Values:
    USB_SUCCESS         - The Write was started successfully
    (USB error code)    - The Write was not started.  See USBHostWrite() for
                            a list of errors.

  Example:
    <code>
    if (!USBHostMIDITransferIsBusy( deviceHandle, currentEndpoint )
    {
        USBHostMIDIWrite( deviceAddress, &buffer, sizeof(buffer) );
    }
    </code>

  Remarks:
    None
  ***************************************************************************/

uint8_t USBHostMIDIWrite(void* handle, uint8_t endpointIndex, void *buffer, uint16_t length);


/*************************************************************************
 * EOF usb_client_MIDI.h
 */

#endif
