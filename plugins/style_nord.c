#include "plugin.h"

#define nord0  RGB_32_hex(2E3440)
#define nord1  RGB_32_hex(3B4252)
#define nord2  RGB_32_hex(434C5E)
#define nord3  RGB_32_hex(4C566A)
#define nord4  RGB_32_hex(D8DEE9)
#define nord5  RGB_32_hex(E5E9F0)
#define nord6  RGB_32_hex(ECEFF4)
#define nord7  RGB_32_hex(8FBCBB)
#define nord8  RGB_32_hex(88C0D0)
#define nord9  RGB_32_hex(81A1C1)
#define nord10 RGB_32_hex(5E81AC)
#define nord11 RGB_32_hex(BF616A)
#define nord12 RGB_32_hex(D08770)
#define nord13 RGB_32_hex(EBCB8B)
#define nord14 RGB_32_hex(A3BE8C)
#define nord15 RGB_32_hex(B48EAD)

int yed_plugin_boot(yed_plugin *self) {
    yed_style s;

    memset(&s, 0, sizeof(s));

    s.active.flags        = ATTR_RGB;
    s.active.fg           = nord5;
    s.active.bg           = nord0;

    s.inactive.flags      = ATTR_RGB;
    s.inactive.fg         = nord4;
    s.inactive.bg         = nord1;

    s.cursor_line.flags   = ATTR_RGB;
    s.cursor_line.fg      = nord6;
    s.cursor_line.bg      = nord2;

    s.search.flags        = ATTR_RGB | ATTR_BOLD;
    s.search.fg           = nord6;
    s.search.bg           = nord10;

    s.search_cursor.flags = ATTR_RGB | ATTR_BOLD;
    s.search_cursor.fg    = nord10;
    s.search_cursor.bg    = nord13;

    s.selection.flags     = ATTR_RGB;
    s.selection.fg        = nord5;
    s.selection.bg        = nord2;

    s.code_comment.flags  = ATTR_RGB;
    s.code_comment.fg     = nord3;

    s.code_keyword.flags  = ATTR_RGB | ATTR_BOLD;
    s.code_keyword.fg     = nord9;

    s.code_preprocessor   = s.code_keyword;

    s.code_fn_call.flags  = ATTR_RGB;
    s.code_fn_call.fg     = nord8;

    s.code_number.flags   = ATTR_RGB;
    s.code_number.fg      = nord15;

    s.code_string.flags   = ATTR_RGB;
    s.code_string.fg      = nord14;

    s.code_character      = s.code_string;

    yed_plugin_set_style(self, "nord", &s);

    return 0;
}