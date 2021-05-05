//
//  SecondsFormatterTests.m
//  SecondsFormatterTests
//
//  Created by Jan on 05.05.21.
//

#import <XCTest/XCTest.h>

#import "SecondsFormatter.h"

@interface SecondsFormatterTests : XCTestCase

@end

@implementation SecondsFormatterTests

- (void)setUp
{
	[super setUp];
	// Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown
{
	// Put teardown code here. This method is called after the invocation of each test method in the class.
	[super tearDown];
}

- (void)testPositive
{
	NSDictionary *testsDict =
	@{
		// key: test name, value: test string
		//@"Example": @"0:00",
		@"One Second": @"0:01",
		@"One Minute": @"1:00",
		@"One Hour": @"1:00:00",
		@"One Day": @"1:00:00:00",
		@"One of Each": @"1:01:01:01",
	};
	
#define TEST_INFO	@"Test name: %@, Source string: %@", testName, string
	
	NSFormatter *secondsFormatter = [[SecondsFormatter alloc] init];

	[testsDict enumerateKeysAndObjectsUsingBlock:
	 ^(NSString *testName, NSString *string, BOOL * _Nonnull stop) {
		NSNumber *value;
		BOOL result =
		[secondsFormatter getObjectValue:&value
							   forString:string
						errorDescription:NULL];
		XCTAssertTrue(result, TEST_INFO);
		NSString *timeString = [secondsFormatter stringForObjectValue:value];
		XCTAssertEqualObjects(string, timeString, TEST_INFO);
	}];
}

- (void)testNegative
{
	NSDictionary *testsDict =
	@{
		// key: test name, value: test string
		@"Negative One Second": @"-0:01",
		@"Negative One Minute": @"-1:00",
		@"Negative One Hour": @"-1:00:00",
		@"Negative One Day": @"-1:00:00:00",
		@"Negative One of Each": @"-1:01:01:01",
	};
	
#define TEST_INFO	@"Test name: %@, Source string: %@", testName, string
	
	NSFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
	
	[testsDict enumerateKeysAndObjectsUsingBlock:
	 ^(NSString *testName, NSString *string, BOOL * _Nonnull stop) {
		NSNumber *value;
		BOOL result =
		[secondsFormatter getObjectValue:&value
							   forString:string
						errorDescription:NULL];
		XCTAssertTrue(result, TEST_INFO);
		NSString *timeString = [secondsFormatter stringForObjectValue:value];
		XCTAssertEqualObjects(string, timeString, TEST_INFO);
	}];
}

- (void)testMalformed
{
	NSDictionary *testsDict =
	@{
		// key: test name, value: test string
		@"Empty String": @"",
		@"Random String": @"abc",
		@"Solitary Minus": @"-",
		@"Malformed Seconds": @"0:60",
		@"Malformed Minutes": @"60:00",
		@"Malformed Hours": @"24:00:00",
		@"Illegal #1": @":00",
		@"Illegal #2": @"-:00",
	};
	
#define TEST_INFO	@"Test name: %@, Source string: %@", testName, string
	
	NSFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
	
	[testsDict enumerateKeysAndObjectsUsingBlock:
	 ^(NSString *testName, NSString *string, BOOL * _Nonnull stop) {
		NSNumber *value;
		BOOL result =
		[secondsFormatter getObjectValue:&value
							   forString:string
						errorDescription:NULL];
		XCTAssertFalse(result, TEST_INFO);
	}];
}


#if 0
- (void)testPerformanceExample
{
	// This is an example of a performance test case.
	[self measureBlock:^{
		// Put the code you want to measure the time of here.
	}];
}
#endif

@end
