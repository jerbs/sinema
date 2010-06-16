#include "XlibHelpers.hpp"

using namespace std;

std::ostream& operator<<(std::ostream& strm, const XVisualInfo& xvi)
{
    strm.setf(ios::showbase);
    strm << std::hex << "XVisualInfo:" << endl;
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
    strm << std::hex << "XvVersionInfo:" << endl;
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
    strm << std::hex;
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
    strm << std::hex;
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
    strm << std::hex;
    strm << "name      = " << att.name << endl;
    strm << "flags     = " << att.flags << endl << std::dec;
    strm << "min_value = " << att.min_value << endl;
    strm << "max_value = " << att.max_value << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvImageFormatValues& ifv)
{
    strm.setf(ios::showbase);
    strm << std::hex;
    strm << "id      = " << ifv.id << endl;
    strm << "type    = " << ifv.type << " = "
	 << ((ifv.type & XvRGB) ? "XvRGB" : "" )
	 << ((ifv.type & XvYUV) ? "XvYUV" : "" ) << endl;
    strm << "byte_order = " << ifv.byte_order << " = "
	 << ((ifv.byte_order & LSBFirst) ? "LSBFirst" : "" )
	 << ((ifv.byte_order & MSBFirst) ? "MSBFirst" : "" ) << endl;
    strm.unsetf(ios::showbase);
    strm << "guid       = 0x";
    for (int i=0; i<16; i++)
	strm << int(ifv.guid[i]);
    strm << endl;
    strm.setf(ios::showbase);
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
