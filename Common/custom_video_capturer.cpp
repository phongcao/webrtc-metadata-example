#include "stdafx.h"

#include <fstream>

#include "custom_video_capturer.h"

CustomVideoCapturer::CustomVideoCapturer() :
	clock_(webrtc::Clock::GetRealTimeClock()),
	running_(false),
	sink_(nullptr),
	sink_wants_observer_(nullptr)
{
	set_enable_video_adapter(false);
	SetCaptureFormat(NULL);
}

cricket::CaptureState CustomVideoCapturer::Start(const cricket::VideoFormat& format)
{
	SetCaptureFormat(&format);
	running_ = true;
	SetCaptureState(cricket::CS_RUNNING);
	return cricket::CS_RUNNING;
}

void CustomVideoCapturer::Stop()
{
	rtc::CritScope cs(&lock_);
	running_ = false;
}

void CustomVideoCapturer::SetSinkWantsObserver(SinkWantsObserver* observer)
{
	rtc::CritScope cs(&lock_);
	RTC_DCHECK(!sink_wants_observer_);
	sink_wants_observer_ = observer;
}

void CustomVideoCapturer::AddOrUpdateSink(
	rtc::VideoSinkInterface<VideoFrame>* sink,
	const rtc::VideoSinkWants& wants)
{
	rtc::CritScope cs(&lock_);
	sink_ = sink;
	if (sink_wants_observer_)
	{
		sink_wants_observer_->OnSinkWantsChanged(sink, wants);
	}
}

bool CustomVideoCapturer::IsRunning()
{
	return running_;
}

bool CustomVideoCapturer::IsScreencast() const
{
	return false;
}

bool CustomVideoCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs)
{
	fourccs->push_back(cricket::FOURCC_H264);
	return true;
}

void CustomVideoCapturer::RemoveSink(rtc::VideoSinkInterface<VideoFrame>* sink)
{
	rtc::CritScope cs(&lock_);
	sink_ = nullptr;
}

void CustomVideoCapturer::SendFrame(webrtc::VideoFrame video_frame)
{
	// The video capturer hasn't started since there is no active connection.
	if (!running_)
	{
		return;
	}

	if (sink_)
	{
		sink_->OnFrame(video_frame);
	}
	else
	{
		OnFrame(video_frame, video_frame.width(), video_frame.height());
	}
}
