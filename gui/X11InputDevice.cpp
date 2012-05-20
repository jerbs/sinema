//
// Remote Control
//
// Copyright (C) Joachim Erbs, 2012
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

#include "gui/X11InputDevice.hpp"
#include "platform/Logging.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xatom.h>
#include <string.h>

// #undef TRACE_DEBUG
// #define TRACE_DEBUG(s) std::cout << __PRETTY_FUNCTION__ << " " s << std::endl;

// Disable the X11 input device having name name.
// On the command line the same can be done with
// "xinput list" and "xinput set prop ....".

void X11InputDevice::disable(std::string name)
{
    TRACE_DEBUG(<< name);

    Display *display = XOpenDisplay(NULL);
    if (display == NULL)
    {
	TRACE_ERROR(<< "XOpenDisplay failed");
	return;
    }

    int xi_opcode;
    int event;
    int error;
    if (!XQueryExtension(display, INAME, &xi_opcode, &event, &error))
    {
        TRACE_ERROR(<< "XInputExtension not available");
        return;
    }

    XExtensionVersion	*version = XGetExtensionVersion(display, INAME);
    short major_version = version->major_version;
    XFree(version);

    if (major_version == XI_2_Major)
    {
	int ndevices;
	XIDeviceInfo *info = XIQueryDevice(display, XIAllDevices, &ndevices);

	for (int i=0; i<ndevices; i++)
	{
	    TRACE_DEBUG(<< info[i].deviceid << ": " << info[i].name);
	    if ( (info[i].use == XISlaveKeyboard) &&
		 (name == info[i].name) )
	    {
		int nprops;
		Atom* props = XIListProperties(display, info[i].deviceid, &nprops);

		for (int j=0; j<nprops; j++)
		{
		    const char* property_name = XGetAtomName(display, props[j]);
		    TRACE_DEBUG(<< "property: " << property_name << "(" << props[j] << ")");

		    Atom act_type;
		    int act_format;
		    unsigned long nitems, bytes_after;
		    unsigned char * data;
		    if (XIGetProperty(display,
				      info[i].deviceid,
				      props[j],
				      0, // offset
				      1000, // length
				      False, // delete_property,
				      AnyPropertyType,
				      &act_type, &act_format,
				      &nitems, &bytes_after, &data) == Success)
		    {
			TRACE_DEBUG(<< "property: " << property_name
				    << ", act_format = " << act_format
				    << ", act_type = " << act_type
				    << ", nitems = " << nitems);
			if (strcmp(property_name, "Device Enabled") == 0 &&
			    nitems == 1 &&
			    act_type == XA_INTEGER)
			{
			    int value = -1;

			    switch (act_format)
			    {
			    case  8: value = *((char*)data); break;
			    case 16: value = *((short*)data); break;
			    case 32: value = *((long*)data); break;
			    }

			    TRACE_DEBUG(<< "property: " << property_name
					<< "(" << props[j] << ") : "
					<< value << "->" << 0);

			    switch (act_format)
			    {
			    case  8: *((char*)data) = 0; break;
			    case 16: *((short*)data) = 0; break;
			    case 32: *((long*)data) = 0; break;
			    }

			    XIChangeProperty(display,
					     info[i].deviceid,
					     props[j],
					     act_type,
					     act_format,
					     PropModeReplace,
					     data,
					     1);               // num_items
			}
		    }
		}

		XFree(props);
	    }
	}

	XIFreeDeviceInfo(info);
    }
    else
    {
	TRACE_ERROR(<< INAME << " major version " << major_version << " not supported.");
    }

    XSync(display, False);
    XCloseDisplay(display);
}
