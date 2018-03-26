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

#ifndef USB_PRINTER_POS_EPSON_TM_T88IV
#define USB_PRINTER_POS_EPSON_TM_T88IV


//------------------------------------------------------------------------------
// Bar Code Support Configuration

// If the printer supports bar code printing, uncomment the following
// definition.  This will provide support for UPC-A, UPC-E, JAN/EAN13, JAN/EAN8,
// CODE39, ITF, and CODABAR bar codes using Format A of the "Print Bar Code"
// command.  If only only bar code format is specified, it is Format A.
#define USB_PRINTER_POS_BARCODE_SUPPORT

// If the printer supports both Format A and Format B of the "Print Bar Code"
// command, also uncomment this definition.  In addition to the bar codes listed
// above, CODE93 and CODE128 bar codes will be supported.
#define USE_PRINTER_POS_EXTENDED_BARCODE_FORMAT


//------------------------------------------------------------------------------
// Bitmap Image Support Configuration

// If the printer supports 24-dot vertical density image printing, uncomment the
// following definition.
#define USB_PRINTER_POS_24_DOT_IMAGE_SUPPORT

// If the printer supports 36-dot vertical density image printing, uncomment the
// following definition.  This support is not common.  Note that 36-dot vertical
// density image printing itself is not supported.  However, printers with this
// capability use different parameter values for other image printing, and must
// be configured appropriately.  If this is not configured correctly, 24-bit
// vertical density images will not print correctly.
//#define USB_PRINTER_POS_36_DOT_IMAGE_SUPPORT

// Set this label to the line spacing required between bit image lines.  Often,
// the value 0 (zero) can be used.  If the printer prints all image lines on
// top of each other, set this value to the height of the printed image line.
#define USB_PRINTER_POS_IMAGE_LINE_SPACING              0


//------------------------------------------------------------------------------
// Text Printing Support Configuration

// If the printer supports reverse text (white letters on a black background)
// printing, uncomment the following definition.
#define USB_PRINTER_POS_REVERSE_TEXT_SUPPORT


//------------------------------------------------------------------------------
// Color Support Configuration

// If the printer supports two color printing, uncomment the following line.
// This is not common.
//#define USB_PRINTER_POS_TWO_COLOR_SUPPORT


//------------------------------------------------------------------------------
// Mechanism Support Configuration

// If the printer has an automatic cutter, uncomment the following line.
#define USB_PRINTER_POS_CUTTER_SUPPORT

#endif

