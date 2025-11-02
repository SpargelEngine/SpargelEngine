import CoreText

@main
struct ListFontApp {
    static func main() {
        let font_collec = CTFontCollectionCreateFromAvailableFonts(nil)
        let font_descs = CTFontCollectionCreateMatchingFontDescriptors(font_collec)!

        for font_desc in font_descs as! [CTFontDescriptor] {
            let font = CTFontCreateWithFontDescriptor(font_desc, 12.0, nil)
            let symbolic_traits = CTFontGetSymbolicTraits(font)
            let name = CTFontCopyFullName(font) as String
            if symbolic_traits.contains(.traitItalic) {
                print("\(name)")
                print("    => italic")
                let traits = CTFontCopyTraits(font) as! [CFString: Any]
                let font_width = traits[kCTFontWidthTrait] as! CFNumber
                print("    => font-width: \(font_width)")
            }
        }
    }
}
