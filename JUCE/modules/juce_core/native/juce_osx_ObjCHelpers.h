/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once


/* This file contains a few helper functions that are used internally but which
   need to be kept away from the public headers because they use obj-C symbols.
*/
namespace
{
    //==============================================================================
    static inline String nsStringToJuce (NSString* s)
    {
        return CharPointer_UTF8 ([s UTF8String]);
    }

    static inline NSString* juceStringToNS (const String& s)
    {
        return [NSString stringWithUTF8String: s.toUTF8()];
    }

    static inline NSString* nsStringLiteral (const char* const s) noexcept
    {
        return [NSString stringWithUTF8String: s];
    }

    static inline NSString* nsEmptyString() noexcept
    {
        return [NSString string];
    }

   #if JUCE_MAC
    template <typename RectangleType>
    static NSRect makeNSRect (const RectangleType& r) noexcept
    {
        return NSMakeRect (static_cast<CGFloat> (r.getX()),
                           static_cast<CGFloat> (r.getY()),
                           static_cast<CGFloat> (r.getWidth()),
                           static_cast<CGFloat> (r.getHeight()));
    }
   #endif
  #if JUCE_MAC || JUCE_IOS
   #if JUCE_COMPILER_SUPPORTS_VARIADIC_TEMPLATES

    // This is necessary as on iOS builds, some arguments may be passed on registers
    // depending on the argument type. The re-cast objc_msgSendSuper to a function
    // take the same arguments as the target method.
    template <typename ReturnValue, typename... Params>
    static inline ReturnValue ObjCMsgSendSuper (struct objc_super* s, SEL sel, Params... params)
    {
        typedef ReturnValue (*SuperFn)(struct objc_super*, SEL, Params...);
        SuperFn fn = reinterpret_cast<SuperFn> (objc_msgSendSuper);
        return fn (s, sel, params...);
    }

   #endif

    // These hacks are a workaround for newer Xcode builds which by default prevent calls to these objc functions..
    typedef id (*MsgSendSuperFn) (struct objc_super*, SEL, ...);
    static inline MsgSendSuperFn getMsgSendSuperFn() noexcept   { return (MsgSendSuperFn) (void*) objc_msgSendSuper; }

   #if ! JUCE_IOS
    typedef double (*MsgSendFPRetFn) (id, SEL op, ...);
    static inline MsgSendFPRetFn getMsgSendFPRetFn() noexcept   { return (MsgSendFPRetFn) (void*) objc_msgSend_fpret; }
   #endif
   #endif
}

//==============================================================================
template <typename ObjectType>
struct NSObjectRetainer
{
    inline NSObjectRetainer (ObjectType* o) : object (o)  { [object retain]; }
    inline ~NSObjectRetainer()                            { [object release]; }

    ObjectType* object;
};

//==============================================================================
template <typename SuperclassType>
struct ObjCClass
{
    ObjCClass (const char* nameRoot)
        : cls (objc_allocateClassPair ([SuperclassType class], getRandomisedName (nameRoot).toUTF8(), 0))
    {
    }

    ~ObjCClass()
    {
        objc_disposeClassPair (cls);
    }

    void registerClass()
    {
        objc_registerClassPair (cls);
    }

    SuperclassType* createInstance() const
    {
        return class_createInstance (cls, 0);
    }

    template <typename Type>
    void addIvar (const char* name)
    {
        BOOL b = class_addIvar (cls, name, sizeof (Type), (uint8_t) rint (log2 (sizeof (Type))), @encode (Type));
        jassert (b); ignoreUnused (b);
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* signature)
    {
        BOOL b = class_addMethod (cls, selector, (IMP) callbackFn, signature);
        jassert (b); ignoreUnused (b);
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* sig1, const char* sig2)
    {
        addMethod (selector, callbackFn, (String (sig1) + sig2).toUTF8());
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* sig1, const char* sig2, const char* sig3)
    {
        addMethod (selector, callbackFn, (String (sig1) + sig2 + sig3).toUTF8());
    }

    template <typename FunctionType>
    void addMethod (SEL selector, FunctionType callbackFn, const char* sig1, const char* sig2, const char* sig3, const char* sig4)
    {
        addMethod (selector, callbackFn, (String (sig1) + sig2 + sig3 + sig4).toUTF8());
    }

    void addProtocol (Protocol* protocol)
    {
        BOOL b = class_addProtocol (cls, protocol);
        jassert (b); ignoreUnused (b);
    }

   #if JUCE_MAC || JUCE_IOS
    static id sendSuperclassMessage (id self, SEL selector)
    {
        objc_super s = { self, [SuperclassType class] };
        return getMsgSendSuperFn() (&s, selector);
    }
   #endif

    template <typename Type>
    static Type getIvar (id self, const char* name)
    {
        void* v = nullptr;
        object_getInstanceVariable (self, name, &v);
        return static_cast<Type> (v);
    }

    Class cls;

private:
    static String getRandomisedName (const char* root)
    {
        return root + String::toHexString (juce::Random::getSystemRandom().nextInt64());
    }

    JUCE_DECLARE_NON_COPYABLE (ObjCClass)
};

#if JUCE_COMPILER_SUPPORTS_VARIADIC_TEMPLATES

template <typename ReturnT, class Class, typename... Params>
ReturnT (^CreateObjCBlock(Class* object, ReturnT (Class::*fn)(Params...))) (Params...)
{
    __block Class* _this = object;
    __block ReturnT (Class::*_fn)(Params...) = fn;

    return [[^ReturnT (Params... params) { return (_this->*_fn) (params...); } copy] autorelease];
}

template <typename BlockType>
class ObjCBlock
{
public:
    ObjCBlock()  { block = nullptr; }
    template <typename R, class C, typename... P>
    ObjCBlock (C* _this, R (C::*fn)(P...))  : block (CreateObjCBlock (_this, fn)) {}
    ObjCBlock (BlockType b) : block ([b copy]) {}
    ObjCBlock& operator= (const BlockType& other) { if (block != nullptr) { [block release]; } block = [other copy]; return *this; }
    bool operator== (const void* ptr) const  { return (block == ptr); }
    bool operator!= (const void* ptr) const  { return (block != ptr); }
    ~ObjCBlock() { if (block != nullptr) [block release]; }

    operator BlockType() { return block; }

private:
    BlockType block;
};

#endif
