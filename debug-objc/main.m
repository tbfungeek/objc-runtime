//
//  main.m
//  debug-objc
//
//  Created by Closure on 2018/12/4.
//

#import <Foundation/Foundation.h>
#import "RuntimeFunctionDemo.h"
#import "RuntimeMetaCls.h"
#import "ObjcMessage.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        RuntimeFunctionDemo *runtimeFuction = [RuntimeFunctionDemo new];
        [runtimeFuction runDemo];
        
        RuntimeMetaCls *runtimeMetaCls = [RuntimeMetaCls new];
        [runtimeMetaCls runDemo];
        
        ObjcMessage *objcMessage = [ObjcMessage new];
        [objcMessage runDemo];
    }
    return 0;
}
