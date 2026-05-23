#import <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

extern "C" void* createMetalLayer(void* glfwWindow, void* mtlDevice) {
    NSWindow* nsWindow = glfwGetCocoaWindow((GLFWwindow*)glfwWindow);
    NSView* view = nsWindow.contentView;
    view.wantsLayer = YES;

    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = (__bridge id<MTLDevice>)mtlDevice;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    layer.framebufferOnly = YES;
    view.layer = layer;

    return (__bridge void*)layer;
}