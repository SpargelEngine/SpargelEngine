#![allow(non_upper_case_globals)]

use core::ffi::c_double;
use core::ffi::c_uchar;
use core::ffi::c_void;

use ffi_utils::define_opaque_type;

#[link(name = "CoreFoundation", kind = "framework")]
unsafe extern "C" {}

// Not CoreFoundation.
// Actually /usr/include/MacTypes.h
pub type Boolean = c_uchar;

unsafe extern "C" {
    pub static kCFCoreFoundationVersionNumber: c_double;
}

pub type CFTypeID = u64;
pub type CFHashCode = u64;
pub type CFIndex = i64;

/// Base "type" of all "CF objects".
pub type CFTypeRef = *const c_void;

define_opaque_type!(__CFString);
pub type CFStringRef = *const __CFString;
pub type CFMutableStringRef = *mut __CFString;

#[repr(C)]
pub struct CFRange {
    location: CFIndex,
    length: CFIndex,
}

define_opaque_type!(__CFAllocator);
pub type CFAllocatorRef = *const __CFAllocator;

unsafe extern "C" {
    pub fn CFGetTypeId(cf: CFTypeRef) -> CFTypeID;
    pub fn CFRetain(cf: CFTypeRef) -> CFTypeRef;
    pub fn CFRelease(cf: CFTypeRef);
    pub fn CFAutorelease(arg: CFTypeRef) -> CFTypeRef;
    pub fn CFGetRetainCount(cf: CFTypeRef) -> CFIndex;
    pub fn CFEqual(cf1: CFTypeRef, cf2: CFTypeRef) -> Boolean;
    pub fn CFHash(cf: CFTypeRef) -> CFHashCode;
}

unsafe extern "C" {
    /// Print the description of any CFType to stderr.
    pub fn CFShow(obj: CFTypeRef) -> c_void;
    /// Print the detailed info about a CFString to stderr.
    pub fn CFShowStr(str: CFStringRef) -> c_void;
}

// CFString.h

pub type CFStringEncoding = u32;

pub const kCFStringEncodingUTF8: CFStringEncoding = 0x08000100;

unsafe extern "C" {
    pub fn CFStringGetTypeID() -> CFTypeID;
    pub fn CFStringCreateWithBytes(
        alloc: CFAllocatorRef,
        bytes: *const u8,
        numBytes: CFIndex,
        encoding: CFStringEncoding,
        isExternalRepresentation: Boolean,
    ) -> CFStringRef;
}
