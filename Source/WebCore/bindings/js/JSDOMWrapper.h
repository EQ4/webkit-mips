/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef JSDOMWrapper_h
#define JSDOMWrapper_h

#include "JSDOMGlobalObject.h"
#include <runtime/JSDestructibleObject.h>

namespace WebCore {

class ScriptExecutionContext;

static const uint8_t JSNodeType = JSC::LastJSCObjectType + 1;
static const uint8_t JSDocumentWrapperType = JSC::LastJSCObjectType + 2;
static const uint8_t JSElementType = JSC::LastJSCObjectType + 3;

class JSDOMWrapper : public JSC::JSDestructibleObject {
public:
    typedef JSC::JSDestructibleObject Base;

    JSDOMGlobalObject* globalObject() const { return JSC::jsCast<JSDOMGlobalObject*>(JSC::JSNonFinalObject::globalObject()); }
    ScriptExecutionContext* scriptExecutionContext() const { return globalObject()->scriptExecutionContext(); }

protected:
    JSDOMWrapper(JSC::Structure* structure, JSC::JSGlobalObject* globalObject) 
        : Base(globalObject->vm(), structure)
    {
        ASSERT(scriptExecutionContext());
    }
};

template<typename ImplementationClass> class JSDOMWrapperWithImplementation : public JSDOMWrapper {
public:
    typedef JSDOMWrapper Base;

    ImplementationClass& impl() const { return *m_impl; }
    ~JSDOMWrapperWithImplementation() { std::exchange(m_impl, nullptr)->deref(); }

protected:
    JSDOMWrapperWithImplementation(JSC::Structure* structure, JSC::JSGlobalObject* globalObject, Ref<ImplementationClass>&& impl)
        : Base(structure, globalObject)
        , m_impl(&impl.leakRef()) { }

private:
    ImplementationClass* m_impl;
};

} // namespace WebCore

#endif // JSDOMWrapper_h
