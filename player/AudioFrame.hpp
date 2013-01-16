//
// Audio Frame
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

#ifndef AUDIO_FRAME_HPP
#define AUDIO_FRAME_HPP

class AudioFrame
{
public:
    AudioFrame(int size)
	: allocatedByteSize(size),
	  frameByteSize(0),
	  buf(new char[size]),
	  pts(0),
	  offset(0)
    {}
    ~AudioFrame()
    {
	delete[](buf);
    }

    void setFrameByteSize(int size) {frameByteSize = size;}
    int getFrameByteSize() {return frameByteSize-offset;}
    int numAllocatedBytes() {return allocatedByteSize;}
    char* data() {return &buf[offset];}
    char* consume(int bytes)
    {
	int off = offset;
	offset += bytes;
	if (offset > frameByteSize)
	{
	    TRACE_ERROR(<<"Consumed too much data.");
	    exit(-1);
	}
	return &buf[off];
    }

    void setPTS(double pts_) {pts = pts_;}
    double getPTS() {return pts;}

    void setNextPTS(double pts_) {nextPts = pts_;}
    double getNextPTS() {return nextPts;}

    void reset()
    {
	frameByteSize = 0;
	pts = 0;
	offset = 0;
    }

    void restore()
    {
	offset = 0;
    }

    bool atBegin()
    {
	return (offset == 0) ? true : false;
    }

    bool atEnd()
    {
	return (offset == frameByteSize) ? true : false;
    }

    char* getBufferAddr() {return buf;}


private:
    AudioFrame();
    AudioFrame(const AudioFrame&);

    int allocatedByteSize;
    int frameByteSize;
    char* buf;
    double pts;
    double nextPts;
    int offset;
};

#endif
