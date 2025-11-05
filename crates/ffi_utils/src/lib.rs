/// Define an opaque type used for FFI.
#[macro_export]
macro_rules! define_opaque_type {
    ($name:ident) => {
        #[repr(C)]
        pub struct $name {
            _data: (),
            _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
        }
    };
}
