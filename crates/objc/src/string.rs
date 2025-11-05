use crate::macros::class;
use crate::runtime::any_class::AnyClass;

pub struct NSString;

impl NSString {
    pub fn class() -> &'static AnyClass {
        let ptr = class!("NSString");
        let ptr = ptr.cast::<AnyClass>();
        unsafe { &*ptr }
    }
}

pub struct NSObject;

impl NSObject {
    pub fn class() -> &'static AnyClass {
        let ptr = class!("NSObject");
        let ptr = ptr.cast::<AnyClass>();
        unsafe { &*ptr }
    }
}
