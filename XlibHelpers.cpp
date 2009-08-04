#include "XlibHelpers.hpp"

using namespace std;

std::ostream& operator<<(std::ostream& strm, const XVisualInfo& xvi)
{
    cout.setf(ios::showbase);
    cout << std::hex << "XVisualInfo:" << endl;
    cout << "Visual        = " << xvi.visual << endl;
    cout << "VisualID      = " << xvi.visualid << endl;
    cout << "screen        = " << xvi.screen << endl;
    cout << "depth         = " << xvi.depth << endl;
    cout << "class         = " << xvi.c_class << endl;
    cout << "red_mask      = " << xvi.red_mask << endl;
    cout << "green_mask    = " << xvi.green_mask << endl;
    cout << "blue_mask     = " << xvi.blue_mask << endl;
    cout << "colormap_size = " << xvi.colormap_size << endl;
    cout << "bits_per_rgb  = " << xvi.bits_per_rgb << endl;

    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvVersionInfo& xvi)
{
    cout.setf(ios::showbase);
    cout << std::hex << "XvVersionInfo:" << endl;
    cout << "version      = " << xvi.version << endl;
    cout << "revision     = " << xvi.revision << endl;
    cout << "request_base = " << xvi.request_base << endl;
    cout << "event_base   = " << xvi.event_base << endl;
    cout << "error_base   = " << xvi.error_base << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvFormat& f)
{
    cout << "depth=" << int(f.depth) << ", visual_id=" << f.visual_id;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvAdaptorInfo& ai)
{
    cout.setf(ios::showbase);
    cout << std::hex;
    cout << "base_id   = " << ai.base_id << endl;
    cout << "num_ports = " << ai.num_ports << endl;
    cout << "type      = " << int(ai.type) << " = "
	 << ((ai.type & XvInputMask) ?  "input | " : "" )
	 << ((ai.type & XvOutputMask) ? "output | " : "" )
	 << ((ai.type & XvVideoMask) ?  "video | " : "" )
	 << ((ai.type & XvStillMask) ?  "still | " : "" )
	 << ((ai.type & XvImageMask) ?  "image | " : "" )
	 << endl;
    cout << "name      = " << ai.name << endl;
    for (unsigned int i=0; i<ai.num_formats; i++)
	cout << "formats["<<i<<"] = " << ai.formats[i] << endl;
    cout << "num_adaptors = " << ai.num_adaptors << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvEncodingInfo& ei)
{
    cout.setf(ios::showbase);
    cout << std::hex;
    cout << "XvEncodingID   = " << ei.encoding_id << endl;
    cout << "name           = " << ei.name << endl;
    cout << "width * height = " << ei.width << " * " 
	                        << ei.height << endl;
    cout << "rate           = " << ei.rate.numerator << "/" 
	                        << ei.rate.denominator << endl;
    cout << "num_encodings  = " << ei.num_encodings << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvAttribute& att)
{
    cout.setf(ios::showbase);
    cout << std::hex;
    cout << "name      = " << att.name << endl;
    cout << "flags     = " << att.flags << endl << std::dec;
    cout << "min_value = " << att.min_value << endl;
    cout << "max_value = " << att.max_value << endl;
    return strm;
}

std::ostream& operator<<(std::ostream& strm, const XvImageFormatValues& ifv)
{
    cout.setf(ios::showbase);
    cout << std::hex;
    cout << "id      = " << ifv.id << endl;
    cout << "type    = " << ifv.type << " = "
	 << ((ifv.type & XvRGB) ? "XvRGB" : "" )
	 << ((ifv.type & XvYUV) ? "XvYUV" : "" ) << endl;
    cout << "byte_order = " << ifv.byte_order << " = "
	 << ((ifv.byte_order & LSBFirst) ? "LSBFirst" : "" )
	 << ((ifv.byte_order & MSBFirst) ? "MSBFirst" : "" ) << endl;
    cout.unsetf(ios::showbase);
    cout << "guid       = 0x";
    for (int i=0; i<16; i++)
	cout << int(ifv.guid[i]);
    cout << endl;
    cout.setf(ios::showbase);
    cout << "bits_per_pixel = " << ifv.bits_per_pixel << endl;
    cout << "format     = " << ifv.format << " = "
	 << ((ifv.format & XvPacked) ? "XvPacked" : "" )
	 << ((ifv.format & XvPlanar) ? "XvPlanar" : "" ) << endl;

    if (ifv.type & XvRGB)
    {
	cout << "depth      = " << ifv.depth << endl;
	cout << "red_mask   = " << ifv.red_mask << endl;
	cout << "green_mask = " << ifv.green_mask << endl;
	cout << "blue_mask  = " << ifv.blue_mask << endl;
    }
    else if (ifv.type & XvYUV)
    {
	cout << "y_sample_bits      = " << ifv.y_sample_bits << endl;
	cout << "u_sample_bits      = " << ifv.u_sample_bits << endl;
	cout << "v_sample_bits      = " << ifv.v_sample_bits << endl;
	cout << "horz_y_period      = " << ifv.horz_y_period << endl;
	cout << "horz_u_period      = " << ifv.horz_u_period << endl;
	cout << "horz_v_period      = " << ifv.horz_v_period << endl;
	cout << "vert_y_period      = " << ifv.vert_y_period << endl;
	cout << "vert_u_period      = " << ifv.vert_u_period << endl;
	cout << "vert_v_period      = " << ifv.vert_v_period << endl;
	cout << "component_order    = ";
	for (int i=0; i<32; i++)
	    cout << ifv.component_order[i];
	cout << endl;
	cout << "scanline_order = " << ifv.scanline_order << " = "
	     << ((ifv.scanline_order & XvTopToBottom) ? "XvTopToBottom" : "" )
	     << ((ifv.scanline_order & XvBottomToTop) ? "XvBottomToTop" : "" ) << endl;
    }

    return strm;
}
