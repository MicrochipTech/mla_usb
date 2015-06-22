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

/*****************************************************************************
Change History:
  Rev         Description
  ----------  ----------------------------------------------------------
  2.6 - 2.6a  No change
*******************************************************************************/


#include <stdlib.h>
#include <string.h>
#include "usb.h"
#include "usb_host_printer_primitives.h"

#ifndef USB_MALLOC
    #define USB_MALLOC(size) malloc(size)
#endif

#ifndef USB_FREE
    #define USB_FREE(ptr) free(ptr)
#endif

#define USB_FREE_AND_CLEAR(ptr) {USB_FREE(ptr); ptr = NULL;}

#ifdef USE_GRAPHICS_LIBRARY_PRINTER_INTERFACE


// *****************************************************************************
// *****************************************************************************
// Section: Subroutines
// *****************************************************************************
// *****************************************************************************




#endif

