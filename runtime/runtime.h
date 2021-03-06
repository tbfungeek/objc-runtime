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

#ifndef _OBJC_RUNTIME_H
#define _OBJC_RUNTIME_H

#include <objc/objc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <Availability.h>
#include <TargetConditionals.h>

#if TARGET_OS_MAC
#include <sys/types.h>
#endif
////===================================================BEGIN 基本结构体定义 BEGIN======================================================///
#if !OBJC_TYPES_DEFINED
// Note add by xiaohai 基本概念
// 1. 类结构体
// 属性、协议和方法都存储在类的可写段中，这些信息可以在运行时被改变，这也是分类的实现原理，但是类的成员变量存储在只读段中，不能被修改，这也是分类不能添加成员变量的原因
// 当我们向一个类的实体对象发送消息时，实体对象会根据其内部的isa指针，寻找其对应的类，找到对应的类之后，会先在类的方法缓存中寻找对应的方法，找到执行，
// 没找到上其自身内部的方法链表去寻找，还没找到，根据其内部的父类上父类去寻找
struct objc_class {
    Class _Nonnull isa  OBJC_ISA_AVAILABILITY;              /*
                                                             对象里面有一个isa指针，这个指针指向了它所属的类，而在类里面也有一个isa指针，这个指针指向元类，元类是一个包含这个类的类方法的结构体，
                                                             它存储了[这个类的所有类方法]，所以当我们调用类方法的时候其实就是在类本身的isa指针指向的元类里面寻找。
                                                             在元类里面也有一个isa指针，这个指针指向根类的元类，而根类的元类的isa指针指向根类自己。
                                                             当向对象发消息，runtime会在这个对象所属类方法列表中查找发送消息对应的方法，
                                                             但当向类发送消息时，runtime就会在这个类的meta class方法列表里查找
                                                             */
#if !__OBJC2__
    /*[3]*/
    Class _Nullable super_class                              OBJC2_UNAVAILABLE;/*父类类型信息*/
    const char * _Nonnull name                               OBJC2_UNAVAILABLE;/*类名*/
    long version                                             OBJC2_UNAVAILABLE;/*类的版本信息，默认为0*/
    long info                                                OBJC2_UNAVAILABLE;/*类信息，供运行期使用的一些位标识*/
    long instance_size                                       OBJC2_UNAVAILABLE;/*类的实例变量的大小*/
    struct objc_ivar_list * _Nullable ivars                  OBJC2_UNAVAILABLE;/*类的成员变量链表*/
    /*[2]*/
    struct objc_method_list * _Nullable * _Nullable methodLists OBJC2_UNAVAILABLE;/*类的方法链表*/
    struct objc_protocol_list * _Nullable protocols          OBJC2_UNAVAILABLE; /*类的协议链表 协议链表用来存储声明遵守的正式协议*/
    /*[1]*/
    struct objc_cache * _Nonnull cache                       OBJC2_UNAVAILABLE; /*方法缓存*/
    /*
     在实际使用中，这个对象只有一部分方法是常用的，很多方法其实很少用或者根本用不上。
     这种情况下，如果每次消息来时，我们都是methodLists中遍历一遍，性能势必很差。这时，cache就派上用场了。
     在我们每次调用过一个方法后，这个方法就会被缓存到cache列表中，下次调用的时候runtime就会优先去cache中查找，
     如果cache没有，才去methodLists中查找方法。这样，对于那些经常用到的方法的调用，但提高了调用的效率。
     */
#endif

} OBJC2_UNAVAILABLE;

// Note add by xiaohai
// 2. 属性
// 我们每定义一个@property都会添加对应的ivar, getter和setter到类结构体objc_class中。具体来说，系统会在objc_ivar_list中添加一个成员变量的描述，然后在methodLists中分别添加setter和getter方法的描述。
// 这也是为什么类结构体中没有属性结构体的原因， property 信息分开存储到了 objc_ivar_list 和 objc_method_list中
typedef struct objc_property *objc_property_t;

// Note add by xiaohai
// 3. 实例变量
typedef struct objc_ivar *Ivar;

// Note add by xiaohai
// 4. 实例方法
typedef struct objc_method *Method;

// Note add by xiaohai
// 5. 分类
typedef struct objc_category *Category;

#endif

//Note add by xiaohai
//实例变量定义
struct objc_ivar {
    char * _Nullable ivar_name                               OBJC2_UNAVAILABLE;
    char * _Nullable ivar_type                               OBJC2_UNAVAILABLE;
    int ivar_offset /*基地址偏移字节*/                          OBJC2_UNAVAILABLE;
#ifdef __LP64__
    int space                                                OBJC2_UNAVAILABLE;
#endif
}
OBJC2_UNAVAILABLE;
//Note add by xiaohai
//实例变量列表
struct objc_ivar_list {
    int ivar_count                                           OBJC2_UNAVAILABLE;
#ifdef __LP64__
    int space                                                OBJC2_UNAVAILABLE;
#endif
    /* variable length structure */
    struct objc_ivar ivar_list[1]                            OBJC2_UNAVAILABLE;
}                                                            OBJC2_UNAVAILABLE;

//Note add by xiaohai
//方法定义 Method = SEL + IMP + method_types 在Mac OS X中SEL其实被映射为一个C字符串，可以看作是方法的名字，它并不一个指向具体方法实现
//Note add by xiaohai Method && SEL && IMP 区别：
//一个类（Class）持有一个分发表，在运行期分发消息，表中的每一个实体代表一个方法（Method），它的名字叫做选择子（SEL），对应着一种方法实现（IMP）。
struct objc_method {
    SEL _Nonnull method_name                                 OBJC2_UNAVAILABLE;//前面提到过相同名字的方法即使在不同类中定义，它们的方法选择器也相同。
    char * _Nullable method_types                            OBJC2_UNAVAILABLE;//存储着方法的参数类型和返回值类型
    IMP _Nonnull method_imp                                  OBJC2_UNAVAILABLE;//指向方法的实现，本质上是一个函数的指针
}                                                            OBJC2_UNAVAILABLE;

struct objc_method_list {
    struct objc_method_list * _Nullable obsolete             OBJC2_UNAVAILABLE;
    int method_count                                         OBJC2_UNAVAILABLE;
#ifdef __LP64__
    int space                                                OBJC2_UNAVAILABLE;
#endif
    /* variable length structure */
    struct objc_method method_list[1]                        OBJC2_UNAVAILABLE;
}                                                            OBJC2_UNAVAILABLE;

//Note add by xiaohai
//协议及协议列表定义
#ifdef __OBJC__
@class Protocol;
#else
typedef struct objc_object Protocol;
#endif

struct objc_protocol_list {
    struct objc_protocol_list * _Nullable next;
    long count;
    __unsafe_unretained Protocol * _Nullable list[1];
};

// Note add by xiaohai
// 6. 缓存定义
struct objc_cache {
    unsigned int mask /* total = mask + 1 */                 OBJC2_UNAVAILABLE;
    unsigned int occupied                                    OBJC2_UNAVAILABLE;
    Method _Nullable buckets[1]                              OBJC2_UNAVAILABLE;
};

//mask：一个整数，指定分配的缓存bucket的总数。在方法查找过程中，Objective-C runtime使用这个字段来确定开始线性查找数组的索引位置。
//指向方法selector的指针与该字段做一个AND位操作(index = (mask & selector))。这可以作为一个简单的hash散列算法。
//occupied：一个整数，指定实际占用的缓存bucket的总数。
//buckets：指向Method数据结构指针的数组。这个数组可能包含不超过mask+1个元素。需要注意的是，指针可能是NULL，
//表示这个缓存bucket没有被占用，另外被占用的bucket可能是不连续的。这个数组可能会随着时间而增长。

// 方法描述
/// Defines a method
struct objc_method_description {
    SEL _Nullable name;               /**< The name of the method */
    char * _Nullable types;           /**< The types of the method arguments */
};

/// Defines a property attribute
typedef struct {
    const char * _Nonnull name;           /**< The name of the attribute */
    const char * _Nonnull value;          /**< The value of the attribute (usually empty) */
} objc_property_attribute_t;

////===================================================END 基本结构体定义 END======================================================///
/* Functions */

/* Working with Instances */

/** 
 * Returns a copy of a given object.
 * 返回一个给定对象的副本
 * @param obj An Objective-C object.            OC 对象
 * @param size The size of the object \e obj.   对象的大小
 * 
 * @return A copy of \e obj.                    OC 对象的副本
 */
OBJC_EXPORT id _Nullable object_copy(id _Nullable obj, size_t size)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARC_UNAVAILABLE;

/** 
 * Frees the memory occupied by a given object.
 * 释放被某个对象占用的内存空间
 *
 * @param obj An Objective-C object.            要释放的对象
 * 
 * @return nil
 */
OBJC_EXPORT id _Nullable
object_dispose(id _Nullable obj)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARC_UNAVAILABLE;

/** 
 * Returns the class of an object.
 * 返回一个实例对象的Class
 *
 * @param obj The object you want to inspect.
 * 
 * @return The class object of which \e object is an instance, 
 *  or \c Nil if \e object is \c nil.
 */
OBJC_EXPORT Class _Nullable
object_getClass(id _Nullable obj) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Sets the class of an object.
 * 设置某个对象的Class,返回object设置之前的Class
 * Note by xiaohai 这个一般用在什么场景？
 * @param obj The object to modify.
 * @param cls A class object.
 * 
 * @return The previous value of \e object's class, or \c Nil if \e object is \c nil.
 */
OBJC_EXPORT Class _Nullable
object_setClass(id _Nullable obj, Class _Nonnull cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns whether an object is a class object.
 * 判断某个类是一个对象还是类对象，类也是一个对象，如果是一个类或者元类返回true
 *
 * @param obj An Objective-C object.
 * 
 * @return true if the object is a class or metaclass, false otherwise.
 */
OBJC_EXPORT BOOL
object_isClass(id _Nullable obj)
    OBJC_AVAILABLE(10.10, 8.0, 9.0, 1.0, 2.0);


///-----------------------------------操作实例变量-------------------------------------

/** 
 * Reads the value of an instance variable in an object.
 * 返回对象中某个实例变量的值
 *
 * @param obj The object containing the instance variable whose value you want to read.
 * @param ivar The Ivar describing the instance variable whose value you want to read.
 * 
 * @return The value of the instance variable specified by \e ivar, or \c nil if \e object is \c nil.
 *
 * @note 在某个实例的Ivart已经知道的情况下 object_getIvar 会比 object_getInstanceVariable快速
 * @note \c object_getIvar is faster than \c object_getInstanceVariable if the Ivar
 *  for the instance variable is already known.
 */
OBJC_EXPORT id _Nullable
object_getIvar(id _Nullable obj, Ivar _Nonnull ivar) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Sets the value of an instance variable in an object.
 * 设置对象中某个实例变量的值
 *
 * @param obj The object containing the instance variable whose value you want to set.
 * @param ivar The Ivar describing the instance variable whose value you want to set.
 * @param value The new value for the instance variable.
 * 
 * @note Instance variables with known memory management (such as ARC strong and weak)
 *  use that memory management. Instance variables with unknown memory management 
 *  are assigned as if they were unsafe_unretained.
 * @note \c object_setIvar is faster than \c object_setInstanceVariable if the Ivar
 *  for the instance variable is already known.
 */
OBJC_EXPORT void
object_setIvar(id _Nullable obj, Ivar _Nonnull ivar, id _Nullable value) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Sets the value of an instance variable in an object.
 *
 * 设置对象中某个实例变量的值，如果指定了某个内存管理类型那么使用指定的类型，否则使用strong作为默认的类型
 *
 * @param obj The object containing the instance variable whose value you want to set.
 * @param ivar The Ivar describing the instance variable whose value you want to set.
 * @param value The new value for the instance variable.
 * 
 * @note Instance variables with known memory management (such as ARC strong and weak)
 *  use that memory management. Instance variables with unknown memory management 
 *  are assigned as if they were strong.
 * @note \c object_setIvar is faster than \c object_setInstanceVariable if the Ivar
 *  for the instance variable is already known.
 */
OBJC_EXPORT void
object_setIvarWithStrongDefault(id _Nullable obj, Ivar _Nonnull ivar,
                                id _Nullable value) 
    OBJC_AVAILABLE(10.12, 10.0, 10.0, 3.0, 2.0);

/** 
 * Changes the value of an instance variable of a class instance.
 *
 * 修改某个实例的实例变量值，如果指定了某个内存管理类型那么使用指定的类型，否则使用unsafe_unretained作为默认的类型
 *
 * @param obj A pointer to an instance of a class. Pass the object containing
 *  the instance variable whose value you wish to modify.
 * @param name A C string. Pass the name of the instance variable whose value you wish to modify.
 * @param value The new value for the instance variable.
 * 
 * @return A pointer to the \c Ivar data structure that defines the type and 
 *  name of the instance variable specified by \e name.
 *
 * @note Instance variables with known memory management (such as ARC strong and weak)
 *  use that memory management. Instance variables with unknown memory management 
 *  are assigned as if they were unsafe_unretained.
 */
OBJC_EXPORT Ivar _Nullable
object_setInstanceVariable(id _Nullable obj, const char * _Nonnull name,
                           void * _Nullable value)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARC_UNAVAILABLE;

/** 
 * Changes the value of an instance variable of a class instance.
 * 修改某个实例的实例变量值，如果指定了某个内存管理类型那么使用指定的类型，否则使用strong作为默认的类型
 *
 * @param obj A pointer to an instance of a class. Pass the object containing
 *  the instance variable whose value you wish to modify.
 * @param name A C string. Pass the name of the instance variable whose value you wish to modify.
 * @param value The new value for the instance variable.
 * 
 * @return A pointer to the \c Ivar data structure that defines the type and 
 *  name of the instance variable specified by \e name.
 *
 * @note Instance variables with known memory management (such as ARC strong and weak)
 *  use that memory management. Instance variables with unknown memory management 
 *  are assigned as if they were strong.
 */
OBJC_EXPORT Ivar _Nullable
object_setInstanceVariableWithStrongDefault(id _Nullable obj,
                                            const char * _Nonnull name,
                                            void * _Nullable value)
    OBJC_AVAILABLE(10.12, 10.0, 10.0, 3.0, 2.0)
    OBJC_ARC_UNAVAILABLE;

/** 
 * Obtains the value of an instance variable of a class instance.
 * 获取某个实例变量的值
 * @param obj A pointer to an instance of a class. Pass the object containing the instance variable whose value you wish to obtain.
 * @param obj 指向某个类的实例，传入包含你要获取实例变量的对象
 * @param name A C string. Pass the name of the instance variable whose value you wish to obtain.
 * @param name 要获取的实例变量的变量名
 * @param outValue On return, contains a pointer to the value of the instance variable.
 * @param outValue 指向实例变量的指针
 * @return A pointer to the \c Ivar data structure that defines the type and name of
 *  the instance variable specified by \e name.
 * 返回指向描述要获取实例变量结构体的指针
 */
OBJC_EXPORT Ivar _Nullable
object_getInstanceVariable(id _Nullable obj, const char * _Nonnull name,
                           void * _Nullable * _Nullable outValue)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARC_UNAVAILABLE;
///-----------------------------------操作实例变量-------------------------------------

///-----------------------------------获取类的定义-------------------------------------

/** 
 * Returns the class definition of a specified class.
 * 传入一个class 的名称，返回指定类的定义
 * @param name The name of the class to look up.
 * 
 * @return The Class object for the named class, or \c nil
 *  if the class is not registered with the Objective-C runtime.
 *  objc_getClass 和 objc_lookUpClass 的区别是如果类没有注册那么objc_getClass 将会调用类的处理回调然后再次确认类是否注册了，objc_lookUpClass 不会调用类的处理回调
 * @note \c objc_getClass is different from \c objc_lookUpClass in that if the class
 *  is not registered, \c objc_getClass calls the class handler callback and then checks
 *  a second time to see whether the class is registered. \c objc_lookUpClass does 
 *  not call the class handler callback.
 * 
 * @warning Earlier implementations of this function (prior to OS X v10.0)
 *  terminate the program if the class does not exist.
 */
OBJC_EXPORT Class _Nullable
objc_getClass(const char * _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the metaclass definition of a specified class.
 * 传入一个class 的名称，返回指定元类的定义
 * @param name The name of the class to look up.
 * 
 * @return The \c Class object for the metaclass of the named class, or \c nil if the class
 *  is not registered with the Objective-C runtime.
 * 
 * @note If the definition for the named class is not registered, this function calls the class handler
 *  callback and then checks a second time to see if the class is registered. However, every class
 *  definition must have a valid metaclass definition, and so the metaclass definition is always returned,
 *  whether it’s valid or not.
 */
OBJC_EXPORT Class _Nullable
objc_getMetaClass(const char * _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the class definition of a specified class.
 * 返回指定类的定义 该方法不会进行二次确认
 * @param name The name of the class to look up.
 * 
 * @return The Class object for the named class, or \c nil if the class
 *  is not registered with the Objective-C runtime.
 * 
 * @note \c objc_getClass is different from this function in that if the class is not
 *  registered, \c objc_getClass calls the class handler callback and then checks a second
 *  time to see whether the class is registered. This function does not call the class handler callback.
 */
OBJC_EXPORT Class _Nullable
objc_lookUpClass(const char * _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the class definition of a specified class.
 * 返回指定类的定义 如果没找到类则杀死进程
 *
 * @param name The name of the class to look up.
 * 
 * @return The Class object for the named class.
 * 
 * @note This function is the same as \c objc_getClass, but kills the process if the class is not found.
 * @note This function is used by ZeroLink, where failing to find a class would be a compile-time link error without ZeroLink.
 */
OBJC_EXPORT Class _Nonnull
objc_getRequiredClass(const char * _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Obtains the list of registered class definitions.
 * 获取注册的类定义列表
 *
 * @param buffer An array of \c Class values. On output, each \c Class value points to
 *  one class definition, up to either \e bufferCount or the total number of registered classes,
 *  whichever is less. You can pass \c NULL to obtain the total number of registered class
 *  definitions without actually retrieving any class definitions.
 * @param bufferCount An integer value. Pass the number of pointers for which you have allocated space
 *  in \e buffer. On return, this function fills in only this number of elements. If this number is less
 *  than the number of registered classes, this function returns an arbitrary subset of the registered classes.
 * 
 * @return An integer value indicating the total number of registered classes.
 * Objective-C 运行时库将会自动注册我们再代码中定义的类，我们也可以在运行时定义一个类并使用objc_addClass方法进行注册
 * @note The Objective-C runtime library automatically registers all the classes defined in your source code.
 *  You can create class definitions at runtime and register them with the \c objc_addClass function.
 * 
 * @warning You cannot assume that class objects you get from this function are classes that inherit from \c NSObject,
 *  so you cannot safely call any methods on such classes without detecting that the method is implemented first.
 */
OBJC_EXPORT int
objc_getClassList(Class _Nonnull * _Nullable buffer, int bufferCount)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Creates and returns a list of pointers to all registered class definitions.
 *
 * 创建并返回所有已经注册的类定义的指针列表
 *
 * @param outCount An integer pointer used to store the number of classes returned by
 *  this function in the list. It can be \c nil.
 * 
 * @return A nil terminated array of classes. It must be freed with \c free().
 * 
 * @see objc_getClassList
 */
OBJC_EXPORT Class _Nonnull * _Nullable
objc_copyClassList(unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.7, 3.1, 9.0, 1.0, 2.0);


///-----------------------------------操作类方法定义-------------------------------------

/** 
 * Returns the name of a class.
 * 返回某个类的名称
 * @param cls A class object.
 * 
 * @return The name of the class, or the empty string if \e cls is \c Nil.
 */
OBJC_EXPORT const char * _Nonnull
class_getName(Class _Nullable cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a Boolean value that indicates whether a class object is a metaclass.
 * 是否是一个元类
 * @param cls A class object.
 * 
 * @return \c YES if \e cls is a metaclass, \c NO if \e cls is a non-meta class, 
 *  \c NO if \e cls is \c Nil.
 */
OBJC_EXPORT BOOL
class_isMetaClass(Class _Nullable cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the superclass of a class.
 *
 * 返回一个类的父类
 *
 * @param cls A class object.
 * 
 * @return The superclass of the class, or \c Nil if
 *  \e cls is a root class, or \c Nil if \e cls is \c Nil.
 *
 * @note You should usually use \c NSObject's \c superclass method instead of this function.
 */
OBJC_EXPORT Class _Nullable
class_getSuperclass(Class _Nullable cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/**
 * Sets the superclass of a given class.
 *
 * @param cls The class whose superclass you want to set.
 * @param newSuper The new superclass for cls.
 *
 * @return The old superclass for cls.
 *
 * @warning You should not use this function.
 */
OBJC_EXPORT Class _Nonnull
class_setSuperclass(Class _Nonnull cls, Class _Nonnull newSuper)
__OSX_DEPRECATED(10.5, 10.5, "not recommended")
__IOS_DEPRECATED(2.0, 2.0, "not recommended")
__TVOS_DEPRECATED(9.0, 9.0, "not recommended")
__WATCHOS_DEPRECATED(1.0, 1.0, "not recommended")
__BRIDGEOS_DEPRECATED(2.0, 2.0, "not recommended");


/** 
 * Returns the version number of a class definition.
 *
 * 返回类的版本信息
 *
 * @param cls A pointer to a \c Class data structure. Pass
 *  the class definition for which you wish to obtain the version.
 * 
 * @return An integer indicating the version number of the class definition.
 *
 * @see class_setVersion
 */
OBJC_EXPORT int
class_getVersion(Class _Nullable cls)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Sets the version number of a class definition.
 *
 * 设置类的版本号
 *
 * @param cls A pointer to an Class data structure. 
 *  Pass the class definition for which you wish to set the version.
 * @param version An integer. Pass the new version number of the class definition.
 *
 * @note You can use the version number of the class definition to provide versioning of the
 *  interface that your class represents to other classes. This is especially useful for object
 *  serialization (that is, archiving of the object in a flattened form), where it is important to
 *  recognize changes to the layout of the instance variables in different class-definition versions.
 * @note Classes derived from the Foundation framework \c NSObject class can set the class-definition
 *  version number using the \c setVersion: class method, which is implemented using the \c class_setVersion function.
 */
OBJC_EXPORT void
class_setVersion(Class _Nullable cls, int version)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the size of instances of a class.
 *
 * 获取一个类的实例大小
 *
 * @param cls A class object.
 * 
 * @return The size in bytes of instances of the class \e cls, or \c 0 if \e cls is \c Nil.
 */
OBJC_EXPORT size_t
class_getInstanceSize(Class _Nullable cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the \c Ivar for a specified instance variable of a given class.
 *
 * 获取某个类的实例变量
 * @param cls The class whose instance variable you wish to obtain.
 * @param name The name of the instance variable definition to obtain.
 * 
 * @return A pointer to an \c Ivar data structure containing information about 
 *  the instance variable specified by \e name.
 */
OBJC_EXPORT Ivar _Nullable
class_getInstanceVariable(Class _Nullable cls, const char * _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the Ivar for a specified class variable of a given class.
 * 返回某个类的类变量
 * @param cls The class definition whose class variable you wish to obtain.
 * @param name The name of the class variable definition to obtain.
 * 
 * @return A pointer to an \c Ivar data structure containing information about the class variable specified by \e name.
 */
OBJC_EXPORT Ivar _Nullable
class_getClassVariable(Class _Nullable cls, const char * _Nonnull name) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Describes the instance variables declared by a class.
 * 某个类实例变量列表
 * @param cls The class to inspect.
 * @param outCount On return, contains the length of the returned array. 
 *  If outCount is NULL, the length is not returned.
 * 
 * @return An array of pointers of type Ivar describing the instance variables declared by the class. 
 *  Any instance variables declared by superclasses are not included. The array contains *outCount 
 *  pointers followed by a NULL terminator. You must free the array with free().
 * 
 *  If the class declares no instance variables, or cls is Nil, NULL is returned and *outCount is 0.
 */
OBJC_EXPORT Ivar _Nonnull * _Nullable
class_copyIvarList(Class _Nullable cls, unsigned int * _Nullable outCount) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a specified instance method for a given class.
 * 获取某个类的特定实例方法
 * @param cls The class you want to inspect.
 * @param name The selector of the method you want to retrieve.
 * 
 * @return The method that corresponds to the implementation of the selector specified by 
 *  \e name for the class specified by \e cls, or \c NULL if the specified class or its 
 *  superclasses do not contain an instance method with the specified selector.
 *
 * @note This function searches superclasses for implementations, whereas \c class_copyMethodList does not.
 */
OBJC_EXPORT Method _Nullable
class_getInstanceMethod(Class _Nullable cls, SEL _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a pointer to the data structure describing a given class method for a given class.
 *
 * 获取某个类的特定类方法结构体
 *
 * @param cls A pointer to a class definition. Pass the class that contains the method you want to retrieve.
 * @param name A pointer of type \c SEL. Pass the selector of the method you want to retrieve.
 * 
 * @return A pointer to the \c Method data structure that corresponds to the implementation of the 
 *  selector specified by aSelector for the class specified by aClass, or NULL if the specified 
 *  class or its superclasses do not contain an instance method with the specified selector.
 *
 * @note Note that this function searches superclasses for implementations, 
 *  whereas \c class_copyMethodList does not.
 */
OBJC_EXPORT Method _Nullable
class_getClassMethod(Class _Nullable cls, SEL _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the function pointer that would be called if a 
 * particular message were sent to an instance of a class.
 *
 * 获取某个类实例的IMP
 *
 * @param cls The class you want to inspect.
 * @param name A selector.
 * 
 * @return The function pointer that would be called if \c [object name] were called
 *  with an instance of the class, or \c NULL if \e cls is \c Nil.
 *
 * @note \c class_getMethodImplementation may be faster than \c method_getImplementation(class_getInstanceMethod(cls, name)).
 * @note The function pointer returned may be a function internal to the runtime instead of
 *  an actual method implementation. For example, if instances of the class do not respond to
 *  the selector, the function pointer returned will be part of the runtime's message forwarding machinery.
 */
OBJC_EXPORT IMP _Nullable
class_getMethodImplementation(Class _Nullable cls, SEL _Nonnull name) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the function pointer that would be called if a particular 
 * message were sent to an instance of a class.
 *
 * @param cls The class you want to inspect.
 * @param name A selector.
 * 
 * @return The function pointer that would be called if \c [object name] were called
 *  with an instance of the class, or \c NULL if \e cls is \c Nil.
 */
OBJC_EXPORT IMP _Nullable
class_getMethodImplementation_stret(Class _Nullable cls, SEL _Nonnull name) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARM64_UNAVAILABLE;

/** 
 * Returns a Boolean value that indicates whether instances of a class respond to a particular selector.
 * 返回当前类是否能够响应某个特定的selector
 * @param cls The class you want to inspect.
 * @param sel A selector.
 * 
 * @return \c YES if instances of the class respond to the selector, otherwise \c NO.
 * 
 * @note You should usually use \c NSObject's \c respondsToSelector: or \c instancesRespondToSelector: 
 *  methods instead of this function.
 */
OBJC_EXPORT BOOL
class_respondsToSelector(Class _Nullable cls, SEL _Nonnull sel) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Describes the instance methods implemented by a class.
 *
 * 返回某个类实现的实例方法列表
 *
 * @param cls The class you want to inspect.
 * @param outCount On return, contains the length of the returned array. 
 *  If outCount is NULL, the length is not returned.
 * 
 * @return An array of pointers of type Method describing the instance methods 
 *  implemented by the class—any instance methods implemented by superclasses are not included. 
 *  The array contains *outCount pointers followed by a NULL terminator. You must free the array with free().
 * 
 *  If cls implements no instance methods, or cls is Nil, returns NULL and *outCount is 0.
 * 
 * @note To get the class methods of a class, use \c class_copyMethodList(object_getClass(cls), &count).
 * @note To get the implementations of methods that may be implemented by superclasses, 
 *  use \c class_getInstanceMethod or \c class_getClassMethod.
 */
OBJC_EXPORT Method _Nonnull * _Nullable
class_copyMethodList(Class _Nullable cls, unsigned int * _Nullable outCount) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a Boolean value that indicates whether a class conforms to a given protocol.
 *
 * 返回一个类是否遵循某个协议
 *
 * @param cls The class you want to inspect.
 * @param protocol A protocol.
 *
 * @return YES if cls conforms to protocol, otherwise NO.
 *
 * @note You should usually use NSObject's conformsToProtocol: method instead of this function.
 */
OBJC_EXPORT BOOL
class_conformsToProtocol(Class _Nullable cls, Protocol * _Nullable protocol) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Describes the protocols adopted by a class.
 *
 * 返回当前类实现的协议
 *
 * @param cls The class you want to inspect.
 * @param outCount On return, contains the length of the returned array. 
 *  If outCount is NULL, the length is not returned.
 * 
 * @return An array of pointers of type Protocol* describing the protocols adopted 
 *  by the class. Any protocols adopted by superclasses or other protocols are not included. 
 *  The array contains *outCount pointers followed by a NULL terminator. You must free the array with free().
 * 
 *  If cls adopts no protocols, or cls is Nil, returns NULL and *outCount is 0.
 */
OBJC_EXPORT Protocol * __unsafe_unretained _Nonnull * _Nullable 
class_copyProtocolList(Class _Nullable cls, unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a property with a given name of a given class.
 *
 * 获取某个类的特定属性
 *
 * @param cls The class you want to inspect.
 * @param name The name of the property you want to inspect.
 * 
 * @return A pointer of type \c objc_property_t describing the property, or
 *  \c NULL if the class does not declare a property with that name, 
 *  or \c NULL if \e cls is \c Nil.
 */
OBJC_EXPORT objc_property_t _Nullable
class_getProperty(Class _Nullable cls, const char * _Nonnull name)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Describes the properties declared by a class.
 *
 * 返回某个类的属性列表
 *
 * @param cls The class you want to inspect.
 * @param outCount On return, contains the length of the returned array. 
 *  If \e outCount is \c NULL, the length is not returned.        
 * 
 * @return An array of pointers of type \c objc_property_t describing the properties 
 *  declared by the class. Any properties declared by superclasses are not included. 
 *  The array contains \c *outCount pointers followed by a \c NULL terminator. You must free the array with \c free().
 * 
 *  If \e cls declares no properties, or \e cls is \c Nil, returns \c NULL and \c *outCount is \c 0.
 */
OBJC_EXPORT objc_property_t _Nonnull * _Nullable
class_copyPropertyList(Class _Nullable cls, unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a description of the \c Ivar layout for a given class.
 * 
 * @param cls The class to inspect.
 * 
 * @return A description of the \c Ivar layout for \e cls.
 */
OBJC_EXPORT const uint8_t * _Nullable
class_getIvarLayout(Class _Nullable cls)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a description of the layout of weak Ivars for a given class.
 * 
 * @param cls The class to inspect.
 * 
 * @return A description of the layout of the weak \c Ivars for \e cls.
 */
OBJC_EXPORT const uint8_t * _Nullable
class_getWeakIvarLayout(Class _Nullable cls)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Adds a new method to a class with a given name and implementation.
 *
 * 为某个类添加指定方法名和实现的新方法
 *
 * @param cls The class to which to add a method.   要添加方法的类
 * @param name A selector that specifies the name of the method being added. 所添加方法方法名的selector
 * @param imp A function which is the implementation of the new method. The function must take at least two arguments—self and _cmd.函数指针
 * @param types An array of characters that describe the types of the arguments to the method. 用于描述参数类型的字符串
 * 
 * @return YES if the method was added successfully, otherwise NO
 *  (for example, the class already contains a method implementation with that name).
 *
 * class_addMethod 可以添加一个父类的重写实现，但是不会替换一个类中已经存在的方法的实现
 * @note class_addMethod will add an override of a superclass's implementation, 
 *  but will not replace an existing implementation in this class. 
 *  To change an existing implementation, use method_setImplementation.
 */
OBJC_EXPORT BOOL
class_addMethod(Class _Nullable cls, SEL _Nonnull name, IMP _Nonnull imp, 
                const char * _Nullable types) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Replaces the implementation of a method for a given class.
 *
 * 替换某个类的特定方法实现
 *
 * @param cls The class you want to modify.
 * @param name A selector that identifies the method whose implementation you want to replace.
 * @param imp The new implementation for the method identified by name for the class identified by cls.
 * @param types An array of characters that describe the types of the arguments to the method. 
 *  Since the function must take at least two arguments—self and _cmd, the second and third characters
 *  must be “@:” (the first character is the return type).
 * 
 * @return The previous implementation of the method identified by \e name for the class identified by \e cls.
 * 
 * @note This function behaves in two different ways:
 *  如果不存在那么添加，如果存在替换
 *  - If the method identified by \e name does not yet exist, it is added as if \c class_addMethod were called. 
 *    The type encoding specified by \e types is used as given.
 *  - If the method identified by \e name does exist, its \c IMP is replaced as if \c method_setImplementation were called.
 *    The type encoding specified by \e types is ignored.
 */
OBJC_EXPORT IMP _Nullable
class_replaceMethod(Class _Nullable cls, SEL _Nonnull name, IMP _Nonnull imp, 
                    const char * _Nullable types) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Adds a new instance variable to a class.
 * 添加一个新的实例变量到一个类
 * @return YES if the instance variable was added successfully, otherwise NO 
 *         (for example, the class already contains an instance variable with that name).
 *
 * @note This function may only be called after objc_allocateClassPair and before objc_registerClassPair. 
 *       Adding an instance variable to an existing class is not supported.
 * @note The class must not be a metaclass. Adding an instance variable to a metaclass is not supported.
 * @note The instance variable's minimum alignment in bytes is 1<<align. The minimum alignment of an instance 
 *       variable depends on the ivar's type and the machine architecture. 
 *       For variables of any pointer type, pass log2(sizeof(pointer_type)).
 */
OBJC_EXPORT BOOL
class_addIvar(Class _Nullable cls, const char * _Nonnull name, size_t size, 
              uint8_t alignment, const char * _Nullable types) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Adds a protocol to a class.
 * 添加一个协议到类实现
 * @param cls The class to modify.
 * @param protocol The protocol to add to \e cls.
 * 
 * @return \c YES if the method was added successfully, otherwise \c NO 
 *  (for example, the class already conforms to that protocol).
 */
OBJC_EXPORT BOOL
class_addProtocol(Class _Nullable cls, Protocol * _Nonnull protocol) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Adds a property to a class.
 * 添加一个属性到一个类中
 * @param cls The class to modify.
 * @param name The name of the property.
 * @param attributes An array of property attributes.
 * @param attributeCount The number of attributes in \e attributes.
 * 
 * @return \c YES if the property was added successfully, otherwise \c NO
 *  (for example, the class already has that property).
 */
OBJC_EXPORT BOOL
class_addProperty(Class _Nullable cls, const char * _Nonnull name,
                  const objc_property_attribute_t * _Nullable attributes,
                  unsigned int attributeCount)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Replace a property of a class. 
 * 替换属性
 * @param cls The class to modify.
 * @param name The name of the property.
 * @param attributes An array of property attributes.
 * @param attributeCount The number of attributes in \e attributes. 
 */
OBJC_EXPORT void
class_replaceProperty(Class _Nullable cls, const char * _Nonnull name,
                      const objc_property_attribute_t * _Nullable attributes,
                      unsigned int attributeCount)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Sets the Ivar layout for a given class.
 * 
 * @param cls The class to modify.
 * @param layout The layout of the \c Ivars for \e cls.
 */
OBJC_EXPORT void
class_setIvarLayout(Class _Nullable cls, const uint8_t * _Nullable layout)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Sets the layout for weak Ivars for a given class.
 * 
 * @param cls The class to modify.
 * @param layout The layout of the weak Ivars for \e cls.
 */
OBJC_EXPORT void
class_setWeakIvarLayout(Class _Nullable cls, const uint8_t * _Nullable layout)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Used by CoreFoundation's toll-free bridging.
 * Return the id of the named class.
 * 
 * @return The id of the named class, or an uninitialized class
 *  structure that will be used for the class when and if it does 
 *  get loaded.
 * 
 * @warning Do not call this function yourself.
 */
OBJC_EXPORT Class _Nonnull
objc_getFutureClass(const char * _Nonnull name) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0)
    OBJC_ARC_UNAVAILABLE;


///-----------------------------------类实例化-------------------------------------

/** 
 * Creates an instance of a class, allocating memory for the class in the 
 * default malloc memory zone.
 * 创建一个类的实例，在默认的对内存中分配出一个内存zone
 * @param cls The class that you wish to allocate an instance of.
 * @param extraBytes An integer indicating the number of extra bytes to allocate. 
 *  The additional bytes can be used to store additional instance variables beyond 
 *  those defined in the class definition.
 * 
 * @return An instance of the class \e cls.
 */
OBJC_EXPORT id _Nullable
class_createInstance(Class _Nullable cls, size_t extraBytes)
    OBJC_RETURNS_RETAINED
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Creates an instance of a class at the specific location provided.
 *
 * 在给定的存储位置创建一个类的实例
 *
 * @param cls The class that you wish to allocate an instance of.
 * @param bytes The location at which to allocate an instance of \e cls.
 *  Must point to at least \c class_getInstanceSize(cls) bytes of well-aligned,
 *  zero-filled memory.
 *
 * @return \e bytes on success, \c nil otherwise. (For example, \e cls or \e bytes
 *  might be \c nil)
 *
 * @see class_createInstance
 */
OBJC_EXPORT id _Nullable
objc_constructInstance(Class _Nullable cls, void * _Nullable bytes) 
    OBJC_AVAILABLE(10.6, 3.0, 9.0, 1.0, 2.0)
    OBJC_ARC_UNAVAILABLE;

/** 
 * Destroys an instance of a class without freeing memory and removes any
 * associated references this instance might have had.
 *
 * 销毁一个类的实例，注意这里不释放内存和任何与这个实例相关联的方法
 *
 * @param obj The class instance to destroy.
 * 
 * @return \e obj. Does nothing if \e obj is nil.
 * 
 * @note CF and other clients do call this under GC.
 */
OBJC_EXPORT void * _Nullable objc_destructInstance(id _Nullable obj) 
    OBJC_AVAILABLE(10.6, 3.0, 9.0, 1.0, 2.0)
    OBJC_ARC_UNAVAILABLE;


///-----------------------------------添加类-------------------------------------

/** 
 * Creates a new class and metaclass.
 * 创建一个新的类和元类
 * @param superclass The class to use as the new class's superclass, or \c Nil to create a new root class.//新类的父类
 * @param name The string to use as the new class's name. The string will be copied. //新类的名称
 * @param extraBytes The number of bytes to allocate for indexed ivars at the end of 
 *  the class and metaclass objects. This should usually be \c 0.
 * 
 * @return The new class, or Nil if the class could not be created (for example, the desired name is already in use).
 * 
 * @note You can get a pointer to the new metaclass by calling \c object_getClass(newClass).
 * @note To create a new class, start by calling \c objc_allocateClassPair. 
 *  Then set the class's attributes with functions like \c class_addMethod and \c class_addIvar.
 *  When you are done building the class, call \c objc_registerClassPair. The new class is now ready for use.
 *  要创建一个新的类，先通过objc_allocateClassPair，然后调用class_addMethod，class_addIvar 为类添加属性和方法，最后调用objc_registerClassPair进行注册。
 * @note Instance methods and instance variables should be added to the class itself. 
 *  Class methods should be added to the metaclass.
 */
OBJC_EXPORT Class _Nullable
objc_allocateClassPair(Class _Nullable superclass, const char * _Nonnull name, 
                       size_t extraBytes) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Registers a class that was allocated using \c objc_allocateClassPair.
 * 注册新创建的一个类
 * @param cls The class you want to register.
 */
OBJC_EXPORT void
objc_registerClassPair(Class _Nonnull cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Used by Foundation's Key-Value Observing.
 * 
 * @warning Do not call this function yourself.
 */
OBJC_EXPORT Class _Nonnull
objc_duplicateClass(Class _Nonnull original, const char * _Nonnull name,
                    size_t extraBytes)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Destroy a class and its associated metaclass. 
 * 销毁一个类以及它相关的元类
 * @param cls The class to be destroyed. It must have been allocated with 
 *  \c objc_allocateClassPair
 * 
 * @warning Do not call if instances of this class or a subclass exist.
 */
OBJC_EXPORT void
objc_disposeClassPair(Class _Nonnull cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);



///-----------------------------------方法相关操作-------------------------------------

/** 
 * Returns the name of a method.
 * 返回方法名
 * @param m The method to inspect.
 * 
 * @return A pointer of type SEL.
 * 
 * @note To get the method name as a C string, call \c sel_getName(method_getName(method)).
 */
OBJC_EXPORT SEL _Nonnull
method_getName(Method _Nonnull m) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the implementation of a method.
 * 返回方法的实现
 * @param m The method to inspect.
 * 
 * @return A function pointer of type IMP.
 */
OBJC_EXPORT IMP _Nonnull
method_getImplementation(Method _Nonnull m) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a string describing a method's parameter and return types.
 * 返回某个方法的参数及返回值类型
 * @param m The method to inspect.
 * 
 * @return A C string. The string may be \c NULL.
 */
OBJC_EXPORT const char * _Nullable
method_getTypeEncoding(Method _Nonnull m) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the number of arguments accepted by a method.
 * 返回某个方法的参数个数
 * @param m A pointer to a \c Method data structure. Pass the method in question.
 * 
 * @return An integer containing the number of arguments accepted by the given method.
 */
OBJC_EXPORT unsigned int
method_getNumberOfArguments(Method _Nonnull m)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a string describing a method's return type.
 * 返回某个方法的返回类型
 * @param m The method to inspect.
 * 
 * @return A C string describing the return type. You must free the string with \c free().
 */
OBJC_EXPORT char * _Nonnull
method_copyReturnType(Method _Nonnull m) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a string describing a single parameter type of a method.
 * 返回方法的某个参数类型
 * @param m The method to inspect.
 * @param index The index of the parameter to inspect.
 * 
 * @return A C string describing the type of the parameter at index \e index, or \c NULL
 *  if method has no parameter index \e index. You must free the string with \c free().
 */
OBJC_EXPORT char * _Nullable
method_copyArgumentType(Method _Nonnull m, unsigned int index) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns by reference a string describing a method's return type.
 * 返回一个方法的返回类型
 * @param m The method you want to inquire about. 
 * @param dst The reference string to store the description.
 * @param dst_len The maximum number of characters that can be stored in \e dst.
 *
 * @note The method's return type string is copied to \e dst.
 *  \e dst is filled as if \c strncpy(dst, parameter_type, dst_len) were called.
 */
OBJC_EXPORT void
method_getReturnType(Method _Nonnull m, char * _Nonnull dst, size_t dst_len) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns by reference a string describing a single parameter type of a method.
 * 返回方法的某个参数类型
 * @param m The method you want to inquire about. 
 * @param index The index of the parameter you want to inquire about.
 * @param dst The reference string to store the description.
 * @param dst_len The maximum number of characters that can be stored in \e dst.
 * 
 * @note The parameter type string is copied to \e dst. \e dst is filled as if \c strncpy(dst, parameter_type, dst_len) 
 *  were called. If the method contains no parameter with that index, \e dst is filled as
 *  if \c strncpy(dst, "", dst_len) were called.
 */
OBJC_EXPORT void
method_getArgumentType(Method _Nonnull m, unsigned int index, 
                       char * _Nullable dst, size_t dst_len) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

OBJC_EXPORT struct objc_method_description * _Nonnull
method_getDescription(Method _Nonnull m) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Sets the implementation of a method.
 * 为某个方法设置方法实现
 * @param m The method for which to set an implementation.
 * @param imp The implemention to set to this method.
 * 
 * @return The previous implementation of the method.
 */
OBJC_EXPORT IMP _Nonnull
method_setImplementation(Method _Nonnull m, IMP _Nonnull imp) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Exchanges the implementations of two methods.
 * 交换两个方法的实现
 * @param m1 Method to exchange with second method.
 * @param m2 Method to exchange with first method.
 * 
 * @note This is an atomic version of the following:
 *  \code 
 *  IMP imp1 = method_getImplementation(m1);
 *  IMP imp2 = method_getImplementation(m2);
 *  method_setImplementation(m1, imp2);
 *  method_setImplementation(m2, imp1);
 *  \endcode
 */
OBJC_EXPORT void
method_exchangeImplementations(Method _Nonnull m1, Method _Nonnull m2) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);


///-----------------------------------实例变量相关操作-------------------------------------

/** 
 * Returns the name of an instance variable.
 * 返回一个实例变量的名称
 * @param v The instance variable you want to enquire about.
 * 
 * @return A C string containing the instance variable's name.
 */
OBJC_EXPORT const char * _Nullable
ivar_getName(Ivar _Nonnull v) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the type string of an instance variable.
 * 返回某个实例变量的类型字符串
 * @param v The instance variable you want to enquire about.
 * 
 * @return A C string containing the instance variable's type encoding.
 *
 * @note For possible values, see Objective-C Runtime Programming Guide > Type Encodings.
 */
OBJC_EXPORT const char * _Nullable
ivar_getTypeEncoding(Ivar _Nonnull v) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the offset of an instance variable.
 * 返回一个实例变量的偏移
 * @param v The instance variable you want to enquire about.
 * 
 * @return The offset of \e v.
 * 
 * @note For instance variables of type \c id or other object types, call \c object_getIvar
 *  and \c object_setIvar instead of using this offset to access the instance variable data directly.
 */
OBJC_EXPORT ptrdiff_t
ivar_getOffset(Ivar _Nonnull v) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);


///-----------------------------------属性相关操作-------------------------------------

/** 
 * Returns the name of a property.
 * 返回某个属性的名称
 * @param property The property you want to inquire about.
 * 
 * @return A C string containing the property's name.
 */
OBJC_EXPORT const char * _Nonnull
property_getName(objc_property_t _Nonnull property) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the attribute string of a property.
 * 返回某个属性的名称
 * @param property A property.
 *
 * @return A C string containing the property's attributes.
 * 
 * @note The format of the attribute string is described in Declared Properties in Objective-C Runtime Programming Guide.
 */
OBJC_EXPORT const char * _Nullable
property_getAttributes(objc_property_t _Nonnull property) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns an array of property attributes for a property. 
 * 返回一个属性的属性
 * @param property The property whose attributes you want copied.
 * @param outCount The number of attributes returned in the array.
 * 
 * @return An array of property attributes; must be free'd() by the caller. 
 */
OBJC_EXPORT objc_property_attribute_t * _Nullable
property_copyAttributeList(objc_property_t _Nonnull property,
                           unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Returns the value of a property attribute given the attribute name.
 * 返回给定属性的属性值
 * @param property The property whose attribute value you are interested in.
 * @param attributeName C string representing the attribute name.
 *
 * @return The value string of the attribute \e attributeName if it exists in
 *  \e property, \c nil otherwise. 
 */
OBJC_EXPORT char * _Nullable
property_copyAttributeValue(objc_property_t _Nonnull property,
                            const char * _Nonnull attributeName)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);


///-----------------------------------协议相关操作-------------------------------------

/** 
 * Returns a specified protocol.
 * 返回特定的协议
 * @param name The name of a protocol.
 * 
 * @return The protocol named \e name, or \c NULL if no protocol named \e name could be found.
 * 
 * @note This function acquires the runtime lock.
 */
OBJC_EXPORT Protocol * _Nullable
objc_getProtocol(const char * _Nonnull name)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns an array of all the protocols known to the runtime.
 * 返回运行时所已经知道的协议
 * @param outCount Upon return, contains the number of protocols in the returned array.
 * 
 * @return A C array of all the protocols known to the runtime. The array contains \c *outCount
 *  pointers followed by a \c NULL terminator. You must free the list with \c free().
 * 
 * @note This function acquires the runtime lock.
 */
OBJC_EXPORT Protocol * __unsafe_unretained _Nonnull * _Nullable
objc_copyProtocolList(unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a Boolean value that indicates whether one protocol conforms to another protocol.
 * 返回某个协议是否实现了另一个协议
 * @param proto A protocol.
 * @param other A protocol.
 * 
 * @return \c YES if \e proto conforms to \e other, otherwise \c NO.
 * 
 * @note One protocol can incorporate other protocols using the same syntax 
 *  that classes use to adopt a protocol:
 *  \code
 *  @protocol ProtocolName < protocol list >
 *  \endcode
 *  All the protocols listed between angle brackets are considered part of the ProtocolName protocol.
 */
OBJC_EXPORT BOOL
protocol_conformsToProtocol(Protocol * _Nullable proto,
                            Protocol * _Nullable other)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a Boolean value that indicates whether two protocols are equal.
 * 返回两个协议是否相等
 * @param proto A protocol.
 * @param other A protocol.
 * 
 * @return \c YES if \e proto is the same as \e other, otherwise \c NO.
 */
OBJC_EXPORT BOOL
protocol_isEqual(Protocol * _Nullable proto, Protocol * _Nullable other)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the name of a protocol.
 * 返回协议的名称
 * @param proto A protocol.
 * 
 * @return The name of the protocol \e p as a C string.
 */
OBJC_EXPORT const char * _Nonnull
protocol_getName(Protocol * _Nonnull proto)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a method description structure for a specified method of a given protocol.
 * 返回给定协议的某个方法的方法描述结构体
 * @param proto A protocol.
 * @param aSel A selector.
 * @param isRequiredMethod A Boolean value that indicates whether aSel is a required method.
 * @param isInstanceMethod A Boolean value that indicates whether aSel is an instance method.
 * 
 * @return An \c objc_method_description structure that describes the method specified by \e aSel,
 *  \e isRequiredMethod, and \e isInstanceMethod for the protocol \e p.
 *  If the protocol does not contain the specified method, returns an \c objc_method_description structure
 *  with the value \c {NULL, \c NULL}.
 * 
 * @note This function recursively searches any protocols that this protocol conforms to.
 */
OBJC_EXPORT struct objc_method_description
protocol_getMethodDescription(Protocol * _Nonnull proto, SEL _Nonnull aSel,
                              BOOL isRequiredMethod, BOOL isInstanceMethod)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns an array of method descriptions of methods meeting a given specification for a given protocol.
 * 返回给定协议的方法描述数组
 * @param proto A protocol.
 * @param isRequiredMethod A Boolean value that indicates whether returned methods should
 *  be required methods (pass YES to specify required methods).
 * @param isInstanceMethod A Boolean value that indicates whether returned methods should
 *  be instance methods (pass YES to specify instance methods).
 * @param outCount Upon return, contains the number of method description structures in the returned array.
 * 
 * @return A C array of \c objc_method_description structures containing the names and types of \e p's methods 
 *  specified by \e isRequiredMethod and \e isInstanceMethod. The array contains \c *outCount pointers followed
 *  by a \c NULL terminator. You must free the list with \c free().
 *  If the protocol declares no methods that meet the specification, \c NULL is returned and \c *outCount is 0.
 * 
 * @note Methods in other protocols adopted by this protocol are not included.
 */
OBJC_EXPORT struct objc_method_description * _Nullable
protocol_copyMethodDescriptionList(Protocol * _Nonnull proto,
                                   BOOL isRequiredMethod,
                                   BOOL isInstanceMethod,
                                   unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the specified property of a given protocol.
 * 返回给定协议的特定属性
 * @param proto A protocol.
 * @param name The name of a property.
 * @param isRequiredProperty \c YES searches for a required property, \c NO searches for an optional property.
 * @param isInstanceProperty \c YES searches for an instance property, \c NO searches for a class property.
 * 
 * @return The property specified by \e name, \e isRequiredProperty, and \e isInstanceProperty for \e proto,
 *  or \c NULL if none of \e proto's properties meets the specification.
 */
OBJC_EXPORT objc_property_t _Nullable
protocol_getProperty(Protocol * _Nonnull proto,
                     const char * _Nonnull name,
                     BOOL isRequiredProperty, BOOL isInstanceProperty)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns an array of the required instance properties declared by a protocol.
 * 返回给定协议的required实例属性
 * @note Identical to 
 * \code
 * protocol_copyPropertyList2(proto, outCount, YES, YES);
 * \endcode
 */
OBJC_EXPORT objc_property_t _Nonnull * _Nullable
protocol_copyPropertyList(Protocol * _Nonnull proto,
                          unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns an array of properties declared by a protocol.
 * 返回某个协议的属性列表
 * @param proto A protocol.
 * @param outCount Upon return, contains the number of elements in the returned array.
 * @param isRequiredProperty \c YES returns required properties, \c NO returns optional properties.
 * @param isInstanceProperty \c YES returns instance properties, \c NO returns class properties.
 * 
 * @return A C array of pointers of type \c objc_property_t describing the properties declared by \e proto.
 *  Any properties declared by other protocols adopted by this protocol are not included. The array contains
 *  \c *outCount pointers followed by a \c NULL terminator. You must free the array with \c free().
 *  If the protocol declares no matching properties, \c NULL is returned and \c *outCount is \c 0.
 */
OBJC_EXPORT objc_property_t _Nonnull * _Nullable
protocol_copyPropertyList2(Protocol * _Nonnull proto,
                           unsigned int * _Nullable outCount,
                           BOOL isRequiredProperty, BOOL isInstanceProperty)
    OBJC_AVAILABLE(10.12, 10.0, 10.0, 3.0, 2.0);

/** 
 * Returns an array of the protocols adopted by a protocol.
 * 返回某个协议遵循的协议
 * @param proto A protocol.
 * @param outCount Upon return, contains the number of elements in the returned array.
 * 
 * @return A C array of protocols adopted by \e proto. The array contains \e *outCount pointers
 *  followed by a \c NULL terminator. You must free the array with \c free().
 *  If the protocol declares no properties, \c NULL is returned and \c *outCount is \c 0.
 */
OBJC_EXPORT Protocol * __unsafe_unretained _Nonnull * _Nullable
protocol_copyProtocolList(Protocol * _Nonnull proto,
                          unsigned int * _Nullable outCount)
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Creates a new protocol instance that cannot be used until registered with
 * \c objc_registerProtocol()
 *  创建一个新的协议
 * @param name The name of the protocol to create.
 *
 * @return The Protocol instance on success, \c nil if a protocol
 *  with the same name already exists. 
 * @note There is no dispose method for this. 
 */
OBJC_EXPORT Protocol * _Nullable
objc_allocateProtocol(const char * _Nonnull name) 
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Registers a newly constructed protocol with the runtime. The protocol
 * will be ready for use and is immutable after this.
 * 将一个协议注册到运行时
 * @param proto The protocol you want to register.
 */
OBJC_EXPORT void
objc_registerProtocol(Protocol * _Nonnull proto) 
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Adds a method to a protocol. The protocol must be under construction.
 * 添加一个方法给协议
 * @param proto The protocol to add a method to.
 * @param name The name of the method to add.
 * @param types A C string that represents the method signature.
 * @param isRequiredMethod YES if the method is not an optional method.
 * @param isInstanceMethod YES if the method is an instance method. 
 */
OBJC_EXPORT void
protocol_addMethodDescription(Protocol * _Nonnull proto, SEL _Nonnull name,
                              const char * _Nullable types,
                              BOOL isRequiredMethod, BOOL isInstanceMethod) 
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Adds an incorporated protocol to another protocol. The protocol being
 * added to must still be under construction, while the additional protocol
 * must be already constructed.
 * 添加一个协议到另一个协议，被添加的协议必须在创建之中，另一个协议协议必须是已经构建好的
 * @param proto The protocol you want to add to, it must be under construction.
 * @param addition The protocol you want to incorporate into \e proto, it must be registered.
 */
OBJC_EXPORT void
protocol_addProtocol(Protocol * _Nonnull proto, Protocol * _Nonnull addition) 
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Adds a property to a protocol. The protocol must be under construction. 
 * 添加一个属性到一个正在g构建到协议中
 * @param proto The protocol to add a property to.
 * @param name The name of the property.
 * @param attributes An array of property attributes.
 * @param attributeCount The number of attributes in \e attributes.
 * @param isRequiredProperty YES if the property (accessor methods) is not optional. 
 * @param isInstanceProperty YES if the property (accessor methods) are instance methods. 
 *  This is the only case allowed fo a property, as a result, setting this to NO will 
 *  not add the property to the protocol at all. 
 */
OBJC_EXPORT void
protocol_addProperty(Protocol * _Nonnull proto, const char * _Nonnull name,
                     const objc_property_attribute_t * _Nullable attributes,
                     unsigned int attributeCount,
                     BOOL isRequiredProperty, BOOL isInstanceProperty)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);


///-----------------------------------Libraries相关操作-------------------------------------

/** 
 * Returns the names of all the loaded Objective-C frameworks and dynamic
 * libraries.
 *
 * 返回oc加载的全部frameworks及动态库
 *
 * @param outCount The number of names returned.
 * 
 * @return An array of C strings of names. Must be free()'d by caller.
 */
OBJC_EXPORT const char * _Nonnull * _Nonnull
objc_copyImageNames(unsigned int * _Nullable outCount) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the dynamic library name a class originated from.
 *
 * 返回某个类的动态库名称
 *
 * @param cls The class you are inquiring about.
 * 
 * @return The name of the library containing this class.
 */
OBJC_EXPORT const char * _Nullable
class_getImageName(Class _Nullable cls) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns the names of all the classes within a library.
 * 返回某个库全部的类名
 * @param image The library or framework you are inquiring about.
 * @param outCount The number of class names returned.
 * 
 * @return An array of C strings representing the class names.
 */
OBJC_EXPORT const char * _Nonnull * _Nullable
objc_copyClassNamesForImage(const char * _Nonnull image,
                            unsigned int * _Nullable outCount) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

///-----------------------------------Selectors相关操作-------------------------------------

/** 
 * Returns the name of the method specified by a given selector.
 * 返回某个selector所指定的方法名
 * @param sel A pointer of type \c SEL. Pass the selector whose name you wish to determine.
 * 
 * @return A C string indicating the name of the selector.
 */
OBJC_EXPORT const char * _Nonnull
sel_getName(SEL _Nonnull sel)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);


/** 
 * Registers a method with the Objective-C runtime system, maps the method 
 * name to a selector, and returns the selector value.
 * 注册一个selector
 * @param str A pointer to a C string. Pass the name of the method you wish to register.
 * 
 * @return A pointer of type SEL specifying the selector for the named method.
 * 
 * @note You must register a method name with the Objective-C runtime system to obtain the
 *  method’s selector before you can add the method to a class definition. If the method name
 *  has already been registered, this function simply returns the selector.
 */
OBJC_EXPORT SEL _Nonnull
sel_registerName(const char * _Nonnull str)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

/** 
 * Returns a Boolean value that indicates whether two selectors are equal.
 * 返回俩个selector是否相等
 * @param lhs The selector to compare with rhs.
 * @param rhs The selector to compare with lhs.
 * 
 * @return \c YES if \e lhs and \e rhs are equal, otherwise \c NO.
 * 
 * @note sel_isEqual is equivalent to ==.
 */
OBJC_EXPORT BOOL
sel_isEqual(SEL _Nonnull lhs, SEL _Nonnull rhs) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

///-----------------------------------Objective-C Language 特性-------------------------------------

/** 
 * This function is inserted by the compiler when a mutation
 * is detected during a foreach iteration. It gets called 
 * when a mutation occurs, and the enumerationMutationHandler
 * is enacted if it is set up. A fatal error occurs if a handler is not set up.
 *
 * @param obj The object being mutated.
 * 
 */
OBJC_EXPORT void
objc_enumerationMutation(id _Nonnull obj) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Sets the current mutation handler. 
 * 
 * @param handler Function pointer to the new mutation handler.
 */
OBJC_EXPORT void
objc_setEnumerationMutationHandler(void (*_Nullable handler)(id _Nonnull )) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Set the function to be called by objc_msgForward.
 * 设置objc_msgForward调用的方法
 * @param fwd Function to be jumped to by objc_msgForward.
 * @param fwd_stret Function to be jumped to by objc_msgForward_stret.
 * 
 * @see message.h::_objc_msgForward
 */
OBJC_EXPORT void
objc_setForwardHandler(void * _Nonnull fwd, void * _Nonnull fwd_stret) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);

/** 
 * Creates a pointer to a function that will call the block
 * when the method is called.
 *
 * @param block The block that implements this method. Its signature should
 *  be: method_return_type ^(id self, method_args...). 
 *  The selector is not available as a parameter to this block.
 *  The block is copied with \c Block_copy().
 * 
 * @return The IMP that calls this block. Must be disposed of with
 *  \c imp_removeBlock.
 */
OBJC_EXPORT IMP _Nonnull
imp_implementationWithBlock(id _Nonnull block)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Return the block associated with an IMP that was created using
 * \c imp_implementationWithBlock.
 * 
 * @param anImp The IMP that calls this block.
 * 
 * @return The block called by \e anImp.
 */
OBJC_EXPORT id _Nullable
imp_getBlock(IMP _Nonnull anImp)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * Disassociates a block from an IMP that was created using
 * \c imp_implementationWithBlock and releases the copy of the 
 * block that was created.
 * 
 * @param anImp An IMP that was created using \c imp_implementationWithBlock.
 * 
 * @return YES if the block was released successfully, NO otherwise. 
 *  (For example, the block might not have been used to create an IMP previously).
 */
OBJC_EXPORT BOOL
imp_removeBlock(IMP _Nonnull anImp)
    OBJC_AVAILABLE(10.7, 4.3, 9.0, 1.0, 2.0);

/** 
 * This loads the object referenced by a weak pointer and returns it, after
 * retaining and autoreleasing the object to ensure that it stays alive
 * long enough for the caller to use it. This function would be used
 * anywhere a __weak variable is used in an expression.
 * 
 * @param location The weak pointer address
 * 
 * @return The object pointed to by \e location, or \c nil if \e *location is \c nil.
 */
OBJC_EXPORT id _Nullable
objc_loadWeak(id _Nullable * _Nonnull location)
    OBJC_AVAILABLE(10.7, 5.0, 9.0, 1.0, 2.0);

/** 
 * This function stores a new value into a __weak variable. It would
 * be used anywhere a __weak variable is the target of an assignment.
 * 
 * @param location The address of the weak pointer itself
 * @param obj The new object this weak ptr should now point to
 * 
 * @return The value stored into \e location, i.e. \e obj
 */
OBJC_EXPORT id _Nullable
objc_storeWeak(id _Nullable * _Nonnull location, id _Nullable obj) 
    OBJC_AVAILABLE(10.7, 5.0, 9.0, 1.0, 2.0);


///-----------------------------------引用关联 特性-------------------------------------

/**
 * Policies related to associative references.
 * These are options to objc_setAssociatedObject()
 */
typedef OBJC_ENUM(uintptr_t, objc_AssociationPolicy) {
    OBJC_ASSOCIATION_ASSIGN = 0,           /**< Specifies a weak reference to the associated object. */
    OBJC_ASSOCIATION_RETAIN_NONATOMIC = 1, /**< Specifies a strong reference to the associated object. 
                                            *   The association is not made atomically. */
    OBJC_ASSOCIATION_COPY_NONATOMIC = 3,   /**< Specifies that the associated object is copied. 
                                            *   The association is not made atomically. */
    OBJC_ASSOCIATION_RETAIN = 01401,       /**< Specifies a strong reference to the associated object.
                                            *   The association is made atomically. */
    OBJC_ASSOCIATION_COPY = 01403          /**< Specifies that the associated object is copied.
                                            *   The association is made atomically. */
};

/** 
 * Sets an associated value for a given object using a given key and association policy.
 * 为某个对象设置关联值
 * @param object The source object for the association.
 * @param key The key for the association.
 * @param value The value to associate with the key key for object. Pass nil to clear an existing association.
 * @param policy The policy for the association. For possible values, see “Associative Object Behaviors.”
 * 
 * @see objc_setAssociatedObject
 * @see objc_removeAssociatedObjects
 */
OBJC_EXPORT void
objc_setAssociatedObject(id _Nonnull object, const void * _Nonnull key,
                         id _Nullable value, objc_AssociationPolicy policy)
    OBJC_AVAILABLE(10.6, 3.1, 9.0, 1.0, 2.0);

/** 
 * Returns the value associated with a given object for a given key.
 * 获取某个对象的关联值
 * @param object The source object for the association.
 * @param key The key for the association.
 * 
 * @return The value associated with the key \e key for \e object.
 * 
 * @see objc_setAssociatedObject
 */
OBJC_EXPORT id _Nullable
objc_getAssociatedObject(id _Nonnull object, const void * _Nonnull key)
    OBJC_AVAILABLE(10.6, 3.1, 9.0, 1.0, 2.0);

/** 
 * Removes all associations for a given object.
 * 删除某个对象的所有关联
 * @param object An object that maintains associated objects.
 * 
 * @note The main purpose of this function is to make it easy to return an object 
 *  to a "pristine state”. You should not use this function for general removal of
 *  associations from objects, since it also removes associations that other clients
 *  may have added to the object. Typically you should use \c objc_setAssociatedObject 
 *  with a nil value to clear an association.
 * 
 * @see objc_setAssociatedObject
 * @see objc_getAssociatedObject
 */
OBJC_EXPORT void
objc_removeAssociatedObjects(id _Nonnull object)
    OBJC_AVAILABLE(10.6, 3.1, 9.0, 1.0, 2.0);


/* Hooks for Swift */

/**
 * Function type for a hook that intercepts class_getImageName().
 *
 * @param cls The class whose image name is being looked up.
 * @param outImageName On return, the result of the image name lookup.
 * @return YES if an image name for this class was found, NO otherwise.
 *
 * @see class_getImageName
 * @see objc_setHook_getImageName
 */
typedef BOOL (*objc_hook_getImageName)(Class _Nonnull cls, const char * _Nullable * _Nonnull outImageName);

/**
 * Install a hook for class_getImageName().
 *
 * @param newValue The hook function to install.
 * @param outOldValue The address of a function pointer variable. On return,
 *  the old hook function is stored in the variable.
 *
 * @note The store to *outOldValue is thread-safe: the variable will be
 *  updated before class_getImageName() calls your new hook to read it,
 *  even if your new hook is called from another thread before this
 *  setter completes.
 * @note The first hook in the chain is the native implementation of
 *  class_getImageName(). Your hook should call the previous hook for
 *  classes that you do not recognize.
 *
 * @see class_getImageName
 * @see objc_hook_getImageName
 */
OBJC_EXPORT void objc_setHook_getImageName(objc_hook_getImageName _Nonnull newValue,
                                           objc_hook_getImageName _Nullable * _Nonnull outOldValue)
    OBJC_AVAILABLE(10.14, 12.0, 12.0, 5.0, 3.0);


#define _C_ID       '@'
#define _C_CLASS    '#'
#define _C_SEL      ':'
#define _C_CHR      'c'
#define _C_UCHR     'C'
#define _C_SHT      's'
#define _C_USHT     'S'
#define _C_INT      'i'
#define _C_UINT     'I'
#define _C_LNG      'l'
#define _C_ULNG     'L'
#define _C_LNG_LNG  'q'
#define _C_ULNG_LNG 'Q'
#define _C_FLT      'f'
#define _C_DBL      'd'
#define _C_BFLD     'b'
#define _C_BOOL     'B'
#define _C_VOID     'v'
#define _C_UNDEF    '?'
#define _C_PTR      '^'
#define _C_CHARPTR  '*'
#define _C_ATOM     '%'
#define _C_ARY_B    '['
#define _C_ARY_E    ']'
#define _C_UNION_B  '('
#define _C_UNION_E  ')'
#define _C_STRUCT_B '{'
#define _C_STRUCT_E '}'
#define _C_VECTOR   '!'
#define _C_CONST    'r'


/* 过时的方法及类型 */
#if !__OBJC2__

#define CLS_GETINFO(cls,infomask)        ((cls)->info & (infomask))
#define CLS_SETINFO(cls,infomask)        ((cls)->info |= (infomask))

// class is not a metaclass
#define CLS_CLASS               0x1
// class is a metaclass
#define CLS_META                0x2
// class's +initialize method has completed
#define CLS_INITIALIZED         0x4
// class is posing
#define CLS_POSING              0x8
// unused
#define CLS_MAPPED              0x10
// class and subclasses need cache flush during image loading
#define CLS_FLUSH_CACHE         0x20
// method cache should grow when full
#define CLS_GROW_CACHE          0x40
// unused
#define CLS_NEED_BIND           0x80
// methodLists is array of method lists
#define CLS_METHOD_ARRAY        0x100
// the JavaBridge constructs classes with these markers
#define CLS_JAVA_HYBRID         0x200
#define CLS_JAVA_CLASS          0x400
// thread-safe +initialize
#define CLS_INITIALIZING        0x800
// bundle unloading
#define CLS_FROM_BUNDLE         0x1000
// C++ ivar support
#define CLS_HAS_CXX_STRUCTORS   0x2000
// Lazy method list arrays
#define CLS_NO_METHOD_ARRAY     0x4000
// +load implementation
#define CLS_HAS_LOAD_METHOD     0x8000
// objc_allocateClassPair API
#define CLS_CONSTRUCTING        0x10000
// class compiled with bigger class structure
#define CLS_EXT                 0x20000


struct objc_method_description_list {
    int count;
    struct objc_method_description list[1];
};


//Catogires
struct objc_category {
    char * _Nonnull category_name                            OBJC2_UNAVAILABLE;
    char * _Nonnull class_name                               OBJC2_UNAVAILABLE;
    struct objc_method_list * _Nullable instance_methods     OBJC2_UNAVAILABLE;
    struct objc_method_list * _Nullable class_methods        OBJC2_UNAVAILABLE;
    struct objc_protocol_list * _Nullable protocols          OBJC2_UNAVAILABLE;
}                                                            OBJC2_UNAVAILABLE;


typedef struct objc_symtab *Symtab                           OBJC2_UNAVAILABLE;

struct objc_symtab {
    unsigned long sel_ref_cnt                                OBJC2_UNAVAILABLE;
    SEL _Nonnull * _Nullable refs                            OBJC2_UNAVAILABLE;
    unsigned short cls_def_cnt                               OBJC2_UNAVAILABLE;
    unsigned short cat_def_cnt                               OBJC2_UNAVAILABLE;
    void * _Nullable defs[1] /* variable size */             OBJC2_UNAVAILABLE;
}                                                            OBJC2_UNAVAILABLE;


typedef struct objc_cache *Cache                             OBJC2_UNAVAILABLE;

#define CACHE_BUCKET_NAME(B)  ((B)->method_name)
#define CACHE_BUCKET_IMP(B)   ((B)->method_imp)
#define CACHE_BUCKET_VALID(B) (B)
#ifndef __LP64__
#define CACHE_HASH(sel, mask) (((uintptr_t)(sel)>>2) & (mask))
#else
#define CACHE_HASH(sel, mask) (((unsigned int)((uintptr_t)(sel)>>3)) & (mask))
#endif

typedef struct objc_module *Module                           OBJC2_UNAVAILABLE;

struct objc_module {
    unsigned long version                                    OBJC2_UNAVAILABLE;
    unsigned long size                                       OBJC2_UNAVAILABLE;
    const char * _Nullable name                              OBJC2_UNAVAILABLE;
    Symtab _Nullable symtab                                  OBJC2_UNAVAILABLE;
}                                                            OBJC2_UNAVAILABLE;

#else

struct objc_method_list;

#endif


/* Obsolete functions */

OBJC_EXPORT IMP _Nullable
class_lookupMethod(Class _Nullable cls, SEL _Nonnull sel) 
    __OSX_DEPRECATED(10.0, 10.5, "use class_getMethodImplementation instead") 
    __IOS_DEPRECATED(2.0, 2.0, "use class_getMethodImplementation instead") 
    __TVOS_DEPRECATED(9.0, 9.0, "use class_getMethodImplementation instead") 
    __WATCHOS_DEPRECATED(1.0, 1.0, "use class_getMethodImplementation instead")
    __BRIDGEOS_DEPRECATED(2.0, 2.0, "use class_getMethodImplementation instead");
OBJC_EXPORT BOOL
class_respondsToMethod(Class _Nullable cls, SEL _Nonnull sel)
    __OSX_DEPRECATED(10.0, 10.5, "use class_respondsToSelector instead") 
    __IOS_DEPRECATED(2.0, 2.0, "use class_respondsToSelector instead") 
    __TVOS_DEPRECATED(9.0, 9.0, "use class_respondsToSelector instead") 
    __WATCHOS_DEPRECATED(1.0, 1.0, "use class_respondsToSelector instead")
    __BRIDGEOS_DEPRECATED(2.0, 2.0, "use class_respondsToSelector instead");

OBJC_EXPORT void
_objc_flush_caches(Class _Nullable cls) 
    __OSX_DEPRECATED(10.0, 10.5, "not recommended") 
    __IOS_DEPRECATED(2.0, 2.0, "not recommended") 
    __TVOS_DEPRECATED(9.0, 9.0, "not recommended") 
    __WATCHOS_DEPRECATED(1.0, 1.0, "not recommended")
    __BRIDGEOS_DEPRECATED(2.0, 2.0, "not recommended");

OBJC_EXPORT id _Nullable
object_copyFromZone(id _Nullable anObject, size_t nBytes, void * _Nullable z) 
    __OSX_DEPRECATED(10.0, 10.5, "use object_copy instead") 
    __IOS_UNAVAILABLE __TVOS_UNAVAILABLE
    __WATCHOS_UNAVAILABLE __BRIDGEOS_UNAVAILABLE
    OBJC_ARC_UNAVAILABLE;

OBJC_EXPORT id _Nullable
object_realloc(id _Nullable anObject, size_t nBytes)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT id _Nullable
object_reallocFromZone(id _Nullable anObject, size_t nBytes, void * _Nullable z)
    OBJC2_UNAVAILABLE;

#define OBSOLETE_OBJC_GETCLASSES 1
OBJC_EXPORT void * _Nonnull
objc_getClasses(void)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT void
objc_addClass(Class _Nonnull myClass)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT void
objc_setClassHandler(int (* _Nullable )(const char * _Nonnull))
    OBJC2_UNAVAILABLE;

OBJC_EXPORT void
objc_setMultithreaded(BOOL flag)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT id _Nullable
class_createInstanceFromZone(Class _Nullable, size_t idxIvars,
                             void * _Nullable z)
    __OSX_DEPRECATED(10.0, 10.5, "use class_createInstance instead") 
    __IOS_UNAVAILABLE __TVOS_UNAVAILABLE
    __WATCHOS_UNAVAILABLE __BRIDGEOS_UNAVAILABLE
    OBJC_ARC_UNAVAILABLE;

OBJC_EXPORT void
class_addMethods(Class _Nullable, struct objc_method_list * _Nonnull)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT void
class_removeMethods(Class _Nullable, struct objc_method_list * _Nonnull)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT void
_objc_resolve_categories_for_class(Class _Nonnull cls)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT Class _Nonnull
class_poseAs(Class _Nonnull imposter, Class _Nonnull original)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT unsigned int
method_getSizeOfArguments(Method _Nonnull m)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT unsigned
method_getArgumentInfo(struct objc_method * _Nonnull m, int arg,
                       const char * _Nullable * _Nonnull type,
                       int * _Nonnull offset)
    UNAVAILABLE_ATTRIBUTE  // This function was accidentally deleted in 10.9.
    OBJC2_UNAVAILABLE;

OBJC_EXPORT Class _Nullable
objc_getOrigClass(const char * _Nonnull name)
    OBJC2_UNAVAILABLE;

#define OBJC_NEXT_METHOD_LIST 1
OBJC_EXPORT struct objc_method_list * _Nullable
class_nextMethodList(Class _Nullable, void * _Nullable * _Nullable)
    OBJC2_UNAVAILABLE;
// usage for nextMethodList
//
// void *iterator = 0;
// struct objc_method_list *mlist;
// while ( mlist = class_nextMethodList( cls, &iterator ) )
//    ;
 
OBJC_EXPORT id _Nullable
(* _Nonnull _alloc)(Class _Nullable, size_t)
    OBJC2_UNAVAILABLE;

OBJC_EXPORT id _Nullable
(* _Nonnull _copy)(id _Nullable, size_t)
     OBJC2_UNAVAILABLE;
     
OBJC_EXPORT id _Nullable
(* _Nonnull _realloc)(id _Nullable, size_t)
     OBJC2_UNAVAILABLE;

OBJC_EXPORT id _Nullable
(* _Nonnull _dealloc)(id _Nullable)
     OBJC2_UNAVAILABLE;
     
OBJC_EXPORT id _Nullable
(* _Nonnull _zoneAlloc)(Class _Nullable, size_t, void * _Nullable)
     OBJC2_UNAVAILABLE;
     
OBJC_EXPORT id _Nullable
(* _Nonnull _zoneRealloc)(id _Nullable, size_t, void * _Nullable)
     OBJC2_UNAVAILABLE;
     
OBJC_EXPORT id _Nullable
(* _Nonnull _zoneCopy)(id _Nullable, size_t, void * _Nullable)
     OBJC2_UNAVAILABLE;
     
OBJC_EXPORT void
(* _Nonnull _error)(id _Nullable, const char * _Nonnull, va_list)
     OBJC2_UNAVAILABLE;

#endif
