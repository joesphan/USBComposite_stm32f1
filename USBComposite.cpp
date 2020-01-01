#include "USBComposite.h"
#undef true
#undef false
#include <Arduino.h>

#define DEFAULT_VENDOR_ID  0x1EAF
#define DEFAULT_PRODUCT_ID 0x0024

static char* putSerialNumber(char* out, int nibbles, uint32 id) {
    for (int i=0; i<nibbles; i++, id >>= 4) {
        uint8 nibble = id & 0xF;
        if (nibble <= 9)
            *out++ = nibble + '0';
        else
            *out++ = nibble - 10 + 'a';
    }
    return out;
}

const char* getDeviceIDString() {
    static char string[80/4+1];
    char* p = string;
    
    uint32 id = (uint32) *(uint16*) (0x1FFFF7E8+0x02);
    p = putSerialNumber(p, 4, id);
    
    id = *(uint32*) (0x1FFFF7E8+0x04);
    p = putSerialNumber(p, 8, id);
    
    id = *(uint32*) (0x1FFFF7E8+0x08);
    p = putSerialNumber(p, 8, id);
    
    *p = 0;
    
    return string;
}

USBCompositeDevice::USBCompositeDevice(void) {
        vendorId = 0;
        productId = 0;
        numParts = 0;
        setManufacturerString(NULL);
        setProductString(NULL);
        setSerialString(NULL);
    }

void USBCompositeDevice::setVendorId(uint16 _vendorId) {
    if (_vendorId != 0)
        vendorId = _vendorId;
    else
        vendorId = DEFAULT_VENDOR_ID;
}

void USBCompositeDevice::setProductId(uint16 _productId) {
    if (_productId != 0)
        productId = _productId;
    else
        productId = DEFAULT_PRODUCT_ID;
}

static void setString(const uint8** outP, uint8* buffer, const char* s, uint32 maxLength) {
    if (s == NULL) {
        *outP = NULL;
    }
    else {        
        uint32 n = strlen(s);
        if (n > maxLength)
            n = maxLength;
        buffer[0] = (uint8)USB_DESCRIPTOR_STRING_LEN(n);
        buffer[1] = USB_DESCRIPTOR_TYPE_STRING;
        for (uint32 i=0; i<n; i++) {
            buffer[2 + 2*i] = (uint8)s[i];
            buffer[2 + 1 + 2*i] = 0;
        }
        *outP = buffer;
    }
}

void USBCompositeDevice::setManufacturerString(const char* s) {
    setString(&iManufacturer, iManufacturer_buffer, s, USB_MAX_MANUFACTURER_LENGTH);
}

void USBCompositeDevice::setProductString(const char* s) {
    setString(&iProduct, iProduct_buffer, s, USB_MAX_PRODUCT_LENGTH);
}

void USBCompositeDevice::setSerialString(const char* s) {
    setString(&iSerialNumber, iSerialNumber_buffer, s, USB_MAX_SERIAL_NUMBER_LENGTH);
}

bool USBCompositeDevice::begin() {
   if (enabled)
        return true;
    for (uint32 i = 0 ; i < numParts ; i++) {
		if (init[i] != NULL && !init[i](plugin[i]))
			return false;
	}
	usb_generic_set_info(vendorId, productId, iManufacturer, iProduct, iSerialNumber);
    if (! usb_generic_set_parts(parts, numParts))
        return false;
    usb_generic_enable();
    enabled = true;  
    return true;
}

void USBCompositeDevice::end() {
    if (!enabled)
        return;
    usb_generic_disable();
    for (uint32 i = 0 ; i < numParts ; i++)
		if (stop[i] != NULL)
			stop[i](plugin[i]);
    enabled = false;
}

void USBCompositeDevice::clear() {
    numParts = 0;
}

bool USBCompositeDevice::add(USBCompositePart* part, void* _plugin, USBPartInitializer _init, USBPartStopper _stop) {
    unsigned i;
    
    for (i = 0; i<numParts; i++) 
        if (plugin[numParts] == _plugin && parts[i] == part)
            break;
    if (i >= USB_COMPOSITE_MAX_PARTS)
        return false;
    parts[i] = part;
    init[i] = _init;
    stop[i] = _stop;
	plugin[i] = _plugin;
    if (i >= numParts)
        numParts++;
    return true;
}

USBCompositeDevice USBComposite;
