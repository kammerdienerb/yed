void yed_init_buffers(void) {
    yed_buffer *yank_buff;
    yed_buffer *log_buff;

    LOG_FN_ENTER();

    ys->buffers = tree_make_c(yed_buffer_name_t, yed_buffer_ptr_t, strcmp);

    yank_buff         = yed_create_buffer("*yank");
    yank_buff->kind   = BUFF_KIND_YANK;
    yank_buff->flags |= BUFF_RD_ONLY | BUFF_SPECIAL;
    ys->yank_buff     = yank_buff;

    log_buff          = yed_create_buffer("*log");
    log_buff->kind    = BUFF_KIND_LOG;
    log_buff->flags  |= BUFF_RD_ONLY | BUFF_SPECIAL;
    ys->log_buff      = log_buff;
}

yed_line yed_new_line(void) {
    yed_line line;

    memset(&line, 0, sizeof(line));

    line.chars = array_make(char);

    return line;
}

yed_line yed_new_line_with_cap(int len) {
    yed_line line;

    memset(&line, 0, sizeof(line));

    line.chars = array_make_with_cap(char, len);

    return line;
}

void yed_free_line(yed_line *line) {
    array_free(line->chars);
}

yed_line * yed_copy_line(yed_line *line) {
    yed_line *new_line;

    new_line  = malloc(sizeof(*new_line));
    *new_line = yed_new_line();

    new_line->visual_width = line->visual_width;
    new_line->n_glyphs     = line->n_glyphs;
    array_copy(new_line->chars, line->chars);

    return new_line;
}

void yed_line_add_glyph(yed_line *line, yed_glyph g, int idx) {
    int len, i;

    len = yed_get_glyph_len(g);
    for (i = len - 1; i >= 0; i -= 1) {
        array_insert(line->chars, idx, g.bytes[i]);
    }
    line->visual_width += yed_get_glyph_width(g);
    line->n_glyphs     += 1;
}

void yed_line_append_glyph(yed_line *line, yed_glyph g) {
    int len, width, i;

    len   = yed_get_glyph_len(g);
    width = yed_get_glyph_width(g);
    for (i = 0; i < len; i += 1) {
        array_push(line->chars, g.bytes[i]);
    }
    line->visual_width += width;
    line->n_glyphs     += 1;
}

void yed_line_delete_glyph(yed_line *line, int idx) {
    yed_glyph *g;
    int        len, width, i;

    g     = array_item(line->chars, idx);
    len   = yed_get_glyph_len(*g);
    width = yed_get_glyph_width(*g);

    for (i = 0; i < len; i += 1) {
        array_delete(line->chars, idx);
    }

    line->visual_width -= width;
    line->n_glyphs     -= 1;
}

void yed_line_pop_glyph(yed_line *line) {
    yed_glyph *g;
    int        idx, len, width, i;

    idx   = yed_line_col_to_idx(line, line->visual_width);
    g     = array_item(line->chars, idx);
    len   = yed_get_glyph_len(*g);
    width = yed_get_glyph_width(*g);

    for (i = 0; i < len; i += 1) {
        array_pop(line->chars);
    }

    line->visual_width -= width;
    line->n_glyphs     -= 1;
}

yed_buffer yed_new_buff(void) {
    yed_buffer  buff;

    buff.lines                = bucket_array_make(1024, yed_line);
    buff.get_line_cache       = NULL;
    buff.get_line_cache_row   = 0;
    buff.path                 = NULL;
    buff.mmap_underlying_buff = NULL;
    buff.has_selection        = 0;
    buff.flags                = 0;
    buff.undo_history         = yed_new_undo_history();
    buff.last_cursor_row      = 1;
    buff.last_cursor_col      = 1;

    yed_buffer_add_line_no_undo(&buff);

    return buff;
}

yed_buffer *yed_create_buffer(char *name) {
    yed_buffer                                   *buff;
    tree_it(yed_buffer_name_t, yed_buffer_ptr_t)  it;

    it = tree_lookup(ys->buffers, name);

    if (tree_it_good(it)) {
        return NULL;
    }

    buff = malloc(sizeof(*buff));

    *buff = yed_new_buff();
    buff->name = strdup(name);

    tree_insert(ys->buffers, strdup(name), buff);

    return buff;
}

yed_buffer * yed_get_buffer(char *name) {
    tree_it(yed_buffer_name_t, yed_buffer_ptr_t)  it;

    it = tree_lookup(ys->buffers, name);

    if (!tree_it_good(it)) {
        return NULL;
    }

    return tree_it_val(it);
}

void yed_free_buffer(yed_buffer *buffer) {
    yed_line *line;

    yed_frames_remove_buffer(buffer);

    if (buffer->name) {
        tree_delete(ys->buffers, buffer->name);
        free(buffer->name);
    }

    if (buffer->path) {
        free(buffer->path);
    }

    if (buffer->mmap_underlying_buff) {
        free(buffer->mmap_underlying_buff);
    }

    bucket_array_traverse(buffer->lines, line) {
        yed_free_line(line);
    }

    bucket_array_free(buffer->lines);

    yed_free_undo_history(&buffer->undo_history);

    free(buffer);
}







void yed_append_to_line_no_undo(yed_buffer *buff, int row, yed_glyph g) {
    yed_line *line;

    line = yed_buff_get_line(buff, row);
    yed_line_append_glyph(line, g);
}

void yed_pop_from_line_no_undo(yed_buffer *buff, int row) {
    yed_line *line;

    line = yed_buff_get_line(buff, row);
    yed_line_pop_glyph(line);
}

void yed_line_clear_no_undo(yed_buffer *buff, int row) {
    yed_line *line;

    line = yed_buff_get_line(buff, row);
    array_clear(line->chars);
    line->visual_width = 0;
}

int yed_buffer_add_line_no_undo(yed_buffer *buff) {
    yed_line new_line;

    new_line = yed_new_line();

    bucket_array_push(buff->lines, new_line);

    yed_mark_dirty_frames(buff);
    buff->get_line_cache     = NULL;
    buff->get_line_cache_row = 0;

    return yed_buff_n_lines(buff);
}

void yed_buff_set_line_no_undo(yed_buffer *buff, int row, yed_line *line) {
    yed_line *old_line;

    old_line = yed_buff_get_line(buff, row);

    yed_free_line(old_line);
    old_line->visual_width = line->visual_width;
    old_line->chars        = array_make(char);
    array_copy(old_line->chars, line->chars);
}

yed_line * yed_buff_insert_line_no_undo(yed_buffer *buff, int row) {
    int      idx;
    yed_line new_line, *line;

    idx = row - 1;

    if (idx < 0 || idx > bucket_array_len(buff->lines)) {
        return NULL;
    }

    new_line = yed_new_line();
    line     = bucket_array_insert(buff->lines, idx, new_line);

    yed_mark_dirty_frames(buff);
    buff->get_line_cache     = NULL;
    buff->get_line_cache_row = 0;

    return line;
}

void yed_buff_delete_line_no_undo(yed_buffer *buff, int row) {
    int       idx;
    yed_line *line;

    idx = row - 1;

    LIMIT(idx, 0, bucket_array_len(buff->lines));

    line = yed_buff_get_line(buff, row);
    yed_free_line(line);
    bucket_array_delete(buff->lines, idx);

    yed_mark_dirty_frames(buff);
    buff->get_line_cache     = NULL;
    buff->get_line_cache_row = 0;
}

void yed_insert_into_line_no_undo(yed_buffer *buff, int row, int col, yed_glyph g) {
    int       idx;
    yed_line *line;

    line = yed_buff_get_line(buff, row);

    idx = yed_line_col_to_idx(line, col);
    yed_line_add_glyph(line, g, idx);

    yed_mark_dirty_frames_line(buff, row);
}

void yed_delete_from_line_no_undo(yed_buffer *buff, int row, int col) {
    int       idx;
    yed_line *line;

    line = yed_buff_get_line(buff, row);

    idx = yed_line_col_to_idx(line, col);
    yed_line_delete_glyph(line, idx);

    yed_mark_dirty_frames_line(buff, row);
}

void yed_buff_clear_no_undo(yed_buffer *buff) {
    yed_line *line;

    bucket_array_traverse(buff->lines, line) {
        yed_free_line(line);
    }
    bucket_array_clear(buff->lines);

    yed_buffer_add_line_no_undo(buff);

    yed_mark_dirty_frames(buff);
    buff->get_line_cache     = NULL;
    buff->get_line_cache_row = 0;
}

/*
 * The following functions are the interface by which everything
 * else should modify buffers.
 * This is meant to preserve undo/redo behavior.
 */

void yed_append_to_line(yed_buffer *buff, int row, yed_glyph g) {
    yed_undo_action uact;

    uact.kind = UNDO_GLYPH_PUSH;
    uact.row  = row;
    uact.g    = g;

    yed_push_undo_action(buff, &uact);

    yed_append_to_line_no_undo(buff, row, g);
}

void yed_pop_from_line(yed_buffer *buff, int row) {
    yed_undo_action  uact;
    yed_line        *line;

    line = yed_buff_get_line(buff, row);

    uact.kind = UNDO_GLYPH_POP;
    uact.row  = row;
    uact.g    = *yed_line_col_to_glyph(line, line->visual_width);

    yed_push_undo_action(buff, &uact);

    yed_pop_from_line_no_undo(buff, row);
}

void yed_line_clear(yed_buffer *buff, int row) {
    yed_line        *line;
    yed_undo_action  uact;
    int              i;

    line = yed_buff_get_line(buff, row);

    uact.kind = UNDO_GLYPH_POP;
    uact.row  = row;
    for (i = line->visual_width; i >= 1; i -= 1) {
        uact.g = *yed_line_col_to_glyph(line, i);
        yed_push_undo_action(buff, &uact);
    }

    yed_line_clear_no_undo(buff, row);
}

int yed_buffer_add_line(yed_buffer *buff) {
    int             row;
    yed_undo_action uact;

    row = yed_buffer_add_line_no_undo(buff);

    uact.kind = UNDO_LINE_ADD;
    uact.row  = row;
    yed_push_undo_action(buff, &uact);

    return row;
}

void yed_buff_set_line(yed_buffer *buff, int row, yed_line *line) {
    yed_line        *old_line;
    yed_undo_action  uact;
    int              i;

    old_line = yed_buff_get_line(buff, row);

    uact.kind = UNDO_GLYPH_POP;
    uact.row  = row;
    for (i = old_line->visual_width; i >= 1; i -= 1) {
        uact.g = *yed_line_col_to_glyph(old_line, i);
        yed_push_undo_action(buff, &uact);
    }

    uact.kind = UNDO_GLYPH_PUSH;
    uact.row  = row;
    for (i = 1; i <= line->visual_width; i += 1) {
        uact.g = *yed_line_col_to_glyph(line, i);
        yed_push_undo_action(buff, &uact);
    }

    yed_buff_set_line_no_undo(buff, row, line);
}

yed_line * yed_buff_insert_line(yed_buffer *buff, int row) {
    yed_line        *line;
    yed_undo_action  uact;

    line = yed_buff_insert_line_no_undo(buff, row);

    if (line) {
        uact.kind = UNDO_LINE_ADD;
        uact.row  = row;
        yed_push_undo_action(buff, &uact);
    }

    return line;
}

void yed_buff_delete_line(yed_buffer *buff, int row) {
    yed_undo_action  uact;
    yed_line        *line;
    int              i;

    line = yed_buff_get_line(buff, row);

    uact.kind = UNDO_GLYPH_POP;
    uact.row  = row;
    for (i = line->visual_width; i >= 1; i -= 1) {
        uact.g = *yed_line_col_to_glyph(line, i);
        yed_push_undo_action(buff, &uact);
    }

    uact.kind = UNDO_LINE_DEL;
    uact.row  = row;
    yed_push_undo_action(buff, &uact);

    yed_buff_delete_line_no_undo(buff, row);
}

void yed_insert_into_line(yed_buffer *buff, int row, int col, yed_glyph g) {
    yed_undo_action uact;

    uact.kind = UNDO_GLYPH_ADD;
    uact.row  = row;
    uact.col  = yed_line_normalize_col(yed_buff_get_line(buff, row),  col);
    uact.g    = g;

    yed_push_undo_action(buff, &uact);

    yed_insert_into_line_no_undo(buff, row, col, g);
}

void yed_delete_from_line(yed_buffer *buff, int row, int col) {
    yed_line        *line;
    yed_undo_action  uact;

    line = yed_buff_get_line(buff, row);

    uact.kind = UNDO_GLYPH_DEL;
    uact.row  = row;
    uact.col  = yed_line_normalize_col(yed_buff_get_line(buff, row),  col);
    uact.g    = *yed_line_col_to_glyph(line, col);

    yed_push_undo_action(buff, &uact);

    yed_delete_from_line_no_undo(buff, row, col);
}

void yed_buff_clear(yed_buffer *buff) {
    int              row, col;
    yed_line        *line;
    yed_undo_action  uact;

    for (row = bucket_array_len(buff->lines); row >= 1; row -= 1) {
        line = yed_buff_get_line(buff, row);

        uact.kind = UNDO_GLYPH_POP;
        uact.row  = row;
        for (col = line->visual_width; col >= 1; col -= 1) {
            uact.g = *yed_line_col_to_glyph(line, col);
            yed_push_undo_action(buff, &uact);
        }

        uact.kind = UNDO_LINE_DEL;
        uact.row  = row;
        yed_push_undo_action(buff, &uact);
    }

    uact.kind = UNDO_LINE_ADD;
    uact.row  = 1;
    yed_push_undo_action(buff, &uact);

    yed_buff_clear_no_undo(buff);
}








int yed_buff_n_lines(yed_buffer *buff) {
    return bucket_array_len(buff->lines);
}




int yed_line_idx_to_col(yed_line *line, int idx) {
    yed_glyph *g;
    int        i, col, len;

    len = array_len(line->chars);

    if (line->n_glyphs == line->visual_width
    &&  line->n_glyphs == array_len(line->chars)) {
        return idx + 1;
    }

    col = 1;
    for (i = 0; i < idx && i < len;) {
        g    = array_item(line->chars, i);
        col += yed_get_glyph_width(*g);
        i   += yed_get_glyph_len(*g);
    }

    return col;
}

int yed_line_col_to_idx(yed_line *line, int col) {
    yed_glyph *g;
    int        c, i;

    if (col == line->visual_width + 1) {
        return array_len(line->chars);
    }

    if (line->n_glyphs == line->visual_width
    &&  line->n_glyphs == array_len(line->chars)) {
        return col - 1;
    }

    ASSERT(col <= line->visual_width, "unable to convert column to glyph index");

    i = 0;
    for (c = 1; c <= line->visual_width;) {
        g  = array_item(line->chars, i);
        c += yed_get_glyph_width(*g);

        if (col < c) { return i; }

        i += yed_get_glyph_len(*g);
    }

    return i;
}

yed_glyph * yed_line_col_to_glyph(yed_line *line, int col) {
    int idx;

    idx = yed_line_col_to_idx(line, col);

    if (idx == -1) {
        return NULL;
    }

    return (yed_glyph*)array_item(line->chars, idx);
}

yed_glyph * yed_line_last_glyph(yed_line *line) {
    return yed_line_col_to_glyph(line, line->visual_width);
}

yed_line * yed_buff_get_line(yed_buffer *buff, int row) {
    int       idx;
    yed_line *line;

    if (row == buff->get_line_cache_row) {
        return buff->get_line_cache;
    }

    idx = row - 1;

    if (idx < 0 || idx >= bucket_array_len(buff->lines)) {
        return NULL;
    }

    line = bucket_array_item(buff->lines, idx);

    buff->get_line_cache     = line;
    buff->get_line_cache_row = row;

    return line;
}

int yed_line_normalize_col(yed_line *line, int col) {
    int idx;

    idx = yed_line_col_to_idx(line, col);
    return yed_line_idx_to_col(line, idx);
}

yed_glyph * yed_buff_get_glyph(yed_buffer *buff, int row, int col) {
    yed_line *line;
    int       idx;

    line = yed_buff_get_line(buff, row);
    if (!line) { return NULL; }

    idx = yed_line_col_to_idx(line, col);
    if (idx == -1) { return NULL; }

    return array_item(line->chars, idx);
}



int yed_fill_buff_from_file(yed_buffer *buff, char *path) {
    char        *mode;
    FILE        *f;
    struct stat  fs;
    int          fd;
    int          status;

    status = BUFF_FILL_STATUS_SUCCESS;
    errno  = 0;
    f      = fopen(path, "r");
    if (f) {
        fd = fileno(f);

        errno = 0;
        if (fstat(fd, &fs) != 0) {
            errno = 0;
            return BUFF_FILL_STATUS_ERR_NOF;
        } else if (S_ISDIR(fs.st_mode)) {
            errno = 0;
            return BUFF_FILL_STATUS_ERR_DIR;
        }
    }

    if (errno) {
        if (errno == ENOENT) {
            status = BUFF_FILL_STATUS_ERR_NOF;
        } else if (errno == EISDIR) {
            status = BUFF_FILL_STATUS_ERR_DIR;
        } else if (errno == EACCES) {
            status = BUFF_FILL_STATUS_ERR_PER;
        } else {
            status = BUFF_FILL_STATUS_ERR_UNK;
        }

        errno = 0;
        return status;
    }

    yed_buff_clear_no_undo(buff);

    if ((mode = yed_get_var("buffer-load-mode"))
    && (strcmp(mode, "map") == 0)) {
        status = yed_fill_buff_from_file_map(buff, fd, fs.st_size);
    } else {
        status = yed_fill_buff_from_file_stream(buff, f);
    }

    if (status != BUFF_FILL_STATUS_SUCCESS) {
        goto cleanup;
    }

    buff->path    = strdup(path);
    buff->file.ft = yed_get_ft(path);
    buff->kind    = BUFF_KIND_FILE;
    yed_mark_dirty_frames(buff);

cleanup:
    fclose(f);

    return status;
}

int yed_fill_buff_from_file_map(yed_buffer *buff, int fd, unsigned long long file_size) {
    int          i, line_len;
    char        *file_data, *underlying_buff, *end, *scan, *tmp, c;
    yed_line    *last_line,
                 line;
    yed_glyph   *g;


    if (file_size == 0) {
        return BUFF_FILL_STATUS_SUCCESS;
    }

    file_data = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);

    if (file_data == MAP_FAILED) {
        errno = 0;
        return BUFF_FILL_STATUS_ERR_MAP;
    }

    /*
     * Add 3 bytes of padding so that we don't violate anything
     * when we call yed_get_string_info().
     * See the comment there (src/utf8.c) for more info.
     */
    underlying_buff = malloc(file_size + 3);

    memcpy(underlying_buff, file_data, file_size);

    /*
     * This buffer is going to come to us with a pre-made
     * empty line.
     * We don't need it though.
     */
    last_line = bucket_array_last(buff->lines);
    yed_free_line(last_line);
    bucket_array_pop(buff->lines);


    scan = underlying_buff;
    end  = underlying_buff + file_size;

    while (scan < end) {
        tmp      = memchr(scan, '\n', end - scan);
        line_len = (tmp ? tmp : end) - scan;

        line                   = yed_new_line_with_cap(line_len);
        line.chars.should_free = 0;
        line.chars.data        = scan;
        line.chars.used        = line_len;

        /* Remove '\r' from line. */
        while (array_len(line.chars)
        &&    ((c = *(char*)array_last(line.chars)) == '\r')) {
            array_pop(line.chars);
            line_len -= 1;
        }

        (void)g;
        (void)i;
        yed_get_string_info(line.chars.data, line_len, &line.n_glyphs, &line.visual_width);

        bucket_array_push(buff->lines, line);

        if (unlikely(!tmp)) { break; }

        scan = tmp + 1;
    }

    munmap(file_data, file_size);
    buff->mmap_underlying_buff = underlying_buff;

    if (bucket_array_len(buff->lines) > 1) {
        last_line = bucket_array_last(buff->lines);
        if (array_len(last_line->chars) == 0) {
            bucket_array_pop(buff->lines);
        }
    } else if (bucket_array_len(buff->lines) == 0) {
        line = yed_new_line();
        bucket_array_push(buff->lines, line);
    }

    return BUFF_FILL_STATUS_SUCCESS;
}

int yed_fill_buff_from_file_stream(yed_buffer *buff, FILE *f) {
    ssize_t      line_len;
    size_t       line_cap;
    char         c, *line_data;
    yed_line    *last_line,
                 line;
    yed_glyph   *g;
    int          j;

    last_line = bucket_array_last(buff->lines);
    yed_free_line(last_line);
    bucket_array_pop(buff->lines);

    while (line_data = NULL, (line_len = getline(&line_data, &line_cap, f)) > 0) {
        line.chars.data      = line_data;
        line.chars.elem_size = 1;
        line.chars.used      = line_len;
        line.chars.capacity  = line_cap;
        line.visual_width    = 0;

        while (array_len(line.chars)
        &&    ((c = *(char*)array_last(line.chars)) == '\n' || c == '\r')) {
            array_pop(line.chars);
            line_len -= 1;
        }

        for (j = 0; j < line_len;) {
            g = array_item(line.chars, j);
            line.visual_width += yed_get_glyph_width(*g);
            line.n_glyphs     += 1;
            j += yed_get_glyph_len(*g);
        }

        bucket_array_push(buff->lines, line);
    }

    if (bucket_array_len(buff->lines) > 1) {
        last_line = bucket_array_last(buff->lines);
        if (array_len(last_line->chars) == 0) {
            bucket_array_pop(buff->lines);
        }
    } else if (bucket_array_len(buff->lines) == 0) {
        line = yed_new_line();
        bucket_array_push(buff->lines, line);
    }

    return BUFF_FILL_STATUS_SUCCESS;
}

int yed_write_buff_to_file(yed_buffer *buff, char *path) {
    FILE      *f;
    yed_line  *line;
    yed_event  event;
    int        status;

    event.kind   = EVENT_BUFFER_PRE_WRITE;
    event.buffer = buff;
    yed_trigger_event(&event);

    status = BUFF_WRITE_STATUS_SUCCESS;
    errno  = 0;
    f      = fopen(path, "w");
    if (!f) {
        if (errno == EISDIR) {
            status = BUFF_WRITE_STATUS_ERR_DIR;
        } else if (errno == EACCES) {
            status = BUFF_WRITE_STATUS_ERR_PER;
        } else {
            status = BUFF_WRITE_STATUS_ERR_UNK;
        }

        errno = 0;
        return status;
    }

    /*
     * @refactor?
     * Should we be doing something smarter with this buffer size?
     */
    setvbuf(f, NULL, _IOFBF, 64 * 1024);

    bucket_array_traverse(buff->lines, line) {
        fwrite(line->chars.data, 1, array_len(line->chars), f);
        fprintf(f, "\n");
    }

    if (!buff->path) {
        buff->path    = strdup(path);
        buff->file.ft = yed_get_ft(path);
    }
    buff->kind = BUFF_KIND_FILE;

    fclose(f);

    if (status == BUFF_WRITE_STATUS_SUCCESS) {
        event.kind   = EVENT_BUFFER_POST_WRITE;
        event.buffer = buff;
        yed_trigger_event(&event);
    }

    return status;
}

void yed_range_sorted_points(yed_range *range, int *r1, int *c1, int *r2, int *c2) {
    *r1 = MIN(range->anchor_row, range->cursor_row);
    *r2 = MAX(range->anchor_row, range->cursor_row);

    if (range->kind == RANGE_NORMAL) {
        if (range->anchor_row == range->cursor_row) {
            *c1 = MIN(range->anchor_col, range->cursor_col);
            *c2 = MAX(range->anchor_col, range->cursor_col);
        } else {
            if (*r1 == range->anchor_row) {
                *c1 = range->anchor_col;
                *c2 = range->cursor_col;
            } else {
                *c1 = range->cursor_col;
                *c2 = range->anchor_col;
            }
        }
    }
}

int yed_is_in_range(yed_range *range, int row, int col) {
    int r1, c1, r2, c2;

    yed_range_sorted_points(range, &r1, &c1, &r2, &c2);

    if (row < r1 || row > r2) {
        return 0;
    }

    if (range->kind == RANGE_NORMAL) {
        if (range->anchor_row == range->cursor_row) {
            if (col < c1 || col >= c2) {
                return 0;
            }
        } else {
            if (row < r1 || row > r2) {
                return 0;
            }

            if (row == r1) {
                if (col < c1) {
                    return 0;
                }
            } else if (row == r2) {
                if (col >= c2) {
                    return 0;
                }
            }
        }
    }

    return 1;
}

void yed_buff_delete_selection(yed_buffer *buff) {
    yed_range *range;
    yed_line  *line1,
              *line2;
    yed_glyph *g;
    int        r1, c1, r2, c2,
               n, i;

    r1 = c1 = r2 = c2 = 0;

    range = &buff->selection;

    yed_range_sorted_points(range, &r1, &c1, &r2, &c2);

    if (range->kind == RANGE_LINE) {
        for (i = r1; i <= r2; i += 1) {
            yed_buff_delete_line(buff, r1);
        }
    } else if (r1 == r2) {
        for (i = c1; i < c2; i += 1) {
            yed_delete_from_line(buff, r1, c1);
        }
    } else {
        line1 = yed_buff_get_line(buff, r1);
        ASSERT(line1, "didn't get line1 in yed_buff_delete_selection()");
        n = line1->visual_width - c1 + 1;
        for (i = 0; i < n; i += 1) {
            yed_pop_from_line(buff, r1);
        }
        for (i = r1 + 1; i < r2; i += 1) {
            yed_buff_delete_line(buff, r1 + 1);
        }
        line2 = yed_buff_get_line(buff, r1 + 1);
        ASSERT(line2, "didn't get line2 in yed_buff_delete_selection()");
        n = line2->visual_width - c2 + 1;
        for (i = 0; i < n; i += 1) {
            g = yed_line_col_to_glyph(line2, c2 + i);
            yed_append_to_line(buff, r1, *g);
        }
        for (i = c2 - 1; i < n; i += 1) {
            yed_delete_from_line(buff, r1 + 1, 1);
        }
        yed_buff_delete_line(buff, r1 + 1);
    }

    if (!bucket_array_len(buff->lines)) {
        yed_buffer_add_line(buff);
    }

    buff->has_selection = 0;
}

int yed_buff_is_visible(yed_buffer *buff) {
    yed_frame **frame_it;

    array_traverse(ys->frames, frame_it) {
        if ((*frame_it)->buffer == buff) {
            return 1;
        }
    }

    return 0;
}

void yed_update_line_visual_widths(void) {
    tree_it(yed_buffer_name_t, yed_buffer_ptr_t)  bit;
    yed_buffer                                   *buff;
    yed_line                                     *line;
    yed_glyph                                    *glyph;

    tree_traverse(ys->buffers, bit) {
        buff = tree_it_val(bit);
        bucket_array_traverse(buff->lines, line) {
            line->visual_width = 0;
            yed_line_glyph_traverse(*line, glyph) {
                line->visual_width += yed_get_glyph_width(*glyph);
            }
        }
    }
}
