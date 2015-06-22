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

/********************************************************************
 Change History:
  Rev    Description
  ----   -----------
  2.6    Minor changes in include file structure.
  2.9h   Implemented USBCheckVendorRequest() function, in order to
         support MS OS Feature Descriptor handling.
********************************************************************

******************************************************************************/

/** I N C L U D E S **********************************************************/
#include "usb.h"
#include "usb_device_generic.h"

#if defined(USB_USE_GEN)

/** V A R I A B L E S ********************************************************/
extern volatile CTRL_TRF_SETUP SetupPkt;    //Common buffer that receives the 
                                            //8-byte SETUP packet data from the 
                                            //host during control transfer 
                                            //requests.

/** P R I V A T E  P R O T O T Y P E S ***************************************/

/** D E C L A R A T I O N S **************************************************/

/** U S E R  A P I ***********************************************************/

/********************************************************************
    Function:
        USB_HANDLE USBGenWrite(uint8_t ep, uint8_t* data, uint16_t len)
        
    Summary:
        Sends the specified data out the specified endpoint

    Description:
        This function sends the specified data out the specified 
        endpoint and returns a handle to the transfer information.

        Typical Usage:
        <code>
        //make sure that the last transfer isn't busy by checking the handle
        if(!USBHandleBusy(USBGenericInHandle))
        {
            //Send the data contained in the INPacket[] array out on
            //  endpoint USBGEN_EP_NUM
            USBGenericInHandle = USBGenWrite(USBGEN_EP_NUM,(uint8_t*)&INPacket[0],sizeof(INPacket));
        }
        </code>
        
    PreCondition:
        None
        
    Parameters:
        uint8_t ep    - the endpoint you want to send the data out of
        uint8_t* data - pointer to the data that you wish to send
        uint16_t len   - the length of the data that you wish to send
        
    Return Values:
        USB_HANDLE - a handle for the transfer.  This information
        should be kept to track the status of the transfer
        
    Remarks:
        None
  
 *******************************************************************/
 // Implemented as a macro. See usb_function_generic.h

/********************************************************************
    Function:
        USB_HANDLE USBGenRead(uint8_t ep, uint8_t* data, uint16_t len)
        
    Summary:
        Receives the specified data out the specified endpoint
        
    Description:
        Receives the specified data out the specified endpoint.

        Typical Usage:
        <code>
        //Read 64-bytes from endpoint USBGEN_EP_NUM, into the OUTPacket array.
        //  Make sure to save the return handle so that we can check it later
        //  to determine when the transfer is complete.
        if(!USBHandleBusy(USBOutHandle))
        {
            USBOutHandle = USBGenRead(USBGEN_EP_NUM,(uint8_t*)&OUTPacket,64);
        }
        </code>

    PreCondition:
        None
        
    Parameters:
        uint8_t ep - the endpoint you want to receive the data into
        uint8_t* data - pointer to where the data will go when it arrives
        uint16_t len - the length of the data that you wish to receive
        
    Return Values:
        USB_HANDLE - a handle for the transfer.  This information
        should be kept to track the status of the transfer
        
    Remarks:
        None
  
 *******************************************************************/
 // Implemented as a macro. See usb_function_generic.h


/********************************************************************
	Function:
		void USBCheckVendorRequest(void)

 	Summary:
 		This routine handles vendor class specific requests that happen on EP0.
        This function should be called from the USBCBCheckOtherReq() call back
        function whenever implementing a custom/vendor class device.

 	Description:
 		This routine handles vendor specific requests that may arrive on EP0 as
 		a control transfer.  These can include, but are not necessarily 
 		limited to, requests for Microsft specific OS feature descriptor(s).  
 		This function should be called from the USBCBCheckOtherReq() call back 
 		function whenever using a vendor class device.

        Typical Usage:
        <code>
        void USBCBCheckOtherReq(void)
        {
            //Since the stack didn't handle the request I need to check
            //  my class drivers to see if it is for them
            USBCheckVendorRequest();
        }
        </code>

	PreCondition:
		None

	Parameters:
		Although this function has a void input, this handler function will
		typically need to look at the 8-byte SETUP packet contents that the
		host just sent, which may contain the vendor class specific request.
		
		Therefore, the statically allocated SetupPkt structure may be looked
		at while in the context of this function, and it will contain the most
		recently received 8-byte SETUP packet data.

	Return Values:
		None

	Remarks:
		This function normally gets called within the same context as the
		USBDeviceTasks() function, just after a new control transfer request
		from the host has arrived.  If the USB stack is operated in 
		USB_INTERRUPT mode (a usb_config.h option), then this function
		will be executed in the interrupt context.  If however the USB stack
		is operated in the USB_POLLING mode, then this function executes in the
		main loop context.
		
		In order to respond to class specific control transfer request(s) in
		this handler function, it is suggested to use one or more of the
		USBEP0SendRAMPtr(), USBEP0SendROMPtr(), or USBEP0Receive() API 
		functions.
 
 *******************************************************************/
void USBCheckVendorRequest(void)
{
    #if defined(IMPLEMENT_MICROSOFT_OS_DESCRIPTOR)
        uint16_t Length;
    
        //Check if the most recent SETUP request is class specific
        if(SetupPkt.bmRequestType == 0b11000000)    //Class specific, device to host, device level target
        {
            //Check if the host is requesting an MS feature descriptor
            if(SetupPkt.bRequest == GET_MS_DESCRIPTOR)
            {
                //Figure out which descriptor is being requested
                if(SetupPkt.wIndex == EXTENDED_COMPAT_ID)
                {
                    //Determine number of bytes to send to host 
                    //Lesser of: requested amount, or total size of the descriptor
                    Length = CompatIDFeatureDescriptor.dwLength;
                    if(SetupPkt.wLength < Length)
                    {
                        Length = SetupPkt.wLength;
                    }    
                         
                    //Prepare to send the requested descriptor to the host
                    USBEP0SendROMPtr((const uint8_t*)&CompatIDFeatureDescriptor, Length, USB_EP0_ROM | USB_EP0_INCLUDE_ZERO);
                }
            }            
        }//if(SetupPkt.bmRequestType == 0b11000000)    
        else if(SetupPkt.bmRequestType == 0b11000001)    //Class specific, device to host, interface target
        {
            //Check if the host is requesting an MS feature descriptor
            if(SetupPkt.bRequest == GET_MS_DESCRIPTOR)
            {
                //Figure out which descriptor is being requested
                if(SetupPkt.wIndex == EXTENDED_PROPERTIES)    
                {
                    //Determine number of bytes to send to host 
                    //Lesser of: requested amount, or total size of the descriptor
                    Length = ExtPropertyFeatureDescriptor.dwLength;
                    if(SetupPkt.wLength < Length)
                    {
                        Length = SetupPkt.wLength;
                    }    
                         
                    //Prepare to send the requested descriptor to the host
                    USBEP0SendROMPtr((const uint8_t*)&ExtPropertyFeatureDescriptor, Length, USB_EP0_ROM | USB_EP0_INCLUDE_ZERO);
                }    
            }                   
        }//else if(SetupPkt.bmRequestType == 0b11000001)    
    #endif  //#if defined(IMPLEMENT_MICROSOFT_OS_DESCRIPTOR)   
}//void USBCheckVendorRequest(void)


#endif //def USB_USE_GEN
/** EOF usbgen.c *************************************************************/
