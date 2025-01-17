/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebAudioSourceProviderAVFObjC_h
#define WebAudioSourceProviderAVFObjC_h

#if ENABLE(WEB_AUDIO) && ENABLE(MEDIA_STREAM)

#include "AVAudioCaptureSource.h"
#include "AudioSourceProvider.h"
#include <wtf/Lock.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

typedef struct AudioBufferList AudioBufferList;
typedef struct OpaqueAudioConverter* AudioConverterRef;
typedef struct AudioStreamBasicDescription AudioStreamBasicDescription;
typedef const struct opaqueCMFormatDescription *CMFormatDescriptionRef;
typedef struct opaqueCMSampleBuffer *CMSampleBufferRef;

namespace WebCore {

class AVAudioCaptureSource;
class CARingBuffer;

class WebAudioSourceProviderAVFObjC : public RefCounted<WebAudioSourceProviderAVFObjC>, public AudioSourceProvider, public AVAudioCaptureSource::Observer {
public:
    static Ref<WebAudioSourceProviderAVFObjC> create(AVAudioCaptureSource&);
    virtual ~WebAudioSourceProviderAVFObjC();

private:
    WebAudioSourceProviderAVFObjC(AVAudioCaptureSource&);

    void startProducingData();
    void stopProducingData();

    // AudioSourceProvider
    void provideInput(AudioBus*, size_t) override;
    void setClient(AudioSourceProviderClient*) override;

    // AVAudioCaptureSource::Observer
    void prepare(const AudioStreamBasicDescription *) override;
    void unprepare() override;
    void process(CMFormatDescriptionRef, CMSampleBufferRef) override;

    size_t m_listBufferSize { 0 };
    std::unique_ptr<AudioBufferList> m_list;
    RetainPtr<AudioConverterRef> m_converter;
    std::unique_ptr<AudioStreamBasicDescription> m_inputDescription;
    std::unique_ptr<AudioStreamBasicDescription> m_outputDescription;
    std::unique_ptr<CARingBuffer> m_ringBuffer;

    uint64_t m_writeAheadCount { 0 };
    uint64_t m_writeCount { 0 };
    uint64_t m_readCount { 0 };
    AudioSourceProviderClient* m_client { nullptr };
    AVAudioCaptureSource* m_captureSource { nullptr };
    bool m_connected { false };
};
    
}

#endif

#endif
