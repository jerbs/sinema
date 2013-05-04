//
// JPEG Writer
//
// Copyright (C) Joachim Erbs, 2013
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

#include "player/JpegWriter.hpp"
#include "platform/Logging.hpp"

#include <jpeglib.h>
#include <string.h>
#include <math.h>

void JpegWriter::write(const char* base_name, double pts, AVFrame* avFrame)
{
    char fileName[1024];
    snprintf(fileName, sizeof(fileName), "/tmp/%s_%.3f.jpg", base_name, pts);

    TRACE_DEBUG(<< fileName);

    switch(avFrame->format)
    {
    case PIX_FMT_YUV420P:
	break;
    default:
	TRACE_ERROR(<< "Unsupported format: " << avFrame->format);
	return;
    }

    FILE* ofh = fopen(fileName, "wb");

    if (!ofh)
    {
	TRACE_ERROR(<< "fopen(" << fileName << ", wb) failed");
	return;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;


    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, ofh);
 
    cinfo.image_width      = avFrame->width;
    cinfo.image_height     = avFrame->height;
    cinfo.input_components = 1;
    cinfo.in_color_space   = JCS_GRAYSCALE;
    // cinfo.input_components = 3;
    // cinfo.in_color_space   = JCS_RGB;

    jpeg_set_defaults(&cinfo);

    /*set the quality [0..100]  */
    jpeg_set_quality (&cinfo, 75, true);
    jpeg_start_compress(&cinfo, true);

    TRACE_DEBUG(<< cinfo.image_width << " x " << cinfo.image_height);

    // JSAMPROW row_pointer;          /* pointer to a single row */

    uint8_t* Y = avFrame->data[0];
    int Yp = avFrame->linesize[0];

    while (cinfo.next_scanline < cinfo.image_height)
    {
	//	row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline*(screen_shot->depth>>3)*screen_shot->width];

	JSAMPROW row_pointer = Y + cinfo.next_scanline * Yp;
	jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    fclose(ofh);
}

class WaveDiagram
{
public:
    WaveDiagram(int pwidth, int pheight)
	: pwidth(pwidth),
	  pheight(pheight),
	  buffer(new uint8_t[pwidth*pheight])
    {
	memset(buffer, 10, pwidth*pheight);
    }

    ~WaveDiagram()
    {
	delete[] buffer;
    }

    void setXRange(double xmin, double xmax)
    {
	this->xmin = xmin;
	this->xmax = xmax;
    }

    void setYRange(double ymin, double ymax)
    {
	this->ymin = ymin;
	this->ymax = ymax;
    }

    void drawPoint(double x, double y)
    {
	int px = (x-xmin)/(xmax-xmin)*double(pwidth);
	int py = (ymax-y)/(ymax-ymin)*double(pheight);

	if (px<0) return;
	if (py<0) py = 0; // return;
	if (px>=pwidth) return;
	if (py>=pheight) py = pheight -1; // return;

	drawPixelPoint(px,py);
    }

    void drawVertical(double x)
    {
	int px = (x-xmin)/(xmax-xmin)*double(pwidth);
	if (px < 0) return;
	if (px >= pwidth) return;
	drawVerticalPixelLine(px,0,pheight-1);
    }

    uint8_t* getLineAddr(int py)
    {
	return &buffer[pwidth * py];
    }

private:
    void drawVerticalPixelLine(int px, int py1, int py2)
    {
	for (int py = py1; py<=py2; py++)
	{
	    drawPixelPoint(px,py);
	}
    }

    inline void drawPixelPoint(int px, int py)
    {
	buffer[py*pwidth + px] = 0xFF;
    }

    int pwidth;
    int pheight;
    uint8_t* buffer;

    double xmin, xmax;
    double ymin, ymax;
};

void JpegWriter::write(const char* base_name, double pts, AudioFrame* audioFrame,
		       unsigned int sample_rate, unsigned int channels, AVSampleFormat sample_format)
{
    char fileName[1024];
    snprintf(fileName, sizeof(fileName), "/tmp/%s_%.3f.jpg", base_name, pts);

    int bytesPerSample = 2;    // FIXME

    int frames = audioFrame->getFrameByteSize()/ (channels * bytesPerSample);
    double duration = double(frames) / double(sample_rate);

    TRACE_DEBUG(<< fileName << ", frames = " << frames << ", duration = " << duration);
    
    FILE* ofh = fopen(fileName, "wb");

    if (!ofh)
    {
	TRACE_ERROR(<< "fopen(" << fileName << ", wb) failed");
	return;
    }

    switch (sample_format)
    {
	//    case AV_SAMPLE_FMT_NONE:
	//    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_S16:
	bytesPerSample = 2;
	//    case AV_SAMPLE_FMT_S32:
	
	//    case AV_SAMPLE_FMT_FLT:
	//    case AV_SAMPLE_FMT_DBL:
	break;
    default:
	TRACE_ERROR(<< "Unsupported sample_format = " << sample_format);
	return;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;


    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, ofh);
 
    cinfo.image_width      = 1000;
    cinfo.image_height     = 100;
    cinfo.input_components = 1;
    cinfo.in_color_space   = JCS_GRAYSCALE;
    // cinfo.input_components = 3;
    // cinfo.in_color_space   = JCS_RGB;


    WaveDiagram waveDiagram(cinfo.image_width, cinfo.image_height);
    waveDiagram.setXRange(pts, pts + duration);
    waveDiagram.setYRange(-32000, 32000);

    
    for (double t = floor(100*pts) / 100; t < ceil((pts + duration)*100) / 100; t+=0.01)
    {
	TRACE_DEBUG(<< "vertical t= " << t);
	waveDiagram.drawVertical(t);
    }

    unsigned char* ptr = (unsigned char*)audioFrame->getBufferAddr();
    unsigned char* bufferEnd = (unsigned char*)ptr + audioFrame->getFrameByteSize();

    TRACE_DEBUG(<< hexDump(ptr, 100));
    int sample = 0;

    while (ptr < bufferEnd)
    {
	int16_t left  = *(ptr++) | *(ptr++) << 8 ;
	int16_t right = *(ptr++) | *(ptr++) << 8 ;
	double t = pts + double(sample) / double(sample_rate);

	if (sample < 100)
	TRACE_DEBUG(<< "audio: " << t << ": " << left << "," << right);

	waveDiagram.drawPoint(t, left);
	waveDiagram.drawPoint(t, right);

	sample++;
    }

    jpeg_set_defaults(&cinfo);

    /*set the quality [0..100]  */
    jpeg_set_quality (&cinfo, 75, true);
    jpeg_start_compress(&cinfo, true);

    TRACE_DEBUG(<< cinfo.image_width << " x " << cinfo.image_height);

    while (cinfo.next_scanline < cinfo.image_height)
    {
	JSAMPROW row_pointer = waveDiagram.getLineAddr(cinfo.next_scanline);
	jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    fclose(ofh);
}
