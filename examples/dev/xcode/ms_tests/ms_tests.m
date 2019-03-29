//
//  ms_tests.m
//  ms_tests
//
//  Created by Jianguo Wu on 2019/2/11.
//  Copyright © 2019 libms. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "run_tests.h"

@interface ms_tests : XCTestCase

@end

@implementation ms_tests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)testExample {
  char *path = "/Users/wujianguo/Documents/wujianguo/githublibms/";
  chdir(path);
  test_setup();
  run_sync_tests(path);
  run_async_tests(path);
  test_tear_down();
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
