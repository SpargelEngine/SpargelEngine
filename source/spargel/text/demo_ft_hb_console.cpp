#include <harfbuzz/hb.h>

#include "spargel/base/string.h"
#include "spargel/logging/logging.h"
#include "spargel/math/function.h"
#include "spargel/resource/directory.h"

// libc
#include <stdlib.h>

// STD
#include <string>

// FreeType 2
#include <ft2build.h>
#include FT_FREETYPE_H

// HarfBuzz
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>

// pixels per point
const float XPPP = 5.0f;
const float YPPP = 2.5f;
const auto hb_direction = HB_DIRECTION_LTR;
const auto hb_script = HB_SCRIPT_LATIN;
const auto hb_language_code = "en";

using namespace spargel;
using namespace spargel::base::literals;

namespace {

    const u32 points_per_inch = 72;

    template <typename T>
    inline T max(T a, T b) {
        return a > b ? a : b;
    }
    template <typename T>
    inline T min(T a, T b) {
        return a < b ? a : b;
    }

    base::String getName(const FT_Face& face) {
        char *family_name = face->family_name, *style_name = face->style_name;
        base::String name(family_name ? family_name : "<unknown>");
        if (style_name) {
            name = name + " (" + style_name + ')';
        }
        return name;
    }

    void dump_bitmap(u8* pixels, int pitch, u32 width, u32 height,
                     u8* foreground = nullptr) {
        spargel_check(pitch != 0 && width >= 1 && height >= 1);
        if (pitch > 0) {
            // stored from UP to BOTTOM
            for (usize r = 0; r < height; r++) {
                int last_code = 0;
                for (usize c = 0; c < width; c++) {
                    u8 data = pixels[r * pitch + c];
                    int code = data * (255 - 232) / 256 + 232;
                    if (code != last_code) {
                        printf("\033[38;5;196;48;5;%dm", code);
                        last_code = code;
                    }

                    if (foreground) {
                        putchar(foreground[r * pitch + c]);
                    } else {
                        putchar(' ');
                    }
                }
                printf("\033[0m");
                putchar('\n');
            }
        } else {
            // TODO
            spargel_panic_here();
        }
    }

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        spargel_log_fatal("Usage: %s <font_file> <size> <text>", argv[0]);
        return 1;
    }

    auto font_fileanme = base::String(argv[1]);
    auto size = std::stof(argv[2]);
    auto text = base::String(argv[3]);

    auto manager = resource::ResourceManagerDirectory(""_sv);
    auto font_file = manager.open(resource::ResourceId(font_fileanme));
    if (!font_file.hasValue()) {
        spargel_log_fatal("Cannot open resource \"%s\".",
                          base::CString(font_fileanme).data());
        return 1;
    }
    auto font_file_data = font_file.value()->mapData();
    auto font_file_length = font_file.value()->size();

    // init FreeType
    FT_Library ft_library;
    FT_Init_FreeType(&ft_library);

    FT_Face ft_face;
    {
        auto error = FT_New_Memory_Face(
            ft_library, reinterpret_cast<FT_Byte*>(font_file_data),
            font_file_length, 0, &ft_face);
        if (error == FT_Err_Unknown_File_Format) {
            spargel_log_fatal(
                "Error calling FT_New_Memory_Face: Unsupported file format.");
            spargel_panic_here();
        } else if (error) {
            spargel_log_fatal("Error calling FT_New_Memory_Face.");
            spargel_panic_here();
        }
    }

    spargel_log_info("Selected font: %s, size=%.fpt.",
                     base::CString(getName(ft_face)).data(), size);

    u16 units_per_EM = ft_face->units_per_EM;
    spargel_log_info("Font Global Metrics: units_per_EM=%d.", units_per_EM);

#define UNIT2PT(n) ((float)((n) * size) / units_per_EM)

    // init HarfBuzz
    hb_buffer_t* hb_buffer = hb_buffer_create();
    hb_buffer_set_direction(hb_buffer, hb_direction);
    hb_buffer_set_script(hb_buffer, hb_script);
    hb_buffer_set_language(hb_buffer,
                           hb_language_from_string(hb_language_code, -1));

    hb_blob_t* hb_blob = hb_blob_create(
        reinterpret_cast<char*>(font_file_data), font_file_length,
        HB_MEMORY_MODE_READONLY, nullptr, nullptr);
    if (!hb_blob) {
        spargel_log_fatal("Error calling hb_blob_create.");
        spargel_panic_here();
    }

    hb_face_t* hb_face = hb_face_create(hb_blob, 0);
    hb_font_t* hb_font = hb_font_create(hb_face);

    // text shaping
    hb_buffer_add_utf8(hb_buffer, text.data(), text.length(), 0, text.length());

    hb_shape(hb_font, hb_buffer, nullptr, 0);

    u32 glyph_count;
    hb_glyph_info_t* glyph_infos =
        hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
    hb_glyph_position_t* glyph_positions =
        hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

    float x_min = 0, x_max = 0, y_min = 0, y_max = 0;
    float cursor_x = 0, cursor_y = 0;
    base::vector<FT_Glyph_Metrics> glyph_metrics;
    spargel_log_info("Shaping Result:");
    for (u32 i = 0; i < glyph_count; i++) {
        auto glyph_id = glyph_infos[i].codepoint;
        auto& pos = glyph_positions[i];

        spargel_log_info("  [%d]:", i);
        spargel_log_info("    glyph_id=%d, offset=(%d, %d), advance=(%d, %d)",
                         glyph_id, pos.x_offset, pos.y_offset, pos.x_advance,
                         pos.y_advance);

        // get glyph info
        if (FT_Load_Glyph(ft_face, glyph_id, FT_LOAD_NO_SCALE) != 0) {
            spargel_log_fatal("FT_Load_Glyph: Unable to load glyph id=%d.",
                              glyph_id);
            spargel_panic_here();
        }
        auto& metrics = ft_face->glyph->metrics;
        spargel_log_info(
            "    glyph info: bearingX=%ld, bearingY=%ld, width=%ld, "
            "height=%ld, advance=%ld",
            metrics.horiBearingX, metrics.horiBearingY, metrics.width,
            metrics.height, metrics.horiAdvance);
        glyph_metrics.push(metrics);

        x_min =
            min(x_min, cursor_x + UNIT2PT(pos.x_offset + metrics.horiBearingX));
        x_max = max(x_max,
                    cursor_x + max(UNIT2PT(pos.x_offset + metrics.horiBearingX +
                                           metrics.width),
                                   UNIT2PT(pos.x_advance)));
        y_min =
            min(y_min, cursor_y + UNIT2PT(pos.y_offset + metrics.horiBearingY -
                                          metrics.height));
        y_max =
            max(y_max, cursor_y + UNIT2PT(pos.y_offset + metrics.horiBearingY));

        cursor_x += UNIT2PT(pos.x_advance);
        cursor_y += UNIT2PT(pos.y_advance);
    }
    spargel_log_info("x_min=%.3f, x_max=%.3f, y_min=%.3f, y_max=%.3f.", x_min,
                     x_max, y_min, y_max);

    // render glyphs
    u32 pixel_width = (u32)ceil((x_max - x_min) * XPPP) + 4,
        pixel_height = (u32)ceil((y_max - y_min) * YPPP) + 5;
    u32 pixel_baseline = (u32)round(-y_min * YPPP) + 2;
    spargel_log_info(
        "computed: pixel_width=%d, pixel_height=%d, pixel_baseline=%d.",
        pixel_width, pixel_height, pixel_baseline);

    u32 pixel_count = pixel_height * pixel_width;
    u8* pixels = reinterpret_cast<u8*>(malloc(pixel_count));
    memset(pixels, 0, pixel_count);
    u8* foreground = reinterpret_cast<u8*>(malloc(pixel_count));
    memset(foreground, ' ', pixel_count);

    if (FT_Set_Char_Size(ft_face, (u32)math::round(size * 64),
                         (u32)math::round(size * 64),
                         (u32)math::round(XPPP * points_per_inch),
                         (u32)math::round(YPPP * points_per_inch)) != 0) {
        spargel_log_fatal("Error calling FT_Set_Char_Size.");
        spargel_panic_here();
    }

    spargel_log_info("Render Result:");
    cursor_x = 0;
    cursor_y = 0;
    for (u32 i = 0; i < glyph_count; i++) {
        auto glyph_id = glyph_infos[i].codepoint;
        auto& pos = glyph_positions[i];

        if (FT_Load_Glyph(ft_face, glyph_id, FT_LOAD_DEFAULT) != 0) {
            spargel_log_fatal("FT_Load_Glyph: Unable to load glyph %d.",
                              glyph_id);
            spargel_panic_here();
        }
        auto& slot = ft_face->glyph;
        if (FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL) != 0) {
            spargel_log_fatal("FT_Render_Glyph: Unable to render glyph id=%d.",
                              glyph_id);
            spargel_panic_here();
        }

        u8* bitmap = slot->bitmap.buffer;
        u32 pitch = slot->bitmap.pitch;
        u32 rows = slot->bitmap.rows;
        u32 width = slot->bitmap.width;

        auto& metrics = glyph_metrics[i];
        i32 x = (i32)math::round(
            (cursor_x + UNIT2PT(pos.x_offset + metrics.horiBearingX) - x_min) *
            XPPP);
        i32 y = (i32)math::round(
                    (cursor_y + UNIT2PT(pos.y_offset + metrics.horiBearingY -
                                        metrics.height)) *
                    YPPP) +
                pixel_baseline;

        spargel_log_info(
            "  [%d]: glyph_id=%d, pitch=%d, rows=%d, width=%d; x=%d, y=%d.", i,
            glyph_id, pitch, rows, width, x, y);

        spargel_check(x >= 0 && x + width < pixel_width);
        spargel_check(y >= 0 && y + rows < pixel_height);

        for (u32 r = 0; r < rows; r++) {
            for (u32 c = 0; c < width; c++) {
                u32 offset = (pixel_height - 1 - y - r) * pixel_width + (x + c);
                u8 data = bitmap[(rows - 1 - r) * pitch + c];
                pixels[offset] = max(pixels[offset], data);
                foreground[offset] = '+';
            }
        }

        cursor_x += UNIT2PT(pos.x_advance);
        cursor_y += UNIT2PT(pos.y_advance);
    }

    // draw baseline
    memset(foreground + (pixel_height - 1 - pixel_baseline) * pixel_width, '=',
           pixel_width);

    // dump to console
    dump_bitmap(pixels, pixel_width, pixel_width, pixel_height, foreground);

    free(pixels);
    free(foreground);

    // cleanup
    hb_buffer_destroy(hb_buffer);
    FT_Done_FreeType(ft_library);

    return 0;
}
