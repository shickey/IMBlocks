//
//  MetalView.m
//  IMBlocks
//
//  Created by Sean Hickey on 6/15/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#import "MetalView.h"

@implementation MetalView

#if TARGET_OS_OSX

- (void)awakeFromNib {
    [super awakeFromNib];
    NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect |  
                             NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);

    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                        options:options
                                                          owner:self
                                                       userInfo:nil];
    [self addTrackingArea:area];
    
    NSEventMask eventMask = NSEventMaskFlagsChanged;
    [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^NSEvent * _Nullable(NSEvent *event) {
        if (event.type == NSEventTypeFlagsChanged) {
            self->_input.commandDown = event.modifierFlags & NSEventModifierFlagCommand ? 1 : 0;
        }
        return event;
    }];
}

- (void)mouseDown:(NSEvent *)event {
    _input.mouseDown = true;
    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event {
    _input.mouseDown = false;
    [super mouseUp:event];
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint rawPt = [event locationInWindow];
    NSPoint pt = [self convertPoint:rawPt toView:nil];
    _input.mouseX = pt.x;
    _input.mouseY = pt.y;
    [super mouseMoved:event];
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint rawPt = [event locationInWindow];
    NSPoint pt = [self convertPoint:rawPt toView:nil];
    _input.mouseX = pt.x;
    _input.mouseY = pt.y;
    [super mouseDragged:event];
}

- (void)scrollWheel:(NSEvent *)event {
    _input.wheelDx = event.scrollingDeltaX;
    _input.wheelDy = event.scrollingDeltaY;
    [super scrollWheel:event];
}


#endif
#if TARGET_OS_IPHONE

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event touchesForView:self] anyObject];
    CGPoint location = [touch locationInView:self];
    _input.touchX = location.x;
    _input.touchY = location.y;
    _input.touching = true;
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event touchesForView:self] anyObject];
    CGPoint location = [touch locationInView:self];
    _input.touchX = location.x;
    _input.touchY = location.y;
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    _input.touching = false;
}

#endif

@end
