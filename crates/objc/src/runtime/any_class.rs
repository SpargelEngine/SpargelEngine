use core::ffi::CStr;
use core::fmt;
use core::os::darwin::objc::objc_class;

use crate::ffi::class_getName;
use crate::ffi::class_getSuperclass;
use crate::ffi::class_isMetaClass;
use crate::ffi::objc_getClass;
use crate::ffi::objc_getClassList;
use crate::ffi::object_getClass;
use crate::runtime::AnyObject;

#[repr(transparent)]
pub struct AnyClass(objc_class);

impl AnyClass {
    /// Get the definition of a class by its name.
    /// Returns [`None`] if the class doesn't exist.
    pub fn get(name: &CStr) -> Option<&'static Self> {
        let cls = unsafe { objc_getClass(name.as_ptr()) };
        unsafe { cls.as_ref() }
    }
    /// Returns total number of registered classes.
    pub fn classes_count() -> usize {
        let number = unsafe { objc_getClassList(core::ptr::null_mut(), 0) };
        number as usize
    }
    /// Returns the name of `self`.
    pub fn name(&self) -> &CStr {
        let cls = self as *const AnyClass as *mut AnyClass;
        let name_ptr = unsafe { class_getName(cls) };
        unsafe { CStr::from_ptr(name_ptr) }
    }
    /// Returns the superclass of `self`, or [`None`] if `self` is a root class.
    pub fn superclass(&self) -> Option<&AnyClass> {
        let cls = self as *const AnyClass as *mut AnyClass;
        let super_cls = unsafe { class_getSuperclass(cls) };
        unsafe { super_cls.as_ref() }
    }
    pub fn is_metaclass(&self) -> bool {
        let cls = self as *const AnyClass as *mut AnyClass;
        unsafe { class_isMetaClass(cls) }
    }
    pub fn metaclass(&self) -> &Self {
        let cls = self as *const AnyClass as *mut AnyClass;
        let obj = cls as *mut AnyObject;
        let metacls = unsafe { object_getClass(obj) };
        unsafe { metacls.as_ref().unwrap() }
    }
}

impl fmt::Debug for AnyClass {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("AnyClass")
            .field("name", &self.name())
            .finish_non_exhaustive()
    }
}
