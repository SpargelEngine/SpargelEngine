# Spargel Text Stack

## Preliminaries

- *typeface*: an overall design of characters.
  Example: Helvetica is a typeface well-known for its clean, sans-serif design.
- *font*: a file

## Apple SPI

The following are private APIs on Apple Platforms.

```c
CTFontDescriptorRef
    CTFontDescriptorCreateForCSSFamily(CFStringRef cssFamily, CFStringRef language);
```

Default fonts:
- `monospace`: `Menlo-Regular`
- `sans-serif` (en): `Helvetica`
- `serif` (en): `.TimesFB`
