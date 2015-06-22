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
#include "system_config.h"
#include "fileio.h"
#include "usb.h"
#include "usb_host_msd.h"
#include "usb_host_msd_scsi.h"

//#define DEBUG_MODE
#if defined(DEBUG_MODE)
    #include "uart2.h"
#endif

#define PERFORM_TEST_UNIT_READY

// *****************************************************************************
// *****************************************************************************
// Section: Constants
// *****************************************************************************
// *****************************************************************************

#define FAT_GOOD_SIGN_0             0x55        // The value in the Signature0 field of a valid partition.
#define FAT_GOOD_SIGN_1             0xAA        // The value in the Signature1 field of a valid partition.
#define FO_MBR                      0L          // Master Boot Record sector LBA
#define FUA_ALLOW_CACHE             0x00        // Force Unit Access, allow cache use
#define INITIALIZATION_ATTEMPTS     100         // How many times to try to initialize the media before failing
#define RDPROTECT_NORMAL            0x00        // Normal Read Protect behavior.
#define WRPROTECT_NORMAL            0x00        // Normal Write Protect behavior.


//******************************************************************************
//******************************************************************************
// Section: Local Prototypes and Macros
//******************************************************************************
//******************************************************************************

#if !defined( USBTasks )
    #define USBTasks()                  \
        {                               \
            USBHostTasks();             \
            USBHostMSDTasks();          \
        }
#endif

#if defined( PERFORM_TEST_UNIT_READY )
    bool    _USBHostMSDSCSI_TestUnitReady( uint8_t * address );
#endif

static bool USBHostMSDSCSIRequestSense(uint8_t * address);

//******************************************************************************
//******************************************************************************
// Section: SCSI MSD Host Global Variables
//******************************************************************************
//******************************************************************************

//******************************************************************************
// Section: Internal Global Variables
//******************************************************************************

static FILEIO_MEDIA_INFORMATION   mediaInformation;   // Information about the attached media.

// *****************************************************************************
// *****************************************************************************
// Section: MSD Host Stack Callback Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    bool USBHostMSDSCSIInitialize( uint8_t address, uint32_t flags, uint8_t clientDriverID )

  Description:
    This function is called when a USB Mass Storage device is being
    enumerated.

  Precondition:
    None

  Parameters:
    uint8_t address    -   Address of the new device
    uint32_t flags     -   Initialization flags
    uint8_t clientDriverID - ID for this layer.  Not used by the media interface layer.

  Return Values:
    true    -   We can support the device.
    false   -   We cannot support the device.

  Remarks:
    None
  ***************************************************************************/

bool USBHostMSDSCSIInitialize( uint8_t address, uint32_t flags, uint8_t clientDriverID )
{
    #ifdef DEBUG_MODE
        UART2PrintString( "SCSI: Device attached.\r\n" );
    #endif
    return true;
}


/****************************************************************************
  Function:
    bool USBHostMSDSCSIEventHandler( uint8_t address, USB_EVENT event,
                        void *data, uint32_t size )

  Description:
    This function is called when various events occur in the USB Host Mass
    Storage client driver.

  Precondition:
    The device has been initialized.

  Parameters:
    uint8_t address    -   Address of the device
    USB_EVENT event -   Event that has occurred
    void *data      -   Pointer to data pertinent to the event
    uint32_t size      -   Size of the data

  Return Values:
    true    -   Event was handled
    false   -   Event was not handled

  Remarks:
    None
  ***************************************************************************/

bool USBHostMSDSCSIEventHandler( uint8_t address, USB_EVENT event, void *data, uint32_t size )
{
    switch( (int) event )
    {
        case EVENT_MSD_NONE:
        case EVENT_MSD_TRANSFER:                 // A MSD transfer has completed
            return true;
            break;

        case EVENT_MSD_RESET:
            #ifdef DEBUG_MODE
                UART2PrintString( "SCSI: MSD Reset performed.\r\n" );
            #endif
            return true;
            break;

        case EVENT_MSD_MAX_LUN:
            {
                uint8_t i;
                #ifdef DEBUG_MODE
                    UART2PrintString( "SCSI: Max LUN set.\r\n" );
                #endif
                mediaInformation.maxLUN                     = *((uint8_t *)data);
                mediaInformation.validityFlags.bits.maxLUN  = 1;

                for (i = 0; i < mediaInformation.maxLUN + 1; i++)
                {
                    USB_HOST_APP_EVENT_HANDLER (address, EVENT_MSD_ATTACH, &i, 1);
                }
            }
            return true;
            break;

        case EVENT_DETACH:                      // USB cable has been detached (data: uint8_t, address of device)
            #ifdef DEBUG_MODE
                UART2PrintString( "SCSI: Device detached.\r\n" );
            #endif
            address                           = 0;
            mediaInformation.validityFlags.value    = 0;
            return true;
            break;

        default:
            return false;
            break;
    }
    return false;
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Callable Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    uint8_t USBHostMSDSCSIMediaDetect( void )

  Description:
    This function determines if a mass storage device is attached and ready
    to use.

  Precondition:
    None

  Parameters:
    None - None

  Return Values:
    true    -   MSD present and ready
    false   -   MSD not present or not ready

  Remarks:
    Since this will often be called in a loop while waiting for a device,
    we need to make sure that USB tasks are executed.
  ***************************************************************************/

uint8_t USBHostMSDSCSIMediaDetect( uint8_t * address)
{
    if (USBHostMSDDeviceStatus( *(uint8_t *)address ) == USB_MSD_NORMAL_RUNNING)
    {
        return true;
    }

    return false;
}


/****************************************************************************
  Function:
    MEDIA_INFORMATION * USBHostMSDSCSIMediaInitialize( void )

  Description:
    This function initializes the media.

  Precondition:
    None

  Parameters:
    None - None

  Returns:
    The function returns a pointer to the MEDIA_INFORMATION structure.  The
    errorCode member may contain the following values:
        * MEDIA_NO_ERROR - The media initialized successfully, and the
                sector size should be valid (confirm using the validityFlags
                bit).
        * MEDIA_DEVICE_NOT_PRESENT - The requested device is not attached.
        * MEDIA_CANNOT_INITIALIZE - Cannot initialize the media.

  Remarks:
    This function performs the following SCSI commands:
                        * READ CAPACITY 10
                        * REQUEST SENSE

    The READ CAPACITY 10 command block is as follows:

    <code>
        Byte/Bit    7       6       5       4       3       2       1       0
           0                    Operation Code (0x25)
           1        [                      Reserved                         ]
           2        [ (MSB)
           3                        Logical Block Address
           4
           5                                                          (LSB) ]
           6        [                      Reserved
           7                                                                ]
           8        [                      Reserved                 ] [ PMI ]
           9        [                    Control                            ]
    </code>

    The REQUEST SENSE command block is as follows:

    <code>
        Byte/Bit    7       6       5       4       3       2       1       0
           0                    Operation Code (0x02)
           1        [                      Reserved                 ] [ DESC]
           2        [                      Reserved
           3                                                                ]
           4        [                  Allocation Length                    ]
           5        [                    Control                            ]
    </code>
  ***************************************************************************/

FILEIO_MEDIA_INFORMATION * USBHostMSDSCSIMediaInitialize( uint8_t * address )
{
    uint8_t        attempts;
    uint32_t       byteCount;
    uint8_t        commandBlock[10];
    uint8_t        errorCode;
    uint8_t        inquiryData[36];

    // Make sure the device is still attached.
    if (*address == 0)
    {
        mediaInformation.errorCode = MEDIA_DEVICE_NOT_PRESENT;
        return &mediaInformation;
    }

    attempts = INITIALIZATION_ATTEMPTS;
    while (attempts != 0)
    {
        attempts --;

        #ifdef DEBUG_MODE
            UART2PrintString( "SCSI: READ CAPACITY 10..." );
        #endif

        // Fill in the command block with the READ CAPACITY 10 parameters.
        commandBlock[0] = 0x25;     // Operation Code
        commandBlock[1] = 0;        //
        commandBlock[2] = 0;        //
        commandBlock[3] = 0;        //
        commandBlock[4] = 0;        //
        commandBlock[5] = 0;        //
        commandBlock[6] = 0;        //
        commandBlock[7] = 0;        //
        commandBlock[8] = 0;        //
        commandBlock[9] = 0x00;     // Control

        errorCode = USBHostMSDRead( *address, 0, commandBlock, 10, inquiryData, 8 );
        #ifdef DEBUG_MODE
            UART2PutHex( errorCode ) ;
            UART2PutChar( ' ' );
        #endif

        if (!errorCode)
        {
            while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
            {
                USBTasks();
            }
        }

        if (!errorCode)
        {
            #ifdef DEBUG_MODE
                UART2PutHex( errorCode ) ;
                UART2PrintString( "\r\n" );
            #endif

            // Determine sector size.
            #ifdef DEBUG_MODE
                UART2PrintString( "SCSI: Sector size:" );
                UART2PutChar( inquiryData[7] + '0' );
                UART2PutChar( inquiryData[6] + '0' );
                UART2PutChar( inquiryData[5] + '0' );
                UART2PutChar( inquiryData[4] + '0' );
                UART2PrintString( "\r\n" );
            #endif
            mediaInformation.sectorSize                     = (inquiryData[7] << 12) + (inquiryData[6] << 8) + (inquiryData[5] << 4) + (inquiryData[4]);
            mediaInformation.validityFlags.bits.sectorSize  = 1;

            mediaInformation.errorCode = MEDIA_NO_ERROR;
            return &mediaInformation;
        }
        else
        {
            // Perform a Request Sense to try to clear the stall.
            #ifdef DEBUG_MODE
                UART2PrintString( "SCSI: REQUEST SENSE..." );
            #endif

            // Fill in the command block with the REQUEST SENSE parameters.
            commandBlock[0] = 0x03;     // Operation Code
            commandBlock[1] = 0;        //
            commandBlock[2] = 0;        //
            commandBlock[3] = 0;        //
            commandBlock[4] = 18;       // Allocation length
            commandBlock[5] = 0;        // Control

            errorCode = USBHostMSDRead( *address, 0, commandBlock, 6, inquiryData, 18 );
            #ifdef DEBUG_MODE
                UART2PutHex( errorCode ) ;
                UART2PutChar( ' ' );
            #endif

            if (!errorCode)
            {
                while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
                {
                    USBTasks();
                }
            }
        }
    }

    attempts = INITIALIZATION_ATTEMPTS;
    while (attempts != 0)
    {
        attempts --;

        // Perform a Request Sense to try to clear the stall.
        #ifdef DEBUG_MODE
            UART2PrintString( "SCSI: REQUEST SENSE..." );
        #endif

        // Fill in the command block with the REQUEST SENSE parameters.
        commandBlock[0] = 0x03;     // Operation Code
        commandBlock[1] = 0;        //
        commandBlock[2] = 0;        //
        commandBlock[3] = 0;        //
        commandBlock[4] = 18;       // Allocation length
        commandBlock[5] = 0;        // Control

        errorCode = USBHostMSDRead( *address, 0, commandBlock, 6, inquiryData, 18 );
        #ifdef DEBUG_MODE
            UART2PutHex( errorCode ) ;
            UART2PutChar( ' ' );
        #endif

        if (!errorCode)
        {
            while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
            {
                USBTasks();
            }
        }


        _USBHostMSDSCSI_TestUnitReady(address);


        // Perform a Request Sense to try to clear the stall.
        #ifdef DEBUG_MODE
            UART2PrintString( "SCSI: REQUEST SENSE..." );
        #endif

        // Fill in the command block with the REQUEST SENSE parameters.
        commandBlock[0] = 0x03;     // Operation Code
        commandBlock[1] = 0;        //
        commandBlock[2] = 0;        //
        commandBlock[3] = 0;        //
        commandBlock[4] = 18;       // Allocation length
        commandBlock[5] = 0;        // Control

        errorCode = USBHostMSDRead( *address, 0, commandBlock, 6, inquiryData, 18 );
        #ifdef DEBUG_MODE
            UART2PutHex( errorCode ) ;
            UART2PutChar( ' ' );
        #endif

        if (!errorCode)
        {
            while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
            {
                USBTasks();
            }
        }

        #ifdef DEBUG_MODE
            UART2PrintString( "SCSI: READ CAPACITY 10..." );
        #endif

        // Fill in the command block with the READ CAPACITY 10 parameters.
        commandBlock[0] = 0x25;     // Operation Code
        commandBlock[1] = 0;        //
        commandBlock[2] = 0;        //
        commandBlock[3] = 0;        //
        commandBlock[4] = 0;        //
        commandBlock[5] = 0;        //
        commandBlock[6] = 0;        //
        commandBlock[7] = 0;        //
        commandBlock[8] = 0;        //
        commandBlock[9] = 0x00;     // Control

        errorCode = USBHostMSDRead( *address, 0, commandBlock, 10, inquiryData, 8 );
        #ifdef DEBUG_MODE
            UART2PutHex( errorCode ) ;
            UART2PutChar( ' ' );
        #endif

        if (!errorCode)
        {
            while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
            {
                USBTasks();
            }
        }

        if (!errorCode)
        {
            #ifdef DEBUG_MODE
                UART2PutHex( errorCode ) ;
                UART2PrintString( "\r\n" );
            #endif

            // Determine sector size.
            #ifdef DEBUG_MODE
                UART2PrintString( "SCSI: Sector size:" );
                UART2PutChar( inquiryData[7] + '0' );
                UART2PutChar( inquiryData[6] + '0' );
                UART2PutChar( inquiryData[5] + '0' );
                UART2PutChar( inquiryData[4] + '0' );
                UART2PrintString( "\r\n" );
            #endif
            mediaInformation.sectorSize                     = (inquiryData[7] << 12) + (inquiryData[6] << 8) + (inquiryData[5] << 4) + (inquiryData[4]);
            mediaInformation.validityFlags.bits.sectorSize  = 1;

            mediaInformation.errorCode = MEDIA_NO_ERROR;
            return &mediaInformation;
        }
        else
        {
            // Perform a Request Sense to try to clear the stall.
            #ifdef DEBUG_MODE
                UART2PrintString( "SCSI: REQUEST SENSE..." );
            #endif

            // Fill in the command block with the REQUEST SENSE parameters.
            commandBlock[0] = 0x03;     // Operation Code
            commandBlock[1] = 0;        //
            commandBlock[2] = 0;        //
            commandBlock[3] = 0;        //
            commandBlock[4] = 18;       // Allocation length
            commandBlock[5] = 0;        // Control

            errorCode = USBHostMSDRead( *address, 0, commandBlock, 6, inquiryData, 18 );
            #ifdef DEBUG_MODE
                UART2PutHex( errorCode ) ;
                UART2PutChar( ' ' );
            #endif

            if (!errorCode)
            {
                while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
                {
                    USBTasks();
                }
            }
        }
    }

    mediaInformation.errorCode = MEDIA_CANNOT_INITIALIZE;
    return &mediaInformation;

}

/****************************************************************************
  Function:
    bool USBHostMSDSCSIMediaDeinitialize( void * mediaConfig )

  Summary:
    This function deinitializes the media.

  Description:
    This function deinitializes the media.

  Precondition:
    None

  Parameters:
    mediaConfig - the media configuration information

  Return Values:
    true - successful
    false - otherwise

  Remarks:
    None
  ***************************************************************************/
bool USBHostMSDSCSIMediaDeinitialize(void *mediaConfig)
{
    return true;
}

/****************************************************************************
  Function:
    bool USBHostMSDSCSIMediaReset( void  )

  Summary:
    This function resets the media.

  Description:
    This function resets the media.  It is called if an operation returns an
    error.  Or the application can call it.

  Precondition:
    None

  Parameters:
    None - None

  Return Values:
    USB_SUCCESS                 - Reset successful
    USB_MSD_DEVICE_NOT_FOUND    - No device with specified address
    USB_ILLEGAL_REQUEST         - Device is in an illegal USB state
                                  for reset

  Remarks:
    None
  ***************************************************************************/

uint8_t USBHostMSDSCSIMediaReset( uint8_t * address )
{
    uint32_t   byteCount;
    uint8_t    errorCode;

    errorCode = USBHostMSDResetDevice( *address );
    if (errorCode)
    {
        return errorCode;
    }

    do
    {
        USBTasks();
        errorCode = USBHostMSDDeviceStatus( *address );
    } while (errorCode == USB_MSD_RESETTING_DEVICE);


    if (USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
    {
        return errorCode;
    }

    return USB_MSD_RESET_ERROR;
}


/****************************************************************************
  Function:
    uint8_t USBHostMSDSCSISectorRead( uint32_t sectorAddress, uint8_t *dataBuffer)

  Summary:
    This function reads one sector.

  Description:
    This function uses the SCSI command READ10 to read one sector.  The size
    of the sector was determined in the USBHostMSDSCSIMediaInitialize()
    function.  The data is stored in the application buffer.

  Precondition:
    None

  Parameters:
    uint32_t   sectorAddress   - address of sector to read
    uint8_t    *dataBuffer     - buffer to store data

  Return Values:
    true    - read performed successfully
    false   - read was not successful

  Remarks:
    The READ10 command block is as follows:

    <code>
        Byte/Bit    7       6       5       4       3       2       1       0
           0                    Operation Code (0x28)
           1        [    RDPROTECT      ]  DPO     FUA      -     FUA_NV    -
           2        [ (MSB)
           3                        Logical Block Address
           4
           5                                                          (LSB) ]
           6        [         -         ][          Group Number            ]
           7        [ (MSB)         Transfer Length
           8                                                          (LSB) ]
           9        [                    Control                            ]
    </code>
  ***************************************************************************/

uint8_t USBHostMSDSCSISectorRead(uint8_t * address, uint32_t sectorAddress, uint8_t *dataBuffer )
{
    uint32_t   byteCount;
    uint8_t    commandBlock[10];
    uint8_t    errorCode;
    uint8_t attempts = 5;

    if (*address == 0)
    {
        return false;       // USB_MSD_DEVICE_NOT_FOUND;
    }

    while(attempts--)
    {
        // Fill in the command block with the READ10 parameters.
        commandBlock[0] = 0x28;     // Operation code
        commandBlock[1] = RDPROTECT_NORMAL | FUA_ALLOW_CACHE;
        commandBlock[2] = (uint8_t) (sectorAddress >> 24);     // Big endian!
        commandBlock[3] = (uint8_t) (sectorAddress >> 16);
        commandBlock[4] = (uint8_t) (sectorAddress >> 8);
        commandBlock[5] = (uint8_t) (sectorAddress);
        commandBlock[6] = 0x00;     // Group Number
        commandBlock[7] = 0x00;     // Number of blocks - Big endian!
        commandBlock[8] = 0x01;
        commandBlock[9] = 0x00;     // Control

        // Currently using LUN=0.  When the File System supports multiple LUN's, this will change.
        errorCode = USBHostMSDRead( *address, 0, commandBlock, 10, dataBuffer, mediaInformation.sectorSize );

        if (!errorCode)
        {
            while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
            {
                USBTasks();
            }
        }

        switch(errorCode)
        {
            case USB_MSD_COMMAND_FAILED:
                USBHostMSDSCSIRequestSense(address);
                break;

            case USB_SUCCESS:
                return true;

            default:
                return false;
        }
    }

    return false;
}

static bool USBHostMSDSCSIRequestSense(uint8_t * address)
{
    uint32_t   byteCount;
    uint8_t    commandBlock[10];
    uint8_t    errorCode;
    uint8_t buffer[18];

    if (*address == 0)
    {
        return false;       // USB_MSD_DEVICE_NOT_FOUND;
    }

    // Fill in the command block with the READ10 parameters.
    commandBlock[0] = 0x03;     // Request Sense
    commandBlock[1] = RDPROTECT_NORMAL | FUA_ALLOW_CACHE;
    commandBlock[2] = 0;     
    commandBlock[3] = 0;
    commandBlock[4] = 0x12;
    commandBlock[5] = 0;
    commandBlock[6] = 0x00;     // Group Number
    commandBlock[7] = 0x00;     // Number of blocks - Big endian!
    commandBlock[8] = 0x00;
    commandBlock[9] = 0x00;     // Control

    // Currently using LUN=0.  When the File System supports multiple LUN's, this will change.
    errorCode = USBHostMSDRead( *address, 0, commandBlock, 10, buffer, 18 );

    if (!errorCode)
    {
        while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
        {
            USBTasks();
        }
    }

    if (!errorCode)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************
  Function:
    uint8_t USBHostMSDSCSISectorWrite( uint32_t sectorAddress, uint8_t *dataBuffer, uint8_t allowWriteToZero )

  Summary:
    This function writes one sector.

  Description:
    This function uses the SCSI command WRITE10 to write one sector.  The size
    of the sector was determined in the USBHostMSDSCSIMediaInitialize()
    function.  The data is read from the application buffer.

  Precondition:
    None

  Parameters:
    uint32_t   sectorAddress   - address of sector to write
    uint8_t    *dataBuffer     - buffer with application data
    uint8_t    allowWriteToZero- If a write to sector 0 is allowed.

  Return Values:
    true    - write performed successfully
    false   - write was not successful

  Remarks:
    To follow convention, this function blocks until the write is complete.

    The WRITE10 command block is as follows:

    <code>
        Byte/Bit    7       6       5       4       3       2       1       0
           0                    Operation Code (0x2A)
           1        [    WRPROTECT      ]  DPO     FUA      -     FUA_NV    -
           2        [ (MSB)
           3                        Logical Block Address
           4
           5                                                          (LSB) ]
           6        [         -         ][          Group Number            ]
           7        [ (MSB)         Transfer Length
           8                                                          (LSB) ]
           9        [                    Control                            ]
    </code>
  ***************************************************************************/

uint8_t USBHostMSDSCSISectorWrite(uint8_t * address, uint32_t sectorAddress, uint8_t *dataBuffer, uint8_t allowWriteToZero )
{
    uint32_t   byteCount;
    uint8_t    commandBlock[10];
    uint8_t    errorCode;
    uint8_t attempts = 5;

    if (*address == 0)
    {
        return false;   //USB_MSD_DEVICE_NOT_FOUND;
    }

    if ((sectorAddress == 0) && (allowWriteToZero == false))
    {
        return false;
    }

    while(attempts--)
    {
        // Fill in the command block with the WRITE 10 parameters.
        commandBlock[0] = 0x2A;     // Operation code
        commandBlock[1] = WRPROTECT_NORMAL | FUA_ALLOW_CACHE;
        commandBlock[2] = (uint8_t) (sectorAddress >> 24);     // Big endian!
        commandBlock[3] = (uint8_t) (sectorAddress >> 16);
        commandBlock[4] = (uint8_t) (sectorAddress >> 8);
        commandBlock[5] = (uint8_t) (sectorAddress);
        commandBlock[6] = 0x00;     // Group Number
        commandBlock[7] = 0x00;     // Number of blocks - Big endian!
        commandBlock[8] = 0x01;
        commandBlock[9] = 0x00;     // Control

        // Currently using LUN=0.  When the File System supports multiple LUN's, this will change.
        errorCode = USBHostMSDWrite( *address, 0, commandBlock, 10, dataBuffer, mediaInformation.sectorSize );

        if (!errorCode)
        {
            while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
            {
                USBTasks();
            }
        }

        switch(errorCode)
        {
            case USB_MSD_COMMAND_FAILED:
                USBHostMSDSCSIRequestSense(address);
                break;

            case USB_SUCCESS:
                return true;

            default:
                return false;
        }
    }

    return false;
}


/****************************************************************************
  Function:
    uint8_t USBHostMSDSCSIWriteProtectState( void )

  Description:
    This function returns the write protect status of the device.

  Precondition:
    None

  Parameters:
    None - None

  Return Values:
    0 - not write protected


  Remarks:
    None
  ***************************************************************************/

uint8_t    USBHostMSDSCSIWriteProtectState( uint8_t * address)
{
    return 0;
}


// *****************************************************************************
// *****************************************************************************
// Section: Internal Functions
// *****************************************************************************
// *****************************************************************************


/*******************************************************************************
  Function:
    bool _USBHostMSDSCSI_TestUnitReady( void )

  Precondition:
    None

  Overview:
    This function sends the TEST UNIT READY SCSI command

  Parameters:
    None - None

  Return Values:
    true    - Command completed without error
    false   - Error while performing command

  Remarks:
    The format of the TEST UNIT READY command is as follows:

    <code>
        Byte/Bit    7       6       5       4       3       2       1       0
           0                    Operation Code (0x00)
           1        [
           2                               Reserved
           3
           4                                                                ]
           5        [                    Control                            ]
    </code>
  ***************************************************************************/

#ifdef PERFORM_TEST_UNIT_READY
bool _USBHostMSDSCSI_TestUnitReady( uint8_t * address)
{
    uint32_t       byteCount;
    uint8_t        commandBlock[10];
    uint8_t        errorCode;
    uint8_t        inquiryData[36];
    uint8_t        unitReadyCount;

    // Issue a TEST UNIT READY
    #ifdef DEBUG_MODE
        UART2PrintString( "SCSI: TEST UNIT READY..." );
    #endif

    unitReadyCount = 0;
    while (unitReadyCount < 5)
    {
        unitReadyCount ++;

        // Fill in the command block with the TEST UNIT READY parameters.
        commandBlock[0] = 0x00;     // Operation Code
        commandBlock[1] = 0;        // Reserved
        commandBlock[2] = 0;        // Reserved
        commandBlock[3] = 0;        // Reserved
        commandBlock[4] = 0;        // Reserved
        commandBlock[5] = 0x00;     // Control

        errorCode = USBHostMSDRead( *address, 0, commandBlock, 6, inquiryData, 0 );
        #ifdef DEBUG_MODE
            UART2PutHex( errorCode ) ;
            UART2PutChar( ' ' );
        #endif

        if (!errorCode)
        {
            while (!USBHostMSDTransferIsComplete( *address, &errorCode, &byteCount ))
            {
                USBTasks();
            }
        }
        #ifdef DEBUG_MODE
            UART2PutHex( errorCode ) ;
            UART2PrintString( "\r\n" );
        #endif
        if (!errorCode)
        {
            return true;
        }
    }

    return false;
}
#endif

