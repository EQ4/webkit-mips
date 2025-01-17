/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "AVAudioCaptureSource.h"

#if ENABLE(MEDIA_STREAM) && USE(AVFOUNDATION)

#import "Logging.h"
#import "MediaConstraints.h"
#import "NotImplemented.h"
#import "RealtimeMediaSourceStates.h"
#import "SoftLinking.h"
#import "WebAudioSourceProviderAVFObjC.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudioTypes.h>
#import <wtf/HashSet.h>

#import "CoreMediaSoftLink.h"

typedef AVCaptureConnection AVCaptureConnectionType;
typedef AVCaptureDevice AVCaptureDeviceType;
typedef AVCaptureDeviceInput AVCaptureDeviceInputType;
typedef AVCaptureOutput AVCaptureOutputType;
typedef AVCaptureAudioDataOutput AVCaptureAudioDataOutputType;

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)

SOFT_LINK_CLASS(AVFoundation, AVCaptureAudioDataOutput)
SOFT_LINK_CLASS(AVFoundation, AVCaptureConnection)
SOFT_LINK_CLASS(AVFoundation, AVCaptureDevice)
SOFT_LINK_CLASS(AVFoundation, AVCaptureDeviceInput)
SOFT_LINK_CLASS(AVFoundation, AVCaptureOutput)

SOFT_LINK_POINTER(AVFoundation, AVMediaTypeAudio, NSString *)

#define AVMediaTypeAudio getAVMediaTypeAudio()

namespace WebCore {

RefPtr<AVMediaCaptureSource> AVAudioCaptureSource::create(AVCaptureDeviceType* device, const AtomicString& id, PassRefPtr<MediaConstraints> constraint)
{
    return adoptRef(new AVAudioCaptureSource(device, id, constraint));
}
    
AVAudioCaptureSource::AVAudioCaptureSource(AVCaptureDeviceType* device, const AtomicString& id, PassRefPtr<MediaConstraints> constraints)
    : AVMediaCaptureSource(device, id, RealtimeMediaSource::Audio, constraints)
{
    currentStates()->setSourceId(id);
    currentStates()->setSourceType(RealtimeMediaSourceStates::Microphone);
    m_inputDescription = std::make_unique<AudioStreamBasicDescription>();
}
    
AVAudioCaptureSource::~AVAudioCaptureSource()
{
}

RefPtr<RealtimeMediaSourceCapabilities> AVAudioCaptureSource::capabilities() const
{
    notImplemented();
    return nullptr;
}

void AVAudioCaptureSource::updateStates()
{
    // FIXME: use [AVCaptureAudioPreviewOutput volume] for volume
}

void AVAudioCaptureSource::addObserver(AVAudioCaptureSource::Observer* observer)
{
    {
        LockHolder lock(m_lock);
        m_observers.append(observer);
    }

    if (m_inputDescription->mSampleRate)
        observer->prepare(m_inputDescription.get());
}

void AVAudioCaptureSource::removeObserver(AVAudioCaptureSource::Observer* observer)
{
    LockHolder lock(m_lock);
    size_t pos = m_observers.find(observer);
    if (pos != notFound)
        m_observers.remove(pos);
}

void AVAudioCaptureSource::setupCaptureSession()
{
    RetainPtr<AVCaptureDeviceInputType> audioIn = adoptNS([allocAVCaptureDeviceInputInstance() initWithDevice:device() error:nil]);
    ASSERT([session() canAddInput:audioIn.get()]);
    if ([session() canAddInput:audioIn.get()])
        [session() addInput:audioIn.get()];
    
    RetainPtr<AVCaptureAudioDataOutputType> audioOutput = adoptNS([allocAVCaptureAudioDataOutputInstance() init]);
    setAudioSampleBufferDelegate(audioOutput.get());
    ASSERT([session() canAddOutput:audioOutput.get()]);
    if ([session() canAddOutput:audioOutput.get()])
        [session() addOutput:audioOutput.get()];
    m_audioConnection = adoptNS([audioOutput.get() connectionWithMediaType:AVMediaTypeAudio]);
}

static bool operator==(const AudioStreamBasicDescription& a, const AudioStreamBasicDescription& b)
{
    return a.mSampleRate == b.mSampleRate
        && a.mFormatID == b.mFormatID
        && a.mFormatFlags == b.mFormatFlags
        && a.mBytesPerPacket == b.mBytesPerPacket
        && a.mFramesPerPacket == b.mFramesPerPacket
        && a.mBytesPerFrame == b.mBytesPerFrame
        && a.mChannelsPerFrame == b.mChannelsPerFrame
        && a.mBitsPerChannel == b.mBitsPerChannel;
}

static bool operator!=(const AudioStreamBasicDescription& a, const AudioStreamBasicDescription& b)
{
    return !(a == b);
}

void AVAudioCaptureSource::captureOutputDidOutputSampleBufferFromConnection(AVCaptureOutputType*, CMSampleBufferRef sampleBuffer, AVCaptureConnectionType*)
{
    Vector<Observer*> observers;
    {
        LockHolder lock(m_lock);
        if (m_observers.isEmpty())
            return;

        copyToVector(m_observers, observers);
    }

    CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(sampleBuffer);
    if (!formatDescription)
        return;

    const AudioStreamBasicDescription* streamDescription = CMAudioFormatDescriptionGetStreamBasicDescription(formatDescription);
    if (*m_inputDescription != *streamDescription) {
        m_inputDescription = std::make_unique<AudioStreamBasicDescription>(*streamDescription);
        for (auto& observer : observers)
            observer->prepare(m_inputDescription.get());
    }

    for (auto& observer : observers)
        observer->process(formatDescription, sampleBuffer);
}

AudioSourceProvider* AVAudioCaptureSource::audioSourceProvider()
{
    if (!m_audioSourceProvider)
        m_audioSourceProvider = WebAudioSourceProviderAVFObjC::create(*this);

    return m_audioSourceProvider.get();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
