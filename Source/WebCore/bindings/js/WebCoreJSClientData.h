/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebCoreJSClientData_h
#define WebCoreJSClientData_h

#include "DOMWrapperWorld.h"
#include "WebCoreBuiltinNames.h"
#include "WebCoreTypedArrayController.h"
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>

#if ENABLE(STREAMS_API)
#include "ByteLengthQueuingStrategyBuiltinsWrapper.h"
#include "CountQueuingStrategyBuiltinsWrapper.h"
#include "ReadableStreamBuiltinsWrapper.h"
#include "ReadableStreamControllerBuiltinsWrapper.h"
#include "ReadableStreamInternalsBuiltinsWrapper.h"
#include "ReadableStreamReaderBuiltinsWrapper.h"
#endif

namespace WebCore {

class WebCoreJSClientData : public JSC::VM::ClientData {
    WTF_MAKE_NONCOPYABLE(WebCoreJSClientData); WTF_MAKE_FAST_ALLOCATED;
    friend class VMWorldIterator;
    friend void initNormalWorldClientData(JSC::VM*);

public:
    explicit WebCoreJSClientData(JSC::VM& vm)
        : m_builtinNames(&vm)
#if ENABLE(STREAMS_API)
        , m_readableStreamBuiltins(&vm)
        , m_readableStreamControllerBuiltins(&vm)
        , m_readableStreamInternalsBuiltins(&vm)
        , m_readableStreamReaderBuiltins(&vm)
        , m_byteLengthQueuingStrategyBuiltins(&vm)
        , m_countQueuingStrategyBuiltins(&vm)
#endif
    {
#if ENABLE(STREAMS_API)
        m_readableStreamInternalsBuiltins.exportNames();
#endif
    }

    virtual ~WebCoreJSClientData()
    {
        ASSERT(m_worldSet.contains(m_normalWorld.get()));
        ASSERT(m_worldSet.size() == 1);
        ASSERT(m_normalWorld->hasOneRef());
        m_normalWorld = nullptr;
        ASSERT(m_worldSet.isEmpty());
    }

    DOMWrapperWorld& normalWorld() { return *m_normalWorld; }

    void getAllWorlds(Vector<Ref<DOMWrapperWorld>>& worlds)
    {
        ASSERT(worlds.isEmpty());

        worlds.reserveInitialCapacity(m_worldSet.size());
        for (auto it = m_worldSet.begin(), end = m_worldSet.end(); it != end; ++it)
            worlds.uncheckedAppend(*(*it));
    }

    void rememberWorld(DOMWrapperWorld& world)
    {
        ASSERT(!m_worldSet.contains(&world));
        m_worldSet.add(&world);
    }

    void forgetWorld(DOMWrapperWorld& world)
    {
        ASSERT(m_worldSet.contains(&world));
        m_worldSet.remove(&world);
    }

    WebCoreBuiltinNames& builtinNames() { return m_builtinNames; }

#if ENABLE(STREAMS_API)
    ReadableStreamBuiltinsWrapper& readableStreamBuiltins() { return m_readableStreamBuiltins; }
    ReadableStreamControllerBuiltinsWrapper& readableStreamControllerBuiltins() { return m_readableStreamControllerBuiltins; }
    ReadableStreamInternalsBuiltinsWrapper& readableStreamInternalsBuiltins() { return m_readableStreamInternalsBuiltins; }
    ReadableStreamReaderBuiltinsWrapper& readableStreamReaderBuiltins() { return m_readableStreamReaderBuiltins; }
    ByteLengthQueuingStrategyBuiltinsWrapper& byteLengthQueuingStrategyBuiltins() { return m_byteLengthQueuingStrategyBuiltins; }
    CountQueuingStrategyBuiltinsWrapper& countQueuingStrategyBuiltins() { return m_countQueuingStrategyBuiltins; }
#endif

private:
    HashSet<DOMWrapperWorld*> m_worldSet;
    RefPtr<DOMWrapperWorld> m_normalWorld;

    WebCoreBuiltinNames m_builtinNames;

#if ENABLE(STREAMS_API)
    ReadableStreamBuiltinsWrapper m_readableStreamBuiltins;
    ReadableStreamControllerBuiltinsWrapper m_readableStreamControllerBuiltins;
    ReadableStreamInternalsBuiltinsWrapper m_readableStreamInternalsBuiltins;
    ReadableStreamReaderBuiltinsWrapper m_readableStreamReaderBuiltins;
    ByteLengthQueuingStrategyBuiltinsWrapper m_byteLengthQueuingStrategyBuiltins;
    CountQueuingStrategyBuiltinsWrapper m_countQueuingStrategyBuiltins;
#endif
};

inline void initNormalWorldClientData(JSC::VM* vm)
{
    WebCoreJSClientData* webCoreJSClientData = new WebCoreJSClientData(*vm);
    vm->clientData = webCoreJSClientData; // ~VM deletes this pointer.
    webCoreJSClientData->m_normalWorld = DOMWrapperWorld::create(*vm, true);
    vm->m_typedArrayController = adoptRef(new WebCoreTypedArrayController());
}

} // namespace WebCore

#endif // WebCoreJSClientData_h
