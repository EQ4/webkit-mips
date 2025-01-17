/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "JSAudioBufferSourceNode.h"

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "JSAudioBuffer.h"
#include <runtime/Error.h>
#include <runtime/JSCJSValueInlines.h>

using namespace JSC;

namespace WebCore {

void JSAudioBufferSourceNode::setBuffer(ExecState& state, JSValue value)
{
    AudioBuffer* buffer = JSAudioBuffer::toWrapped(value);
    if (!buffer) {
        state.vm().throwException(&state, createTypeError(&state, "Value is not of type AudioBuffer"));
        return;
    }
    
    if (!impl().setBuffer(buffer))
        state.vm().throwException(&state, createTypeError(&state, "AudioBuffer unsupported number of channels"));
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
