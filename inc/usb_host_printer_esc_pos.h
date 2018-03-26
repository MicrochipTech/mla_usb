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


// *****************************************************************************
// *****************************************************************************
// Section: Constants
// *****************************************************************************
// *****************************************************************************

        // This is the string that the printer language support determination
        // routine will look for to determine if the printer supports this
        // printer language.  This string is case sensive.  See the function
        // USBHostPrinterLanguageESCPOSIsSupported() for more information
        // about using or changing this string.  Dynamic language determination
        // is not recommended when using POS printers.
#define LANGUAGE_ID_STRING_ESCPOS       "ESC"
        // These are the support flags that are set for this language.
#define LANGUAGE_SUPPORT_FLAGS_ESCPOS   USB_PRINTER_FUNCTION_SUPPORT_POS


/****************************************************************************
  Function:
    uint8_t USBHostPrinterLanguageESCPOS( uint8_t address,
        USB_PRINTER_COMMAND command, USB_DATA_POINTER data, uint32_t size, uint8_t transferFlags )

  Summary:
    This function executes printer commands for an ESC/POS printer.

  Description:
    This function executes printer commands for an ESC/POS printer.  When
    the application issues a printer command, the printer client driver
    determines what language to use to communicate with the printer, and
    transfers the command to that language support routine.  As much as
    possible, commands are designed to produce the same output regardless
    of what printer language is used.

    Not all printer commands support data from both RAM and ROM.  Unless
    otherwise noted, the data pointer is assumed to point to RAM, regardless of
    the value of transferFlags.  Refer to the specific command to see if ROM
    data is supported.

  Preconditions:
    None

  Parameters:
    uint8_t address                - Device's address on the bus
    USB_PRINTER_COMMAND command - Command to execute.  See the enumeration
                                    USB_PRINTER_COMMAND for the list of
                                    valid commands and their requirements.
    USB_DATA_POINTER data    - Pointer to the required data.  Note that
                                    the caller must set transferFlags
                                    appropriately to indicate if the pointer is
                                    a RAM pointer or a ROM pointer.
    uint32_t size                  - Size of the data.  For some commands, this
                                    parameter is used to hold the data itself.
    uint8_t transferFlags          - Flags that indicate details about the
                                    transfer operation.  Refer to these flags
                                    * USB_PRINTER_TRANSFER_COPY_DATA
                                    * USB_PRINTER_TRANSFER_STATIC_DATA
                                    * USB_PRINTER_TRANSFER_NOTIFY
                                    * USB_PRINTER_TRANSFER_FROM_ROM
                                    * USB_PRINTER_TRANSFER_FROM_RAM

  Return Values:
    USB_PRINTER_SUCCESS             - The command was executed successfully.
    USB_PRINTER_UNKNOWN_DEVICE      - A printer with the indicated address is not
                                        attached
    USB_PRINTER_TOO_MANY_DEVICES    - The printer status array does not have
                                        space for another printer.
    USB_PRINTER_OUT_OF_MEMORY       - Not enough available heap space to
                                        execute the command.
    other                           - See possible return codes from the
                                        function USBHostPrinterWrite().

  Remarks:
    When developing new commands, keep in mind that the function
    USBHostPrinterCommandReady() will be used before calling this function to
    see if there is space available in the output transfer queue.
    USBHostPrinterCommandReady() will routine true if a single space is
    available in the output queue.  Therefore, each command can generate only
    one output transfer.

    Multiple printer languages may be used in a single application.  The USB
    Embedded Host Printer Client Driver will call the routine required for the
    attached device.
  ***************************************************************************/

uint8_t USBHostPrinterLanguageESCPOS( uint8_t address,
        USB_PRINTER_COMMAND command, USB_DATA_POINTER data, uint32_t size, uint8_t transferFlags );


/****************************************************************************
  Function:
    bool USBHostPrinterLanguageESCPOSIsSupported( char *deviceID,
                USB_PRINTER_FUNCTION_SUPPORT *support )

  Description:
    This function determines if the printer with the given device ID string
    supports the ESC/POS printer language.

  Preconditions:
    None

  Parameters:
    char *deviceID  - Pointer to the "COMMAND SET:" portion of the device ID
                        string of the attached printer.
    USB_PRINTER_FUNCTION_SUPPORT *support   - Pointer to returned information
                        about what types of functions this printer supports.

  Return Values:
    true    - The printer supports ESC/POS.
    false   - The printer does not support ESC/POS.

  Remarks:
    The caller must first locate the "COMMAND SET:" section of the device ID
    string.  To ensure that only the "COMMAND SET:" section of the device ID
    string is checked, the ";" at the end of the section should be temporarily
    replaced with a NULL.  Otherwise, this function may find the printer
    language string in the comments or other section, and incorrectly indicate
    that the printer supports the language.

    Device ID strings are case sensitive.

    See the file header comments for this file (usb_host_printer_esc_pos.h)
    for cautions regarding dynamic printer language selection with POS
    printers.
  ***************************************************************************/

bool USBHostPrinterLanguageESCPOSIsSupported( char *deviceID,
                USB_PRINTER_FUNCTION_SUPPORT *support );


/****************************************************************************
  Function:
    USB_DATA_POINTER USBHostPrinterPOSImageDataFormat( USB_DATA_POINTER image,
        uint8_t imageLocation, uint16_t imageHeight, uint16_t imageWidth, uint16_t *currentRow,
        uint8_t uint8_tDepth, uint8_t *imageData )

  Summary:
    This function formats data for a bitmapped image into the format required
    for sending to a POS printer.

  Description:
    This function formats data for a bitmapped image into the format required
    for sending to a POS printer.  Bitmapped images are stored one row of pixels
    at a time.  Suppose we have an image with vertical black bars, eight pixels
    wide and eight pixels deep.  The image would appear as the following pixels,
    where 0 indicates a black dot and 1 indicates a white dot:
    <code>
    0 1 0 1 0 1 0 1
    0 1 0 1 0 1 0 1
    0 1 0 1 0 1 0 1
    0 1 0 1 0 1 0 1
    0 1 0 1 0 1 0 1
    0 1 0 1 0 1 0 1
    0 1 0 1 0 1 0 1
    0 1 0 1 0 1 0 1
    </code>
    The stored bitmap of the data would contain the data uint8_ts, where each uint8_t
    is one row of data:
    <code>
    0x55 0x55 0x55 0x55 0x55 0x55 0x55 0x55
    </code>
    When printing to a full sheet printer, eight separate
    USB_PRINTER_IMAGE_DATA_HEADER / USB_PRINTER_IMAGE_DATA command combinations
    are required to print this image.

    POS printers, however, require image data formated either 8 dots or 24 dots
    deep, depending on the desired (and supported) vertical print density.  For
    a POS printer performing an 8-dot vertical density print, the data needs to
    be in this format:
    <code>
    0x00 0xFF 0x00 0xFF 0x00 0xFF 0x00 0xFF
    </code>
    When printing to a POS printer, only one
    USB_PRINTER_IMAGE_DATA_HEADER / USB_PRINTER_IMAGE_DATA command combination
    is required to print this image.

    This function supports 8-dot and 24-dot vertical densities by specifying
    the uint8_tDepth parameter as either 1 (8-dot) or 3 (24-dot).

  Precondition:
    None

  Parameters:
    USB_DATA_POINTER image  Pointer to the image bitmap data.
    uint8_t imageLocation      Location of the image bitmap data.  Valid values
                            are USB_PRINTER_TRANSFER_FROM_ROM and
                            USB_PRINTER_TRANSFER_FROM_RAM.
    uint16_t imageHeight        Height of the image in pixels.
    uint16_t imageWidth         Width of the image in pixels.
    uint16_t *currentRow        The current pixel row.  Upon return, this value is
                            updated to the next pixel row to print.
    uint8_t uint8_tDepth          The uint8_t depth of the print.  Valid values are 1
                            (8-dot vertical density) and 3 (24-dot vertical
                            density).
    uint8_t *imageData         Pointer to a RAM data buffer that will receive the
                            manipulated data to send to the printer.

  Returns:
    The function returns a pointer to the next uint8_t of image data.

  Example:
    The following example code will send a complete bitmapped image to a POS
    printer.
    <code>
        uint16_t                    currentRow;
        uint8_t                    depthuint8_ts;
        uint8_t                    *imageDataPOS;
        USB_PRINTER_IMAGE_INFO  imageInfo;
        uint8_t                    returnCode;
        #if defined (__C30__) || defined __XC16__
            uint8_t __prog__       *ptr;
            ptr = (uint8_t __prog__ *)logoMCHP.address;
        #elif defined (__PIC32MX__)
            const uint8_t          *ptr;
            ptr = (const uint8_t *)logoMCHP.address;
        #endif

        imageInfo.densityVertical   = 24;   // 24-dot density
        imageInfo.densityHorizontal = 2;    // Double density

        // Extract the image height and width
        imageInfo.width    = ((uint16_t)ptr[5] << 8) + ptr[4];
        imageInfo.height   = ((uint16_t)ptr[3] << 8) + ptr[2];

        depthuint8_ts         = imageInfo.densityVertical / 8;
        imageDataPOS       = (uint8_t *)malloc( imageInfo.width *
                                       depthuint8_ts );

        if (imageDataPOS == NULL)
        {
            // Error - not enough heap space
        }

        USBHostPrinterCommandWithReadyWait( &returnCode,
              printerInfo.deviceAddress, USB_PRINTER_IMAGE_START,
              USB_DATA_POINTER_RAM(&imageInfo),
              sizeof(USB_PRINTER_IMAGE_INFO ),
              0 );

        ptr += 10; // skip the header info

        currentRow = 0;
        while (currentRow < imageInfo.height)
        {
            USBHostPrinterCommandWithReadyWait( &returnCode,
              printerInfo.deviceAddress,
              USB_PRINTER_IMAGE_DATA_HEADER, USB_NULL,
              imageInfo.width, 0 );

            ptr = USBHostPrinterPOSImageDataFormat(
              USB_DATA_POINTER_ROM(ptr),
              USB_PRINTER_TRANSFER_FROM_ROM, imageInfo.height,
              imageInfo.width, &currentRow, depthuint8_ts,
              imageDataPOS ).pointerROM;

            USBHostPrinterCommandWithReadyWait( &returnCode,
              printerInfo.deviceAddress, USB_PRINTER_IMAGE_DATA,
              USB_DATA_POINTER_RAM(imageDataPOS), imageInfo.width,
              USB_PRINTER_TRANSFER_COPY_DATA);
        }

        free( imageDataPOS );

        USBHostPrinterCommandWithReadyWait( &returnCode,
              printerInfo.deviceAddress, USB_PRINTER_IMAGE_STOP,
              USB_NULL, 0, 0 );
      </code>

  Remarks:
    This routine currently does not support 36-dot density printing.  Since
    the output for 36-dot vertical density is identical to 24-dot vertical
    density, 24-dot vertical density should be used instead.

    This routine does not yet support reading from external memory.
  ***************************************************************************/

USB_DATA_POINTER USBHostPrinterPOSImageDataFormat( USB_DATA_POINTER image,
        uint8_t imageLocation, uint16_t imageHeight, uint16_t imageWidth, uint16_t *currentRow,
        uint8_t uint8_tDepth, uint8_t *imageData );

