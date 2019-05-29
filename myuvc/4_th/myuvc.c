#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <asm/atomic.h>
#include <asm/unaligned.h>

#include <media/v4l2-common.h>

static void parse_videocontrol_interface(struct usb_interface *intf, unsigned char *buf, int buflen)
{
    static const char * const ctrlnames[] = {
        "Brightness", "Contrast", "Hue", "Saturation", "Sharpness", "Gamma",
        "White Balance Temperature", "White Balance Component", "Backlight Compensation",
        "Gain", "Power Line Frequency", "Hue, Auto", "White Balance Temperature, Auto",
        "White Balance Component, Auto", "Digital Multiplier", "Digital Multiplier Limit",
        "Analog Video Standard", "Analog Video Lock Status"
    };
    static const char * const camctrlnames[] = {
        "Scanning Mode", "Auto-Exposure Mode", "Auto-Exposure Priority",
        "Exposure Time (Absolute)", "Exposure Time (Relative)", "Focus (Absolute)",
        "Focus (Relative)", "Iris (Absolute)", "Iris (Relative)", "Zoom (Absolute)",
        "Zoom (Relative)", "PanTilt (Absolute)", "PanTilt (Relative)",
        "Roll (Absolute)", "Roll (Relative)", "Reserved", "Reserved", "Focus, Auto",
        "Privacy"
    };
    static const char * const stdnames[] = {
        "None", "NTSC - 525/60", "PAL - 625/50", "SECAM - 625/50",
        "NTSC - 625/50", "PAL - 525/60" };
    unsigned int i, ctrls, stds, n, p, termt, freq;

    while (buflen > 0)
    {

        if (buf[1] != USB_DT_CS_INTERFACE)
            printk("      Warning: Invalid descriptor\n");
        else if (buf[0] < 3)
            printk("      Warning: Descriptor too short\n");
        printk("      VideoControl Interface Descriptor:\n"
               "        bLength             %5u\n"
               "        bDescriptorType     %5u\n"
               "        bDescriptorSubtype  %5u ",
               buf[0], buf[1], buf[2]);
        switch (buf[2]) {
        case 0x01:  /* HEADER */
            printk("(HEADER)\n");
            n = buf[11];
            if (buf[0] < 12+n)
                printk("      Warning: Descriptor too short\n");
            freq = buf[7] | (buf[8] << 8) | (buf[9] << 16) | (buf[10] << 24);
            printk("        bcdUVC              %2x.%02x\n"
                   "        wTotalLength        %5u\n"
                   "        dwClockFrequency    %5u.%06uMHz\n"
                   "        bInCollection       %5u\n",
                   buf[4], buf[3], buf[5] | (buf[6] << 8), freq / 1000000,
                   freq % 1000000, n);
            for (i = 0; i < n; i++)
                printk("        baInterfaceNr(%2u)   %5u\n", i, buf[12+i]);
            break;

        case 0x02:  /* INPUT_TERMINAL */
            printk("(INPUT_TERMINAL)\n");
            termt = buf[4] | (buf[5] << 8);
            n = termt == 0x0201 ? 7 : 0;
            if (buf[0] < 8 + n)
                printk("      Warning: Descriptor too short\n");
            printk("        bTerminalID         %5u\n"
                   "        wTerminalType      0x%04x\n"
                   "        bAssocTerminal      %5u\n",
                   buf[3], termt, buf[6]);
            printk("        iTerminal           %5u\n",
                   buf[7]);
            if (termt == 0x0201) {
                n += buf[14];
                printk("        wObjectiveFocalLengthMin  %5u\n"
                       "        wObjectiveFocalLengthMax  %5u\n"
                       "        wOcularFocalLength        %5u\n"
                       "        bControlSize              %5u\n",
                       buf[8] | (buf[9] << 8), buf[10] | (buf[11] << 8),
                       buf[12] | (buf[13] << 8), buf[14]);
                ctrls = 0;
                for (i = 0; i < 3 && i < buf[14]; i++)
                    ctrls = (ctrls << 8) | buf[8+n-i-1];
                printk("        bmControls           0x%08x\n", ctrls);
                for (i = 0; i < 19; i++)
                    if ((ctrls >> i) & 1)
                        printk("          %s\n", camctrlnames[i]);
            }
            break;

        case 0x03:  /* OUTPUT_TERMINAL */
            printk("(OUTPUT_TERMINAL)\n");
            termt = buf[4] | (buf[5] << 8);
            if (buf[0] < 9)
                printk("      Warning: Descriptor too short\n");
            printk("        bTerminalID         %5u\n"
                   "        wTerminalType      0x%04x\n"
                   "        bAssocTerminal      %5u\n"
                   "        bSourceID           %5u\n"
                   "        iTerminal           %5u\n",
                   buf[3], termt, buf[6], buf[7], buf[8]);
            break;

        case 0x04:  /* SELECTOR_UNIT */
            printk("(SELECTOR_UNIT)\n");
            p = buf[4];
            if (buf[0] < 6+p)
                printk("      Warning: Descriptor too short\n");

            printk("        bUnitID             %5u\n"
                   "        bNrInPins           %5u\n",
                   buf[3], p);
            for (i = 0; i < p; i++)
                printk("        baSource(%2u)        %5u\n", i, buf[5+i]);
            printk("        iSelector           %5u\n",
                   buf[5+p]);
            break;

        case 0x05:  /* PROCESSING_UNIT */
            printk("(PROCESSING_UNIT)\n");
            n = buf[7];
            if (buf[0] < 10+n)
                printk("      Warning: Descriptor too short\n");
            printk("        bUnitID             %5u\n"
                   "        bSourceID           %5u\n"
                   "        wMaxMultiplier      %5u\n"
                   "        bControlSize        %5u\n",
                   buf[3], buf[4], buf[5] | (buf[6] << 8), n);
            ctrls = 0;
            for (i = 0; i < 3 && i < n; i++)
                ctrls = (ctrls << 8) | buf[8+n-i-1];
            printk("        bmControls     0x%08x\n", ctrls);
            for (i = 0; i < 18; i++)
                if ((ctrls >> i) & 1)
                    printk("          %s\n", ctrlnames[i]);
            stds = buf[9+n];
            printk("        iProcessing         %5u\n"
                   "        bmVideoStandards     0x%2x\n", buf[8+n], stds);
            for (i = 0; i < 6; i++)
                if ((stds >> i) & 1)
                    printk("          %s\n", stdnames[i]);
            break;

        case 0x06:  /* EXTENSION_UNIT */
            printk("(EXTENSION_UNIT)\n");
            p = buf[21];
            n = buf[22+p];
            if (buf[0] < 24+p+n)
                printk("      Warning: Descriptor too short\n");
            printk("        bUnitID             %5u\n"
                   "        guidExtensionCode         %s\n"
                   "        bNumControl         %5u\n"
                   "        bNrPins             %5u\n",
                   buf[3], get_guid(&buf[4]), buf[20], buf[21]);
            for (i = 0; i < p; i++)
                printk("        baSourceID(%2u)      %5u\n", i, buf[22+i]);
            printk("        bControlSize        %5u\n", buf[22+p]);
            for (i = 0; i < n; i++)
                printk("        bmControls(%2u)       0x%02x\n", i, buf[23+p+i]);
            printk("        iExtension          %5u\n",
                   buf[23+p+n]);
            break;

        default:
            printk("(unknown)\n"
                   "        Invalid desc subtype:");
            break;
        }

        buflen -= buf[0];
        buf    += buf[0];
    }
}


static void parse_videostreaming_interface(struct usb_interface *intf, unsigned char *buf, int buflen)
{
    static const char * const colorPrims[] = { "Unspecified", "BT.709,sRGB",
        "BT.470-2 (M)", "BT.470-2 (B,G)", "SMPTE 170M", "SMPTE 240M" };
    static const char * const transferChars[] = { "Unspecified", "BT.709",
        "BT.470-2 (M)", "BT.470-2 (B,G)", "SMPTE 170M", "SMPTE 240M",
        "Linear", "sRGB"};
    static const char * const matrixCoeffs[] = { "Unspecified", "BT.709",
        "FCC", "BT.470-2 (B,G)", "SMPTE 170M (BT.601)", "SMPTE 240M" };
    unsigned int i, m, n, p, flags, len;

    while (buflen > 0)
    {

        if (buf[1] != USB_DT_CS_INTERFACE)
            printk("      Warning: Invalid descriptor\n");
        else if (buf[0] < 3)
            printk("      Warning: Descriptor too short\n");
        printk("      VideoStreaming Interface Descriptor:\n"
               "        bLength                         %5u\n"
               "        bDescriptorType                 %5u\n"
               "        bDescriptorSubtype              %5u ",
               buf[0], buf[1], buf[2]);
        switch (buf[2]) {
        case 0x01: /* INPUT_HEADER */
            printk("(INPUT_HEADER)\n");
            p = buf[3];
            n = buf[12];
            if (buf[0] < 13+p*n)
                printk("      Warning: Descriptor too short\n");
            printk("        bNumFormats                     %5u\n"
                   "        wTotalLength                    %5u\n"
                   "        bEndPointAddress                %5u\n"
                   "        bmInfo                          %5u\n"
                   "        bTerminalLink                   %5u\n"
                   "        bStillCaptureMethod             %5u\n"
                   "        bTriggerSupport                 %5u\n"
                   "        bTriggerUsage                   %5u\n"
                   "        bControlSize                    %5u\n",
                   p, buf[4] | (buf[5] << 8), buf[6], buf[7], buf[8],
                   buf[9], buf[10], buf[11], n);
            for (i = 0; i < p; i++)
                printk(
                "        bmaControls(%2u)                 %5u\n",
                    i, buf[13+p*n]);
            break;

        case 0x02: /* OUTPUT_HEADER */
            printk("(OUTPUT_HEADER)\n");
            p = buf[3];
            n = buf[8];
            if (buf[0] < 9+p*n)
                printk("      Warning: Descriptor too short\n");
            printk("        bNumFormats                 %5u\n"
                   "        wTotalLength                %5u\n"
                   "        bEndpointAddress            %5u\n"
                   "        bTerminalLink               %5u\n"
                   "        bControlSize                %5u\n",
                   p, buf[4] | (buf[5] << 8), buf[6], buf[7], n);
            for (i = 0; i < p; i++)
                printk(
                "        bmaControls(%2u)             %5u\n",
                    i, buf[9+p*n]);
            break;

        case 0x03: /* STILL_IMAGE_FRAME */
            printk("(STILL_IMAGE_FRAME)\n");
            n = buf[4];
            m = buf[5+4*n];
            if (buf[0] < 6+4*n+m)
                printk("      Warning: Descriptor too short\n");
            printk("        bEndpointAddress                %5u\n"
                   "        bNumImageSizePatterns             %3u\n",
                   buf[3], n);
            for (i = 0; i < n; i++)
                printk("        wWidth(%2u)                      %5u\n"
                       "        wHeight(%2u)                     %5u\n",
                       i, buf[5+4*i] | (buf[6+4*i] << 8),
                       i, buf[7+4*i] | (buf[8+4*i] << 8));
            printk("        bNumCompressionPatterns           %3u\n", n);
            for (i = 0; i < m; i++)
                printk("        bCompression(%2u)                %5u\n",
                       i, buf[6+4*n+i]);
            break;

        case 0x04: /* FORMAT_UNCOMPRESSED */
        case 0x10: /* FORMAT_FRAME_BASED */
            if (buf[2] == 0x04) {
                printk("(FORMAT_UNCOMPRESSED)\n");
                len = 27;
            } else {
                printk("(FORMAT_FRAME_BASED)\n");
                len = 28;
            }
            if (buf[0] < len)
                printk("      Warning: Descriptor too short\n");
            flags = buf[25];
            printk("        bFormatIndex                    %5u\n"
                   "        bNumFrameDescriptors            %5u\n"
                   "        guidFormat                            %s\n"
                   "        bBitsPerPixel                   %5u\n"
                   "        bDefaultFrameIndex              %5u\n"
                   "        bAspectRatioX                   %5u\n"
                   "        bAspectRatioY                   %5u\n"
                   "        bmInterlaceFlags                 0x%02x\n",
                   buf[3], buf[4], get_guid(&buf[5]), buf[21], buf[22],
                   buf[23], buf[24], flags);
            printk("          Interlaced stream or variable: %s\n",
                   (flags & (1 << 0)) ? "Yes" : "No");
            printk("          Fields per frame: %u fields\n",
                   (flags & (1 << 1)) ? 1 : 2);
            printk("          Field 1 first: %s\n",
                   (flags & (1 << 2)) ? "Yes" : "No");
            printk("          Field pattern: ");
            switch ((flags >> 4) & 0x03) {
            case 0:
                printk("Field 1 only\n");
                break;
            case 1:
                printk("Field 2 only\n");
                break;
            case 2:
                printk("Regular pattern of fields 1 and 2\n");
                break;
            case 3:
                printk("Random pattern of fields 1 and 2\n");
                break;
            }
            printk("          bCopyProtect                  %5u\n", buf[26]);
            if (buf[2] == 0x10)
                printk("          bVariableSize                 %5u\n", buf[27]);
            break;

        case 0x05: /* FRAME UNCOMPRESSED */
        case 0x07: /* FRAME_MJPEG */
        case 0x11: /* FRAME_FRAME_BASED */
            if (buf[2] == 0x05) {
                printk("(FRAME_UNCOMPRESSED)\n");
                n = 25;
            } else if (buf[2] == 0x07) {
                printk("(FRAME_MJPEG)\n");
                n = 25;
            } else {
                printk("(FRAME_FRAME_BASED)\n");
                n = 21;
            }
            len = (buf[n] != 0) ? (26+buf[n]*4) : 38;
            if (buf[0] < len)
                printk("      Warning: Descriptor too short\n");
            flags = buf[4];
            printk("        bFrameIndex                     %5u\n"
                   "        bmCapabilities                   0x%02x\n",
                   buf[3], flags);
            printk("          Still image %ssupported\n",
                   (flags & (1 << 0)) ? "" : "un");
            if (flags & (1 << 1))
                printk("          Fixed frame-rate\n");
            printk("        wWidth                          %5u\n"
                   "        wHeight                         %5u\n"
                   "        dwMinBitRate                %9u\n"
                   "        dwMaxBitRate                %9u\n",
                   buf[5] | (buf[6] <<  8), buf[7] | (buf[8] << 8),
                   buf[9] | (buf[10] << 8) | (buf[11] << 16) | (buf[12] << 24),
                   buf[13] | (buf[14] << 8) | (buf[15] << 16) | (buf[16] << 24));
            if (buf[2] == 0x11)
                printk("        dwDefaultFrameInterval      %9u\n"
                       "        bFrameIntervalType              %5u\n"
                       "        dwBytesPerLine              %9u\n",
                       buf[17] | (buf[18] << 8) | (buf[19] << 16) | (buf[20] << 24),
                       buf[21],
                       buf[22] | (buf[23] << 8) | (buf[24] << 16) | (buf[25] << 24));
            else
                printk("        dwMaxVideoFrameBufferSize   %9u\n"
                       "        dwDefaultFrameInterval      %9u\n"
                       "        bFrameIntervalType              %5u\n",
                       buf[17] | (buf[18] << 8) | (buf[19] << 16) | (buf[20] << 24),
                       buf[21] | (buf[22] << 8) | (buf[23] << 16) | (buf[24] << 24),
                       buf[25]);
            if (buf[n] == 0)
                printk("        dwMinFrameInterval          %9u\n"
                       "        dwMaxFrameInterval          %9u\n"
                       "        dwFrameIntervalStep         %9u\n",
                       buf[26] | (buf[27] << 8) | (buf[28] << 16) | (buf[29] << 24),
                       buf[30] | (buf[31] << 8) | (buf[32] << 16) | (buf[33] << 24),
                       buf[34] | (buf[35] << 8) | (buf[36] << 16) | (buf[37] << 24));
            else
                for (i = 0; i < buf[n]; i++)
                    printk("        dwFrameInterval(%2u)         %9u\n",
                           i, buf[26+4*i] | (buf[27+4*i] << 8) |
                           (buf[28+4*i] << 16) | (buf[29+4*i] << 24));
            break;

        case 0x06: /* FORMAT_MJPEG */
            printk("(FORMAT_MJPEG)\n");
            if (buf[0] < 11)
                printk("      Warning: Descriptor too short\n");
            flags = buf[5];
            printk("        bFormatIndex                    %5u\n"
                   "        bNumFrameDescriptors            %5u\n"
                   "        bFlags                          %5u\n",
                   buf[3], buf[4], flags);
            printk("          Fixed-size samples: %s\n",
                   (flags & (1 << 0)) ? "Yes" : "No");
            flags = buf[9];
            printk("        bDefaultFrameIndex              %5u\n"
                   "        bAspectRatioX                   %5u\n"
                   "        bAspectRatioY                   %5u\n"
                   "        bmInterlaceFlags                 0x%02x\n",
                   buf[6], buf[7], buf[8], flags);
            printk("          Interlaced stream or variable: %s\n",
                   (flags & (1 << 0)) ? "Yes" : "No");
            printk("          Fields per frame: %u fields\n",
                   (flags & (1 << 1)) ? 2 : 1);
            printk("          Field 1 first: %s\n",
                   (flags & (1 << 2)) ? "Yes" : "No");
            printk("          Field pattern: ");
            switch ((flags >> 4) & 0x03) {
            case 0:
                printk("Field 1 only\n");
                break;
            case 1:
                printk("Field 2 only\n");
                break;
            case 2:
                printk("Regular pattern of fields 1 and 2\n");
                break;
            case 3:
                printk("Random pattern of fields 1 and 2\n");
                break;
            }
            printk("          bCopyProtect                  %5u\n", buf[10]);
            break;

        case 0x0a: /* FORMAT_MPEG2TS */
            printk("(FORMAT_MPEG2TS)\n");
            len = buf[0] < 23 ? 7 : 23;
            if (buf[0] < len)
                printk("      Warning: Descriptor too short\n");
            printk("        bFormatIndex                    %5u\n"
                   "        bDataOffset                     %5u\n"
                   "        bPacketLength                   %5u\n"
                   "        bStrideLength                   %5u\n",
                   buf[3], buf[4], buf[5], buf[6]);
            if (len > 7)
                printk("        guidStrideFormat                      %s\n",
                       get_guid(&buf[7]));
            break;

        case 0x0d: /* COLORFORMAT */
            printk("(COLORFORMAT)\n");
            if (buf[0] < 6)
                printk("      Warning: Descriptor too short\n");
            printk("        bColorPrimaries                 %5u (%s)\n",
                   buf[3], (buf[3] <= 5) ? colorPrims[buf[3]] : "Unknown");
            printk("        bTransferCharacteristics        %5u (%s)\n",
                   buf[4], (buf[4] <= 7) ? transferChars[buf[4]] : "Unknown");
            printk("        bMatrixCoefficients             %5u (%s)\n",
                   buf[5], (buf[5] <= 5) ? matrixCoeffs[buf[5]] : "Unknown");
            break;

        default:
            printk("        Invalid desc subtype:");
            break;
        }
        buflen -= buf[0];
        buf    += buf[0];
    }
}


static void dump_endpoint(const struct usb_endpoint_descriptor *endpoint)
{
	static const char * const typeattr[] = {
		"Control",
		"Isochronous",
		"Bulk",
		"Interrupt"
	};
	static const char * const syncattr[] = {
		"None",
		"Asynchronous",
		"Adaptive",
		"Synchronous"
	};
	static const char * const usage[] = {
		"Data",
		"Feedback",
		"Implicit feedback Data",
		"(reserved)"
	};
	static const char * const hb[] = { "1x", "2x", "3x", "(?\?)" };
	unsigned wmax = le16_to_cpu(endpoint->wMaxPacketSize);

	printk("      Endpoint Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bEndpointAddress     0x%02x  EP %u %s\n"
	       "        bmAttributes        %5u\n"
	       "          Transfer Type            %s\n"
	       "          Synch Type               %s\n"
	       "          Usage Type               %s\n"
	       "        wMaxPacketSize     0x%04x  %s %d bytes\n"
	       "        bInterval           %5u\n",
	       endpoint->bLength,
	       endpoint->bDescriptorType,
	       endpoint->bEndpointAddress,
	       endpoint->bEndpointAddress & 0x0f,
	       (endpoint->bEndpointAddress & 0x80) ? "IN" : "OUT",
	       endpoint->bmAttributes,
	       typeattr[endpoint->bmAttributes & 3],
	       syncattr[(endpoint->bmAttributes >> 2) & 3],
	       usage[(endpoint->bmAttributes >> 4) & 3],
	       wmax, hb[(wmax >> 11) & 3], wmax & 0x7ff,
	       endpoint->bInterval);
	/* only for audio endpoints */
	if (endpoint->bLength == 9)
		printk("        bRefresh            %5u\n"
		       "        bSynchAddress       %5u\n",
		       endpoint->bRefresh, endpoint->bSynchAddress);

}

static int myuvc_probe(struct usb_interface *intf,
		     const struct usb_device_id *id)
{
	static int cnt = 0;
	//interface_to_usbdev，有些函数需要的参数就是struct usb_device，而不是usb_interface
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_device_descriptor *descriptor = &dev->descriptor;		//设备描述符
	
	//usb_host_config包含在usb_device 结构体中
    struct usb_host_config *hostconfig;
	
	//usb_config_descriptor 是配置描述符的意思，包含于hostconfig结构体中
	struct usb_config_descriptor *config;						//配置描述符

	struct usb_interface_assoc_descriptor *assoc_desc;
	
	
	struct usb_interface_descriptor *interface;					//接口描述符

	struct usb_endpoint_descriptor  *endpoint;					//端点描述符

	int i=0, j=0, k=0, l=0, m=0;
	
	unsigned char *buffer = NULL;
	int bufferlen=0;
	int desc_len=0;
	int desc_cnt=0;
	
	//同样跟disconnect也是一样的，会重复两次拔出
	printk("myuvc_probe : cnt = %d\n", cnt++);
	
	
	/* 打印设备描述符 */
	printk("Device Descriptor:\n"
	       "  bLength             %5u\n"
	       "  bDescriptorType     %5u\n"
	       "  bcdUSB              %2x.%02x\n"
	       "  bDeviceClass        %5u \n"
	       "  bDeviceSubClass     %5u \n"
	       "  bDeviceProtocol     %5u \n"
	       "  bMaxPacketSize0     %5u\n"
	       "  idVendor           0x%04x \n"
	       "  idProduct          0x%04x \n"
	       "  bcdDevice           %2x.%02x\n"
	       "  iManufacturer       %5u\n"
	       "  iProduct            %5u\n"
	       "  iSerial             %5u\n"
	       "  bNumConfigurations  %5u\n",
	       descriptor->bLength, descriptor->bDescriptorType,
	       descriptor->bcdUSB >> 8, descriptor->bcdUSB & 0xff,
	       descriptor->bDeviceClass, 
	       descriptor->bDeviceSubClass,
	       descriptor->bDeviceProtocol, 
	       descriptor->bMaxPacketSize0,
	       descriptor->idVendor,  descriptor->idProduct,
	       descriptor->bcdDevice >> 8, descriptor->bcdDevice & 0xff,
	       descriptor->iManufacturer, 
	       descriptor->iProduct, 
	       descriptor->iSerialNumber, 
	       descriptor->bNumConfigurations);
	/* 打印配置描述符 */
	for(i=0; i<descriptor->bNumConfigurations; i++)
	{
		hostconfig = &dev->config[i];
		
		config = &hostconfig->desc;
		
		printk("  Configuration Descriptor %d:\n"
               "    bLength             %5u\n"
               "    bDescriptorType     %5u\n"
               "    wTotalLength        %5u\n"
               "    bNumInterfaces      %5u\n"
               "    bConfigurationValue %5u\n"
               "    iConfiguration      %5u\n"
               "    bmAttributes         0x%02x\n",
               i, 
               config->bLength, config->bDescriptorType,
               le16_to_cpu(config->wTotalLength),
               config->bNumInterfaces, config->bConfigurationValue,
               config->iConfiguration,
               config->bmAttributes);
			   
		//接口联合体描述符
        assoc_desc = hostconfig->intf_assoc[0];
		printk("    Interface Association:\n"
               "      bLength             %5u\n"
               "      bDescriptorType     %5u\n"
               "      bFirstInterface     %5u\n"
               "      bInterfaceCount     %5u\n"
               "      bFunctionClass      %5u\n"
               "      bFunctionSubClass   %5u\n"
               "      bFunctionProtocol   %5u\n"
               "      iFunction           %5u\n",
            assoc_desc->bLength,
            assoc_desc->bDescriptorType,
            assoc_desc->bFirstInterface,
            assoc_desc->bInterfaceCount,
            assoc_desc->bFunctionClass,
            assoc_desc->bFunctionSubClass,
            assoc_desc->bFunctionProtocol,
            assoc_desc->iFunction); 
		//打印接口描述符
		for(j=0; j<intf->num_altsetting; i++)
		{
			interface = &intf->altsetting[j].desc;
			printk("    Interface Descriptor altsetting %d:\n"
                       "      bLength             %5u\n"
                       "      bDescriptorType     %5u\n"
                       "      bInterfaceNumber    %5u\n"
                       "      bAlternateSetting   %5u\n"
                       "      bNumEndpoints       %5u\n"
                       "      bInterfaceClass     %5u\n"
                       "      bInterfaceSubClass  %5u\n"
                       "      bInterfaceProtocol  %5u\n"
                       "      iInterface          %5u\n",
                       j, 
                       interface->bLength, interface->bDescriptorType, interface->bInterfaceNumber,
                       interface->bAlternateSetting, interface->bNumEndpoints, interface->bInterfaceClass,
                       interface->bInterfaceSubClass, interface->bInterfaceProtocol,
                       interface->iInterface);
		}	
		
		/* 打印端点描述符 */
		for (m = 0; m < interface->bNumEndpoints; m++)
		{
			endpoint = &intf->altsetting[j].endpoint[m].desc;
			dump_endpoint(endpoint);
		}
			

		//打印额外描述符
		buffer = intf->cur_altsetting->extra;
		bufferlen = intf->cur_altsetting->extralen;
		
		printk("extra buffer of interface %d:\n", cnt-1);
		
		while( k <= bufferlen )
		{
			desc_len = buffer[k];
			printk("extra desc is %d", desc_cnt);
			for (l=0; l<desc_len; l++, k++)
			{
				printk("%02x", buffer[k+1]);
			}
			desc_cnt++;
			printk("\n");
		}
		
			
		
	}
	return 0;
	

}



static void myuvc_disconnect(struct usb_interface *intf)
{
	static int cnt = 0;
	//会打印两次，因为uvc摄像头有两个接口：VideoStreaming和VideoControl Interface
    printk("myuvc_disconnect : cnt = %d\n", cnt++);
}


static struct usb_device_id myuvc_ids[] = {
	/* Generic USB Video Class */
	{ USB_INTERFACE_INFO(USB_CLASS_VIDEO, 1, 0) },  /* VideoControl Interface */
    { USB_INTERFACE_INFO(USB_CLASS_VIDEO, 2, 0) },  /* VideoStreaming Interface */
	{}
};


/* 1. 分配usb_driver */
/* 2. 设置 */
static struct usb_driver myuvc_driver = {
    .name       = "myuvc",
    .probe      = myuvc_probe,
    .disconnect = myuvc_disconnect,
    .id_table   = myuvc_ids,
};

static int myuvc_init(void)
{
    /* 3. 注册 */
    usb_register(&myuvc_driver);
    return 0;
}

static void myuvc_exit(void)
{
    usb_deregister(&myuvc_driver);
}

module_init(myuvc_init);
module_exit(myuvc_exit);

MODULE_LICENSE("GPL");