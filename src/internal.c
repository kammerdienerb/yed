/*
 * This memory is overwritten by a sneaky dd in install.sh
 */

__attribute__((used))
_path_patch_guide
_path_patch_guide_default_plug_dir = {
    {},
    /* rot13 of "default_plug_dir" */
    {'q', 'r', 's', 'n', 'h', 'y', 'g', '_', 'c', 'y', 'h', 't', '_', 'q', 'v', 'e'}
};

__attribute__((used))
_path_patch_guide
_path_patch_guide_installed_lib_dir = {
    {},
    /* rot13 of "installed_lib_dir" */
    {'v', 'a', 'f', 'g', 'n', 'y', 'y', 'r', 'q', '_', 'y', 'v', 'o', '_', 'q', 'v', 'e'}
};

#ifdef YED_DO_ASSERTIONS
void yed_assert_fail(const char *msg, const char *fname, int line, const char *cond_str) {
    volatile int *trap;

    yed_term_exit();

    fprintf(stderr, "Assertion failed -- %s\n"
                    "at  %s :: line %d\n"
                    "    Condition: '%s'\n",
                    msg, fname, line, cond_str);

    trap = 0;
    (void)*trap;
}
#endif




uint64_t next_power_of_2(uint64_t x) {
    if (x == 0) {
        return 2;
    }

    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}



char * pretty_bytes(uint64_t n_bytes) {
    uint64_t    s;
    double      count;
    char       *r;
    const char *suffixes[]
        = { " B", " KB", " MB", " GB", " TB", " PB", " EB" };

    s     = 0;
    count = (double)n_bytes;

    while (count >= 1024 && s < 7) {
        s     += 1;
        count /= 1024;
    }

    r = calloc(64, 1);

    if (count - floor(count) == 0.0) {
        sprintf(r, "%d", (int)count);
    } else {
        sprintf(r, "%.2f", count);
    }

    strcat(r, suffixes[s]);

    return r;
}


void yed_init_output_stream(void) {
    ys->output_buffer = array_make_with_cap(char, 4 * ys->term_cols * ys->term_rows);
    ys->writer_buffer = array_make_with_cap(char, 4 * ys->term_cols * ys->term_rows);
}

int output_buff_len(void) { return array_len(ys->output_buffer); }

void append_n_to_output_buff(char *s, int n) {
    array_push_n(ys->output_buffer, s, n);
}

void append_to_output_buff(char *s) {
    append_n_to_output_buff(s, strlen(s));
}

static char *itoa(char *p, unsigned x) {
    p += 3*sizeof(int);
    *--p = 0;
    do {
        *--p = '0' + x % 10;
        x /= 10;
    } while (x);
    return p;
}

void append_int_to_output_buff(int i) {
    char  s[16],
         *p;

    p = itoa(s, i);

    append_to_output_buff(p);
}

void flush_writer_buff(void) {
    if (!array_len(ys->writer_buffer)) {
        return;
    }
    write(1, array_data(ys->writer_buffer), array_len(ys->writer_buffer));
    array_clear(ys->writer_buffer);
}

void flush_output_buff(void) {
    if (!array_len(ys->output_buffer)) {
        return;
    }
    write(1, array_data(ys->output_buffer), array_len(ys->output_buffer));
    array_clear(ys->output_buffer);
}

void yed_set_small_message(char *msg) {
    if (ys->small_message) {
        free(ys->small_message);
    }

    if (msg) {
        ys->small_message = strdup(msg);
    } else {
        ys->small_message = NULL;
    }
}

static void write_status_bar(int key) {
    int         sav_x, sav_y;
    char       *path;
    char       *status_line_var;
    char        right_side_buff[256];
    char       *ft_name;
    int         i;
    yed_frame **fit;

    (void)key; /* We don't show this on the status bar any more. */

    sav_x = ys->cur_x;
    sav_y = ys->cur_y;

    yed_set_cursor(1, ys->term_rows - 1);
    if (ys->active_style) {
        yed_set_attr(yed_active_style_get_status_line());
    } else {
        append_to_output_buff(TERM_INVERSE);
    }
    append_n_to_output_buff(ys->_4096_spaces, ys->term_cols);

    right_side_buff[0] = 0;

    if (ys->active_frame) {
        yed_set_cursor(1, ys->term_rows - 1);
        append_n_to_output_buff(" ", 1);

        i = 0;
        array_traverse(ys->frames, fit) {
            if (*fit == ys->active_frame) {
                append_n_to_output_buff("[", 1);
                append_int_to_output_buff(i);
                append_n_to_output_buff("]", 2);
            } else {
                append_n_to_output_buff(" ", 1);
                append_int_to_output_buff(i);
                append_n_to_output_buff(" ", 1);
            }
            i += 1;
        }
        append_n_to_output_buff(" ", 1);

        ft_name = "";
        if (ys->active_frame->buffer) {
            path     = ys->active_frame->buffer->name;
            append_to_output_buff(path);

            if (ys->active_frame->buffer->flags & BUFF_SPECIAL) {
                ft_name = "<special>";
            } else {
                ft_name = yed_get_ft_name(ys->active_frame->buffer->ft);
                if (ft_name == NULL) {
                    ft_name = "<unknown file type>";
                }
            }
        }

        snprintf(right_side_buff, MIN(ys->term_cols, sizeof(right_side_buff)),
                 "%s  %7d :: %-3d",
                 ft_name, ys->active_frame->cursor_line, ys->active_frame->cursor_col);
    }

    yed_set_cursor(ys->term_cols - strlen(right_side_buff) - 2, ys->term_rows - 1);
    append_to_output_buff(right_side_buff);


    if ((status_line_var = yed_get_var("status-line-var"))) {
        status_line_var = yed_get_var(status_line_var);
        if (status_line_var) {
            yed_set_small_message(status_line_var);
        } else {
            yed_set_small_message(NULL);
        }
    } else {
        yed_set_small_message(NULL);
    }

    if (ys->small_message) {
        yed_set_cursor((ys->term_cols / 2) - (strlen(ys->small_message) / 2), ys->term_rows - 1);
        if (ys->active_style) {
            yed_set_attr(yed_active_style_get_status_line());
        } else {
            append_to_output_buff(TERM_INVERSE);
        }
        append_to_output_buff(ys->small_message);
    }


    append_to_output_buff(TERM_RESET);
    append_to_output_buff(TERM_CURSOR_HIDE);
    yed_set_cursor(sav_x, sav_y);
}

int yed_check_version_breaking(void) {
    int   breaks;
    char *env;
    char  cmd_buff[1024];
    FILE *p;
    char  ver_buff[32];
    int   new_ver;

    breaks = 1;

    if ((env = getenv("LD_LIBRARY_PATH"))) {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "LD_LIBRARY_PATH='%s' %s --no-init --version",
                 env, ys->argv0);
    } else if ((env = getenv("DYLD_LIBRARY_PATH"))) {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "DYLD_LIBRARY_PATH='%s' %s --no-init --version",
                 env, ys->argv0);
    } else {
        snprintf(cmd_buff, sizeof(cmd_buff),
                 "%s --version",
                 ys->argv0);
    }

    p = popen(cmd_buff, "r");
    if (p == NULL) { goto out; }

    if (fgets(ver_buff, sizeof(ver_buff), p) == NULL) { goto out; }

    pclose(p);

    sscanf(ver_buff, "%d", &new_ver);

    breaks = (new_ver / 100) > (yed_version / 100);

out:;
    return breaks;
}

void yed_service_reload(void) {
    tree_it(yed_plugin_name_t, yed_plugin_ptr_t)     plug_it;
    tree_it(yed_command_name_t, yed_command)         cmd_it;
    tree_it(yed_completion_name_t, yed_completion)   compl_it;
    tree_it(yed_var_name_t, yed_var_val_t)           var_it;
    tree_it(yed_style_name_t, yed_style_ptr_t)       style_it;
    char                                            *key,
                                                    *val;
    yed_style                                       *style;
    char                                           **ft_name_it;

    tree_reset_fns(yed_style_name_t,      yed_style_ptr_t,       ys->styles,              strcmp);
    tree_reset_fns(yed_var_name_t,        yed_var_val_t,         ys->vars,                strcmp);
    tree_reset_fns(yed_buffer_name_t,     yed_buffer_ptr_t,      ys->buffers,             strcmp);
    tree_reset_fns(int,                   yed_key_binding_ptr_t, ys->vkey_binding_map,    NULL);
    tree_reset_fns(yed_command_name_t,    yed_command,           ys->commands,            strcmp);
    tree_reset_fns(yed_command_name_t,    yed_command,           ys->default_commands,    strcmp);
    tree_reset_fns(yed_completion_name_t, yed_completion,        ys->completions,         strcmp);
    tree_reset_fns(yed_completion_name_t, yed_completion,        ys->default_completions, strcmp);
    tree_reset_fns(yed_plugin_name_t,     yed_plugin_ptr_t,      ys->plugins,             strcmp);

    ys->cur_log_name = NULL; /* This could be memory from a plugin that got unloaded. */

    tree_traverse(ys->plugins, plug_it) {
        yed_plugin_uninstall_features(tree_it_val(plug_it));
    }

    array_traverse(ys->ft_array, ft_name_it) {
        free(ft_name_it);
    }
    array_clear(ys->ft_array);

    /*
     * Clear out all of the old vars.
     */
     (void)var_it;
     (void)val;
/*     while (tree_len(ys->vars)) { */
/*         var_it = tree_begin(ys->vars); */
/*         key = tree_it_key(var_it); */
/*         val = tree_it_val(var_it); */
/*         tree_delete(ys->vars, key); */
/*         free(key); */
/*         free(val); */
/*     } */
    /*
     * Reset the defaults.
     */
/*     yed_set_default_vars(); */

    /*
     * Clear out all of the old styles.
     */
    while (tree_len(ys->styles)) {
        style_it = tree_begin(ys->styles);
        key   = tree_it_key(style_it);
        style = tree_it_val(style_it);
        tree_delete(ys->styles, key);
        free(key);
        free(style);
    }

    ys->active_style = NULL;
    /*
     * Reset the defaults.
     */
    yed_set_default_styles();

    /*
     * Clear out all of the old commands.
     */
    while (tree_len(ys->commands)) {
        cmd_it = tree_begin(ys->commands);
        key = tree_it_key(cmd_it);
        tree_delete(ys->commands, key);
        free(key);
    }
    while (tree_len(ys->default_commands)) {
        cmd_it = tree_begin(ys->default_commands);
        key = tree_it_key(cmd_it);
        tree_delete(ys->default_commands, key);
        free(key);
    }
    /*
     * Reset the defaults.
     */
    yed_set_default_commands();
    /*
     * Clear out all of the old completions.
     */
    while (tree_len(ys->completions)) {
        compl_it = tree_begin(ys->completions);
        key      = tree_it_key(compl_it);
        tree_delete(ys->completions, key);
        free(key);
    }
    while (tree_len(ys->default_completions)) {
        compl_it = tree_begin(ys->default_completions);
        key      = tree_it_key(compl_it);
        tree_delete(ys->default_completions, key);
        free(key);
    }
    /*
     * Reset the defaults.
     */
    yed_set_default_completions();

    yed_reload_default_event_handlers();
    yed_reload_plugins();

    yed_register_sigwinch_handler();
    yed_register_sigstop_handler();
    yed_register_sigcont_handler();

    ys->redraw = ys->redraw_cls = 1;
    append_to_output_buff(TERM_CURSOR_HIDE);
    yed_set_attr(yed_active_style_get_active());
    yed_clear_screen();
    yed_cursor_home();
    yed_write_welcome();
    append_to_output_buff(TERM_RESET);
    memset(ys->written_cells, 0, ys->term_rows * ys->term_cols);
    yed_update_frames();

    yed_draw_command_line();
    write_status_bar(0);

    ys->redraw = ys->redraw_cls = 0;

    if (ys->interactive_command) {
        yed_set_cursor(ys->cmd_cursor_x, ys->term_rows);
        append_to_output_buff(TERM_CURSOR_SHOW);
    } else if (ys->active_frame) {
        append_to_output_buff(TERM_CURSOR_SHOW);
    }
}

int s_to_i(const char *s) {
    int i;

    sscanf(s, "%d", &i);

    return i;
}

#include "array.c"
#include "bucket_array.c"
#include "term.c"
#include "key.c"
#include "wcwidth.c"
#include "utf8.c"
#include "undo.c"
#include "buffer.c"
#include "attrs.c"
#include "ft.c"
#include "frame.c"
#include "log.c"
#include "command.c"
#include "getRSS.c"
#include "measure_time.c"
#include "event.c"
#include "plugin.c"
#include "boyer_moore.c"
#include "find.c"
#include "var.c"
#include "util.c"
#include "style.c"
#include "subproc.c"
#include "complete.c"
#include "direct_draw.c"
#include "frame_tree.c"
#include "version.c"
#include "print_backtrace.c"
#include "cmd_line.c"
