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

/** I N C L U D E S *******************************************************/
#include "usb_config.h"
#include "usb.h"
#include "usb_device_audio.h"

#ifdef USB_USE_AUDIO_CLASS

#if defined USB_AUDIO_INPUT_TERMINAL_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_INPUT_TERMINAL_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_OUTPUT_TERMINAL_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_OUTPUT_TERMINAL_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_MIXER_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_MIXER_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_SELECTOR_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_SELECTOR_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_FEATURE_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_FEATURE_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_PROCESSING_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_PROCESSING_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_EXTENSION_UNIT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_EXTENSION_UNIT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_INTRFACE_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_INTRFACE_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_ENDPOINT_CONTROL_REQUESTS_HANDLER
    void USB_AUDIO_ENDPOINT_CONTROL_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_MEMORY_REQUESTS_HANDLER
    void USB_AUDIO_MEMORY_REQUESTS_HANDLER(void);
#endif

#if defined USB_AUDIO_STATUS_REQUESTS_HANDLER
    void USB_AUDIO_STATUS_REQUESTS_HANDLER(void);
#endif



/** V A R I A B L E S ********************************************************/

/** C L A S S  S P E C I F I C  R E Q ****************************************/
/******************************************************************************
 	Function:
 		void USBCheckAudioRequest(void)

 	Description:
 		This routine checks the setup data packet to see if it
 		knows how to handle it

 	PreCondition:
 		None

	Parameters:
		None

	Return Values:
		None

	Remarks:
		None

 *****************************************************************************/

void USBCheckAudioRequest(void)
{
    /*
     * If request recipient is not an interface then return
     */
    if(SetupPkt.Recipient != USB_SETUP_RECIPIENT_INTERFACE_BITFIELD) return;
    /*
     * If request type is not class-specific then return
     */
    if(SetupPkt.RequestType != USB_SETUP_TYPE_CLASS_BITFIELD) return;
    /*
     * Interface ID must match interface numbers associated with
     * Audio class, else return
     */
    if((SetupPkt.bIntfID != AUDIO_CONTROL_INTERFACE_ID)&&
       (SetupPkt.bIntfID != AUDIO_STREAMING_INTERFACE_ID)) return;

    switch(SetupPkt.wIndex >> 8)// checking for the Entity ID (Entity ID are defined in the config.h file)
    {
        case ID_INPUT_TERMINAL:
             #if defined USB_AUDIO_INPUT_TERMINAL_CONTROL_REQUESTS_HANDLER
                 USB_AUDIO_INPUT_TERMINAL_CONTROL_REQUESTS_HANDLER();
             #endif
             break;
        case ID_OUTPUT_TERMINAL:
             #if defined USB_AUDIO_OUTPUT_TERMINAL_CONTROL_REQUESTS_HANDLER
                 USB_AUDIO_OUTPUT_TERMINAL_CONTROL_REQUESTS_HANDLER();
             #endif
             break;
        case ID_MIXER_UNIT:
            #if defined USB_AUDIO_MIXER_UNIT_CONTROL_REQUESTS_HANDLER
                USB_AUDIO_MIXER_UNIT_CONTROL_REQUESTS_HANDLER();
            #endif
            break;
        case ID_SELECTOR_UNIT:
            #if defined USB_AUDIO_SELECTOR_UNIT_CONTROL_REQUESTS_HANDLER
                USB_AUDIO_SELECTOR_UNIT_CONTROL_REQUESTS_HANDLER();
            #endif
            break;
        case ID_FEATURE_UNIT:
            #if defined USB_AUDIO_FEATURE_UNIT_CONTROL_REQUESTS_HANDLER
                USB_AUDIO_FEATURE_UNIT_CONTROL_REQUESTS_HANDLER();
            #endif
            break;
        case ID_PROCESSING_UNIT:
            #if defined USB_AUDIO_PROCESSING_UNIT_CONTROL_REQUESTS_HANDLER
                USB_AUDIO_PROCESSING_UNIT_CONTROL_REQUESTS_HANDLER();
            #endif
            break;
        case ID_EXTENSION_UNIT:
            #if defined USB_AUDIO_EXTENSION_UNIT_CONTROL_REQUESTS_HANDLER
                USB_AUDIO_EXTENSION_UNIT_CONTROL_REQUESTS_HANDLER();
            #endif
            break;
        default:
            break;

    }//end switch(SetupPkt.bRequest
}//end USBCheckAudioRequest

#endif
