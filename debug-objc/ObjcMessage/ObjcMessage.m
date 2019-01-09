//
//  ObjcMessage.m
//  debug-objc
//
//  Created by linxiaohai on 2019/1/8.
//

#import "ObjcMessage.h"
#import "Person.h"

@implementation ObjcMessage

- (void)runDemo {
    [super runDemo];
    Person *person = [Person new];
    [person speakLan:@"linxiaohai"];
}

@end
