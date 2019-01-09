//
//  RuntimeFunctionDemo.m
//  debug-objc
//
//  Created by linxiaohai on 2018/12/30.
//

#import "RuntimeFunctionDemo.h"

@interface Father ()

@property(nonatomic, strong) NSString *name;

@end

@implementation Father

- (void)printName:(NSString *)testParam1 testParam2:(NSString *)testParam2 {
    NSLog(@"%@ %@",testParam1,testParam2);
}

@end

@implementation Son

@end

@implementation RuntimeFunctionDemo

+ (void)initialize {
    NSLog(@"RuntimeFunctionDemo");
}

- (void)runDemo {
    [super runDemo];
    
    Father *father = [Father new];
    Son *son = [Son new];
    
    //打印类名
    NSLog(@"%@",NSStringFromClass([father class]));
    
    //是否是本身类以及父类
    if ([son isKindOfClass:[Father class]]) {
        NSLog(@"son isKindOfClass");
    }
    
    //是否是本身类
    if ([son isMemberOfClass:[Father class]]) {
        NSLog(@"son isMemberOfClass");
    }
    
    //包括父类中是否响应该selector
    if ([father respondsToSelector:@selector(printName:testParam2:)]) {
        NSLog(@"father respondsToSelector printName");
    }
    if ([son respondsToSelector:@selector(printName:testParam2:)]) {
        NSLog(@"son respondsToSelector printName");
    }
    
    //包括父类中是否遵循某个协议
    if ([father conformsToProtocol:@protocol(testProtocal)]) {
        NSLog(@"father conformsToProtocol testProtocal");
    }
    
    if ([son conformsToProtocol:@protocol(testProtocal)]) {
        NSLog(@"son conformsToProtocol testProtocal");
    }
    
    //methodForSelector 用法 为什么不直接调用
    IMP imp = [father methodForSelector:@selector(printName:testParam2:)];
    void (*func)(id,SEL,...) = (void *)imp;
    func(father,@selector(printName:testParam2:),@"testName1",@"testName2");
}

@end
