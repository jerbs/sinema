//
// Media Recorder Events
//
// Copyright (C) Joachim Erbs
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
		       boost::detail::thread_move_t<boost::promise<boost::shared_ptr<StartRecordingResp> > > promise)
	: request(request),
	  promise(promise)
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
		      boost::detail::thread_move_t<boost::promise<boost::shared_ptr<StopRecordingResp> > > promise)
	: request(request),
	  promise(promise)
    {}

    boost::shared_ptr<StopRecordingReq> request;
    boost::promise<boost::shared_ptr<StopRecordingResp> > promise;
};

#endif
