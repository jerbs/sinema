//
// X11 Helpers and Debug Functions
//
// Copyright (C) Joachim Erbs, 2009-2010
//

#ifndef XV_HELPERS_HPP
#define XV_HELPERS_HPP

#include <iostream>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XShm.h>  // has to be included before Xvlib.h
#include <X11/extensions/Xvlib.h>

// http://www.fourcc.org/yuv.php#YV12
// Label: YV12  (Planar)
// FOURCC in hex: 0x32315659
// Bits per pixel: 12
// 8 bit Y plane followed by 8 bit 2x2 subsampled V and U planes.
// This is the format of choice for many software MPEG codecs. 
// It comprises an NxM Y plane followed by (N/2)x(M/2) V and U planes.
#define GUID_YUV12_PLANAR 0x32315659

// http://www.fourcc.org/yuv.php#UYVY
// Label: UYVY (Packed)
// 0x59565955
// Bits per pixel: 16
// YUV 4:2:2 (Y sample at every pixel, U and V sampled at every second 
// pixel horizontally on each line). A macropixel contains 2 pixels 
// in 1 u_int32.
// UYVY is probably the most popular of the various YUV 4:2:2 formats.
// Macropixel: (lowest byte) U0 Y0 V0 Y1 (highest byte)
#define GUID_UYVY_PACKED  0x59565955

// http://www.fourcc.org/yuv.php#YUY2
// Label: YUY2 (Packed)
// 0x32595559
// Bits per pixel: 16
// YUV 4:2:2 similar to the UYVY format, but with a different component 
// ordering within the u_int32 macropixel.
// Macropixel: (lowest byte) Y0 U0 Y0 V0 (highest byte)
#define GUID_YUY2_PACKED  0x32595559

struct XvVersionInfo
{
    unsigned int version;
    unsigned int revision;
    unsigned int request_base;
    unsigned int event_base;
    unsigned int error_base;
};

std::ostream& operator<<(std::ostream& strm, const XVisualInfo& xvi);
std::ostream& operator<<(std::ostream& strm, const XvVersionInfo& xvi);
std::ostream& operator<<(std::ostream& strm, const XvFormat& f);
std::ostream& operator<<(std::ostream& strm, const XvAdaptorInfo& ai);
std::ostream& operator<<(std::ostream& strm, const XvEncodingInfo& ei);
std::ostream& operator<<(std::ostream& strm, const XvAttribute& att);
std::ostream& operator<<(std::ostream& strm, const XvImageFormatValues& ifv);

const XvPortID INVALID_XV_PORT_ID = -1;

#endif
