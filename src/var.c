void yed_init_vars(void) {
    ys->vars = tree_make_c(yed_var_name_t, yed_var_val_t, strcmp);
    yed_set_default_vars();
}

void yed_set_default_vars(void) {
    yed_set_var("tab-width",                 XSTR(DEFAULT_TABW));
    yed_set_var("ctrl-h-is-backspace",       "yes");
    yed_set_var("buffer-load-mode",          "map");
    yed_set_var("bracketed-paste-mode",      "on");
    yed_set_var("enable-search-cursor-move", "yes");
    yed_set_var("default-scroll-offset",     XSTR(DEFAULT_SCROLL_OFF));
    yed_set_var("command-prompt-string",     DEFAULT_CMD_PROMPT_STRING);
    yed_set_var("fill-string",               DEFAULT_FILL_STRING);
}

void yed_set_var(char *var, char *val) {
    tree_it(yed_var_name_t,
            yed_var_val_t)     it;
    char                      *old_val;

    if (!var || !val) {
        return;
    }

    it = tree_lookup(ys->vars, var);

    if (!tree_it_good(it)) {
        tree_insert(ys->vars, strdup(var), strdup(val));
    } else {
        old_val = tree_it_val(it);
        tree_insert(ys->vars, var, strdup(val));
        free(old_val);
    }
}

char *yed_get_var(char *var) {
    tree_it(yed_var_name_t,
            yed_var_val_t)     it;

    if (!var) {
        return NULL;
    }

    it = tree_lookup(ys->vars, var);

    if (!tree_it_good(it)) {
        return NULL;
    }

    return tree_it_val(it);
}

void yed_unset_var(char *var) {
    tree_it(yed_var_name_t,
            yed_var_val_t)       it;
    char                        *old_var,
                                *old_val;

    if (!var) {
        return;
    }

    it = tree_lookup(ys->vars, var);

    if (!tree_it_good(it)) {
        return;
    }

    old_var = tree_it_key(it);
    old_val = tree_it_val(it);

    tree_delete(ys->vars, var);
    free(old_var);
    free(old_val);
}

int yed_var_is_truthy(char *var) {
    char *val;

    if (!(val = yed_get_var(var))) {
        return 0;
    }

    if (strlen(val)          == 0
    ||  strcmp(val, "0")     == 0
    ||  strcmp(val, "off")   == 0
    ||  strcmp(val, "Off")   == 0
    ||  strcmp(val, "OFF")   == 0
    ||  strcmp(val, "no")    == 0
    ||  strcmp(val, "No")    == 0
    ||  strcmp(val, "NO")    == 0
    ||  strcmp(val, "false") == 0
    ||  strcmp(val, "False") == 0
    ||  strcmp(val, "FALSE") == 0) {
        return 0;
    }

    return 1;
}

int yed_get_var_as_int(char *var, int *out) {
    char *val;

    if (!(val = yed_get_var(var))) {
        return 0;
    }

    sscanf(val, "%d", out);

    return 1;
}

int yed_get_tab_width(void) {
    int tabw;

    if (!yed_get_var_as_int("tab-width", &tabw)
    ||  tabw <= 0) {
        tabw = DEFAULT_TABW;
    }

    return tabw;
}

int yed_get_default_scroll_offset(void) {
    int scroll_off;

    if (!yed_get_var_as_int("default-scroll-offset", &scroll_off)
    ||  scroll_off < 0) {
        scroll_off = DEFAULT_SCROLL_OFF;
    }

    return scroll_off;
}
