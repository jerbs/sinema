//
// Media Recorder Events
//
// Copyright (C) Joachim Erbs, 2010
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

#ifndef RECORDER_GENERAL_EVENTS_HPP
#define RECORDER_GENERAL_EVENTS_HPP

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <string>

class MediaRecorder;
class Recorder;
class RecorderAdapter;

class RecorderThreadNotification
{
public:
    typedef void (*fct_t)();

    RecorderThreadNotification();
    static void setCallback(fct_t fct);

private:
    static fct_t m_fct;
};

struct RecorderInitEvent
{
    MediaRecorder* mediaRecorder;
    boost::shared_ptr<Recorder> recorder;
    boost::shared_ptr<RecorderAdapter> recorderAdapter;
};

struct StartRecordingReq
{
    StartRecordingReq(std::string filename)
	: filename(filename)
    {}

    std::string filename;
};

struct StartRecordingResp
{
    StartRecordingResp(std::string tempFilename, int error)
	: tempFilename(tempFilename),
	  error(error)
    {}

    std::string tempFilename;
    int error;
};

struct StartRecordingSReq
{
    StartRecordingSReq(boost::shared_ptr<StartRecordingReq> request,
		       boost::promise<boost::shared_ptr<StartRecordingResp> >&& promise)
	: request(request),
	  promise(std::move(promise))
    {}

    boost::shared_ptr<StartRecordingReq> request;
    boost::promise<boost::shared_ptr<StartRecordingResp> > promise;
};

struct StopRecordingReq
{
};

struct StopRecordingResp
{
    StopRecordingResp(int error)
	: error(error)
    {}

    int error;
};

struct StopRecordingSReq
{
    StopRecordingSReq(boost::shared_ptr<StopRecordingReq> request,
		      boost::promise<boost::shared_ptr<StopRecordingResp> >&& promise)
	: request(request),
	  promise(std::move(promise))
    {}

    boost::shared_ptr<StopRecordingReq> request;
    boost::promise<boost::shared_ptr<StopRecordingResp> > promise;
};

#endif
