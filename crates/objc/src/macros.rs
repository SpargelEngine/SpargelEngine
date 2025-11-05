pub use core::os::darwin::objc::class;

#[macro_export]
macro_rules! sel {
    ($s:expr) => {{
        // TODO(tianjiao): use static?
        let sel = ::core::os::darwin::objc::selector!($s);
        let sel = sel.cast::<$crate::runtime::Selector>();
        unsafe { sel.as_ref().unwrap() }
    }};
}
