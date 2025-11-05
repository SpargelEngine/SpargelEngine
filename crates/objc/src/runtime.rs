use core::ffi::CStr;
use core::os::darwin::objc::objc_selector;

use ffi_utils::define_opaque_type;

use crate::ffi::sel_getName;

pub mod any_class;

// `core` doesn't have `objc_object`.
define_opaque_type!(AnyObject);

define_opaque_type!(Ivar);
define_opaque_type!(Method);

#[repr(transparent)]
pub struct Selector(objc_selector);

impl Selector {
    pub fn name(&self) -> &CStr {
        let sel = self as *const Selector as *mut Selector;
        let name_ptr = unsafe { sel_getName(sel) };
        unsafe { CStr::from_ptr(name_ptr) }
    }
}
