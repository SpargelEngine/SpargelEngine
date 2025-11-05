use objc::runtime::any_class::AnyClass;
use objc::string::NSObject;
use objc::string::NSString;

fn main() {
    println!("classes count: {:?}", AnyClass::classes_count());
    println!(
        "NSString <- {:?}, meta: {:?}",
        NSString::class().superclass(),
        NSString::class().metaclass()
    );
    println!(
        "NSObject <- {:?}, meta: {:?}",
        NSObject::class().superclass(),
        NSObject::class().metaclass()
    );
}
