/*
 * Copyright (c) 1999-2007 Apple Inc.  All Rights Reserved.
 * 
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef _OBJC_MESSAGE_H
#define _OBJC_MESSAGE_H

#include <objc/objc.h>
#include <objc/runtime.h>

#ifndef OBJC_SUPER
#define OBJC_SUPER

/// Specifies the superclass of an instance. 
struct objc_super {
    /// Specifies an instance of a class.
    __unsafe_unretained _Nonnull id receiver;

    /// Specifies the particular superclass of the instance to message. 
#if !defined(__cplusplus)  &&  !__OBJC2__
    /* For compatibility with old objc-runtime.h header */
    __unsafe_unretained _Nonnull Class class;
#else
    __unsafe_unretained _Nonnull Class super_class;
#endif
    /* super_class is the first class to search */
};
#endif


/* Basic Messaging Primitives
 *
 * On some architectures, use objc_msgSend_stret for some struct return types.
 * On some architectures, use objc_msgSend_fpret for some float return types.
 * On some architectures, use objc_msgSend_fp2ret for some float return types.
 *
 * These functions must be cast to an appropriate function pointer type 
 * before being called. 
 */
#if !OBJC_OLD_DISPATCH_PROTOTYPES
OBJC_EXPORT void
objc_msgSend(void /* id self, SEL op, ... */ )
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

OBJC_EXPORT void
objc_msgSendSuper(void /* struct objc_super *super, SEL op, ... */ )
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);
#else

/**
 * Note added by xiaohai
 * 向一个类的实例发送一个消息
 * Sends a message with a simple return value to an instance of a class.
 *
 * 接收这个消息的实例
 * @param self A pointer to the instance of the class that is to receive the message.
 *
 * 处理这个消息的方法选择器
 * @param op The selector of the method that handles the message.
 *
 * 发给方法的可变参数列表
 * @param ...
 *   A variable argument list containing the arguments to the method.
 *
 * 方法的返回值
 * @return The return value of the method.
 *
 * 当遇到一个方法调用的时候，编译器会产生一个对objc_msgSend objc_msgSend_stret objc_msgSendSuper objc_msgSendSuper_stret的函数调用，
 * 如果调用的是[super xxxx] 那么将会调用objc_msgSendSuper objc_msgSendSuper_stret 也就是有带super的函数
 * 如果调用的是[obj xxxxx] 那么将会调用objc_msgSend objc_msgSend_stret
 * 如果返回的值是带有结构的将会调用 objc_msgSendSuper_stret objc_msgSend_stret 也就是带有stret的
 *
 * Todo by xiaohai 带super。stret和不带的有啥区别
 * 这里需要注意的是SEL SEL与类不是强关联的只与函数名相关联，与函数参数类型也无关，
 * 在receiver拿到对应的selector之后，如果自己无法执行这个方法，那么该条消息要被转发。或者临时动态的添加方法实现。如果转发到最后依旧没法处理，程序就会崩溃。
 * 检查这个selector是否需要忽略（什么情况下需要忽略？）
 * 检查消息的接收者是否为空，如果是空并且有相应的nil处理函数，就跳转到相应的处理函数
 * 如果没有处理nil的函数，就自动清理现场并返回。
 * 无锁的缓存查找
 * 如果类没有实现（isRealized）或者初始化（isInitialized），实现或者初始化类
 * 如果消息接收者不是空的情况下会先在class的方法缓存中查找，如果没有找到就到class的方法列表中查找，如果没有找到就继续顺着继承链继续查找
 * 如果没有找到还有一个机会就是通过指定resolver，如果resolver还是没有找到，那就到事件的转发机制
 * 解锁、返回实现
 * @note When it encounters a method call, the compiler generates a call to one of the
 *  functions \c objc_msgSend, \c objc_msgSend_stret, \c objc_msgSendSuper, or \c objc_msgSendSuper_stret.
 *  Messages sent to an object’s superclass (using the \c super keyword) are sent using \c objc_msgSendSuper; 
 *  other messages are sent using \c objc_msgSend. Methods that have data structures as return values
 *  are sent using \c objc_msgSendSuper_stret and \c objc_msgSend_stret.
 */
OBJC_EXPORT id _Nullable
objc_msgSend(id _Nullable self, SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);
/** 
 * Sends a message with a simple return value to the superclass of an instance of a class.
 * 
 * @param super A pointer to an \c objc_super data structure. Pass values identifying the
 *  context the message was sent to, including the instance of the class that is to receive the
 *  message and the superclass at which to start searching for the method implementation.
 * @param op A pointer of type SEL. Pass the selector of the method that will handle the message.
 * @param ...
 *   A variable argument list containing the arguments to the method.
 * 
 * @return The return value of the method identified by \e op.
 * 
 * @see objc_msgSend
 */
OBJC_EXPORT id _Nullable
objc_msgSendSuper(struct objc_super * _Nonnull super, SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);
#endif


/* Struct-returning Messaging Primitives
 *
 * Use these functions to call methods that return structs on the stack. 
 * On some architectures, some structures are returned in registers. 
 * Consult your local function call ABI documentation for details.
 * 
 * These functions must be cast to an appropriate function pointer type 
 * before being called. 
 */
#if !OBJC_OLD_DISPATCH_PROTOTYPES
OBJC_EXPORT void
objc_msgSend_stret(void /* id self, SEL op, ... */ )
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;

OBJC_EXPORT void
objc_msgSendSuper_stret(void /* struct objc_super *super, SEL op, ... */ )
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;
#else
/** 
 * Sends a message with a data-structure return value to an instance of a class.
 * 
 * @see objc_msgSend
 */
OBJC_EXPORT void
objc_msgSend_stret(id _Nullable self, SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;

/** 
 * Sends a message with a data-structure return value to the superclass of an instance of a class.
 * 
 * @see objc_msgSendSuper
 */
OBJC_EXPORT void
objc_msgSendSuper_stret(struct objc_super * _Nonnull super,
                        SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;
#endif


/* Floating-point-returning Messaging Primitives
 * 
 * Use these functions to call methods that return floating-point values 
 * on the stack. 
 * Consult your local function call ABI documentation for details.
 * 
 * arm:    objc_msgSend_fpret not used
 * i386:   objc_msgSend_fpret used for `float`, `double`, `long double`.
 * x86-64: objc_msgSend_fpret used for `long double`.
 *
 * arm:    objc_msgSend_fp2ret not used
 * i386:   objc_msgSend_fp2ret not used
 * x86-64: objc_msgSend_fp2ret used for `_Complex long double`.
 *
 * These functions must be cast to an appropriate function pointer type 
 * before being called. 
 */
#if !OBJC_OLD_DISPATCH_PROTOTYPES

# if defined(__i386__)

OBJC_EXPORT void
objc_msgSend_fpret(void /* id self, SEL op, ... */ )
    OBJC_AVAILABLE(10.4, 2.0, 9.0, 1.0, 2.0);

# elif defined(__x86_64__)

OBJC_EXPORT void
objc_msgSend_fpret(void /* id self, SEL op, ... */ )
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

OBJC_EXPORT void
objc_msgSend_fp2ret(void /* id self, SEL op, ... */ )
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

# endif

// !OBJC_OLD_DISPATCH_PROTOTYPES
#else
// OBJC_OLD_DISPATCH_PROTOTYPES
# if defined(__i386__)

/** 
 * Sends a message with a floating-point return value to an instance of a class.
 * 
 * @see objc_msgSend
 * @note On the i386 platform, the ABI for functions returning a floating-point value is
 *  incompatible with that for functions returning an integral type. On the i386 platform, therefore, 
 *  you must use \c objc_msgSend_fpret for functions returning non-integral type. For \c float or 
 *  \c long \c double return types, cast the function to an appropriate function pointer type first.
 */
OBJC_EXPORT double
objc_msgSend_fpret(id _Nullable self, SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.4, 2.0, 9.0, 1.0, 2.0);

/* Use objc_msgSendSuper() for fp-returning messages to super. */
/* See also objc_msgSendv_fpret() below. */

# elif defined(__x86_64__)
/** 
 * Sends a message with a floating-point return value to an instance of a class.
 * 
 * @see objc_msgSend
 */
OBJC_EXPORT long double
objc_msgSend_fpret(id _Nullable self, SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

#  if __STDC_VERSION__ >= 199901L
OBJC_EXPORT _Complex long double
objc_msgSend_fp2ret(id _Nullable self, SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);
#  else
OBJC_EXPORT void objc_msgSend_fp2ret(id _Nullable self, SEL _Nonnull op, ...)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);
#  endif

/* Use objc_msgSendSuper() for fp-returning messages to super. */
/* See also objc_msgSendv_fpret() below. */

# endif

// OBJC_OLD_DISPATCH_PROTOTYPES
#endif


/* Direct Method Invocation Primitives
 * Use these functions to call the implementation of a given Method.
 * This is faster than calling method_getImplementation() and method_getName().
 *
 * The receiver must not be nil.
 *
 * These functions must be cast to an appropriate function pointer type 
 * before being called. 
 */
#if !OBJC_OLD_DISPATCH_PROTOTYPES
OBJC_EXPORT void
method_invoke(void /* id receiver, Method m, ... */ ) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

OBJC_EXPORT void
method_invoke_stret(void /* id receiver, Method m, ... */ ) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;
#else
OBJC_EXPORT id _Nullable
method_invoke(id _Nullable receiver, Method _Nonnull m, ...) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

OBJC_EXPORT void
method_invoke_stret(id _Nullable receiver, Method _Nonnull m, ...) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;
#endif


/* Message Forwarding Primitives
 * Use these functions to forward a message as if the receiver did not 
 * respond to it. 
 *
 * The receiver must not be nil.
 * 
 * class_getMethodImplementation() may return (IMP)_objc_msgForward.
 * class_getMethodImplementation_stret() may return (IMP)_objc_msgForward_stret
 * 
 * These functions must be cast to an appropriate function pointer type 
 * before being called. 
 *
 * Before Mac OS X 10.6, _objc_msgForward must not be called directly 
 * but may be compared to other IMP values.
 */
#if !OBJC_OLD_DISPATCH_PROTOTYPES
OBJC_EXPORT void
_objc_msgForward(void /* id receiver, SEL sel, ... */ ) 
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

OBJC_EXPORT void
_objc_msgForward_stret(void /* id receiver, SEL sel, ... */ ) 
    OBJC_AVAILABLE(10.6, 3.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;
#else
OBJC_EXPORT id _Nullable
_objc_msgForward(id _Nonnull receiver, SEL _Nonnull sel, ...) 
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

OBJC_EXPORT void
_objc_msgForward_stret(id _Nonnull receiver, SEL _Nonnull sel, ...) 
    OBJC_AVAILABLE(10.6, 3.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;
#endif


/* Variable-argument Messaging Primitives
 *
 * Use these functions to call methods with a list of arguments, such 
 * as the one passed to forward:: .
 *
 * The contents of the argument list are architecture-specific. 
 * Consult your local function call ABI documentation for details.
 * 
 * These functions must be cast to an appropriate function pointer type 
 * before being called, except for objc_msgSendv_stret() which must not 
 * be cast to a struct-returning type.
 */

typedef void* marg_list;

OBJC_EXPORT id _Nullable
objc_msgSendv(id _Nullable self, SEL _Nonnull op, size_t arg_size,
              marg_list _Nonnull arg_frame)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT void
objc_msgSendv_stret(void * _Nonnull stretAddr, id _Nullable self,
                    SEL _Nonnull op, size_t arg_size,
                    marg_list _Nullable arg_frame)
    OBJC2_UNAVAILABLE;
/* Note that objc_msgSendv_stret() does not return a structure type, 
 * and should not be cast to do so. This is unlike objc_msgSend_stret() 
 * and objc_msgSendSuper_stret().
 */
#if defined(__i386__)
OBJC_EXPORT double
objc_msgSendv_fpret(id _Nullable self, SEL _Nonnull op,
                    unsigned arg_size, marg_list _Nullable arg_frame)
    OBJC2_UNAVAILABLE;
#endif


/* The following marg_list macros are of marginal utility. They
 * are included for compatibility with the old objc-class.h header. */

#if !__OBJC2__

#define marg_prearg_size	0

#define marg_malloc(margs, method) \
	do { \
		margs = (marg_list *)malloc (marg_prearg_size + ((7 + method_getSizeOfArguments(method)) & ~7)); \
	} while (0)

#define marg_free(margs) \
	do { \
		free(margs); \
	} while (0)
	
#define marg_adjustedOffset(method, offset) \
	(marg_prearg_size + offset)

#define marg_getRef(margs, offset, type) \
	( (type *)((char *)margs + marg_adjustedOffset(method,offset) ) )

#define marg_getValue(margs, offset, type) \
	( *marg_getRef(margs, offset, type) )

#define marg_setValue(margs, offset, type, value) \
	( marg_getValue(margs, offset, type) = (value) )

#endif

#endif
