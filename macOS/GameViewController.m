//
//  GameViewController.m
//  IMBlocks macOS
//
//  Created by Sean Hickey on 6/15/20.
//  Copyright © 2020 Lifelong Kindergarten. All rights reserved.
//

#import "GameViewController.h"
#import "Renderer.h"
#import "MetalView.h"

@implementation GameViewController
{
    MetalView *_view;

    Renderer *_renderer;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _view = (MetalView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = [[Renderer alloc] initWithMetalKitView:_view];

    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];

    _view.delegate = _renderer;
}

@end
