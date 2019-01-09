//
//  RuntimeDemoBase.h
//  debug-objc
//
//  Created by linxiaohai on 2018/12/30.
//

#import <Foundation/Foundation.h>
#import "RuntimeDemoBaseProtocol.h"
#import <objc/runtime.h>
#import <objc/message.h>

NS_ASSUME_NONNULL_BEGIN

@interface RuntimeDemoBase : NSObject<RuntimeDemoBaseProtocol>

@end

NS_ASSUME_NONNULL_END
