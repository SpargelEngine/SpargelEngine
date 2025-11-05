use core_foundation::ffi::CFStringGetTypeID;
use core_foundation::ffi::kCFCoreFoundationVersionNumber;

fn main() {
    println!("CF version: {:?}", unsafe {
        kCFCoreFoundationVersionNumber
    });
    println!("CFString: {:?}", unsafe { CFStringGetTypeID() });
}
