//
//  RuntimeFunctionDemo.h
//  debug-objc
//
//  Created by linxiaohai on 2018/12/30.
//

#import <Foundation/Foundation.h>
#import "RuntimeDemoBase.h"

NS_ASSUME_NONNULL_BEGIN

@protocol testProtocal <NSObject>

@optional
- (void)testMethod;

@end

@interface Father : NSObject<testProtocal>

- (void)printName:(NSString *)testParam1 testParam2:(NSString *)testParam2;

@end

@interface Son : Father

@end

@interface RuntimeFunctionDemo : RuntimeDemoBase

@end

NS_ASSUME_NONNULL_END
