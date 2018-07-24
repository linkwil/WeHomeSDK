#import <Foundation/Foundation.h>

@interface NSMutableArray (QueueStack)
-(id)queuePop;
-(void)queuePush:(id)obj;
-(void)queueEmpty;
-(id)stackPop;
-(void)stackPush:(id)obj;
@end
