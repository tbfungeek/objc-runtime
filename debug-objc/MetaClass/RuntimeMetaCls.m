//
//  RuntimeMetaCls.m
//  debug-objc
//
//  Created by linxiaohai on 2018/12/30.
//

#import "RuntimeMetaCls.h"

@implementation MetaClass

- (void)instanceMethod {
    NSLog(@"instanceMethod");
}

+ (void)classMethod {
    NSLog(@"classMethod");
}

@end

@implementation RuntimeMetaCls

- (void)runDemo {
    [super runDemo];
    
    //----------------------------  动态创建类  ----------------------------
    //为Class Pair 分配存储空间
    Class newClass = objc_allocateClassPair([NSError class], "RuntimeSubClass", 0);
    //为类添加方法和实例变量
    class_addMethod(newClass, @selector(report), (IMP)reportFunction, "v@:");
    //注册class Paire
    objc_registerClassPair(newClass);
    //通过alloc 初始化类的实例对象
    id instanceOfNewClass = [[newClass alloc] initWithDomain:@"someDomain" code:0 userInfo:nil];
    //通过实例对象调用对应的方法
    [instanceOfNewClass performSelector:@selector(report)];
    
    
    //----------------------------  元类例子  ----------------------------
    unsigned int methodCount = 0;
    Method *methodList = class_copyMethodList([MetaClass class], &methodCount);
    NSLog(@"%u",methodCount);
    for (unsigned int index = 0; index < methodCount; index++) {
        NSLog(@"%s",sel_getName(method_getName(methodList[index])));
    }
    
    Class metaCls = objc_getMetaClass(class_getName([MetaClass class]));
    methodList = class_copyMethodList(metaCls, &methodCount);
    NSLog(@"%u",methodCount);
    for (unsigned int index = 0; index < methodCount; index++) {
        NSLog(@"%s",sel_getName(method_getName(methodList[index])));
    }
    
}

void reportFunction(id self, SEL _cmd) {
    NSLog(@"This object is %p.", self);
    NSLog(@"Class is %@, and super is %@.", [self class], [self superclass]);
    Class currentClass = [self class];
    for (int i = 1; i < 5; i++) {
        NSLog(@"Following the isa pointer %d times gives %p", i, currentClass);
        currentClass = object_getClass(currentClass);
    }

    NSLog(@"NSObject's class is %p", [NSObject class]);
    NSLog(@"NSObject's meta class is %p", object_getClass([NSObject class]));
}

@end
