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

#include <stdint.h>
#include <stdbool.h>

//If the user hasn't specified a timeout period, make one up for them
#ifndef ANDROID_DEVICE_ATTACH_TIMEOUT
    #define ANDROID_DEVICE_ATTACH_TIMEOUT 1500
#endif

uint8_t AndroidAppWrite_Pv2(void* handle, uint8_t* data, uint32_t size);
bool AndroidAppIsWriteComplete_Pv2(void* handle, uint8_t* errorCode, uint32_t* size);
uint8_t AndroidAppRead_Pv2(void* handle, uint8_t* data, uint32_t size);
bool AndroidAppIsReadComplete_Pv2(void* handle, uint8_t* errorCode, uint32_t* size);
void* AndroidInitialize_Pv2 ( uint8_t address, uint32_t flags, uint8_t clientDriverID );
void AndroidTasks_Pv2(void);
bool AndroidAppEventHandler_Pv2( uint8_t address, USB_EVENT event, void *data, uint32_t size );
bool AndroidAppDataEventHandler_Pv2( uint8_t address, USB_EVENT event, void *data, uint32_t size );
void AndroidAppStart_Pv2(void);
