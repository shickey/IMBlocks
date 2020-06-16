//
//  MetalView.m
//  IMBlocks
//
//  Created by Sean Hickey on 6/15/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#import "MetalView.h"

@implementation MetalView

- (void)awakeFromNib {
    [super awakeFromNib];
    NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect |  
                             NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);

    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                        options:options
                                                          owner:self
                                                       userInfo:nil];
    [self addTrackingArea:area];
}

- (void)mouseDown:(NSEvent *)event {
    _input.mouseDown = true;
}

- (void)mouseUp:(NSEvent *)event {
    _input.mouseDown = false;
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint rawPt = [event locationInWindow];
    NSPoint pt = [self convertPoint:rawPt toView:nil];
    _input.mouseX = pt.x;
    _input.mouseY = pt.y;
}

@end
