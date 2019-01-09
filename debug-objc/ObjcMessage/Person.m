//
//  Person.m
//  debug-objc
//
//  Created by linxiaohai on 2019/1/8.
//

#import "Person.h"
#import <objc/runtime.h>

@implementation Person

void speakWithLanguage(id self,SEL cmd,NSString * language) {
    NSLog(@"speakWithLanguage %@",language);
}


+ (BOOL)resolveInstanceMethod:(SEL)sel {
    if(sel == @selector(speakLan:)) {
        class_addMethod(self, sel, (IMP)speakWithLanguage, "v@:@");
        return YES;
    }
    return [super resolveInstanceMethod:sel];
}


@end
