//
// X11 and Xv Extension Interface
//
// Copyright (C) Joachim Erbs, 2009
//

#ifndef XLIB_FACADE_HPP
#define XLIB_FACADE_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XShm.h>  // has to be included before Xvlib.h
#include <X11/extensions/Xvlib.h>

#include <boost/shared_ptr.hpp>
#include <string>

struct XFException
{
    XFException(const std::string &s) : s(s) {}
    std::string s;
};

class XFVideoImage;

class XFVideo
{
    friend class XFVideoImage;

public:
    XFVideo(Display* display, Window window,
	    unsigned int width, unsigned int height);
    ~XFVideo();

    void selectEvents();
    void resize(unsigned int width, unsigned int height);
    boost::shared_ptr<XFVideoImage> show(boost::shared_ptr<XFVideoImage> xfVideoImage);
    void show();
    void handleConfigureEvent();
    void handleExposeEvent();
    Display* display() {return m_display;}
    Window window() {return m_window;}
    
private:
    XFVideo();
    XFVideo(const XFVideo&);

    void calculateDestinationArea();
    void paintBorder();

    boost::shared_ptr<XFVideoImage> m_displayedImage;
    Display* m_display;
    Window m_window;
    
    XvPortID xvPortId;
    int imageFormat;
    GC gc;

    unsigned int yuvWidth;
    unsigned int yuvHeight;
    double ratio;   //  yuvWidth / yuvHeight
    double iratio;  //  yuvHeight / yuvWidth

    // window size:
    unsigned int width;
    unsigned int height;

    // sub area of window displaying the video:
    unsigned int leftDest;
    unsigned int topDest;
    unsigned int widthDest;
    unsigned int heightDest;
};

class XFVideoImage
{
    friend class XFVideo;

public:
    XFVideoImage(boost::shared_ptr<XFVideo> xfVideo);
    ~XFVideoImage();

    void createBlackImage();
    void createDemoImage();

    unsigned int width() {return yuvImage->width;}
    unsigned int height() {return yuvImage->height;}
    char* data() {return yuvImage->data;}
    XvImage* xvImage() {return yuvImage;}

    void setPTS(double pts_) {pts = pts_;}
    double getPTS() {return pts;}

private:
    XShmSegmentInfo yuvShmInfo;
    XvImage* yuvImage;
    void* shmAddr;
    double pts;
};

#endif
