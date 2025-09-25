#include "spargel/ui/ui_xcb.h"

#include "spargel/logging/logging.h"

// X11/XCB
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

/* for clock_gettime */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* libc */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace spargel::ui {

    base::unique_ptr<Platform> makePlatformXcb() {
        return base::make_unique<PlatformXcb>();
    }

    PlatformXcb::PlatformXcb() : Platform(PlatformKind::xcb) {
        /*
         * Open the connection to the X server.
         * Use the DISPLAY environment variable as the default display name.
         */
        display = XOpenDisplay(NULL);
        spargel_log_debug("X display opened");

        connection = XGetXCBConnection(display);
        spargel_log_debug("XCB connection established");

        /* Get the first screen */
        screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

        spargel_log_info("Information of XCB screen %d:", screen->root);
        spargel_log_info("  width: \t%d", screen->width_in_pixels);
        spargel_log_info("  height:\t%d", screen->height_in_pixels);
        spargel_log_info("  white pixel:\t%08x", screen->white_pixel);
        spargel_log_info("  black pixel:\t%08x", screen->black_pixel);

#if SPARGEL_ENABLE_OPENGL
        visual_info = nullptr;

        int glx_major, glx_minor;
        glXQueryVersion(display, &glx_major, &glx_minor);
        spargel_log_info("GLX version: %d.%d", glx_major, glx_minor);
#endif

        xcb_intern_atom_cookie_t cookie;
        xcb_intern_atom_reply_t* reply;

        cookie = intern_atom_cookie(1, "WM_PROTOCOLS");
        reply = intern_atom_reply(cookie);
        atom_wm_protocols = reply->atom;
        free(reply);

        cookie = intern_atom_cookie(0, "WM_DELETE_WINDOW");
        reply = intern_atom_reply(cookie);
        atom_wm_delete_window = reply->atom;
        free(reply);

        initXKBContext();
    }

    PlatformXcb::~PlatformXcb() {
#if SPARGEL_ENABLE_OPENGL
        XFree(visual_info);
#endif

        // should not do this
        // xcb_disconnect(connection);

        XCloseDisplay(display);
    }

    const float FPS = 60;
    const unsigned int SECOND_NS = 1000000000U;
    const unsigned int FRAME_DELTA_NS = (unsigned int)(SECOND_NS / FPS);

    static bool in_delta(struct timespec* t1, struct timespec* t2,
                         unsigned int delta) {
        if (t1->tv_sec == t2->tv_sec) {
            return t2->tv_nsec < t1->tv_nsec + delta;
        } else if (t1->tv_sec == t2->tv_sec - 1) {
            return t2->tv_nsec + SECOND_NS < t1->tv_nsec + delta;
        } else {
            return false;
        }
    }

    void PlatformXcb::startLoop() {
        xcb_generic_event_t* event;
        struct timespec t1, t2, duration = {.tv_sec = 0, .tv_nsec = 0};
        while (true) {
            bool should_stop = true;
            for (auto& window : windows) {
                if (!window->closed) {
                    should_stop = false;
                    break;
                }
            }
            if (should_stop) break;

            event = xcb_poll_for_event(connection);
            if (!event) {
                clock_gettime(CLOCK_MONOTONIC, &t1);

                run_render_callbacks();
                xcb_flush(connection);

                /*
                 * wait for the next frame
                 * Normally the sleeping only occurs once each frame.
                 */
                while (true) {
                    clock_gettime(CLOCK_MONOTONIC, &t2);
                    if (in_delta(&t1, &t2, FRAME_DELTA_NS)) {
                        duration.tv_nsec = FRAME_DELTA_NS;
                        clock_nanosleep(CLOCK_MONOTONIC, 0, &duration, NULL);
                    } else {
                        break;
                    }
                }

                continue;
            }

            switch (event->response_type & 0x7f) {
            case XCB_CLIENT_MESSAGE: {
                auto* client_message_event = (xcb_client_message_event_t*)event;
                if (client_message_event->data.data32[0] ==
                    atom_wm_delete_window) {
                    for (auto& window : windows) {
                        if (window->id == client_message_event->window) {
                            window->closed = true;
                            window->getDelegate()->onCloseRequested();
                            window->getDelegate()->onClosed();
                            break;
                        }
                    }
                }
            } break;
            case XCB_EXPOSE: {
                run_render_callbacks();
                xcb_flush(connection);
            } break;
            case XCB_KEY_PRESS: {
                auto* key_press_event = (xcb_key_press_event_t*)event;
                for (auto& window : windows) {
                    if (window->id == key_press_event->event) {
                        KeyboardEvent keyboard_event = {
                            .action = KeyboardAction::press,
                            .key = xkb.translatePhysicalKey(
                                key_press_event->detail)};
                        window->getDelegate()->onKeyboard(keyboard_event);
                        break;
                    }
                }
            } break;
            case XCB_KEY_RELEASE: {
                auto* key_release_event = (xcb_key_release_event_t*)event;
                for (auto& window : windows) {
                    if (window->id == key_release_event->event) {
                        KeyboardEvent keyboard_event = {
                            .action = KeyboardAction::release,
                            .key = xkb.translatePhysicalKey(
                                key_release_event->detail)};
                        window->getDelegate()->onKeyboard(keyboard_event);
                        break;
                    }
                }
            } break;
            default:
                break;
            }

            free(event);
        }
    }

    base::unique_ptr<Window> PlatformXcb::makeWindow(u32 width, u32 height) {
        xcb_window_t id = xcb_generate_id(connection);

#if SPARGEL_ENABLE_OPENGL
        if (!visual_info) {
            static int attribs[] = {GLX_RENDER_TYPE,
                                    GLX_RGBA_BIT,
                                    GLX_RED_SIZE,
                                    8,
                                    GLX_GREEN_SIZE,
                                    8,
                                    GLX_BLUE_SIZE,
                                    8,
                                    GLX_ALPHA_SIZE,
                                    8,
                                    GLX_DEPTH_SIZE,
                                    24,
                                    GLX_STENCIL_SIZE,
                                    8,
                                    GLX_DOUBLEBUFFER,
                                    True,
                                    None};

            visual_info = glXChooseVisual(display, 0, attribs);
            if (visual_info) {
                spargel_log_info("selected visual 0x%lx for GLX",
                                 visual_info->visualid);
            } else {
                spargel_log_fatal("cannot find appropriate visual for GLX");
                spargel_panic_here();
            }
        }

        xcb_visualid_t visualid = visual_info->visualid;
        uint8_t depth = visual_info->depth;

#else
        xcb_visualid_t visualid = _screen->root_visual;
        uint8_t depth = XCB_COPY_FROM_PARENT;
#endif

        u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        u32 values[2] = {screen->black_pixel, XCB_EVENT_MASK_EXPOSURE |
                                                  XCB_EVENT_MASK_KEY_PRESS |
                                                  XCB_EVENT_MASK_KEY_RELEASE};

        spargel_log_debug("creating X window %d", id);

        xcb_create_window(connection, depth,             /* depth*/
                          id, screen->root,              /* parent window */
                          0, 0,                          /* x, y */
                          width, height,                 /* width, height */
                          0,                             /* border width */
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class */
                          visualid,                      /* visual */
                          mask, values);

        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, id,
                            atom_wm_protocols, XCB_ATOM_ATOM, XCB_ATOM_VISUALID,
                            1, &atom_wm_delete_window);

        xcb_map_window(connection, id);
        xcb_flush(connection);

        return base::make_unique<WindowXcb>(this, id);
    }

    void PlatformXcb::run_render_callbacks() {
        for (auto& window : windows) {
            auto delegate = window->getDelegate();
            if (!delegate) {
                spargel_log_fatal("window delegate not set");
                spargel_panic_here();
            }
            delegate->onRender();
        }
    }

    xcb_intern_atom_cookie_t PlatformXcb::intern_atom_cookie(u8 only_if_exists,
                                                             const char* name) {
        return xcb_intern_atom(connection, only_if_exists, strlen(name), name);
    }

    xcb_intern_atom_reply_t* PlatformXcb::intern_atom_reply(
        xcb_intern_atom_cookie_t cookie) {
        return xcb_intern_atom_reply(connection, cookie, nullptr);
    }

    void PlatformXcb::initXKBContext() {
        xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!xkb.context) {
            spargel_log_fatal("XKB: failed to create context");
            spargel_panic_here();
        }

        i32 device_id = xkb_x11_get_core_keyboard_device_id(connection);
        if (device_id < 0) {
            spargel_log_fatal("XKB: failed to get core keyboard device id");
            spargel_panic_here();
        }
        spargel_log_info("XKB: core keyboard device id is %d", device_id);

        xkb.keymap = xkb_x11_keymap_new_from_device(
            xkb.context, connection, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (!xkb.keymap) {
            spargel_log_fatal("XKB: failed to get keymap from device");
            spargel_panic_here();
        }

        xkb.state =
            xkb_x11_state_new_from_device(xkb.keymap, connection, device_id);
        if (!xkb.state) {
            spargel_log_fatal("XKB: failed to create state from device");
            spargel_panic_here();
        }
    }

    WindowXcb::WindowXcb(PlatformXcb* platform, xcb_window_t id)
        : platform(platform), id(id), closed(false) {
        platform->windows.emplace(this);
    }

    WindowXcb::~WindowXcb() {
        spargel_log_debug("X window %d is closing", id);
        xcb_destroy_window(platform->connection, id);
    }

    void WindowXcb::setTitle(char const* title) {
        spargel_log_debug("setting the title of X window %d to \"%s\"", id,
                          title);
        xcb_change_property(platform->connection, XCB_PROP_MODE_REPLACE, id,
                            XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title),
                            title);
    }

    WindowHandle WindowXcb::getHandle() {
        WindowHandle handle{};
        auto& xcb_handle = handle.value.xcb;

        xcb_handle.connection = platform->connection;
        xcb_handle.window = id;

        xcb_handle.display = platform->display;
#if SPARGEL_ENABLE_OPENGL
        xcb_handle.visual_info = platform->visual_info;
#endif

        return handle;
    }

}  // namespace spargel::ui
