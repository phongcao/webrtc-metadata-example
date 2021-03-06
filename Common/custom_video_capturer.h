/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once

#include <string.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

#include "webrtc/api/video/i420_buffer.h"
#include "webrtc/api/video/video_frame.h"
#include "webrtc/base/asyncinvoker.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/timeutils.h"
#include "webrtc/media/base/videocapturer.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/system_wrappers/include/clock.h"
#include "webrtc/typedefs.h"

#include "libyuv/convert.h"

using namespace webrtc;

class SinkWantsObserver
{
public:
	// OnSinkWantsChanged is called when FrameGeneratorCapturer::AddOrUpdateSink
	// is called.
	virtual void OnSinkWantsChanged(rtc::VideoSinkInterface<VideoFrame>* sink,
		const rtc::VideoSinkWants& wants) = 0;

protected:
	virtual ~SinkWantsObserver() {}
};

// Custom video capturer that allows sending frame buffers.
class CustomVideoCapturer : public cricket::VideoCapturer
{
public:
	CustomVideoCapturer();

	~CustomVideoCapturer()
	{
		SignalDestroyed(this);
	}

	cricket::CaptureState Start(const cricket::VideoFormat& capture_format) override;
	void Stop() override;

	void SetSinkWantsObserver(SinkWantsObserver* observer);
	bool IsRunning() override;
	bool IsScreencast() const override;
	bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override;

	void AddOrUpdateSink(rtc::VideoSinkInterface<VideoFrame>* sink,
		const rtc::VideoSinkWants& wants) override;

	void RemoveSink(rtc::VideoSinkInterface<VideoFrame>* sink) override;

	void SendFrame(webrtc::VideoFrame video_frame);

	sigslot::signal1<CustomVideoCapturer*> SignalDestroyed;

protected:
	bool running_;
	rtc::VideoSinkInterface<VideoFrame>* sink_;
	SinkWantsObserver* sink_wants_observer_;
	rtc::CriticalSection lock_;
};
