/*
 * Copyright (C) 2011, 2013-2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "AssemblyHelpers.h"

#if ENABLE(JIT)

#include "JITOperations.h"
#include "JSCInlines.h"

namespace JSC {

ExecutableBase* AssemblyHelpers::executableFor(const CodeOrigin& codeOrigin)
{
    if (!codeOrigin.inlineCallFrame)
        return m_codeBlock->ownerExecutable();
    
    return codeOrigin.inlineCallFrame->executable.get();
}

Vector<BytecodeAndMachineOffset>& AssemblyHelpers::decodedCodeMapFor(CodeBlock* codeBlock)
{
    ASSERT(codeBlock == codeBlock->baselineVersion());
    ASSERT(codeBlock->jitType() == JITCode::BaselineJIT);
    ASSERT(codeBlock->jitCodeMap());
    
    HashMap<CodeBlock*, Vector<BytecodeAndMachineOffset>>::AddResult result = m_decodedCodeMaps.add(codeBlock, Vector<BytecodeAndMachineOffset>());
    
    if (result.isNewEntry)
        codeBlock->jitCodeMap()->decode(result.iterator->value);
    
    return result.iterator->value;
}

AssemblyHelpers::JumpList AssemblyHelpers::branchIfNotType(
    JSValueRegs regs, GPRReg tempGPR, const InferredType::Descriptor& descriptor, TagRegistersMode mode)
{
    AssemblyHelpers::JumpList result;

    switch (descriptor.kind()) {
    case InferredType::Bottom:
        result.append(jump());
        break;

    case InferredType::Boolean:
        result.append(branchIfNotBoolean(regs, tempGPR));
        break;

    case InferredType::Other:
        result.append(branchIfNotOther(regs, tempGPR));
        break;

    case InferredType::Int32:
        result.append(branchIfNotInt32(regs, mode));
        break;

    case InferredType::Number:
        result.append(branchIfNotNumber(regs, tempGPR, mode));
        break;

    case InferredType::String:
        result.append(branchIfNotCell(regs, mode));
        result.append(branchIfNotString(regs.payloadGPR()));
        break;

    case InferredType::ObjectWithStructure:
        result.append(branchIfNotCell(regs, mode));
        result.append(
            branchStructure(
                NotEqual,
                Address(regs.payloadGPR(), JSCell::structureIDOffset()),
                descriptor.structure()));
        break;

    case InferredType::ObjectWithStructureOrOther: {
        Jump ok = branchIfOther(regs, tempGPR);
        result.append(branchIfNotCell(regs, mode));
        result.append(
            branchStructure(
                NotEqual,
                Address(regs.payloadGPR(), JSCell::structureIDOffset()),
                descriptor.structure()));
        ok.link(this);
        break;
    }

    case InferredType::Object:
        result.append(branchIfNotCell(regs, mode));
        result.append(branchIfNotObject(regs.payloadGPR()));
        break;

    case InferredType::ObjectOrOther: {
        Jump ok = branchIfOther(regs, tempGPR);
        result.append(branchIfNotCell(regs, mode));
        result.append(branchIfNotObject(regs.payloadGPR()));
        ok.link(this);
        break;
    }

    case InferredType::Top:
        break;
    }

    return result;
}

void AssemblyHelpers::purifyNaN(FPRReg fpr)
{
    MacroAssembler::Jump notNaN = branchDouble(DoubleEqual, fpr, fpr);
    static const double NaN = PNaN;
    loadDouble(TrustedImmPtr(&NaN), fpr);
    notNaN.link(this);
}

#if ENABLE(SAMPLING_FLAGS)
void AssemblyHelpers::setSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    or32(TrustedImm32(1u << (flag - 1)), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}

void AssemblyHelpers::clearSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    and32(TrustedImm32(~(1u << (flag - 1))), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}
#endif

#if !ASSERT_DISABLED
#if USE(JSVALUE64)
void AssemblyHelpers::jitAssertIsInt32(GPRReg gpr)
{
#if CPU(X86_64)
    Jump checkInt32 = branch64(BelowOrEqual, gpr, TrustedImm64(static_cast<uintptr_t>(0xFFFFFFFFu)));
    abortWithReason(AHIsNotInt32);
    checkInt32.link(this);
#else
    UNUSED_PARAM(gpr);
#endif
}

void AssemblyHelpers::jitAssertIsJSInt32(GPRReg gpr)
{
    Jump checkJSInt32 = branch64(AboveOrEqual, gpr, GPRInfo::tagTypeNumberRegister);
    abortWithReason(AHIsNotJSInt32);
    checkJSInt32.link(this);
}

void AssemblyHelpers::jitAssertIsJSNumber(GPRReg gpr)
{
    Jump checkJSNumber = branchTest64(MacroAssembler::NonZero, gpr, GPRInfo::tagTypeNumberRegister);
    abortWithReason(AHIsNotJSNumber);
    checkJSNumber.link(this);
}

void AssemblyHelpers::jitAssertIsJSDouble(GPRReg gpr)
{
    Jump checkJSInt32 = branch64(AboveOrEqual, gpr, GPRInfo::tagTypeNumberRegister);
    Jump checkJSNumber = branchTest64(MacroAssembler::NonZero, gpr, GPRInfo::tagTypeNumberRegister);
    checkJSInt32.link(this);
    abortWithReason(AHIsNotJSDouble);
    checkJSNumber.link(this);
}

void AssemblyHelpers::jitAssertIsCell(GPRReg gpr)
{
    Jump checkCell = branchTest64(MacroAssembler::Zero, gpr, GPRInfo::tagMaskRegister);
    abortWithReason(AHIsNotCell);
    checkCell.link(this);
}

void AssemblyHelpers::jitAssertTagsInPlace()
{
    Jump ok = branch64(Equal, GPRInfo::tagTypeNumberRegister, TrustedImm64(TagTypeNumber));
    abortWithReason(AHTagTypeNumberNotInPlace);
    breakpoint();
    ok.link(this);
    
    ok = branch64(Equal, GPRInfo::tagMaskRegister, TrustedImm64(TagMask));
    abortWithReason(AHTagMaskNotInPlace);
    ok.link(this);
}
#elif USE(JSVALUE32_64)
void AssemblyHelpers::jitAssertIsInt32(GPRReg gpr)
{
    UNUSED_PARAM(gpr);
}

void AssemblyHelpers::jitAssertIsJSInt32(GPRReg gpr)
{
    Jump checkJSInt32 = branch32(Equal, gpr, TrustedImm32(JSValue::Int32Tag));
    abortWithReason(AHIsNotJSInt32);
    checkJSInt32.link(this);
}

void AssemblyHelpers::jitAssertIsJSNumber(GPRReg gpr)
{
    Jump checkJSInt32 = branch32(Equal, gpr, TrustedImm32(JSValue::Int32Tag));
    Jump checkJSDouble = branch32(Below, gpr, TrustedImm32(JSValue::LowestTag));
    abortWithReason(AHIsNotJSNumber);
    checkJSInt32.link(this);
    checkJSDouble.link(this);
}

void AssemblyHelpers::jitAssertIsJSDouble(GPRReg gpr)
{
    Jump checkJSDouble = branch32(Below, gpr, TrustedImm32(JSValue::LowestTag));
    abortWithReason(AHIsNotJSDouble);
    checkJSDouble.link(this);
}

void AssemblyHelpers::jitAssertIsCell(GPRReg gpr)
{
    Jump checkCell = branch32(Equal, gpr, TrustedImm32(JSValue::CellTag));
    abortWithReason(AHIsNotCell);
    checkCell.link(this);
}

void AssemblyHelpers::jitAssertTagsInPlace()
{
}
#endif // USE(JSVALUE32_64)

void AssemblyHelpers::jitAssertHasValidCallFrame()
{
    Jump checkCFR = branchTestPtr(Zero, GPRInfo::callFrameRegister, TrustedImm32(7));
    abortWithReason(AHCallFrameMisaligned);
    checkCFR.link(this);
}

void AssemblyHelpers::jitAssertIsNull(GPRReg gpr)
{
    Jump checkNull = branchTestPtr(Zero, gpr);
    abortWithReason(AHIsNotNull);
    checkNull.link(this);
}

void AssemblyHelpers::jitAssertArgumentCountSane()
{
    Jump ok = branch32(Below, payloadFor(JSStack::ArgumentCount), TrustedImm32(10000000));
    abortWithReason(AHInsaneArgumentCount);
    ok.link(this);
}

void AssemblyHelpers::jitAssertNoException()
{
    Jump noException;
#if USE(JSVALUE64)
    noException = branchTest64(Zero, AbsoluteAddress(vm()->addressOfException()));
#elif USE(JSVALUE32_64)
    noException = branch32(Equal, AbsoluteAddress(vm()->addressOfException()), TrustedImm32(0));
#endif
    abortWithReason(JITUncoughtExceptionAfterCall);
    noException.link(this);
}

#endif // !ASSERT_DISABLED

void AssemblyHelpers::callExceptionFuzz()
{
    if (!Options::enableExceptionFuzz())
        return;

    EncodedJSValue* buffer = vm()->exceptionFuzzingBuffer(sizeof(EncodedJSValue) * (GPRInfo::numberOfRegisters + FPRInfo::numberOfRegisters));

    for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i) {
#if USE(JSVALUE64)
        store64(GPRInfo::toRegister(i), buffer + i);
#else
        store32(GPRInfo::toRegister(i), buffer + i);
#endif
    }
    for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
        move(TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
        storeDouble(FPRInfo::toRegister(i), Address(GPRInfo::regT0));
    }

    // Set up one argument.
#if CPU(X86)
    poke(GPRInfo::callFrameRegister, 0);
#else
    move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
#endif
    move(TrustedImmPtr(bitwise_cast<void*>(operationExceptionFuzz)), GPRInfo::nonPreservedNonReturnGPR);
    call(GPRInfo::nonPreservedNonReturnGPR);

    for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
        move(TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
        loadDouble(Address(GPRInfo::regT0), FPRInfo::toRegister(i));
    }
    for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i) {
#if USE(JSVALUE64)
        load64(buffer + i, GPRInfo::toRegister(i));
#else
        load32(buffer + i, GPRInfo::toRegister(i));
#endif
    }
}

AssemblyHelpers::Jump AssemblyHelpers::emitExceptionCheck(ExceptionCheckKind kind, ExceptionJumpWidth width)
{
    callExceptionFuzz();

    if (width == FarJumpWidth)
        kind = (kind == NormalExceptionCheck ? InvertedExceptionCheck : NormalExceptionCheck);
    
    Jump result;
#if USE(JSVALUE64)
    result = branchTest64(kind == NormalExceptionCheck ? NonZero : Zero, AbsoluteAddress(vm()->addressOfException()));
#elif USE(JSVALUE32_64)
    result = branch32(kind == NormalExceptionCheck ? NotEqual : Equal, AbsoluteAddress(vm()->addressOfException()), TrustedImm32(0));
#endif
    
    if (width == NormalJumpWidth)
        return result;

    PatchableJump realJump = patchableJump();
    result.link(this);
    
    return realJump.m_jump;
}

AssemblyHelpers::Jump AssemblyHelpers::emitNonPatchableExceptionCheck()
{
    callExceptionFuzz();

    Jump result;
#if USE(JSVALUE64)
    result = branchTest64(NonZero, AbsoluteAddress(vm()->addressOfException()));
#elif USE(JSVALUE32_64)
    result = branch32(NotEqual, AbsoluteAddress(vm()->addressOfException()), TrustedImm32(0));
#endif
    
    return result;
}

void AssemblyHelpers::emitStoreStructureWithTypeInfo(AssemblyHelpers& jit, TrustedImmPtr structure, RegisterID dest)
{
    const Structure* structurePtr = static_cast<const Structure*>(structure.m_value);
#if USE(JSVALUE64)
    jit.store64(TrustedImm64(structurePtr->idBlob()), MacroAssembler::Address(dest, JSCell::structureIDOffset()));
    if (!ASSERT_DISABLED) {
        Jump correctStructure = jit.branch32(Equal, MacroAssembler::Address(dest, JSCell::structureIDOffset()), TrustedImm32(structurePtr->id()));
        jit.abortWithReason(AHStructureIDIsValid);
        correctStructure.link(&jit);

        Jump correctIndexingType = jit.branch8(Equal, MacroAssembler::Address(dest, JSCell::indexingTypeOffset()), TrustedImm32(structurePtr->indexingType()));
        jit.abortWithReason(AHIndexingTypeIsValid);
        correctIndexingType.link(&jit);

        Jump correctType = jit.branch8(Equal, MacroAssembler::Address(dest, JSCell::typeInfoTypeOffset()), TrustedImm32(structurePtr->typeInfo().type()));
        jit.abortWithReason(AHTypeInfoIsValid);
        correctType.link(&jit);

        Jump correctFlags = jit.branch8(Equal, MacroAssembler::Address(dest, JSCell::typeInfoFlagsOffset()), TrustedImm32(structurePtr->typeInfo().inlineTypeFlags()));
        jit.abortWithReason(AHTypeInfoInlineTypeFlagsAreValid);
        correctFlags.link(&jit);
    }
#else
    // Do a 32-bit wide store to initialize the cell's fields.
    jit.store32(TrustedImm32(structurePtr->objectInitializationBlob()), MacroAssembler::Address(dest, JSCell::indexingTypeOffset()));
    jit.storePtr(structure, MacroAssembler::Address(dest, JSCell::structureIDOffset()));
#endif
}

} // namespace JSC

#endif // ENABLE(JIT)

