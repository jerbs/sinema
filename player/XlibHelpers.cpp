//
// X11 Helpers and Debug Functions
//
// Copyright (C) Joachim Erbs, 2009-2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#include "player/XlibHelpers.hpp"
#include "platform/Logging.hpp"

#include <execinfo.h> // for backtrace

using namespace std;

void dumpBacktrace()
{
    // Call linker with option -rdynamic to see the function names
    // in the generated backtrace.
    // Use addr2line to get file name and line number.

    // See http://www.gnu.org/s/libc/manual/html_node/Backtraces.html

    const int arraySize = 100;
    void *array[arraySize];
    size_t size;
    char **strings;
    size_t i;
    
    size = backtrace(array, arraySize);
    strings = backtrace_symbols(array, size);
    
    for (i = 0; i < size; i++)
	TRACE_DEBUG(<< strings[i]);
    
    free (strings);
}

int sinema_x_error(Display*, 
		   XErrorEvent* error)
{
    TRACE_ERROR( << *error);
    dumpBacktrace();    

    return 0;
    
}
int sinema_x_io_error(Display*)
{
    TRACE_ERROR();
    return 0;
}

void attachOwnXErrorHandler()
{
    // Start debugger with:
    //    ddd --args  /home/joachim/bin/sinema --sync *
    // and set brekpoint on sinema_x_error.
    XSetErrorHandler(sinema_x_error);
    XSetIOErrorHandler(sinema_x_io_error);
}

std::ostream& operator<<(std::ostream& strm, const XVisualInfo& xvi)
{
    strm.setf(ios::showbase);
    strm << "XVisualInfo:" << endl;
    strm << "Visual        = " << xvi.visual << endl;
    strm << "VisualID      = " << xvi.visualid << endl;
    strm << "screen        = " << xvi.screen << endl;
    strm << "depth         = " << xvi.depth << endl;
    strm << "class         = " << xvi.c_class << endl;
    strm << "red_mask      = " << xvi.red_mask << endl;
    strm << "green_mask    = " << xvi.green_mask << endl;
    strm << "blue_mask     = " << xvi.blue_mask << endl;
    strm << "colormap_size = " << xvi.colormap_size << endl;
    strm << "bits_per_rgb  = " << xvi.bits_per_rgb << endl;

    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvVersionInfo& xvi)
{
    strm.setf(ios::showbase);
    strm << "XvVersionInfo:" << endl;
    strm << "version      = " << xvi.version << endl;
    strm << "revision     = " << xvi.revision << endl;
    strm << "request_base = " << xvi.request_base << endl;
    strm << "event_base   = " << xvi.event_base << endl;
    strm << "error_base   = " << xvi.error_base << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvFormat& f)
{
    strm << "depth=" << int(f.depth) << ", visual_id=" << f.visual_id;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvAdaptorInfo& ai)
{
    strm.setf(ios::showbase);
    strm << "base_id   = " << ai.base_id << endl;
    strm << "num_ports = " << ai.num_ports << endl;
    strm << "type      = " << int(ai.type) << " = "
	 << ((ai.type & XvInputMask) ?  "input | " : "" )
	 << ((ai.type & XvOutputMask) ? "output | " : "" )
	 << ((ai.type & XvVideoMask) ?  "video | " : "" )
	 << ((ai.type & XvStillMask) ?  "still | " : "" )
	 << ((ai.type & XvImageMask) ?  "image | " : "" )
	 << endl;
    strm << "name      = " << ai.name << endl;
    for (unsigned int i=0; i<ai.num_formats; i++)
	strm << "formats["<<i<<"] = " << ai.formats[i] << endl;
    strm << "num_adaptors = " << ai.num_adaptors << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvEncodingInfo& ei)
{
    strm.setf(ios::showbase);
    strm << "XvEncodingID   = " << ei.encoding_id << endl;
    strm << "name           = " << ei.name << endl;
    strm << "width * height = " << ei.width << " * " 
	                        << ei.height << endl;
    strm << "rate           = " << ei.rate.numerator << "/" 
	                        << ei.rate.denominator << endl;
    strm << "num_encodings  = " << ei.num_encodings << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvAttribute& att)
{
    strm.setf(ios::showbase);
    strm << "name      = " << att.name << endl;
    strm << "flags     = " << att.flags << endl;
    strm << "min_value = " << att.min_value << endl;
    strm << "max_value = " << att.max_value << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvImageFormatValues& ifv)
{
    strm.setf(ios::showbase);
    strm << "id      = " << std::hex << ifv.id << std::dec << endl;
    strm << "type    = " << ifv.type << " = "
	 << ((ifv.type & XvRGB) ? "XvRGB" : "" )
	 << ((ifv.type & XvYUV) ? "XvYUV" : "" ) << endl;
    strm << "byte_order = " << ifv.byte_order << " = "
	 << ((ifv.byte_order & LSBFirst) ? "LSBFirst" : "" )
	 << ((ifv.byte_order & MSBFirst) ? "MSBFirst" : "" ) << endl;
    // Globally Unique IDentifier (char guid[16];)
    strm << "guid       = " << &ifv.guid[0] << endl;
    strm << "bits_per_pixel = " << ifv.bits_per_pixel << endl;
    strm << "format     = " << ifv.format << " = "
	 << ((ifv.format & XvPacked) ? "XvPacked" : "" )
	 << ((ifv.format & XvPlanar) ? "XvPlanar" : "" ) << endl;

    if (ifv.type & XvRGB)
    {
	strm << "depth      = " << ifv.depth << endl;
	strm << "red_mask   = " << ifv.red_mask << endl;
	strm << "green_mask = " << ifv.green_mask << endl;
	strm << "blue_mask  = " << ifv.blue_mask << endl;
    }
    else if (ifv.type & XvYUV)
    {
	strm << "y_sample_bits      = " << ifv.y_sample_bits << endl;
	strm << "u_sample_bits      = " << ifv.u_sample_bits << endl;
	strm << "v_sample_bits      = " << ifv.v_sample_bits << endl;
	strm << "horz_y_period      = " << ifv.horz_y_period << endl;
	strm << "horz_u_period      = " << ifv.horz_u_period << endl;
	strm << "horz_v_period      = " << ifv.horz_v_period << endl;
	strm << "vert_y_period      = " << ifv.vert_y_period << endl;
	strm << "vert_u_period      = " << ifv.vert_u_period << endl;
	strm << "vert_v_period      = " << ifv.vert_v_period << endl;
	strm << "component_order    = ";
	for (int i=0; i<32; i++)
	    strm << ifv.component_order[i];
	strm << endl;
	strm << "scanline_order = " << ifv.scanline_order << " = "
	     << ((ifv.scanline_order & XvTopToBottom) ? "XvTopToBottom" : "" )
	     << ((ifv.scanline_order & XvBottomToTop) ? "XvBottomToTop" : "" ) << endl;
    }

    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvImage& xi)
{
    strm.setf(ios::showbase);
    strm << "XvImage(id=" << std::hex << xi.id << std::dec;
    strm << ", width=" << xi.width;
    strm << ", height=" << xi.height;
    strm << ", data_size=" << xi.data_size;
    strm << ", num_planes=" << xi.num_planes << ")";
    // strm << " = " <<  << endl;

    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XErrorEvent& xee)
{
    const int len = 256;
    char buffer[len];

    XGetErrorText(xee.display, xee.error_code, buffer, len);

    strm.setf(ios::showbase);
    strm << "XErrorEvent(type=" << xee.type
	 << ", resourceid=" << xee.resourceid
	 << ", serial=" << xee.serial
	 << ", error_code=" << (int)xee.error_code
	 << ", request_code=" << (int)xee.request_code 
	 << ", minor_code=" << (int)xee.minor_code
	 << ", " << buffer << ")";
    return strm;
}
