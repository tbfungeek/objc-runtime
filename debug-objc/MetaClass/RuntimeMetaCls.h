//
//  RuntimeMetaCls.h
//  debug-objc
//
//  Created by linxiaohai on 2018/12/30.
//

#import "RuntimeDemoBase.h"

NS_ASSUME_NONNULL_BEGIN

@interface MetaClass : NSObject

- (void)instanceMethod;

+ (void)classMethod;

@end

@interface RuntimeMetaCls : RuntimeDemoBase

@end

NS_ASSUME_NONNULL_END
