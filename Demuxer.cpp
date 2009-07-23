#include "Demuxer.hpp"
#include "FileReader.hpp"

void Demuxer::process(boost::shared_ptr<StartEvent> event)
{
    DEBUG();
    fileReader->queue_event(boost::make_shared<SystemStreamGetMoreDataEvent>());
}

void Demuxer::process(boost::shared_ptr<StopEvent> event)
{
    DEBUG();
}

void Demuxer::process(boost::shared_ptr<SystemStreamChunkEvent> event)
{
    // DEBUG();
    fileReader->queue_event(boost::make_shared<SystemStreamGetMoreDataEvent>());
}
