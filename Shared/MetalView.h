//
//  MetalView.h
//  IMBlocks
//
//  Created by Sean Hickey on 6/15/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#import <MetalKit/MetalKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef struct Input {
    union {
        float mouseX;
        float touchX;
    };
    union {
        float mouseY;
        float touchY;
    };
    union {
        bool mouseDown;
        bool touching;
    };
    float wheelDx;
    float wheelDy;
    bool commandDown;
} Input;

@interface MetalView: MTKView
{
    @public Input _input;
}

@end

NS_ASSUME_NONNULL_END
