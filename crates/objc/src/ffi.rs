//! We follow Swift's binding to the Objective-C runtime.

use core::ffi::c_char;
use core::ffi::c_int;
use core::ptr::NonNull;

use crate::runtime::AnyObject;
use crate::runtime::Ivar;
use crate::runtime::Selector;
use crate::runtime::any_class::AnyClass;

#[link(name = "Foundation", kind = "framework")]
unsafe extern "C" {}

unsafe extern "C" {
    // Obtaining Class
    // ===============
    pub fn objc_getClass(name: *const c_char) -> *mut AnyClass;
    pub fn objc_getClassList(buffer: *mut *const AnyClass, bufferCount: c_int) -> c_int;

    // Adding Classes
    // ==============
    pub fn objc_allocateClassPair(
        superclass: *mut AnyClass,
        name: *const c_char,
        extraBytes: usize,
    ) -> *mut AnyClass;
    pub fn objc_registerClassPair(cls: NonNull<AnyClass>);

    // Working with Classes
    // ====================
    pub fn class_getName(cls: *mut AnyClass) -> *const c_char;
    pub fn class_getSuperclass(cls: *mut AnyClass) -> *mut AnyClass;
    pub fn class_isMetaClass(cls: *mut AnyClass) -> bool;

    // Instantiating Classes
    // =====================
    pub fn class_createInstance(cls: *mut AnyClass, extraBytes: usize) -> *mut AnyObject;

    // Working with Instances
    // ======================
    pub fn object_getClass(obj: *mut AnyObject) -> *mut AnyClass;
    pub fn object_getClassName(obj: *mut AnyObject) -> *const c_char;

    // Working with Instance Variables
    // ===============================
    pub fn ivar_getName(v: *mut Ivar) -> *const c_char;
    pub fn ivar_getTypeEncoding(v: *mut Ivar) -> *const c_char;
    pub fn ivar_getOffset(v: *mut Ivar) -> isize;

    // Working with Selectors
    // ======================
    pub fn sel_getName(sel: *mut Selector) -> *const c_char;
}

#[cfg(test)]
mod tests {
    use std::ffi::CStr;

    use crate::ffi::class_getName;
    use crate::ffi::objc_getClass;
    use crate::sel;
    use crate::string::NSString;

    #[test]
    fn class_name() {
        let cls = unsafe { objc_getClass(c"NSString".as_ptr()) };
        let name = unsafe { CStr::from_ptr(class_getName(cls)) };
        assert_eq!(name, c"NSString");
    }

    #[test]
    fn static_class() {
        let cls = NSString::class();
        let name = cls.name();
        assert_eq!(name, c"NSString");
    }

    #[test]
    fn static_selector() {
        let sel = sel!("terminate:");
        assert_eq!(sel.name(), c"terminate:");
        let sel2 = sel!("alloc");
        assert_eq!(sel2.name(), c"alloc");
    }
}
