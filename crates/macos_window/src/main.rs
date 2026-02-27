use std::cell::OnceCell;

use objc2::DefinedClass;
use objc2::MainThreadMarker;
use objc2::MainThreadOnly;
use objc2::define_class;
use objc2::msg_send;
use objc2::rc::Retained;
use objc2::runtime::ProtocolObject;
use objc2_app_kit::NSApp;
use objc2_app_kit::NSApplication;
use objc2_app_kit::NSApplicationActivationPolicy;
use objc2_app_kit::NSApplicationDelegate;
use objc2_app_kit::NSBackingStoreType;
use objc2_app_kit::NSScreen;
use objc2_app_kit::NSWindow;
use objc2_app_kit::NSWindowStyleMask;
use objc2_foundation::NSNotification;
use objc2_foundation::NSObject;
use objc2_foundation::NSObjectProtocol;
use objc2_foundation::NSPoint;
use objc2_foundation::NSRect;
use objc2_foundation::NSSize;
use objc2_foundation::ns_string;

#[derive(Default)]
struct DelegateIvars {
    window: OnceCell<Retained<NSWindow>>,
}

define_class! {
    // SAFETY: ???
    #[unsafe(super = NSObject)]
    #[thread_kind = MainThreadOnly]
    #[ivars = DelegateIvars]
    struct Delegate;

    // SAFETY: ???
    unsafe impl NSObjectProtocol for Delegate {}

    // SAFETY: ???
    unsafe impl NSApplicationDelegate for Delegate {
        // SAFETY: ???
        #[unsafe(method(applicationDidFinishLaunching:))]
        fn did_finish_launching(&self, notification: &NSNotification) {
            println!("[delegate] did finish launching");

            let mtm = self.mtm();

            let app = notification.object()
                .unwrap()
                .downcast::<NSApplication>()
                .unwrap();

            let style = NSWindowStyleMask::Closable
                | NSWindowStyleMask::Miniaturizable
                | NSWindowStyleMask::Resizable
                | NSWindowStyleMask::Titled;
            let screen = NSScreen::mainScreen(mtm).unwrap();

            // SAFETY: The lifetime is managed by Rust side. See below.
            let window = unsafe {
                NSWindow::initWithContentRect_styleMask_backing_defer_screen(
                    NSWindow::alloc(mtm),
                    NSRect::new(NSPoint::new(0.0, 0.0), NSSize::new(500.0, 500.0)),
                    style,
                    NSBackingStoreType::Buffered,
                    false,
                    Some(screen.as_ref()),
                )
            };
            // SAFETY: Disable auto-release when closing windows.
            unsafe { window.setReleasedWhenClosed(false) };

            window.setTitle(ns_string!("Spargel Engine"));

            window.makeKeyAndOrderFront(None);

            self.ivars().window.set(window).unwrap();

            app.activate();
        }

        // SAFETY: ???
        #[unsafe(method(applicationShouldTerminateAfterLastWindowClosed:))]
        fn should_terminate_window_closed(&self, sender: &NSApplication) -> bool {
            true
        }
    }
}

impl Delegate {
    fn new(mtm: MainThreadMarker) -> Retained<Self> {
        let this = Self::alloc(mtm).set_ivars(DelegateIvars::default());
        // SAFETY: ???
        unsafe { msg_send![super(this), init] }
    }
}

fn main() {
    let mtm = MainThreadMarker::new().unwrap();
    let app = NSApp(mtm);
    app.setActivationPolicy(NSApplicationActivationPolicy::Regular);
    let delegate = Delegate::new(mtm);
    app.setDelegate(Some(ProtocolObject::from_ref(&*delegate)));
    println!("[app] set delegate");
    app.run();
}
