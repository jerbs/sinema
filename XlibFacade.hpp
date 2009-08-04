#ifndef XLIB_FACADE_HPP
#define XLIB_FACADE_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/XShm.h>  // has to be included before Xvlib.h
#include <X11/extensions/Xvlib.h>

#include <boost/shared_ptr.hpp>

struct XFException
{
    XFException(const char* s) : s(s) {}
    const char* s;
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

class XFVideoImage;

class XFVideo
{
    friend class XFVideoImage;

public:
    XFVideo(unsigned int width, unsigned int height);
    ~XFVideo();

    void selectEvents();
    void resize(unsigned int width, unsigned int height);
    void show(boost::shared_ptr<XFVideoImage> xfVideoImage);
    Display* display() {return m_display;}
    Window window() {return m_window;}
    
private:
    XFVideo();
    XFVideo(const XFVideo&);

    boost::shared_ptr<XFWindow> m_xfWindow;
    Display* m_display;
    Window m_window;
    
    XvPortID xvPortId;
    int imageFormat;
    GC gc;

    unsigned int yuvWidth;
    unsigned int yuvHeight;
    double ratio;   // w/h
    double iratio;  // h/w
};

class XFVideoImage
{
    friend class XFVideo;

public:
    XFVideoImage(boost::shared_ptr<XFVideo> xfVideo);
    ~XFVideoImage();

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
