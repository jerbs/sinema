//
// X11 and Xv Extension Interface
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

#ifndef XLIB_FACADE_HPP
#define XLIB_FACADE_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XShm.h>  // has to be included before Xvlib.h
#include <X11/extensions/Xvlib.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>
#include <memory>
#include <string>

#include "player/GeneralEvents.hpp"
#include "player/XlibHelpers.hpp"

struct XFException
{
    XFException(const std::string &s) : s(s) {}
    std::string s;
};


class XFDisplay
{
public:
    XFDisplay();
    ~XFDisplay();

    Display* display() {return m_display;}

private:
    Display* m_display;
};


class XFWindow
{
public:
    XFWindow(unsigned int width, unsigned int height);
    ~XFWindow();

    Display* display() {return m_display;}
    Window window() {return m_window;}
    //unsigned int width() {return m_width;}
    //unsigned int height() {return m_height;}

private:
    XFWindow();
    XFWindow(const XFWindow&);

    boost::shared_ptr<XFDisplay> m_xfDisplay;
    Display* m_display;
    Window m_window;

    unsigned int m_width;
    unsigned int m_height;
};


class NeedsXShm
{
public:
    NeedsXShm(Display* display);

private:
    NeedsXShm();
};


class NeedsXv
{
public:
    NeedsXv(Display* display);

protected:
    XvVersionInfo xvVersionInfo;

private:
    NeedsXv();
};


class XFVideoImage;


class XFVideo : private NeedsXShm,
		private NeedsXv
{
    friend class XFVideoImage;
    friend class XvWindowSystemEventFilterFunctor;

public:
    typedef boost::function<void (boost::shared_ptr<NotificationVideoSize>)> send_notification_video_size_fct_t;
    typedef boost::function<void (boost::shared_ptr<NotificationClipping>)> send_notification_clipping_fct_t;
    typedef boost::function<void (boost::shared_ptr<NotificationVideoAttribute>)> send_notification_video_attribute_fct_t;

    XFVideo(Display* display, Window window,
	    unsigned int width, unsigned int height,
	    send_notification_video_size_fct_t fct,
	    send_notification_clipping_fct_t fct2,
	    send_notification_video_attribute_fct_t fct3,
	    boost::shared_ptr<AddWindowSystemEventFilterFunctor> addWindowSystemEventFilter);
    ~XFVideo();

    void selectEvents();
    void resize(unsigned int width, unsigned int height,
		unsigned int sarNom, unsigned int sarDen,
		int fourccFormat);
    std::unique_ptr<XFVideoImage> show(std::unique_ptr<XFVideoImage> xfVideoImage);
    void show();
    void handleConfigureEvent(boost::shared_ptr<WindowConfigureEvent> event);
    void handleExposeEvent();
    void clipDst(int windowLeft, int windowRight, int windowTop, int windowBottom);
    void clipSrc(int videoLeft, int videoRight, int videoTop, int videoBottom);
    void enableXvClipping();
    void disableXvClipping();
    void setXvPortAttributes(std::string const& name, int value);
    Display* display() {return m_display;}
    Window window() {return m_window;}

private:
    XFVideo();
    XFVideo(const XFVideo&);

    void calculateDestinationArea(NotificationVideoSize::Reason reason);
    void paintBorder();

    void clip(XvImage* yuvImageIn,
	      XvImage* yuvImageOut);

    int xWindow(int xVideo);
    int yWindow(int yVideo);
    int xVideo(int xWindow);
    int yVideo(int yWindow);

    bool isFourccFormatValid(int fourccFormat);

    bool grabXvPort(XvPortID xvPortId);
    void grabXvPort();
    void ungrabXvPort();
    void regrabXvPort();

    void dumpXvEncodings(XvPortID adaptorPort);
    void dumpXvAttributes(XvPortID adaptorPort);
    void dumpXvImageFormat(XvPortID adaptorPort);
    void fillFourccFormatList();
    void notifyXvPortAttributes();
    void selectXvPortNotify();

    std::unique_ptr<XFVideoImage> m_displayedImage;
    std::unique_ptr<XFVideoImage> m_displayedImageClipped;
    Display* m_display;
    Window m_window;
    
    XvPortID xvPortId;
    int fourccFormat;
    int displayedFourccFormat;
    std::vector<int> fourccFormatList;
    GC gc;

    // video size:
    unsigned int widthVid;
    unsigned int heightVid;

    // window size:
    unsigned int widthWin;
    unsigned int heightWin;

    // sub area of window displaying the video:
    unsigned int leftDest;
    unsigned int topDest;
    unsigned int widthDest;
    unsigned int heightDest;

    // sub area of video shown on display:
    unsigned int leftSrc;
    unsigned int topSrc;
    unsigned int widthSrc;
    unsigned int heightSrc;

    // pixel aspect ratio:
    unsigned int parNum;
    unsigned int parDen;

    // calculated value:
    bool m_noClippingNeeded;

    // configuration value:
    bool m_useXvClipping;

    send_notification_video_size_fct_t sendNotificationVideoSize;
    send_notification_clipping_fct_t sendNotificationClipping;
    send_notification_video_attribute_fct_t sendNotificationVideoAttribute;

    boost::shared_ptr<AddWindowSystemEventFilterFunctor> addWindowSystemEventFilter;
};


class XFVideoImage
{
    friend class XFVideo;

public:
    XFVideoImage(boost::shared_ptr<XFVideo> xfVideo);
    XFVideoImage(XFVideo* xfVideo, int width, int height, int fourccFormat);
    ~XFVideoImage();

    void createPatternImage();
    void createBlackImage();
    void createDemoImage();

    unsigned int width() {return yuvImage->width;}
    unsigned int height() {return yuvImage->height;}
    int requestedWidth() {return m_requestedWidth;}
    int requestedHeight() {return m_requestedHeight;}
    
    char* data() {return yuvImage->data;}
    XvImage* xvImage() {return yuvImage;}

    void setPTS(double pts_) {pts = pts_;}
    double getPTS() {return pts;}

private:
    XFVideoImage();  // No implementation.
    void init(XFVideo* xfVideo, int width, int height, int fourccFormat);

    XShmSegmentInfo yuvShmInfo;
    XvImage* yuvImage;
    void* shmAddr;
    double pts;
    Display* m_display;

    int m_requestedWidth;
    int m_requestedHeight;
};

#endif
