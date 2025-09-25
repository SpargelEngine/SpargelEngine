#include "spargel/ui/xkb_util.h"

#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "spargel/logging/logging.h"

namespace spargel::ui::xkb {

    PhysicalKey Context::translatePhysicalKey(xkb_keycode_t keycode) {
        // spargel_log_debug("%d", keycode);

        xkb_keysym_t keysym = xkb_state_key_get_one_sym(state, keycode);

        /*
        char name[64];
        xkb_keysym_get_name(keysym, name, sizeof(name));
        spargel_log_debug("NAME: %s", name);
        */

        switch (keysym) {
        case XKB_KEY_0:
            return PhysicalKey::key_0;
        case XKB_KEY_1:
            return PhysicalKey::key_1;
        case XKB_KEY_2:
            return PhysicalKey::key_2;
        case XKB_KEY_3:
            return PhysicalKey::key_3;
        case XKB_KEY_4:
            return PhysicalKey::key_4;
        case XKB_KEY_5:
            return PhysicalKey::key_5;
        case XKB_KEY_6:
            return PhysicalKey::key_6;
        case XKB_KEY_7:
            return PhysicalKey::key_7;
        case XKB_KEY_8:
            return PhysicalKey::key_8;
        case XKB_KEY_9:
            return PhysicalKey::key_9;
        case XKB_KEY_a:
            return PhysicalKey::key_a;
        case XKB_KEY_b:
            return PhysicalKey::key_b;
        case XKB_KEY_c:
            return PhysicalKey::key_c;
        case XKB_KEY_d:
            return PhysicalKey::key_d;
        case XKB_KEY_e:
            return PhysicalKey::key_e;
        case XKB_KEY_f:
            return PhysicalKey::key_f;
        case XKB_KEY_g:
            return PhysicalKey::key_g;
        case XKB_KEY_h:
            return PhysicalKey::key_h;
        case XKB_KEY_i:
            return PhysicalKey::key_i;
        case XKB_KEY_j:
            return PhysicalKey::key_j;
        case XKB_KEY_k:
            return PhysicalKey::key_k;
        case XKB_KEY_l:
            return PhysicalKey::key_l;
        case XKB_KEY_m:
            return PhysicalKey::key_m;
        case XKB_KEY_n:
            return PhysicalKey::key_n;
        case XKB_KEY_o:
            return PhysicalKey::key_o;
        case XKB_KEY_p:
            return PhysicalKey::key_p;
        case XKB_KEY_q:
            return PhysicalKey::key_q;
        case XKB_KEY_r:
            return PhysicalKey::key_r;
        case XKB_KEY_s:
            return PhysicalKey::key_s;
        case XKB_KEY_t:
            return PhysicalKey::key_t;
        case XKB_KEY_u:
            return PhysicalKey::key_u;
        case XKB_KEY_v:
            return PhysicalKey::key_v;
        case XKB_KEY_w:
            return PhysicalKey::key_w;
        case XKB_KEY_x:
            return PhysicalKey::key_x;
        case XKB_KEY_y:
            return PhysicalKey::key_y;
        case XKB_KEY_z:
            return PhysicalKey::key_z;
        case XKB_KEY_Escape:
            return PhysicalKey::escape;
        case XKB_KEY_space:
            return PhysicalKey::space;
        case XKB_KEY_Delete:
            return PhysicalKey::key_delete;
        default:
            return PhysicalKey::unknown;
        }
    }

}  // namespace spargel::ui::xkb
