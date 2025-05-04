#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <ncurses.h>
#include <pwd.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#define CMD_MAX 16384
#define KEY_ALT_ENTER    0507  /* custom alt-enter key */
#define KEY_ALT_a        0506  /* custom alt-a key */
#define KEY_ALT_s        0505  /* custom alt-s key */
#define KEY_SHIFT_F7     0504  /* custom shift+f7 key */

typedef enum {
    SORT_BY_NAME_ASC = 0,
    SORT_BY_SIZE_ASC,
    SORT_BY_TIME_ASC,
    SORT_BY_NAME_DESC,
    SORT_BY_SIZE_DESC,
    SORT_BY_TIME_DESC,
    SORT_BY_NAME_DIRSFIRST_ASC,
    SORT_BY_SIZE_DIRSFIRST_ASC,
    SORT_BY_TIME_DIRSFIRST_ASC,
    SORT_BY_NAME_DIRSFIRST_DESC,
    SORT_BY_SIZE_DIRSFIRST_DESC,
    SORT_BY_TIME_DIRSFIRST_DESC
} SortOrders;

typedef struct FileNode {
    char* name;
    time_t mtime;
    off_t size;
    mode_t chmod;
    uid_t chown;
    int is_dir;
    int is_executable;
    int is_link;
    int is_link_to_dir;  // link points to a directory
    int is_link_broken; // invalid link
    int is_device;
    char* link_target;
    int is_selected; // with Insert key
    struct FileNode *next;
} FileNode;

typedef struct PanelProp {
    int selected_index;
    int scroll_index;
    SortOrders sort_order;
    char path[CMD_MAX];
    char file_under_cursor[CMD_MAX];
    int num_selected_files;
    off_t bytes_selected_files;
    int files_count;
    int search_mode;
    char search_text[CMD_MAX];
    char prev_search_text[CMD_MAX];
    FileNode *files;
} PanelProp;

typedef struct file_lines {
    char *line;
    int line_length;
    struct file_lines *next;
} file_lines;

enum Color {
    COLOR_WHITE_ON_BLACK = 1,
    COLOR_WHITE_ON_BLUE,
    COLOR_WHITE_ON_RED,
    COLOR_BLACK_ON_WHITE,
    COLOR_BLACK_ON_CYAN,
    COLOR_BLACK_ON_CYAN_BTN,
    COLOR_BLACK_ON_CYAN_PMPT,
    COLOR_YELLOW_ON_CYAN,
    COLOR_YELLOW_ON_BLUE,
    COLOR_GREEN_ON_BLUE,
    COLOR_MAGENTA_ON_BLUE,
    COLOR_CYAN_ON_BLUE,
    COLOR_CYAN_ON_BLACK,
    COLOR_RED_ON_BLUE
};


typedef struct operationContext {
    off_t current_size;
    off_t current_items;
    off_t total_size;
    off_t total_items;
    int confirm_all_yes;
    int confirm_all_no;
    int skip_all;
    int keep_item_selected;
    char confirm_yes_prefix[CMD_MAX];
    int abort;
} operationContext;

enum operationResult {
    OPERATION_OK = 0,
    OPERATION_RETRY,
    OPERATION_PARENT_OK_PROCESS_CHILDS,
    OPERATION_RETRY_AFTER_CHILDS,
    OPERATION_SKIP,
    OPERATION_ABORT
};


typedef int (*OperationFunc)(const char *, const char *, operationContext *);


// Define a struct to pair regex patterns with their associated colors.
typedef struct {
    char *pattern;
    int color_pair;
    int is_bold;
    regex_t regex;
} PatternColorPair;


 PanelProp left_panel;
 PanelProp right_panel;
 PanelProp* active_panel;

// Global windows
 WINDOW *win1;
 WINDOW *win2;
 WINDOW *progress;

 struct utsname unameData;
 struct passwd *pw;
 const char *username;

 struct timeval last_click_time;
 struct timeval current_time;
 struct timeval diff_time;

 int cursor_pos;
 int cmd_offset;
 int prompt_length;

 char cmd[CMD_MAX];
 int cmd_len;

 int color_enabled;
#ifndef GLOBALS_H
#define GLOBALS_H

void initialize_ncurses(void);
void draw_buttons(int maxY, int maxX);
void draw_windows(int maxY, int maxX);
void shorten(char *name, int width, char *result);
void update_panel(WINDOW *win, PanelProp *panel);
void update_panel_cursor(void);
void init_screen(void);
void cleanup(void);
void redraw_ui(void);
int compare_nodes(FileNode *a, FileNode *b, SortOrders sort_order);
void sort_file_nodes(FileNode **head_ref, SortOrders sort_order);
int update_panel_files(PanelProp *panel);
void update_files_in_both_panels(void);
void free_file_nodes(FileNode *head);
int lines(char * title);
WINDOW *create_dialog(char *title, char *buttons[], int prompt_is_present, int is_danger, int vertical_buttons);
void update_dialog_buttons(WINDOW *win, char * title, char *buttons[], int selected, int prompt_present, int editing_prompt, int is_danger, int vertical_buttons);
int show_dialog(char *title, char *buttons[], int selected, char *prompt, int is_danger, int vertical_buttons);
void show_errormsg(char * msg);
void cursor_to_cmd(void);
void update_cmd(void);
void display_line(WINDOW *win, file_lines *line, int max_x, int current_col, int editor_mode, PatternColorPair* patterns, int num_patterns);
int view_file(char *filename);
int edit_file(char *filename);
int file_has_extension(const char *filename, const char *extensions[]);
void dive_into_directory(FileNode *current);
int noesc(int ch);
void format_size_with_units(off_t size, char *size_str, size_t len, int maxlen);
void show_shadow(WINDOW *win);
void dialog_save_screen();
void dialog_restore_screen();
void create_progress_dialog(int title_lines);
int file_exists(const char *path);
int update_progress_dialog(char *title, int current_progress, int total_progress, char *infotext);
int update_progress_dialog_delta(char *title, int current_progress, int total_progress, char *infotext);
int panel_mass_action(OperationFunc func, char *tgt, operationContext *context);
int recursive_operation(const char *src, const char *tgt, operationContext *context, OperationFunc func);
int copy_operation(const char *src, const char *tgt, operationContext *context);
int move_operation(const char *src, const char *tgt, operationContext *context);
int delete_operation(const char *src, const char *tgt, operationContext *context);
int countstats_operation(const char *src, const char *tgt, operationContext *context);
int mkdir_recursive(const char *path, mode_t mode);
int format_number(off_t num, char *str);

// Macro to use shorten inline
#define SHORTEN(name, width) ({ \
    static char result_buf[CMD_MAX] = {0}; \
    shorten((name), (width), result_buf); \
    result_buf; \
})

#define SPRINTF(fmt, ...) ({ \
    char tmp[CMD_MAX]; \
    sprintf(tmp, fmt, ##__VA_ARGS__); \
    tmp; \
})

#endif // GLOBALS_H

void cursor_to_cmd() {
    // move cursor where it belongs
    move(LINES - 2, prompt_length + cursor_pos - cmd_offset);
    curs_set(1);
}

void update_cmd() {

    attron(COLOR_PAIR(COLOR_WHITE_ON_BLACK));

    // Print username, hostname, and current directory path
    move(LINES - 2, 0);
    clrtoeol();
    printw("%s@%s:%s# ", username, unameData.nodename, active_panel->path);

    // Calculate max command display length
    prompt_length = strlen(username) + strlen(unameData.nodename) + strlen(active_panel->path) + 4;  // 5 accounts for '@', ':', '#', and spaces.
    int max_cmd_display = COLS - prompt_length;

    // Print the visible part of the command, limited to max_cmd_display characters
    printw("%.*s", max_cmd_display, cmd + cmd_offset);

    cursor_to_cmd();

    // Refresh only the changed parts
    refresh();

    return;
}


int lines(char * title)
{
    if (title == NULL) return 0;
    int newlines = 0;
    for (int i = 0; title[i]; i++) {
        if (title[i] == '\n') {
            newlines++;
        }
    }
    if (strlen(title) != 0) newlines++;
    return newlines;
}

void show_shadow(WINDOW *win) {
    int start_y, start_x, height, width;

    int cur_y, cur_x;
    getyx(win, cur_y, cur_x);

    // Get the position and size of the window
    getbegyx(win, start_y, start_x);
    getmaxyx(win, height, width);

    WINDOW *wholescreen = dupwin(newscr);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < 2; j++) {
            int x = start_x + width + j;
            int y = start_y + i + 1;
            chtype ch = mvwinch(wholescreen, y, x);
            mvaddch(y, x, (ch & A_CHARTEXT));
        }
    }

    for (int i = 2; i < width + 2; i++) {
        int x = start_x + i;
        int y = start_y + height;
        chtype ch = mvwinch(wholescreen, y, x);
        mvaddch(y, x, (ch & A_CHARTEXT));
    }

    delwin(wholescreen);
    wmove(win, cur_y, cur_x);
    refresh();
}


// Function to create a dialog window with a title, buttons, and an optional text prompt
WINDOW *create_dialog(char *title, char *buttons[], int prompt_is_present, int is_danger, int vertical_buttons) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int total_buttons = 0;
    int total_button_width = 6;
    while (buttons[total_buttons] != NULL) {
        total_button_width += strlen(buttons[total_buttons]) + 4;
        total_buttons++;
    }
    total_button_width += 2 * (total_buttons - 1);

    int title_width = 0;
    char *title_copy = strdup(title);
    char *line = strtok(title_copy, "\n");
    while (line) {
        int line_length = strlen(line) + 4;
        title_width = line_length > title_width ? line_length : title_width;
        line = strtok(NULL, "\n");
    }
    free(title_copy);

    int width;
    if (vertical_buttons) {
        int longest_button_width = 0;
        for (int i = 0; buttons[i] != NULL; i++) {
            int button_width = strlen(buttons[i]) + 10;
            if (button_width > longest_button_width) {
                longest_button_width = button_width;
            }
        }
        width = title_width > longest_button_width ? title_width: longest_button_width;
    } else {
        width = total_button_width > title_width ? total_button_width : title_width;
        if (!is_danger) {
            width = width < max_x / 3 ? max_x / 3 : width;
        }
    }

    width = width + 2;

    int height = (prompt_is_present ? 6 : 5) + lines(title) + 1;
    if (vertical_buttons) {
        height += total_buttons - 1; // Increase height by total_buttons - 1 if vertical layout
    }

    int start_y = (max_y - height) / 2 - (is_danger ? 8 : 0);
    int start_x = (max_x - width) / 2;

    // Increase the size of the window by 2 in both dimensions
    WINDOW *win = newwin(height, width, start_y, start_x);

    if (is_danger) {
        wbkgd(win, COLOR_PAIR(COLOR_WHITE_ON_RED));
        wattron(win, COLOR_PAIR(COLOR_WHITE_ON_RED));
        wattron(win, A_BOLD);
    } else {
        wbkgd(win, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
        wattron(win, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    }

    show_shadow(win);

    mvwaddch(win, 1, 1, '+'); // Top left corner
    mvwaddch(win, 1, width - 2, '+'); // Top right corner
    mvwaddch(win, height - 2, 1, '+'); // Bottom left corner
    mvwaddch(win, height - 2, width - 2, '+'); // Bottom right corner
    mvwhline(win, 1, 2, '-', width - 4); // Top border
    mvwhline(win, height - 2, 2, '-', width - 4); // Bottom border
    mvwvline(win, 2, 1, '|', height - 4); // Left border
    mvwvline(win, 2, width - 2, '|', height - 4); // Right border
    mvwhline(win, height - 4 - (vertical_buttons ? total_buttons - 1: 0), 2, '-', width - 4); // Horizontal line above buttons
    mvwaddch(win, height - 4 - (vertical_buttons ? total_buttons - 1: 0), 1, '+'); // Left intersection
    mvwaddch(win, height - 4 - (vertical_buttons ? total_buttons - 1: 0), width - 2, '+'); // Right intersection

    int title_line = 2;
    title_copy = strdup(title);
    line = strtok(title_copy, "\n");
    while (line) {
        mvwprintw(win, title_line, 3, "%s", line);
        line = strtok(NULL, "\n");
        title_line++;
    }
    free(title_copy);

    return win;
}


void update_dialog_buttons(WINDOW *win, char * title, char *buttons[], int selected, int prompt_present, int editing_prompt, int is_danger, int vertical_buttons) {
    int width, height;
    getmaxyx(win, height, width);

    int total_buttons_width = 0;
    int i = 0;
    while (buttons[i] != NULL) {
        total_buttons_width += strlen(buttons[i]) + 4;
        i++;
    }
    total_buttons_width += 2 * (i - 1);

    int cursor_pos = (width - total_buttons_width) / 2;
    if (vertical_buttons) cursor_pos = 3;
    int move_cursor_pos_x = 0;
    int move_cursor_pos_y = 0;

    // Adjust the y position based on whether a prompt is present
    int y_pos = prompt_present ? 4 : 3;
    i = 0;
    while (buttons[i] != NULL) {
        if (i == selected && !editing_prompt) {
            if (is_danger) {
                wattron(win, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
                wattroff(win, A_BOLD);
            } else {
                wattron(win, COLOR_PAIR(COLOR_BLACK_ON_CYAN_BTN));
            }
            move_cursor_pos_x = cursor_pos + 2;
            move_cursor_pos_y = y_pos;
        }
        if (vertical_buttons) {
            mvwprintw(win, y_pos + lines(title), cursor_pos, " [%s] %-*s ", i == selected && !editing_prompt ? "x" : " ", width - 12, buttons[i]);
        } else {
            mvwprintw(win, y_pos + lines(title), cursor_pos, "[ %s ]", buttons[i]);
        }
        if (i == selected && !editing_prompt) {
            if (is_danger) {
                wattron(win, COLOR_PAIR(COLOR_WHITE_ON_RED));
                wattron(win, A_BOLD);
            } else {
                wattron(win, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
            }
        }

        if (vertical_buttons) {
            y_pos += 1; // Move to the next line for vertical layout
        } else {
            cursor_pos += strlen(buttons[i]) + 6;
        }
        i++;
    }

    if (move_cursor_pos_x > 0 || move_cursor_pos_y > 0) {
        wmove(win, move_cursor_pos_y + lines(title), move_cursor_pos_x);
    }

    wrefresh(win);
}

WINDOW *dialog_saved_screen;

void dialog_save_screen() {
   dialog_saved_screen = dupwin(newscr);
}

void dialog_restore_screen() {
   overwrite(dialog_saved_screen, newscr);
   wrefresh(newscr);
   delwin(dialog_saved_screen);
}


int show_dialog(char *title, char *buttons[], int selected, char *prompt, int is_danger, int vertical_buttons) {
    int prompt_is_present = prompt ? 1 : 0;
    int editing_prompt = prompt ? 1 : 0;

    if (!prompt_is_present) {
        prompt = "";
    }

    dialog_save_screen();

    WINDOW *win = create_dialog(title, buttons, prompt_is_present, is_danger, vertical_buttons);
    update_dialog_buttons(win, title, buttons, selected, prompt_is_present, editing_prompt, is_danger, vertical_buttons);

    int buttons_count = 0;
    while (buttons[buttons_count] != NULL) {
        buttons_count++;
    }

    int ch;
    int cursor_position = strlen(prompt);
    int prompt_offset = 0;
    int width, height;
    getmaxyx(win, height, width);
    int max_prompt_display = width - 6;
    int prompt_modified = 0;

    while (1) {
        if (editing_prompt) {
            if (cursor_position - prompt_offset >= max_prompt_display) {
                prompt_offset = cursor_position - max_prompt_display + 1;
            } else if (cursor_position < prompt_offset) {
                prompt_offset = cursor_position;
            }
            wattron(win, COLOR_PAIR(COLOR_BLACK_ON_CYAN_PMPT));
            if (!prompt_modified) wattron(win, A_BOLD);
            mvwprintw(win, 2 + lines(title), 3, "%-*.*s", max_prompt_display, max_prompt_display, prompt + prompt_offset);
            wattron(win, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
            if (!prompt_modified) wattroff(win, A_BOLD);
            wmove(win, 2 + lines(title), 3 + cursor_position - prompt_offset);
        }
        wrefresh(win);

        ch = noesc(getch());
        switch (ch) {
            case KEY_LEFT:
                if (editing_prompt && cursor_position > 0) {
                    cursor_position--;
                    prompt_modified = 1;
                } else if (!editing_prompt) {
                    if (selected > 0) {
                        selected--;
                    } else {
                        selected = buttons_count - 1;
                    }
                }
                break;
            case KEY_RIGHT:
                if (editing_prompt) {
                    if (cursor_position < strlen(prompt)) {
                        cursor_position++;
                    }
                    prompt_modified = 1;
                } else if (!editing_prompt) {
                    if (buttons[selected + 1] != NULL) {
                        selected++;
                    } else {
                        selected = 0;
                    }
                }
                break;
            case KEY_BACKSPACE:
                if (editing_prompt && cursor_position > 0) {
                    memmove(&prompt[cursor_position - 1], &prompt[cursor_position], strlen(prompt) - cursor_position + 1);
                    cursor_position--;
                    prompt_modified = 1;
                }
                break;
            case KEY_DC: // Handling the Del key
               if (editing_prompt && cursor_position < strlen(prompt)) {
                   memmove(&prompt[cursor_position], &prompt[cursor_position + 1], strlen(prompt) - cursor_position);
                   prompt_modified = 1;
               }
               break;
            case KEY_F(10):
            case 27:
                dialog_restore_screen();
                return -1;
                break;
            case KEY_UP:
            case KEY_BTAB:
                if (editing_prompt) {
                    editing_prompt = 0;
                    selected = buttons_count - 1;
                } else {
                    if (selected > 0) {
                        selected --;
                    } else {
                        if (prompt_is_present) {
                            editing_prompt = 1;
                        } else {
                            selected = buttons_count - 1;
                        }
                    }
                }
                break;
            case KEY_DOWN:
            case '\t':
                if (editing_prompt) {
                    editing_prompt = 0;
                    selected = 0;
                } else {
                    if (buttons[selected + 1] != NULL) {
                        selected++;
                    } else {
                        if (prompt_is_present) {
                            editing_prompt = 1;
                        } else {
                            selected = 0;
                        }
                    }
                }
                break;
            case KEY_HOME:
                if (editing_prompt) {
                    cursor_position = 0;
                    prompt_modified = 1;
                }
                break;
            case KEY_END:
                if (editing_prompt) {
                    cursor_position = strlen(prompt);
                    prompt_modified = 1;
                }
                break;
            case '\n':
                if (editing_prompt) {
                    selected = 0;
                }
                delwin(win);
                dialog_restore_screen();
                return selected + 1;
                break;
            default:
                if (editing_prompt && isprint(ch) && strlen(prompt) < CMD_MAX) {
                    if (!prompt_modified) { // Check if the prompt is not modified
                        strcpy(prompt, ""); // Clear the prompt
                        cursor_position = 0; // Reset the cursor position
                        prompt_modified = 1; // Set the flag to indicate the prompt is modified
                    }
                    memmove(&prompt[cursor_position + 1], &prompt[cursor_position], strlen(prompt) - cursor_position + 1);
                    prompt[cursor_position] = ch;
                    cursor_position++;
                    prompt_modified = 1;
                }
                break;
        }
        update_dialog_buttons(win, title, buttons, selected, prompt_is_present, editing_prompt, is_danger, vertical_buttons);
    }
}


void show_errormsg(char * msg) {
    show_dialog(msg, (char *[]) {"OK", NULL}, 0, NULL, 1, 0);
}

int compare_nodes(FileNode *a, FileNode *b, SortOrders sort_order) {
    int result = 0;
    int dirs_first = sort_order >= SORT_BY_NAME_DIRSFIRST_ASC;

    // Check if either node is ".."
    if (strcmp(a->name, "..") == 0) return -1;
    if (strcmp(b->name, "..") == 0) return 1;

    if (dirs_first && (a->is_dir != b->is_dir)) {
        return a->is_dir ? -1 : 1;
    }

    switch (sort_order % 6) {  // 6 because there are 6 basic sort types
        case SORT_BY_NAME_ASC:
        case SORT_BY_NAME_DESC:
            result = strcmp(a->name, b->name);
            break;
        case SORT_BY_SIZE_ASC:
        case SORT_BY_SIZE_DESC:
            result = (a->size > b->size) - (a->size < b->size);
            break;
        case SORT_BY_TIME_ASC:
        case SORT_BY_TIME_DESC:
            result = (a->mtime > b->mtime) - (a->mtime < b->mtime);
            break;
    }

    // DESC sort? revert result
    if (sort_order == SORT_BY_NAME_DESC || sort_order == SORT_BY_SIZE_DESC || sort_order == SORT_BY_TIME_DESC ||
        sort_order == SORT_BY_NAME_DIRSFIRST_DESC || sort_order == SORT_BY_SIZE_DIRSFIRST_DESC || sort_order == SORT_BY_TIME_DIRSFIRST_DESC) {
        result = -result;
    }

    return result;
}


void sort_file_nodes(FileNode **head_ref, SortOrders sort_order) {
    FileNode *sorted = NULL;
    FileNode *current = *head_ref;

    while (current != NULL) {
        FileNode *next = current->next;

        if (sorted == NULL || compare_nodes(current, sorted, sort_order) <= 0) {
            current->next = sorted;
            sorted = current;
        } else {
            FileNode *temp = sorted;
            while (temp->next != NULL && compare_nodes(current, temp->next, sort_order) > 0) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
        }

        current = next;
    }

    *head_ref = sorted;
}


int update_panel_files(PanelProp *panel) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    struct stat link_stat;
    FileNode *head = NULL, *current = NULL, *original_head = NULL;

    original_head = panel->files;
    panel->files = NULL;
    panel->files_count = 0;
    panel->num_selected_files = 0;
    panel->bytes_selected_files = 0;

    if ((dir = opendir(panel->path)) == NULL) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0 && strcmp(panel->path, "/") == 0) continue;

        char full_path[CMD_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", panel->path, entry->d_name);
        lstat(full_path, &file_stat);

        FileNode *new_node = (FileNode*) calloc(1,sizeof(FileNode));

        panel->files_count++;
        new_node->next = NULL;
        new_node->name = strdup(entry->d_name);

        new_node->mtime = file_stat.st_mtime;
        new_node->size = file_stat.st_size;
        new_node->chmod = file_stat.st_mode;
        new_node->chown = file_stat.st_uid;
        new_node->is_dir = S_ISDIR(file_stat.st_mode);
        new_node->is_executable = (file_stat.st_mode & S_IXUSR) || (file_stat.st_mode & S_IXGRP) || (file_stat.st_mode & S_IXOTH);
        new_node->is_link = S_ISLNK(file_stat.st_mode);
        new_node->is_link_broken = 0;
        new_node->is_link_to_dir = 0;
        new_node->is_device = S_ISBLK(file_stat.st_mode) || S_ISCHR(file_stat.st_mode);

        if (new_node->is_link) {
            new_node->link_target = NULL;
            char target[CMD_MAX];
            ssize_t len = readlink(full_path, target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                new_node->link_target = strdup(target);
            }

            if (stat(full_path, &link_stat) != 0) {
                new_node->is_link_broken = 1;  // Link is broken
            } else {
                new_node->is_link_to_dir = S_ISDIR(link_stat.st_mode);
            }
        }

        if (new_node->is_link_to_dir) new_node->is_dir = 1;

        // Check if this file was selected in the original list
        FileNode *old_node = original_head;
        while (old_node != NULL) {
            if (old_node->is_selected && strcmp(new_node->name, old_node->name) == 0) {
                new_node->is_selected = true;
                panel->num_selected_files++;
                if (!new_node->is_dir) panel->bytes_selected_files+=new_node->size;
                break;
            }
            old_node = old_node->next;
        }

        if (head == NULL) {
            head = new_node;
            current = head;
        } else {
            current->next = new_node;
            current = new_node;
        }
    }

    closedir(dir);
    panel->files = head;

    free_file_nodes(original_head);

    return panel->files_count;
}

void free_file_nodes(FileNode *head) {
    FileNode *tmp;
    while (head != NULL) {
        free(head->name);
        free(head->link_target);
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

void dive_into_directory(FileNode *current) {
   if (strcmp(current->name, "..") == 0) {
       // Store the last directory name before going up
       char * last_slash = strrchr(active_panel->path, '/');
       strncpy(active_panel->file_under_cursor, last_slash + 1, CMD_MAX - 1);

       // Go back to upper dir
       last_slash = strrchr(active_panel->path, '/');
       int is_root = (last_slash == active_panel->path);
       memset(last_slash + is_root, 0, strlen(last_slash));
   } else {
       // Dive into the selected directory
       if (strlen(active_panel->path) > 1) strcat(active_panel->path, "/");
       strcat(active_panel->path, current->name);
       active_panel->file_under_cursor[0] = '\0';
   }

   free_file_nodes(active_panel->files);
   active_panel->files = NULL;
   active_panel->num_selected_files = 0;
   active_panel->bytes_selected_files = 0;
   active_panel->files_count = 0;

   // Update the file list for the new directory
   update_panel_files(active_panel);
   sort_file_nodes(&active_panel->files, active_panel->sort_order);
   update_panel_cursor();
}



SCREEN *screen = NULL;

void initialize_ncurses() {
    if (screen) return;

    const char *terms[] = {NULL, "xterm", "xfce", "linux"};
    screen = NULL;
    for (int i = 0; i < 4 && screen == NULL; ++i) {
        screen = newterm(terms[i], stdout, stdin);
    }

    if (screen == NULL) {
        // last attempt
        initscr();
    }
}


void init_screen() {
    initialize_ncurses();
    refresh();
    mouseinterval(50);
    ESCDELAY = 50;
    start_color();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);

    if (color_enabled) {
        init_pair(COLOR_WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_WHITE_ON_RED, COLOR_WHITE, COLOR_RED);
        init_pair(COLOR_WHITE_ON_BLUE, COLOR_WHITE, COLOR_BLUE);
        init_pair(COLOR_YELLOW_ON_BLUE, COLOR_YELLOW, COLOR_BLUE);
        init_pair(COLOR_GREEN_ON_BLUE, COLOR_GREEN, COLOR_BLUE);
        init_pair(COLOR_RED_ON_BLUE, COLOR_RED, COLOR_BLUE);
        init_pair(COLOR_MAGENTA_ON_BLUE, COLOR_MAGENTA, COLOR_BLUE);
        init_pair(COLOR_CYAN_ON_BLUE, COLOR_CYAN, COLOR_BLUE);
        init_pair(COLOR_CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_YELLOW_ON_CYAN, COLOR_YELLOW, COLOR_CYAN);
        init_pair(COLOR_BLACK_ON_CYAN, COLOR_BLACK, COLOR_CYAN);
        init_pair(COLOR_BLACK_ON_CYAN_BTN, COLOR_BLACK, COLOR_CYAN);
        init_pair(COLOR_BLACK_ON_CYAN_PMPT, COLOR_BLACK, COLOR_CYAN);
    } else { // black and white mode
        init_pair(COLOR_WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_WHITE_ON_RED, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_WHITE_ON_BLUE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_YELLOW_ON_BLUE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_GREEN_ON_BLUE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_RED_ON_BLUE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_MAGENTA_ON_BLUE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_CYAN_ON_BLUE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_CYAN_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_YELLOW_ON_CYAN, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_BLACK_ON_CYAN, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_BLACK_ON_CYAN_BTN, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_BLACK_ON_CYAN_PMPT, COLOR_WHITE, COLOR_BLACK);
    }
}

void cleanup() {
    delwin(win1);
    delwin(win2);
    endwin();
}

void redraw_ui() {
   // Get screen dimensions
   int maxY, maxX;
   getmaxyx(stdscr, maxY, maxX);

   draw_windows(maxY, maxX);
   draw_buttons(maxY, maxX);
   update_cmd();
   refresh();
}

PanelProp left_panel = {0};
PanelProp right_panel = {0};
PanelProp* active_panel = &left_panel;

// Global windows
WINDOW *win1;
WINDOW *win2;

struct utsname unameData;
struct passwd *pw;
const char *username;

struct timeval last_click_time = {0};
struct timeval current_time = {0};
struct timeval diff_time = {0};

int cursor_pos = 0;
int cmd_offset = 0;
int prompt_length = 0;

char cmd[CMD_MAX] = {0};
int cmd_len = 0;

int color_enabled = 1;

int noesc(int ch) {

    // some special cases, who knows why
    if (ch == 362) return KEY_HOME;
    if (ch == 385) return KEY_END;

    int num = 0;
    if (ch == 27) {  // Escape character
        ch = getch();
        while (ch == '[') {  // Discard the '[' character
            ch = getch();
        }

        if (ch == 10) return KEY_ALT_ENTER;
        if (ch == 'a') return KEY_ALT_a;
        if (ch == 's') return KEY_ALT_s;

        while (ch >= '0' && ch <= '9') {  // Read numbers
            num = num * 10 + (ch - '0');
            ch = getch();
        }

        if (ch == '~') {
            switch (num) {
                case 1:
                    ch = KEY_HOME;
                    break;
                case 2:
                    ch = KEY_IC;
                    break;
                case 4:
                    ch = KEY_END;
                    break;
                case 12:
                    ch = KEY_F(2);
                    break;
                case 13:
                    ch = KEY_F(3);
                    break;
                case 14:
                    ch = KEY_F(4);
                    break;
                case 31:
                    ch = KEY_SHIFT_F7;
                    break;
                default:
                    break;
            }
        }
    }

    return ch;
}


int main(int argc, char *argv[]) {

    // Define the long options
    static struct option long_options[] = {
        {"nocolor", no_argument, 0, 'b'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    // parse commandline arguments
    while ((opt = getopt_long(argc, argv, "bhv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'b':
                color_enabled = 0;
                break;
            case 'h':
                fprintf(stderr, "Mini Commander (c) 2023 Tomas Matejicek + ChatGPT\n");
                fprintf(stderr, "Usage: %s [-b|--nocolor] [-h|--help]\n", argv[0]);
                return 1;
                break;
            case 'v':
                fprintf(stderr, "Version 1.1\n");
                return 1;
                break;
        }
    }


    getcwd(left_panel.path, sizeof(left_panel.path));
    strcpy(right_panel.path, left_panel.path);

    left_panel.sort_order = SORT_BY_NAME_DIRSFIRST_ASC;
    right_panel.sort_order = SORT_BY_NAME_DIRSFIRST_ASC;

    update_files_in_both_panels();

    init_screen();

    MEVENT event;

    uname(&unameData);
    pw = getpwuid(getuid());
    username = pw->pw_name;

    mousemask(ALL_MOUSE_EVENTS, NULL);
    redraw_ui(); // initial screen

    while (1) {
        update_cmd();

        // Print file names in left and right windows
        update_panel(win1, &left_panel);
        update_panel(win2, &right_panel);
        int visible_items = getmaxy(win1) - 5;

        // get current file under cursor
        FileNode *current = active_panel->files;
        int index = 0;
        while (current != NULL && index < active_panel->selected_index) {
            current = current->next;
            index++;
        }

        memset(active_panel->file_under_cursor, 0, CMD_MAX);
        strncpy(active_panel->file_under_cursor, current->name, strlen(current->name));
        chdir(active_panel->path);

        int ch = noesc(getch());

        if (ch == 0) { // Ctrl+Space
            // TODO: fix when files are selected
            // TODO: fix when cursor is at ..
            operationContext stats = {0};
            panel_mass_action(countstats_operation, "", &stats);
            if (stats.abort != 1) {
                current->size = stats.total_size;
            }
        }

        if (ch == KEY_F(2)) { // F2
            int sort = show_dialog("Sort files and directories by:", (char *[]) {
            "Sort by name, from a to z, mix dirs",
            "Sort by size, from small to big, mix dirs",
            "Sort by modify time, from old to new, mix dirs",
            "Sort by name, from z to a, mix dirs",
            "Sort by size, from big to small, mix dirs",
            "Sort by modify time, from new to old, mix dirs",
            "Sort by name, from a to z, dirs first",
            "Sort by size, from small to big, dirs first",
            "Sort by modify time, from old to new, dirs first",
            "Sort by name, from z to a, dirs first",
            "Sort by size, from big to small, dirs first",
            "Sort by modify time, from new to old, dirs first", NULL}, active_panel->sort_order, NULL, 0, 1);
            if (sort != -1) active_panel->sort_order = sort - 1;
            update_files_in_both_panels();
        }

        if (ch == KEY_F(3)) { // F3
            if (current) {
                if (current->is_dir) {
                    dive_into_directory(current);
                } else {
                    char file[CMD_MAX] = {};
                    sprintf(file, "%s/%s", active_panel->path, active_panel->file_under_cursor);
                    view_file(file);
                    redraw_ui();
                }
            }
        }


        if (ch == KEY_F(4)) { // F4
            if (current) {
                if (current->is_dir) {
                    dive_into_directory(current);
                } else {
                    char file[CMD_MAX] = {};
                    sprintf(file, "%s/%s", active_panel->path, active_panel->file_under_cursor);
                    edit_file(file);
                    redraw_ui();
                    update_files_in_both_panels();
               }
           }
        }

        if (ch == KEY_F(5)) { // F5
            if (active_panel->num_selected_files == 0 && strcmp(active_panel->file_under_cursor, "..") == 0) {
                show_errormsg("Cannot operate on \"..\"");
                continue;
            }
            char title[CMD_MAX] = {0};
            char prompt[CMD_MAX] = {0};
            sprintf(prompt, "%s", active_panel == &left_panel ? right_panel.path : left_panel.path);
            sprintf(title, "Copy %d file%s/director%s to:", active_panel->num_selected_files > 0 ? active_panel->num_selected_files : 1, active_panel->num_selected_files > 1 ? "s" : "", active_panel->num_selected_files > 1 ? "ies" : "y");
            int btn = show_dialog(title, (char *[]) {"OK", "Cancel", NULL}, 0, prompt, 0, 0);
            if (btn == 1) {
                operationContext stats = {0};
                operationContext context = {0};
                panel_mass_action(countstats_operation, "", &stats);
                if (stats.abort != 1) {
                    context.total_items = stats.total_items;
                    context.total_size =  stats.total_size;
                    panel_mass_action(copy_operation, prompt, &context);
                }
            }
            update_files_in_both_panels();
        }

        if (ch == KEY_F(6)) { // F6
            if (active_panel->num_selected_files == 0 && strcmp(active_panel->file_under_cursor, "..") == 0) {
                show_errormsg("Cannot operate on \"..\"");
                continue;
            }
            char title[CMD_MAX] = {0};
            char prompt[CMD_MAX] = {0};
            sprintf(prompt, "%s", active_panel == &left_panel ? right_panel.path : left_panel.path);
            sprintf(title, "Move %d file%s/director%s to:", active_panel->num_selected_files > 0 ? active_panel->num_selected_files : 1, active_panel->num_selected_files > 1 ? "s" : "", active_panel->num_selected_files > 1 ? "ies" : "y");
            int btn = show_dialog(title, (char *[]) {"OK", "Cancel", NULL}, 0, prompt, 0, 0);
            if (btn == 1) {
                operationContext stats = {0};
                operationContext context = {0};
                panel_mass_action(countstats_operation, "", &stats);
                if (stats.abort != 1) {
                    context.total_items = stats.total_items;
                    context.total_size =  stats.total_size;
                    panel_mass_action(move_operation, prompt, &context);
                }
            }
            update_files_in_both_panels();
        }

        if (ch == KEY_F(7)) { // F7
            char title[CMD_MAX] = {0};
            char prompt[CMD_MAX] = {0};
            if (strcmp(active_panel->file_under_cursor, "..") != 0) {
                sprintf(prompt, "%s", active_panel->file_under_cursor);
            }
            sprintf(title, "Enter directory name to create:");
            int btn = show_dialog(title, (char *[]) {"OK", "Cancel", NULL}, 0, prompt, 0, 0);
            if (btn == 1 && strlen(prompt) > 0) {
                int err = mkdir_recursive(prompt, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
                if (!err) {
                    // Check if prompt is relative (doesn't start with '/')
                    if (prompt[0] != '/') {
                        sprintf(active_panel->file_under_cursor, "%s", prompt);
                    }
                    // Check if prompt starts with active_panel->path
                    else if (strncmp(prompt, active_panel->path, strlen(active_panel->path)) == 0) {
                        // Fill only the remaining part
                        sprintf(active_panel->file_under_cursor, "%s", prompt + strlen(active_panel->path) + 1);
                    }

                    char *slash_position = strchr(active_panel->file_under_cursor, '/');
                    if (slash_position) {
                        *slash_position = '\0';
                    }
                } else {
                    show_errormsg(SPRINTF("Operation failed\n%s (%d)", strerror(err), err));
                }
                update_files_in_both_panels();
            }
        }

        if (ch == KEY_F(8)) {
            if (active_panel->num_selected_files == 0 && strcmp(active_panel->file_under_cursor, "..") == 0) {
                show_errormsg("Cannot operate on \"..\"");
                continue;
            }

            char title[CMD_MAX] = {};
            sprintf(title, "Delete %d file%s/director%s?", active_panel->num_selected_files > 0 ? active_panel->num_selected_files : 1, active_panel->num_selected_files > 1 ? "s" : "", active_panel->num_selected_files > 1 ? "ies" : "y");
            int btn = show_dialog(title, (char *[]) {"Yes", "No", NULL}, 0, NULL, 1, 0);

            if (btn == 1) {
                operationContext stats = {0};
                operationContext context = {0};
                panel_mass_action(countstats_operation, "", &stats);
                if (stats.abort != 1) {
                    context.total_items = stats.total_items;
                    context.total_size =  stats.total_size;
                    panel_mass_action(delete_operation, "", &context);
                }
            }
            update_files_in_both_panels();
            redraw_ui();
        }

        if (ch == KEY_F(10)) {
            break;
        }


        if (ch == KEY_RESIZE) {  // Handle terminal resize
            endwin();
            init_screen();
            redraw_ui();
        }

        if (ch == KEY_MOUSE) { // handle mouse events
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_PRESSED) {
                    // Determine which window was clicked and set the active panel
                    if (wenclose(win1, event.y, event.x)) {
                        active_panel = &left_panel;
                    } else if (wenclose(win2, event.y, event.x)) {
                        active_panel = &right_panel;
                    }

                    // Select item by mouse click
                    int index = active_panel->scroll_index + event.y - 2;
                    if (index >= 0 && index < active_panel->files_count) {
                        active_panel->selected_index = index;
                    }
                }

                if (event.bstate & BUTTON1_RELEASED || event.bstate & BUTTON1_CLICKED || event.bstate & BUTTON1_DOUBLE_CLICKED) {
                   gettimeofday(&current_time, NULL);
                   timersub(&current_time, &last_click_time, &diff_time);
                   if (diff_time.tv_sec == 0 && diff_time.tv_usec < 300000) {
                       // Double click finished
                       ch = '\n';
                   }
                   last_click_time = current_time;
                }

                // Handle mouse wheel scrolling
                if (event.bstate & BUTTON4_PRESSED) {
                    active_panel->selected_index--;
                } else if (event.bstate & BUTTON5_PRESSED) {
                    active_panel->selected_index++;
                }
            }
        }


        if (ch == KEY_ALT_ENTER) { // Check for Enter key after Alt
            char *filename = active_panel->file_under_cursor;
            int filename_len = strlen(filename);

            // Check if there's enough space for the filename and the space character
            if (cmd_len + filename_len + 1 < CMD_MAX) {
                // Move the existing command to make space for the filename and space
                memmove(cmd + cursor_pos + filename_len + 1, cmd + cursor_pos, cmd_len - cursor_pos);

                // Copy the filename to the command buffer
                memcpy(cmd + cursor_pos, filename, filename_len);

                // Add a space after the filename
                cmd[cursor_pos + filename_len] = ' ';

                // Update the command length and cursor position
                cmd_len += filename_len + 1;
                cursor_pos += filename_len + 1;
            }
        }


        if (ch == KEY_ALT_a) {
            char *path = active_panel->path;
            int path_len = strlen(path);

            // Check if there's enough space for the path and the / character
            if (cmd_len + path_len + 1 < CMD_MAX) {
                // Move the existing command to make space for the path and /
                memmove(cmd + cursor_pos + path_len + 1, cmd + cursor_pos, cmd_len - cursor_pos);

                // Copy the path to the command buffer
                memcpy(cmd + cursor_pos, path, path_len);

                // Add a / after the path
                cmd[cursor_pos + path_len] = '/';

                // Update the command length and cursor position
                cmd_len += path_len + 1;
                cursor_pos += path_len + 1;
            }
        }


        int continue_search_mode = 0;
        int search_skip_current = 0;

        if (ch == KEY_ALT_s) {
            if (active_panel->search_mode == 1) { // already searching
                memcpy(active_panel->search_text, active_panel->prev_search_text, sizeof(active_panel->search_text));
                search_skip_current = 1;
            } else { // new search
                memset(active_panel->search_text, 0, sizeof(active_panel->search_text));
                active_panel->search_mode = 1;
            }
            continue_search_mode = 1;
        }


        if (ch == '\n')
        {
            if (cmd_len == 0) {
                if (current) {
                   if (current->is_dir) {
                       dive_into_directory(current);
                   } else if (current->is_executable && cmd_len == 0) {
                       snprintf(cmd, CMD_MAX, "%s/%s", active_panel->path, current->name);
                       cmd_len = strlen(cmd);
                   }
                }
            }

            if (cmd_len > 0) {
                if (strcmp(cmd, "exit") == 0) exit(0);

                endwin();  // End ncurses mode
                printf("%s@%s:%s# %s\n", username, unameData.nodename, active_panel->path, cmd);
                system(cmd);  // Execute the command
                init_screen();
                memset(cmd, 0, CMD_MAX);
                cmd_len = cursor_pos = cmd_offset = prompt_length = 0;
                update_files_in_both_panels();
            }
        }

        if (ch == 12) {  // Ctrl+L
            endwin();
            init_screen();
            redraw_ui();
        }

        if (ch == 15) {  // Ctrl+O
            endwin();
            initialize_ncurses();
            raw();
            getch();
            init_screen();
            redraw_ui();
        }


        if (ch == 18 || ch == KEY_F(9)) { // Ctrl+R
            update_files_in_both_panels();
        }


        if (ch == KEY_BACKSPACE) {
            if (active_panel->search_mode == 1) {
                continue_search_mode = 1;
                if (strlen(active_panel->search_text) > 0) {
                    active_panel->search_text[strlen(active_panel->search_text) - 1] = '\0';
                    memcpy(active_panel->prev_search_text, active_panel->search_text, sizeof(active_panel->search_text));
                }
            } else if (cursor_pos > 0) {
                memmove(cmd + cursor_pos - 1, cmd + cursor_pos, cmd_len - cursor_pos);
                cmd[--cmd_len] = '\0';
                cursor_pos--;
            }
        }


        if (ch == KEY_DC) {
            if (cursor_pos < cmd_len) {
                memmove(cmd + cursor_pos, cmd + cursor_pos + 1, cmd_len - cursor_pos - 1);
                cmd[--cmd_len] = '\0';
            }
        }


        if (ch == KEY_LEFT) {
            if (cursor_pos > 0) {
                cursor_pos--;
            } else if (cmd_offset > 0) {
                cmd_offset--;
            }
        }

        if (ch == KEY_RIGHT && cursor_pos < cmd_len) {
            cursor_pos++;
        }

        if (ch == KEY_UP) {
            active_panel->selected_index--;
        }

        if (ch == KEY_DOWN) {
            active_panel->selected_index++;
        }

        if (ch == KEY_PPAGE) {  // Handle PgUp key
            active_panel->selected_index -= visible_items;
            if (active_panel->selected_index < 0) {
                active_panel->selected_index = 0;
            }
        }

        if (ch == KEY_NPAGE) {  // Handle PgDn key
            active_panel->selected_index += visible_items;
            if (active_panel->selected_index > active_panel->files_count - 1) {
                active_panel->selected_index = active_panel->files_count - 1;
            }
        }

        if (ch == KEY_HOME) {  // Handle Home key
            active_panel->selected_index = 0;
        }

        if (ch == KEY_END) {  // Handle End key
            active_panel->selected_index = active_panel->files_count - 1;
        }

        if (ch == KEY_IC) {  // Insert key
            if (current && strcmp(current->name, "..") != 0) {
                current->is_selected = !current->is_selected;
                if (!current->is_dir) active_panel->bytes_selected_files += current->is_selected ? current->size : -1 * current->size;
                active_panel->num_selected_files += current->is_selected ? 1 : -1;
            }
            active_panel->selected_index++;
        }


        if (isprint(ch)) {
            if (active_panel->search_mode == 1) {
                continue_search_mode = 1;
                if (strlen(active_panel->search_text) < CMD_MAX - 1) {
                    active_panel->search_text[strlen(active_panel->search_text) + 1] = 0;
                    active_panel->search_text[strlen(active_panel->search_text)] = ch;
                }
                memcpy(active_panel->prev_search_text, active_panel->search_text, sizeof(active_panel->search_text));
            } else if (cmd_len < CMD_MAX - 1) {
                memmove(cmd + cursor_pos + 1, cmd + cursor_pos, cmd_len - cursor_pos);
                cmd[cursor_pos] = ch;
                cmd[++cmd_len] = '\0';
                cursor_pos++;
            }
        }

        if (ch == '\t') {
            if (active_panel == &left_panel) {
                active_panel = &right_panel;
            } else {
                active_panel = &left_panel;
            }
        }


        if (!continue_search_mode) {
            active_panel->search_mode = 0;
        }


        if (active_panel->search_mode) {
            FileNode *files = active_panel->files;
            int index = 0;
            int found = -1;

            // search from beginning while we get to current item anyway
            while (files != NULL && index < active_panel->selected_index) {
                if (strncmp(active_panel->search_text, files->name, strlen(active_panel->search_text)) == 0) {
                    if (found == -1) found = index;
                }
                index++;
                files = files->next;
            }

            if (search_skip_current) { index++; files = files->next; }

            while (files != NULL) {
                if (strncmp(active_panel->search_text, files->name, strlen(active_panel->search_text)) == 0) {
                    found = index;
                    break;
                }
                index++;
                files = files->next;
            }

            if (found >= 0) active_panel->selected_index = found;
        }


        // make sure scroll does not overflow
        if (active_panel->selected_index < 0) {
                active_panel->selected_index = 0;
        }
        if (active_panel->selected_index > active_panel->files_count - 1) {
                active_panel->selected_index = active_panel->files_count - 1;
        }


        // Handle scrolling in files
        if (active_panel->selected_index < active_panel->scroll_index) {
            // Scroll up to place the selected item in the middle or at the top if near the start
            active_panel->scroll_index = active_panel->selected_index - visible_items / 2;
            if (active_panel->scroll_index < 0) {
                active_panel->scroll_index = 0;
            }
        } else if (active_panel->selected_index > active_panel->scroll_index + visible_items - 1) {
            // Scroll down to place the selected item in the middle or at the bottom if near the end
            active_panel->scroll_index = active_panel->selected_index - visible_items / 2;
            if (active_panel->selected_index > active_panel->files_count - visible_items / 2) {
                active_panel->scroll_index = active_panel->files_count - visible_items;
            }
        }

        // Handle scrolling in command line
        int max_cmd_display = COLS - (strlen(username) + strlen(unameData.nodename) + strlen(active_panel->path) + 6) - 1;
        if (cursor_pos - cmd_offset >= max_cmd_display) {
            cmd_offset++;
        } else if (cursor_pos - cmd_offset < 0 && cmd_offset > 0) {
            cmd_offset--;
        }

        if (cmd_offset < 0) {
                cmd_offset = 0;
        }
    }

    cleanup();
    return 0;
}



int panel_mass_action(OperationFunc operation, char *tgt, operationContext *context) {
    int err = 0;
    char source_path[CMD_MAX] = {0};
    char target_path[CMD_MAX] = {0};
    char target[CMD_MAX] = {0};
    FileNode *unselect_item = NULL;

    WINDOW *saved_screen;
    saved_screen = dupwin(newscr);

    create_progress_dialog(1);

    if (active_panel->num_selected_files == 0) {

        FileNode *current = active_panel->files;
        while (current != NULL) {
            if (strcmp(current->name, active_panel->file_under_cursor) == 0) {
                current->is_selected = 1;
                active_panel->num_selected_files = 1;
                active_panel->bytes_selected_files = current->size;
                unselect_item = current;
                break;
            }
            current = current->next;
        }
    }

    int initial_num_selected = active_panel->num_selected_files;

    // process selected files
    FileNode *current = active_panel->files;
    while (current != NULL) {
        if (current->is_selected) {
            context->keep_item_selected = 0;
            sprintf(source_path, "%s/%s", active_panel->path, current->name);

            if (tgt != NULL && strlen(tgt) > 0)
            {
                if (tgt[0] == '/') { // absolute path
                    sprintf(target, "%s", tgt);
                } else { // relative path
                    sprintf(target, "%s/%s", active_panel->path, tgt);
                }

                if (initial_num_selected == 1 && !file_exists(target)) {
                    sprintf(target_path, "%s", target);
                } else {
                    sprintf(target_path, "%s/%s", target, current->name);
                }
            }
            err = recursive_operation(source_path, target_path, context, operation);
            if (context->abort == 1) break;
            if (err == OPERATION_OK && context->keep_item_selected == 0) {
                if (current->is_selected) {
                    active_panel->num_selected_files--;
                }
                current->is_selected = 0;
            }
        }
        current = current->next;
    }

    if (unselect_item != NULL) {
        unselect_item->is_selected = 0;
        active_panel->num_selected_files = 0;
        active_panel->bytes_selected_files = 0;
    }

    update_progress_dialog_delta(NULL, 0, 0, NULL); // reset internal count of lines, and internal time counter
    delwin(progress); // was created by create_progress_dialog

    overwrite(saved_screen, newscr);
    delwin(saved_screen);
    wrefresh(newscr);
    return 0;
}


int recursive_operation(const char *src, const char *tgt, operationContext *context, OperationFunc operation) {
    int ret;
    context->current_items++;

    // try the operation right away
    ret = operation(src, tgt, context);
    if (context->abort == 1) return OPERATION_ABORT;

    if (ret == OPERATION_OK) {
        // operation on parent item was OK, finish here
        return ret;
    } else if (ret == OPERATION_SKIP) {
        // do nothing, return skip
        return ret;
    } else if (ret == OPERATION_PARENT_OK_PROCESS_CHILDS || ret == OPERATION_RETRY_AFTER_CHILDS) {
        // Recursive operation on a directory is needed for further processing
        struct stat statbuf = {0};
        lstat(src, &statbuf); // no error checking, we assume that if original operation was ok, this will be ok too
        if (S_ISDIR(statbuf.st_mode)) {
            DIR *dir = opendir(src);
            if (!dir) return -1;
            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
                char source_path[CMD_MAX];
                char target_path[CMD_MAX];
                sprintf(source_path, "%s%s%s", src, src[strlen(src) - 1] == '/' ? "" : "/", entry->d_name);
                sprintf(target_path, "%s%s%s", tgt, tgt[strlen(tgt) - 1] == '/' ? "" : "/", entry->d_name);
                recursive_operation(source_path, target_path, context, operation);
                if (context->abort == 1) return 0;
            }
            closedir(dir);
            if (ret == OPERATION_RETRY_AFTER_CHILDS) {
                // try again the initial src
                ret = operation(src, tgt, context);
                if (context->abort == 1) return 0;
                if (ret == OPERATION_OK) {
                    // operation on parent item was OK, finish here
                } else {
                    // print error
                    return ret;
                }
            }
        } else {
            // no childs, end ok
            return OPERATION_OK;
        }
    }

    return 0;
}




int countstats_operation(const char *src, const char *tgt, operationContext *context) {

    struct stat statbuf;
    if (lstat(src, &statbuf) != -1) {
        context->total_items++;
        if (!S_ISDIR(statbuf.st_mode)) {
           context->total_size += statbuf.st_size;
        }
    }
    char infotext[CMD_MAX];
    char num[30];

    context->keep_item_selected = 1; // don't unselect items on stat
    format_number(context->total_size, num);
    sprintf(infotext, "Items: %ld\nSize: %s bytes", context->total_items, num);

    int delta = update_progress_dialog_delta(SPRINTF("Scanning %s", src), 0, 0, infotext);
    if (delta == 1) {
        // ignored here
    }
    if (delta == 2) {
        context->abort = 1;
        return OPERATION_ABORT;
    }

    return OPERATION_PARENT_OK_PROCESS_CHILDS;
}



int delete_operation(const char *src, const char *tgt, operationContext *context) {
    // tgt is ignored for delete operation
    int ret = OPERATION_RETRY;
    int btn = 0;
    errno = 0;

    int delta = update_progress_dialog_delta(SPRINTF("Delete\n%s", src), 100, context->total_items > 0 ? context->current_items * 100 / context->total_items : 0, NULL);
    if (delta == 1) {
        // ignored
    }
    if (delta == 2) {
        context->abort = 1;
        return OPERATION_ABORT;
    }

    while (ret == OPERATION_RETRY) {

        struct stat statbuf;
        ret = lstat(src, &statbuf);
        if (ret != 0) {
            if (context->skip_all == 1) return OPERATION_SKIP;
            btn = show_dialog(SPRINTF("Stat failed for \"%s\"\n%s (%d)", src, strerror(errno), errno), (char *[]) {"Skip", "Skip all", "Retry", "Abort", NULL}, 0, NULL, 1, 0);
            if (btn == 1 || btn == 0) { context->keep_item_selected = 1; return OPERATION_SKIP; }
            if (btn == 2) { context->keep_item_selected = 1; context->skip_all = 1; return OPERATION_SKIP; }
            if (btn == 3) { ret = OPERATION_RETRY; continue; }
            if (btn == 4) { context->abort = 1; return OPERATION_ABORT; }
        }

        if (S_ISDIR(statbuf.st_mode)) {
            ret = rmdir(src);
            if (ret == 0) return OPERATION_OK;

            // if error is directory not empty, ask user to delete subdirectories
            if (errno == ENOTEMPTY || errno == EEXIST) {
                // directory not empty, ask user what to do next
                btn = 0;
                int prefix_already_matches = 0;
                if (context->confirm_all_yes == 1) {
                    btn = 1;
                }
                if (strlen(context->confirm_yes_prefix) != 0 && strncmp(context->confirm_yes_prefix, src, strlen(context->confirm_yes_prefix)) == 0) {
                    btn = 1;
                    prefix_already_matches = 1;
                }
                if (context->confirm_all_no == 1) {
                    btn = 2;
                }

                if (btn == 0) {
                    char title[CMD_MAX] = {};
                    sprintf(title, "Directory \"%s\" not empty.\nDelete it recursively?\n", src);
                    btn = show_dialog(title, (char *[]) {"Yes", "No", "All", "None", "Abort", NULL}, 0, NULL, 1, 0);
                }

                if (btn == 1) { // yes
                    if (!prefix_already_matches) {
                        sprintf(context->confirm_yes_prefix, "%s", src);
                    }
                    return OPERATION_RETRY_AFTER_CHILDS;
                } else if (btn == 2 || btn == 0) { // no
                    context->keep_item_selected = 1;
                    return OPERATION_SKIP;
                } else if (btn == 3) { // all
                    context->confirm_all_yes = 1;
                    return OPERATION_RETRY_AFTER_CHILDS;
                } else if (btn == 4) { // none
                    context->keep_item_selected = 1;
                    context->confirm_all_no = 1;
                    return OPERATION_SKIP;
                } else if (btn == 5) { // abort
                    context->abort = 1;
                    return OPERATION_SKIP;
                }
            } else {
                if (context->skip_all == 1) return OPERATION_SKIP;
                btn = show_dialog(SPRINTF("Cannot remove \"%s\"\n%s (%d)", src, strerror(errno), errno), (char *[]) {"Skip", "Skip all", "Retry", "Abort", NULL}, 0, NULL, 1, 0);
                if (btn == 0 || btn == 1) { context->keep_item_selected = 1; return OPERATION_SKIP; }
                if (btn == 2) { context->keep_item_selected = 1; context->skip_all = 1; return OPERATION_SKIP; }
                if (btn == 3) { ret = OPERATION_RETRY; continue; }
                if (btn == 4) { context->abort = 1; return OPERATION_ABORT; }
            }
        } else {
            ret = unlink(src);
            if (ret == 0) return OPERATION_OK;
            else {
                if (context->skip_all == 1) return OPERATION_SKIP;
                btn = show_dialog(SPRINTF("Cannot remove \"%s\"\n%s (%d)", src, strerror(errno), errno), (char *[]) {"Skip", "Skip all", "Retry", "Abort", NULL}, 0, NULL, 1, 0);
                if (btn == 0 || btn == 1) return OPERATION_SKIP;
                if (btn == 2) { context->keep_item_selected = 1; context->skip_all = 1; return OPERATION_SKIP; }
                if (btn == 3) { ret = OPERATION_RETRY; continue; }
                if (btn == 4) { context->abort = 1; return OPERATION_ABORT; }
            }
        }
    }
    return 0;
}


int copy_operation(const char *src, const char *tgt, operationContext *context) {
    int ret = OPERATION_RETRY;
    errno = 0; // reset

    int delta = update_progress_dialog_delta(SPRINTF("Copying\n%s\nTo\n%s", src, tgt), 0, context->total_items > 0 ? context->current_items * 100 / context->total_items : 0, NULL);
    if (delta == 1) {
        // ignored here
    }
    if (delta == 2) {
        context->abort = 1;
        return OPERATION_ABORT;
    }

    while (ret == OPERATION_RETRY) {

        int btn = 0;
        char errmsg[CMD_MAX] = {0};
        int target_exists = 1;

        do {
            struct stat statbufsrc;
            if (lstat(src, &statbufsrc) != 0) {
                sprintf(errmsg,"Stat operation failed for %s", src);
                break;
            }

            struct stat statbuftgt;
            if (lstat(tgt, &statbuftgt) != 0) {
                if (errno == ENOENT) {
                    target_exists = 0;
                } else { // other error
                    sprintf(errmsg,"Stat operation failed for %s", tgt);
                    break;
                }
            }

            // source is a regular file
            if (S_ISREG(statbufsrc.st_mode)) {

                if (target_exists && S_ISDIR(statbuftgt.st_mode)) {
                    sprintf(errmsg,"Cannot overwrite directory\n%s\nwith a file\n%s", tgt, src);
                    break;
                }

                int src_fd = open(src, O_RDONLY);
                if (src_fd == -1) {
                    sprintf(errmsg,"Cannot open source file for reading:\n%s", src);
                    break;
                }

                int tgt_fd = open(tgt, O_WRONLY | O_CREAT | O_EXCL, statbufsrc.st_mode);
                if (tgt_fd == -1) {
                    if (errno == EEXIST) {
                        // ask user if overwrite
                        btn = 0;
                        if (context->confirm_all_yes == 1) btn = 1;
                        if (context->confirm_all_no == 1) btn = 2;
                        if (btn == 0) {
                            btn = show_dialog(SPRINTF("Target file exists:\n%s\nOverwrite this file?", tgt), (char *[]) {"Yes", "No", "All", "None", "Abort", NULL}, 0, NULL, 1, 0);
                        }
                        if (btn == 3) { // All
                            context->confirm_all_yes = 1;
                            btn = 1;
                        }
                        if (btn == 1) { // Yes
                            tgt_fd = open(tgt, O_WRONLY | O_CREAT | O_TRUNC, statbufsrc.st_mode);
                            if (tgt_fd == -1) {
                                close(src_fd);
                                sprintf(errmsg,"Cannot open target file for writing:\n%s", tgt);
                            }
                        }
                        if (btn == 2) { // No
                            close(src_fd);
                            return OPERATION_SKIP;
                        }
                        if (btn == 4) { // None
                            context->confirm_all_no = 1;
                            return OPERATION_SKIP;
                        }
                        if (btn == 5) {
                            context->abort = 1;
                            return OPERATION_ABORT;
                        }
                    } else {
                        close(src_fd);
                        sprintf(errmsg,"Cannot open target file for writing:\n%s", tgt);
                        break;
                    }
                }

                char buffer[16384];
                ssize_t bytes = 0;
                ssize_t total_bytes = 0;
                while ((bytes = read(src_fd, buffer, sizeof(buffer))) > 0) {
                    if (write(tgt_fd, buffer, bytes) != bytes) {
                        close(src_fd);
                        close(tgt_fd);
                        sprintf(errmsg,"Cannot write data to:\n%s", tgt);
                        break;
                    }
                    total_bytes += bytes;
                    int delta = update_progress_dialog_delta(SPRINTF("Copying\n%s\nTo\n%s", src, tgt), statbufsrc.st_size > 0 ? total_bytes * 100 / statbufsrc.st_size : 0, context->total_items > 0 ? context->current_items * 100 / context->total_items : 0, NULL);
                    if (delta > 0) {
                        close(src_fd);
                        close(tgt_fd);
                        int answer = show_dialog(SPRINTF("Incomplete file was retrieved. Keep it?\n%s", tgt), (char *[]) {"Keep it", "Delete", NULL}, 1, NULL, 1, 0);
                        if (answer == 2) unlink(tgt);
                        if (delta == 2) {
                            context->abort = 1;
                            return OPERATION_ABORT;
                        }
                        return OPERATION_SKIP;
                    }
                }

                if (strlen(errmsg) > 0) break; // second level break

                if (bytes == -1) {
                    // Handle error
                    close(src_fd);
                    close(tgt_fd);
                    sprintf(errmsg,"Cannot read data from:\n%s", src);
                    break;
                }

                int delta = update_progress_dialog_delta(SPRINTF("Copying\n%s\nTo\n%s", src, tgt), statbufsrc.st_size > 0 ? total_bytes * 100 / statbufsrc.st_size : 0, context->total_items > 0 ? context->current_items * 100 / context->total_items : 0, NULL);

                close(src_fd);
                close(tgt_fd);
                ret = 0;
            }
            // source is a directory
            else if (S_ISDIR(statbufsrc.st_mode)) {
                if (target_exists && S_ISDIR(statbuftgt.st_mode)) {
                    // do not overwrite existing directory
                    ret = 0;
                } else if (mkdir(tgt, statbufsrc.st_mode) == -1) {
                    sprintf(errmsg,"Failed to create directory:\n%s", tgt);
                    break;
                } else {
                    ret = 0;
                }
            }
            // source is a symlink
            else if (S_ISLNK(statbufsrc.st_mode)) {
                char buffer[CMD_MAX];
                ssize_t len = readlink(src, buffer, sizeof(buffer) - 1);
                if (len == -1) {
                    sprintf(errmsg,"Failed to read symbolic link from\n%s", src);
                    break;
                }
                buffer[len] = '\0';

                if (target_exists) {
                    // Ask user if they want to overwrite
                    btn = 0;
                    if (context->confirm_all_yes == 1) btn = 1;
                    if (context->confirm_all_no == 1) btn = 2;
                    if (btn == 0) {
                        btn = show_dialog(SPRINTF("Target file exists:\n%s\nOverwrite this file?", tgt), (char *[]) {"Yes", "No", "All", "None", "Abort", NULL}, 0, NULL, 1, 0);
                    }
                    if (btn == 3) { // All
                        context->confirm_all_yes = 1;
                        btn = 1;
                    }
                    if (btn == 1) { // Yes
                        // Remove the existing target
                        if (unlink(tgt) == -1) {
                            sprintf(errmsg, "Failed to remove existing target file\n%s", tgt);
                            break;
                        }
                    } else if (btn == 2 || btn == 0) { // No
                        return OPERATION_SKIP;
                    } else if (btn == 4) { // None
                        context->confirm_all_no = 1;
                        return OPERATION_SKIP;
                    } else if (btn == 5) { // abort
                        context->abort = 1;
                        return OPERATION_ABORT;
                    }
                }

                if (symlink(buffer, tgt) == -1) {
                    sprintf(errmsg,"Failed to create symbolic link\n%s", tgt);
                    break;
                } else {
                    ret = 0;
                }
            }
            // source is a character device or block device
            else if (S_ISCHR(statbufsrc.st_mode) || S_ISBLK(statbufsrc.st_mode)) {
                if (mknod(tgt, statbufsrc.st_mode, statbufsrc.st_rdev) == -1) {
                    sprintf(errmsg,"Failed to create special file\n%s", tgt);
                    break;
                } else {
                    ret = 0;
                }
            }
        } while (false);


        if (strlen(errmsg) > 0) {
            if (context->skip_all == 1) return OPERATION_SKIP;
            if (errno != 0) {
                btn = show_dialog(SPRINTF("%s\n%s (%d)", errmsg, strerror(errno), errno), (char *[]) {"Skip", "Skip all", "Retry", "Abort", NULL}, 0, NULL, 1, 0);
            } else {
                btn = show_dialog(SPRINTF("%s", errmsg), (char *[]) {"Skip", "Skip all", "Retry", "Abort", NULL}, 0, NULL, 1, 0);
            }
            if (btn == 1 || btn == 0) { context->keep_item_selected = 1; return OPERATION_SKIP; }
            if (btn == 2) { context->keep_item_selected = 1; context->skip_all = 1; return OPERATION_SKIP; }
            if (btn == 3) { ret = OPERATION_RETRY; continue; }
            if (btn == 4) { context->abort = 1; return OPERATION_ABORT; }
        }

        return OPERATION_PARENT_OK_PROCESS_CHILDS;
    }

    return 0;
}


int move_operation(const char *src, const char *tgt, operationContext *context) {
    int ret = OPERATION_RETRY;
    errno = 0; // reset

    int delta = update_progress_dialog_delta(SPRINTF("Renaming\n%s\nTo\n%s", src, tgt), 0, context->total_items > 0 ? context->current_items * 100 / context->total_items : 0, NULL);
    if (delta == 1) {
        // ignored here
    }
    if (delta == 2) {
        context->abort = 1;
        return OPERATION_ABORT;
    }

    while (ret == OPERATION_RETRY) {
        int btn = 0;
        char errmsg[CMD_MAX] = {0};

        ret = rename(src, tgt);
        if (ret != 0) {
            if (context->skip_all == 1) return OPERATION_SKIP;
            btn = show_dialog(SPRINTF("Failed to rename\n%s\nTo\n%s\n%s (%d)", src, tgt, strerror(errno), errno), (char *[]) {"Skip", "Skip all", "Retry", "Abort", NULL}, 0, NULL, 1, 0);
            if (btn == 1 || btn == 0) { context->keep_item_selected = 1; return OPERATION_SKIP; }
            if (btn == 2) { context->keep_item_selected = 1; context->skip_all = 1; return OPERATION_SKIP; }
            if (btn == 3) { ret = OPERATION_RETRY; continue; }
            if (btn == 4) { context->abort = 1; return OPERATION_ABORT; }
        }
    }
    return 0;
}



int mkdir_recursive(const char *path, mode_t mode) {
    struct stat st;

    // Check if the directory exists and is really a directory
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0; // Directory already exists
        } else {
            return EEXIST; // Path exists but is not a directory
        }
    } else if (errno != ENOENT) {
        // If the error is not "no such file or directory", return with an error
        return errno;
    }

    // try mkdir directly, if OK return
    if (mkdir(path, mode) == 0) return 0;

    // If the directory does not exist and could not be created so far, start the recursive creation
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (stat(tmp, &st) != 0) {
                if (errno == ENOENT) {
                    if (mkdir(tmp, mode) != 0) {
                        return errno; // Error creating directory
                    }
                } else {
                    return errno; // Some other error occurred
                }
            } else if (!S_ISDIR(st.st_mode)) {
                return ENOTDIR; // Path exists but is not a directory
            }
            *p = '/';
        }
    }

    return mkdir(tmp, mode) ? errno : 0;
}


int file_exists(const char *path) {
    struct stat info;

    if (lstat(path, &info) != 0) {
        // If stat fails, the path does not exist
        return 0;
    }

    return 1;
}


void shorten(char *name, int width, char *result) {
    int length = strlen(name);
    if (length <= width) {
        strcpy(result, name);
    } else {
        int halfWidth = (width - 1) / 2;
        strncpy(result, name, halfWidth);
        result[halfWidth] = '~';
        strncpy(result + halfWidth + 1, name + length - (width - 1 - halfWidth), width - 1 - halfWidth);
        result[width] = '\0';
    }
}

int file_has_extension(const char *filename, const char *extensions[]) {
    for (int i = 0; extensions[i]; i++) {
        if (strcmp(filename + strlen(filename) - strlen(extensions[i]), extensions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


int format_number(off_t num, char *str) {
    static char buf[20]; // Assuming number won't exceed 20 characters with commas
    char rev[20], *p = rev;
    int count = 0;

    do {
        if (count++ % 3 == 0 && count > 1) *p++ = ',';
        *p++ = '0' + num % 10;
        num /= 10;
    } while (num);

    *p = '\0';
    for (int i = 0, j = strlen(rev) - 1; j >= 0; j--, i++) {
        buf[i] = rev[j];
    }
    buf[strlen(rev)] = '\0';
    sprintf(str, "%s", buf);
    return 0;
}


void format_size_with_units(off_t size, char *size_str, size_t len, int maxlen) {
    snprintf(size_str, len, "%ld", size);

    if (strlen(size_str) > maxlen) {
        size /= 1024;
        snprintf(size_str, len, "%ldK", size);
        if (strlen(size_str) > maxlen) {
            size /= 1024;
            snprintf(size_str, len, "%ldM", size);
            if (strlen(size_str) > maxlen) {
                size /= 1024;
                snprintf(size_str, len, "%ldG", size);
                if (strlen(size_str) > maxlen) {
                    size /= 1024;
                    snprintf(size_str, len, "%ldT", size);
                }
            }
        }
    }
}



void update_panel(WINDOW *win, PanelProp *panel) {
    FileNode *current = panel->files;
    int line = 1;  // Start from the second row to avoid the border
    int width = getmaxx(win) - 2;
    int height = getmaxy(win);
    int name_width = width - 12 - 7 - 3;
    char info[CMD_MAX];

    // Get the current year
    time_t now = time(NULL);
    struct tm *current_tm = localtime(&now);
    int current_year = current_tm->tm_year;

    // reset color to default
    wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
    wattroff(win, A_BOLD);

    // Fill separator columns
    mvwvline(win, 1, width - 12, '|', height -3);
    mvwvline(win, 1, width - 7 - 12 - 1, '|', height -3);
    mvwhline(win, height - 3, 1, '-', width);

    // Header of the file list
    wattron(win, A_BOLD);
    wattron(win, COLOR_PAIR(COLOR_YELLOW_ON_BLUE));
    mvwprintw(win, line, 1, "%*s%s", ((width - 12 - 7 - 2) / 2) - (strlen("Name") / 2), "", "Name");
    mvwprintw(win, line, width - 12 - 7 + 1, "%s", "Size");
    mvwprintw(win, line, width - 7 - 4, "%s", "Modify time");
    wattroff(win, A_BOLD);

    line++;

    // Ignore first items based on the scroll index
    for (int i = 0; i < panel->scroll_index && current != NULL; i++) {
        current = current->next;
    }

    int index = panel->scroll_index;
    while (current != NULL && line < height - 3) {
        int is_active_item = (index == panel->selected_index);
        char prefix = ' ';

        // reset default color
        wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
        wattroff(win, A_BOLD);

        // use some colors for regular files based by extension
        if (!current->is_dir)
        {
           if (file_has_extension(current->name, (const char*[]){".gz",".tar",".xz",NULL})) {
              wattron(win, COLOR_PAIR(COLOR_MAGENTA_ON_BLUE));
              wattron(win, A_BOLD);
           }

           if (file_has_extension(current->name, (const char*[]){".c",".php",".sh",".h",NULL})) {
              wattron(win, COLOR_PAIR(COLOR_CYAN_ON_BLUE));
           }
        }


        if (current->is_link_broken) {
            prefix = '!';
            wattron(win, COLOR_PAIR(COLOR_RED_ON_BLUE));
            wattron(win, A_BOLD);
        } else if (current->is_link_to_dir) {
            prefix = '~';
            wattron(win, A_BOLD);
        } else if (current->is_dir) {
            prefix = '/';
            wattron(win, A_BOLD);
        } else if (current->is_link) {
            prefix = '@';
        } else if (current->is_device) {
            prefix = '-';
            wattron(win, COLOR_PAIR(COLOR_MAGENTA_ON_BLUE));
            wattron(win, A_BOLD);
        } else if (current->is_executable) {
            prefix = '*';
            wattron(win, COLOR_PAIR(COLOR_GREEN_ON_BLUE));
            wattron(win, A_BOLD);
        }

        if (current->is_selected) {
            wattron(win, COLOR_PAIR(COLOR_YELLOW_ON_BLUE));
            wattron(win, A_BOLD);
        }

        char date_str[13];
        struct tm *tm = localtime(&current->mtime);

        if (tm->tm_year != current_year) {
            strftime(date_str, sizeof(date_str), "%b %d  %Y", tm); // Display year if different
        } else {
            strftime(date_str, sizeof(date_str), "%b %d %H:%M", tm); // Display time if same year
        }

        char size_str[50];  // Buffer to hold the size and suffix
        format_size_with_units(current->size, size_str, sizeof(size_str), 7);

        int updir = (current->is_dir && strcmp(current->name, "..") == 0);
        if (updir) {
           snprintf(size_str, sizeof(size_str), "UP--DIR");
        }

        if (is_active_item) {
            if (updir) {
                snprintf(info, sizeof(info), "UP--DIR");
            } else if (current->is_link) {
                snprintf(info, sizeof(info), "-> %s", current->link_target);
            } else {
                snprintf(info, sizeof(info), "%c%s", prefix, current->name);
            }

            if (panel == active_panel) {
                wattron(win, COLOR_PAIR(COLOR_BLACK_ON_CYAN));
                wattroff(win, A_BOLD);

                mvwprintw(win, line, width - 7 - 12 - 1, "|");
                mvwprintw(win, line, width - 12, "|");

                if (current->is_selected) {
                   wattron(win, COLOR_PAIR(COLOR_YELLOW_ON_CYAN));
                   wattron(win, A_BOLD);
                }
           }
        }

        mvwhline(win, line, 1, ' ', name_width + 1);
        mvwprintw(win, line, 1, "%c", prefix);

        mvwaddnstr(win, line, 2, SHORTEN(current->name, name_width), name_width);

        mvwprintw(win, line, width - 7 - 12, "%7s", size_str);
        mvwprintw(win, line, width - 12 + 1, "%12s", date_str);

        line++;
        index++;
        current = current->next;
    }

    // path goes to window title
    if ((win == win1 && active_panel == &left_panel) || (win == win2 && active_panel == &right_panel)) {
       wattron(win, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    } else {
       wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
    }
    wattroff(win, A_BOLD);
    mvwprintw(win, 0, 3, " %s ", SHORTEN(panel->path, name_width + 12 + 7 - 2));

    // reset color to default
    wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));

    mvwhline(win, 0, strlen(panel->path) + 5, '-', width - strlen(panel->path) - 4);

    while(line < height - 3) {
        mvwhline(win, line, 1, ' ', name_width + 1);
        mvwprintw(win, line, width - 7 - 12, "       ");
        mvwprintw(win, line, width - 12 + 1, "            ");
        line++;
    }

    if (panel->search_mode == 1) {
        // search input
        wattron(win, COLOR_PAIR(COLOR_BLACK_ON_CYAN));
        mvwprintw(win, height - 2, 1, "/%-*s", width - 1, panel->search_text);
        wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
    } else {
        // print info for active file
        mvwprintw(win, height - 2, 1, "%-*s", width, SHORTEN(info,width));
    }


    // print selected
    wattron(win, COLOR_PAIR(COLOR_YELLOW_ON_BLUE));
    wattron(win, A_BOLD);
    char num[20];
    format_number(panel->bytes_selected_files, num);
    sprintf(info," %s B in %d file%s ", num, panel->num_selected_files, panel->num_selected_files == 1 ? "" : "s");
    if (panel->num_selected_files > 0) mvwprintw(win, height - 3, width - strlen(info) - 3, "%s", info);
    wattroff(win, A_BOLD);

    wrefresh(win);
    cursor_to_cmd();
}


void update_panel_cursor() {
   if (strlen(active_panel->file_under_cursor) >0) {
       // Search for the last selected item and set it as the active item
       FileNode *node = active_panel->files;
       int index = 0;
       while (node) {
           if (strcmp(node->name, active_panel->file_under_cursor) == 0) {
               active_panel->selected_index = index;
               break;
           }
           node = node->next;
           index++;
       }
   } else {
       active_panel->selected_index = 0;
   }
   active_panel->scroll_index = 0;
}


void update_files_in_both_panels() {
    update_panel_files(&left_panel);
    update_panel_files(&right_panel);
    sort_file_nodes(&left_panel.files, left_panel.sort_order);
    sort_file_nodes(&right_panel.files, right_panel.sort_order);
    update_panel_cursor();
}

WINDOW *progress;

// Function to create a dialog window with a title and two progress bars
void create_progress_dialog(int title_lines) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int width = max_x / 2;
    int height = 9 + title_lines;

    int start_y = (max_y - height) / 2;
    int start_x = (max_x - width) / 2;

    progress = newwin(height, width, start_y, start_x);
    wbkgd(progress, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    wattron(progress, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
    show_shadow(progress);

    // Draw the borders and title
    mvwaddch(progress, 1, 1, '+'); // Top left corner
    mvwaddch(progress, 1, width - 2, '+'); // Top right corner
    mvwaddch(progress, height - 2, 1, '+'); // Bottom left corner
    mvwaddch(progress, height - 2, width - 2, '+'); // Bottom right corner
    mvwhline(progress, 1, 2, '-', width - 4); // Top border
    mvwhline(progress, height - 2, 2, '-', width - 4); // Bottom border
    mvwvline(progress, 2, 1, '|', height - 4); // Left border
    mvwvline(progress, 2, width - 2, '|', height - 4); // Right border
    mvwhline(progress, height - 4, 2, '-', width - 4); // Horizontal line above buttons
    mvwaddch(progress, height - 4, 1, '+'); // Left intersection
    mvwaddch(progress, height - 4, width - 2, '+'); // Right intersection

    wrefresh(progress);
}


int update_progress_dialog(char *title, int current_progress, int total_progress, char *infotext) {
    int width, height;
    getmaxyx(progress, height, width);

    if (current_progress > 100) current_progress = 100;
    if (total_progress > 100) total_progress = 100;

    static int active_button = 0;

    int title_lines = lines(title);
    if (title_lines < 1) title_lines = 1;

    static int previous_title_lines = 1;
    if (previous_title_lines < title_lines) {
        delwin(progress);
        create_progress_dialog(title_lines);
        previous_title_lines = title_lines;
    }

    // cleanup stastic var for another dialog, when calling with empty or zero arguments
    if (title == NULL && current_progress == 0 && total_progress == 0 && infotext == NULL) {
        previous_title_lines = 1;
        return -1;
    }

    // print title if provided
    if (title != NULL) {
        int title_line = 2;
        char * title_copy = strdup(title);
        char * line = strtok(title_copy, "\n");
        while (line) {
            mvwprintw(progress, title_line, 3, "%s", SHORTEN(line, width - 6));
            line = strtok(NULL, "\n");
            title_line++;
        }
        free(title_copy);
    }

    // Draw the progress bars if progress is provideed
    if (infotext == NULL) {
        mvwaddch(progress, 3 + title_lines, 3, '[');
        mvwaddch(progress, 3 + title_lines, width - 8, ']');
        mvwaddch(progress, 4 + title_lines, 3, '[');
        mvwaddch(progress, 4 + title_lines, width - 8, ']');
        mvwhline(progress, 3 + title_lines, 4, '.', width - 12);
        mvwhline(progress, 4 + title_lines, 4, '.', width - 12);

        mvwprintw(progress, 3 + title_lines, width - 7, "%3d%%", current_progress);
        mvwprintw(progress, 4 + title_lines, width - 7, "%3d%%", total_progress);

        int bar_width = width - 12;
        int current_fill = (current_progress * bar_width) / 100;
        int total_fill = (total_progress * bar_width) / 100;

        for (int i = 0; i < current_fill; i++) { mvwaddch(progress, 3 + title_lines, 4 + i, '#'); }
        for (int i = 0; i < total_fill; i++) { mvwaddch(progress, 4 + title_lines, 4 + i, '#'); }
    } else {
        int info_line = 2 + title_lines;
        char * info_copy = strdup(infotext);
        char * line = strtok(info_copy, "\n");
        while (line) {
            mvwprintw(progress, info_line, 3, "%s", SHORTEN(line, width - 6));
            line = strtok(NULL, "\n");
            info_line++;
        }
        free(info_copy);
    }

    char * buttons[] = {"Skip", "Abort", NULL};

    int move_cursor_pos = 0;
    int total_buttons_width = 0;
    int i = 0;
    while (buttons[i] != NULL) {
        total_buttons_width += strlen(buttons[i]) + 4;
        i++;
    }
    total_buttons_width += 2 * (i - 1);

    int cursor_pos = (width - total_buttons_width) / 2;
    int y_pos = 6;
    i = 0;
    while (buttons[i] != NULL) {
        if (i == active_button) {
            wattron(progress, COLOR_PAIR(COLOR_BLACK_ON_CYAN));
            move_cursor_pos = cursor_pos + 2;
        }
        mvwprintw(progress, y_pos + title_lines, cursor_pos, "[ %s ]", buttons[i]);
        if (i == active_button) {
            wattron(progress, COLOR_PAIR(COLOR_BLACK_ON_WHITE));
        }
        cursor_pos += strlen(buttons[i]) + 6;
        i++;
    }

    if (move_cursor_pos > 0) {
        wmove(progress, y_pos + title_lines, move_cursor_pos);
    }

    wrefresh(progress);

    timeout(0);
    int ch = getch();
    timeout(-1);

    if (ch == KEY_LEFT) active_button--;
    if (ch == KEY_RIGHT) active_button++;
    if (active_button > 1) active_button = 0;
    if (active_button < 0) active_button = 1;

    if (ch == '\n') return active_button + 1;

    return -1;
}


int update_progress_dialog_delta(char *title, int current_progress, int total_progress, char *infotext) {
    static struct timeval last_time = {0};
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    if (title == NULL && current_progress == 0 && total_progress == 0 && infotext == NULL) {
        last_time.tv_sec = 0;
        last_time.tv_usec = 0;
        update_progress_dialog(title, current_progress, total_progress, infotext);
        return -1;
    }

    // Calculate the elapsed time in milliseconds
    long elapsed_ms = (current_time.tv_sec - last_time.tv_sec) * 1000 +
                      (current_time.tv_usec - last_time.tv_usec) / 1000;

    if ( current_progress == 100 || (last_time.tv_sec == 0 && last_time.tv_usec == 0) || elapsed_ms > 200) {
        int t = update_progress_dialog(title, current_progress, total_progress, infotext);
        if (t != -1) return t;
        last_time = current_time;  // update the last_time to current_time
    }

    return -1;
}


void draw_buttons(int maxY, int maxX) {
    move(maxY - 1, 0);
    clrtoeol();

    char *buttons[] = {"Sort", "View", "Edit", "Copy", "Move", "Mkdir", "Del", "Refresh", "Quit"};
    int num_buttons = sizeof(buttons) / sizeof(char *);

    int total_width = maxX - (num_buttons - 1);  // Subtract (num_buttons - 1) to account for spaces between buttons
    int button_width = (total_width - 1) / num_buttons;  // -1 to account for the extra character in "F10"

    int extra_space = total_width - (button_width * num_buttons) - 1;  // -1 to account for the extra character in "F10"

    int x = 0;
    for (int i = 0; i < num_buttons; ++i) {
        int extra = 0;
        if (extra_space > 0) {
            extra = 1;
            extra_space--;
        }

        attrset(A_NORMAL);
        if (i == num_buttons - 1) {  // Last button (F10)
            mvprintw(maxY - 1, x, "F%d ", i + 2);
        } else {
            mvprintw(maxY - 1, x, "F%d", i + 2);
        }

        attron(COLOR_PAIR(COLOR_BLACK_ON_CYAN));
        mvprintw(maxY - 1, x + 2 + (i == num_buttons - 1), "%-*s", button_width - 2 + extra, buttons[i]);

        x += button_width + extra + 1 + (i == num_buttons - 1);  // +1 spacer between buttons, +1 for the last button (F10)
    }
}

void draw_windows(int maxY, int maxX) {
    // Refresh stdscr to ensure it's updated
    refresh();

    // Calculate window dimensions
    int winHeight = maxY - 2;
    int winWidth1 = maxX / 2;
    int winWidth2 = maxX / 2;

    // Adjust for odd COLS
    if (maxX % 2 != 0) {
        winWidth2 += 1;
    }

    // Delete old windows
    delwin(win1);
    delwin(win2);

    // Create new windows
    win1 = newwin(winHeight, winWidth1, 0, 0);
    win2 = newwin(winHeight, winWidth2, 0, winWidth1);

    // Apply the color pair to the window
    wbkgd(win1, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
    wbkgd(win2, COLOR_PAIR(COLOR_WHITE_ON_BLUE));

    // Add borders to windows using wborder()
    wborder(win1, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(win2, '|', '|', '-', '-', '+', '+', '+', '+');

    // Refresh windows to make borders visible
    wrefresh(win1);
    wrefresh(win2);
}


char *find_newline(char *buffer, size_t length) {
    char *pos_r = memchr(buffer, '\r', length);
    char *pos_n = memchr(buffer, '\n', length);

    if (pos_r && pos_n) {
        if (pos_r + 1 == pos_n) {
            return pos_n; // if \r\n is encountered, break on the later
        }
        return pos_r < pos_n ? pos_r : pos_n;
    } else if (pos_r) {
        return pos_r;
    } else {
        return pos_n;
    }
}



int write_file_lines(const char *filename, file_lines *lines) {
    char temp_filename[strlen(filename) + 10];  // Space for ".tmpN\0"
    int counter = 0;

    do {
        snprintf(temp_filename, sizeof(temp_filename), "%s.tmp%d", filename, counter++);
    } while (access(temp_filename, F_OK) != -1);

    int fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) return -1;

    file_lines *current = lines;
    while (current) {
        write(fd, current->line, current->line_length);
        if (current->next) write(fd, "\n", 1);
        current = current->next;
    }

    close(fd);
    return rename(temp_filename, filename) == 0 ? 0 : -1;
}



file_lines* read_file_lines(const char *filename, off_t *num_lines, off_t *num_bytes) {
    // Initialize linked list and counters
    file_lines *head = NULL, *tail = NULL;
    *num_lines = 0;

    // Open the file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    // Get the file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        return NULL;
    }

    *num_bytes = sb.st_size;

    // Handle empty file scenario separately
    if (sb.st_size == 0) {
        head = malloc(sizeof(file_lines));
        head->line = malloc(0);
        head->line_length = 0;
        head->next = NULL;
        *num_lines = 1;
        return head;
    }


    // Memory map the file
    char *file_in_memory = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    char *line_start = file_in_memory;

    // Iterate through mapped memory

    // Iterate through mapped memory
    for (char *current = file_in_memory; current <= file_in_memory + sb.st_size; ++current) {
        // Check for end of file or newline character
        if (current == file_in_memory + sb.st_size || *current == '\n') {
            file_lines *new_node = malloc(sizeof(file_lines));

            // Allocate memory for the line data and copy it from the mapped memory
            int line_length = current - line_start;
            char *line_copy = malloc(line_length);
            memcpy(line_copy, line_start, line_length);

            // Fill in the new node
            new_node->line = line_copy;
            new_node->line_length = line_length;
            new_node->next = NULL;

            // Append to the linked list
            if (!head) {
                head = new_node;
            } else {
                tail->next = new_node;
            }
            tail = new_node;

            // Prepare for next block
            line_start = current + 1;
            (*num_lines)++;
        }
    }

    // Clean up
    munmap(file_in_memory, sb.st_size);
    close(fd);

    return head;
}


void free_file_lines(file_lines *head) {
    while (head != NULL) {
        file_lines *temp = head;
        head = head->next;
        free(temp->line);
        free(temp);
    }
}

void free_pattern_regexes(PatternColorPair* patterns, int num_patterns) {
    for (int i = 0; i < num_patterns; i++) {
        regfree(&patterns[i].regex);  // Free each compiled regex
    }
}

void display_line(WINDOW *win, file_lines *line, int max_x, int current_col, int editor_mode, PatternColorPair* patterns, int num_patterns) {
    char *ptr = line->line;
    int offset = 0;
    regmatch_t pmatch[1];

    while (offset < line->line_length && offset < current_col) {
        ptr++;
        offset++;
    }

    if (offset < current_col) {
        wclrtoeol(win);
    } else {
        for (int x = 0; x < max_x && offset < line->line_length; x++, ptr++, offset++) {

            int matched = 0;
            // copy line to temporary buffer for matching, to make sure it is terminated by zero byte even if the original is not
            int remaining_length = line->line_length - offset;
            char *temp_str = strndup(ptr, remaining_length);

            for (int i = 0; i < num_patterns; i++) {
                if (regexec(&patterns[i].regex, temp_str, 1, pmatch, 0) == 0 && pmatch[0].rm_so == 0) {
                    matched = 1;
                    wattron(win, patterns[i].color_pair);
                    if (patterns[i].is_bold) {
                        wattron(win, A_BOLD);
                    }

                    for (int j = 0; j < pmatch[0].rm_eo; j++) {
                        waddch(win, ptr[j]);
                        if (j < pmatch[0].rm_eo - 1) {
                            x++;
                            offset++;
                        }
                    }

                    wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
                    wattroff(win, A_BOLD);
                    ptr += pmatch[0].rm_eo - 1;
                    break;  // Stop checking other patterns after the first match
                }
            }

            free(temp_str);
            if (matched) continue;

            if (isprint((unsigned char)*ptr)) {
                waddch(win, *ptr);
            } else if (*ptr != '\n') {
                if (*ptr == 9) {
                    wattron(win, COLOR_PAIR(COLOR_CYAN_ON_BLUE));
                    waddch(win, '>');
                    wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
                } else {
                    if (editor_mode) {
                        wattron(win, COLOR_PAIR(COLOR_WHITE_ON_RED));
                        waddch(win, *ptr >= 0 && *ptr < 32 ? '@' + *ptr : '.');
                        wattron(win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
                    } else {
                        waddch(win, '.');
                    }
                }
            }
        }
    }

    wrefresh(win);
}


int view_edit_file(char *filename, int editor_mode) {
    int input;
    int max_y, max_x;
    int screen_start_line = 0;
    int screen_start_col = 0; // Add a variable to keep track of the current column offset
    int cursor_row = 0;
    int cursor_col = 0;
    int skip_refresh = 0;
    int is_modified = 0;
    PatternColorPair patterns[100] = {0};
    int num_patterns = 0;
    char find_str[CMD_MAX] = {0};

    // Get the screen dimensions
    getmaxyx(stdscr, max_y, max_x);

    // Top line on screen
    WINDOW *toprow_win = newwin(1, max_x, 0, 0);
    wbkgd(toprow_win, COLOR_PAIR(COLOR_BLACK_ON_CYAN));
    wattron(toprow_win, COLOR_PAIR(COLOR_BLACK_ON_CYAN));

    // Create a new window for displaying the file content
    WINDOW *content_win = newwin(max_y - 2, max_x, 1, 0);

    werase(content_win); // Clear the window
    wbkgd(content_win, COLOR_PAIR(COLOR_WHITE_ON_BLUE)); // Set the background color
    wrefresh(content_win); // Refresh the window to apply the changes
    wattron(content_win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));

    // Build the linked list of line pointers
    off_t num_lines, num_bytes;
    file_lines *lines = read_file_lines(filename, &num_lines, &num_bytes);

    // Extract file extension
    char *file_type = strrchr(filename, '.');  // find last '.' in filename

    if (editor_mode)
    {
        // Syntax highlighting for editor mode
        if (file_type && (strcmp(file_type, ".c") == 0 || strcmp(file_type, ".h") == 0)) {
            patterns[num_patterns++] = (PatternColorPair) {"\".*\"", COLOR_PAIR(COLOR_GREEN_ON_BLUE), 0, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"^(#include|#define).*$", COLOR_PAIR(COLOR_RED_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"//.*$", COLOR_PAIR(COLOR_YELLOW_ON_BLUE), 0, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\b(auto|break|case|char|const|continue|default|do|double|else|enum|extern|float|for|goto|if|int|long|register|return|short|signed|sizeof|static|struct|switch|typedef|union|unsigned|void|volatile|while|asm|inline|wchar_t|[.][.][.])\\b", COLOR_PAIR(COLOR_YELLOW_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"!|%|==|!=|&&|[*]|->|[+]|-|[|][|]|=|>|<|/", COLOR_PAIR(COLOR_YELLOW_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"[(){},:?]|\\[|\\]", COLOR_PAIR(COLOR_CYAN_ON_BLUE), 0, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"[;&^~|]", COLOR_PAIR(COLOR_MAGENTA_ON_BLUE), 1, NULL};
        }

        if ((file_type && (strcmp(file_type, ".sh") == 0)) || (lines != NULL && lines->line_length > 3 && strncmp(lines->line, "#!/", 3) == 0)) {
            patterns[num_patterns++] = (PatternColorPair) {"^#!/.*", COLOR_PAIR(COLOR_CYAN_ON_BLACK), 0, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"#.*$", COLOR_PAIR(COLOR_YELLOW_ON_BLUE), 0, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"[;{}]", COLOR_PAIR(COLOR_CYAN_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\$[(].*[)]|\\$[{].*[}]", COLOR_PAIR(COLOR_GREEN_ON_BLUE), 0, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\$[*]|\\$@|\\$#|\\$[?]|\\$-|\\$\\$|\\$!|\\$_", COLOR_PAIR(COLOR_RED_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"2>&1|1>&2|2>|1>", COLOR_PAIR(COLOR_RED_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\$[0123456789]", COLOR_PAIR(COLOR_RED_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\$[a-zA-Z0-9_]+", COLOR_PAIR(COLOR_GREEN_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\$", COLOR_PAIR(COLOR_GREEN_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\bfunction\\b.*[(][)]", COLOR_PAIR(COLOR_MAGENTA_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"[a-zA-Z0-9_]+[(][)]", COLOR_PAIR(COLOR_MAGENTA_ON_BLUE), 1, NULL};
            patterns[num_patterns++] = (PatternColorPair) {"\\b(break|case|clear|continue|declare|done|do|echo|elif|else|esac|exit|export|fi|for|getopts|if|in|read|return|select|set|shift|source|then|trap|until|unset|wait|while)\\b", COLOR_PAIR(COLOR_YELLOW_ON_BLUE), 1, NULL};
        }

        for (int i = 0; i < num_patterns; i++) {
            regcomp(&patterns[i].regex, patterns[i].pattern, REG_EXTENDED);
    }
    }

    // Initial display
    file_lines *current = lines;
    for (int i = 0; i < max_y - 2 && current != NULL; i++) {
        wmove(content_win, i, 0);
        display_line(content_win, current, max_x, screen_start_col, editor_mode, patterns, num_patterns);
        current = current->next;
    }

    current = lines;

    // Handle user input for scrolling
    while(1) {

        int seek = 0;

        // globally get current line, since it may be used on many places later
        file_lines *current_line = lines;
        for (int i = 0; i < screen_start_line + cursor_row; i++) {
            seek += current_line->line_length + 1;
            current_line = current_line->next;
        }

        int shown_line_max = screen_start_line + max_y - 2;
        if (shown_line_max > num_lines) shown_line_max = num_lines;

        int absolute_cursor_col = cursor_col + screen_start_col;
        int absolute_cursor_row = cursor_row + screen_start_line;
        char charcode[10] = {0};

        // Initial top row stats

        if (editor_mode) {
            if (absolute_cursor_col < current_line->line_length) {
                unsigned char current_char = current_line->line[absolute_cursor_col];
                sprintf(charcode, "#%d", (int)current_char);
            } else if (seek + absolute_cursor_col >= num_bytes) {
                sprintf(charcode, "<EOF>");
            } else sprintf(charcode, "#10");
            mvwprintw(toprow_win, 0, 0, "%s   [-%s--] %3d L:[%3d+%3d %3d/%3lld] *(%4d/%lldb)   %s     ", filename, is_modified ? "M" : "-", absolute_cursor_col, screen_start_line + 1, cursor_row, absolute_cursor_row + 1, num_lines, seek + absolute_cursor_col, num_bytes, charcode);
        } else {
            mvwprintw(toprow_win, 0, 0, "%s", filename);
            int num_width = snprintf(NULL, 0, "        %d/%ld   %ld%%", shown_line_max, num_lines, num_lines > 0 ? 100 * shown_line_max / num_lines : 100);
            mvwprintw(toprow_win, 0, max_x - num_width, "        %d/%ld   %ld%%", shown_line_max, num_lines, num_lines > 0 ? 100 * shown_line_max / num_lines : 100);
        }

        wrefresh(toprow_win);

        if (editor_mode) {
            curs_set(1); // Make cursor visible
        } else {
            curs_set(0); // Hide cursor
        }

        move(cursor_row + 1, cursor_col);
        skip_refresh = 0;

        input = noesc(getch());
        switch (input) {
            case KEY_F(3):
                if (editor_mode) { 
                    // TODO: perhaps some Mark function
                } else {
                    delwin(content_win);
                    free_file_lines(lines);
                    free_pattern_regexes(patterns, num_patterns);
                    curs_set(1);
                    return 0;
                }
                break;
            case KEY_F(10):
            case 27:
            {
                if (is_modified) {
                    int btn = show_dialog(SPRINTF("File %s was modified.\nSave before close?", filename), (char *[]) {"Yes", "No", "Cancel", NULL}, 2, NULL, 0, 0);
                    if (btn == 1) {
                        write_file_lines(filename, lines);
                    }
                    if (btn != 1 && btn != 2) { // continue editing
                        break;
                    }
                }
                delwin(content_win);
                free_file_lines(lines);
                free_pattern_regexes(patterns, num_patterns);
                curs_set(1);
                return 0;
            }
            case KEY_F(2):
            {
                int btn = show_dialog(SPRINTF("Confirm save file:\n%s", filename), (char *[]) {"Save", "Cancel", NULL}, 0, NULL, 0, 0);
                if (btn == 1) {
                    write_file_lines(filename, lines);
                    is_modified = 0;
                }
                break;
            }

            case KEY_F(7): // F7 search
            case KEY_SHIFT_F7: // Shift+F7 search
            {
                int ret;
                if (strlen(find_str)==0 || input == KEY_F(7)) {
                    ret = show_dialog("Enter search string:", (char *[]) {"Find", "Cancel", NULL}, 0, find_str, 0, 0);
                } else ret = 1;

                if (ret == 1) {
                    if (strlen(find_str) == 0) break;
                    file_lines *search_line = current_line;
                    int start_column = absolute_cursor_col + 1;
                    int found_line = absolute_cursor_row;
                    int found_column = -1;

                    while (search_line) {
                        if (search_line->line_length >= strlen(find_str)) {
                            // Find the find_str in the current line starting from start_column
                            for (int pos = start_column; pos < search_line->line_length - strlen(find_str); pos++) {
                                if (strncasecmp(search_line->line + pos, find_str, strlen(find_str)) == 0) {
                                      found_column = pos;
                                      search_line = NULL;
                                      break;
                                }
                            }
                        }
                        start_column = 0;
                        if (search_line == NULL) break; // found it, stop
                        found_line++;
                        search_line = search_line->next;
                    }
                    if (found_column < 0) {
                        show_dialog("Search string not found", (char *[]) {"OK", NULL}, 0, NULL, 0, 0);
                    } else {
                        cursor_row = found_line;
                        cursor_col = found_column;
                    }
                }
                break;
            }

            case KEY_UP:
                if (editor_mode && cursor_row > 0) {
                    cursor_row--;
                    // TODO fix cursor_col position
                    skip_refresh = 1;
                } else {
                    if (screen_start_line > 0) {
                        screen_start_line--;
                    }
                }
                break;
            case KEY_DOWN:
                if (editor_mode && cursor_row < max_y - 3 && absolute_cursor_row < num_lines - 1) {
                    cursor_row++;
                    skip_refresh = 1;
                } else {
                    if (screen_start_line < num_lines - (max_y - 2)) {
                        screen_start_line++;
                    }
                }
                break;

            case KEY_LEFT:
                if (editor_mode) {
                    if (cursor_col > 0) {
                        cursor_col--;
                        skip_refresh = 1;
                    } else if (screen_start_col > 0) {
                        screen_start_col--;
                    } else if (cursor_row > 0) {
                        cursor_row--;
                        file_lines *prev_line = lines;
                        for (int i = 0; i < screen_start_line + cursor_row; i++) {
                            prev_line = prev_line->next;
                        }
                        cursor_col = prev_line->line_length;
                        if (prev_line->line[prev_line->line_length - 1] == '\n') cursor_col--;
                        if (cursor_col < 0) cursor_col = 0;
                        if (cursor_col >= max_x) {
                            screen_start_col = cursor_col - max_x + 1;
                            cursor_col = max_x - 1;
                        } else {
                            screen_start_col = 0;
                        }
                    } else if (screen_start_line > 0) {
                        screen_start_line--;
                        file_lines *prev_line = lines;
                        for (int i = 0; i < screen_start_line; i++) {
                            prev_line = prev_line->next;
                        }
                        cursor_col = prev_line->line_length;
                        if (prev_line->line[prev_line->line_length - 1] == '\n') cursor_col--;
                        if (cursor_col < 0) cursor_col = 0;
                        if (cursor_col >= max_x) {
                            screen_start_col = cursor_col - max_x + 1;
                            cursor_col = max_x - 1;
                        } else {
                            screen_start_col = 0;
                        }
                    }

                } else {
                    if (screen_start_col > 0) screen_start_col -= 10;
                    if (screen_start_col < 0) screen_start_col = 0;
                }
                break;

            case KEY_RIGHT:
                if (editor_mode) {
                    if (absolute_cursor_col < current_line->line_length) {
                        if (cursor_col < max_x - 1) {
                            cursor_col++;
                        } else {
                            screen_start_col++;
                        }
                    } else if (current_line->next) {
                        cursor_col = 0;
                        if (cursor_row < max_y - 3) {
                            cursor_row++;
                        } else {
                            screen_start_line++;
                        }
                        screen_start_col = 0; // Reset horizontal scrolling when moving to a new line
                    }
                } else {
                    screen_start_col += 10;
                }
                break;

            case KEY_PPAGE: // PgUp
                if (editor_mode && screen_start_line == 0) {
                    cursor_row = 0;  // Move cursor to the first line
                }
                screen_start_line -= max_y - 2;
                if (screen_start_line < 0) screen_start_line = 0;
                break;

            case KEY_NPAGE: // PgDn
                if (editor_mode && screen_start_line + max_y > num_lines - 2) {
                    cursor_row = num_lines - screen_start_line - 1;  // Move cursor to the last line
                    if (cursor_row > max_y - 3) cursor_row = max_y - 3;
                }

                screen_start_line += max_y - 2;
                if (screen_start_line > num_lines - (max_y - 2)) {
                    screen_start_line = num_lines - (max_y - 2);
                }
                if (screen_start_line < 0) screen_start_line = 0;

                break;

            case KEY_HOME: // Handle Home key
                if (editor_mode) {
                    cursor_col = 0;
                    screen_start_col = 0;
                } else {
                    screen_start_line = 0;
                }
                break;

            case KEY_END: // Handle End key
                if (editor_mode) {
                    cursor_col = current_line->line_length;
                    if (cursor_col >= max_x) {
                        screen_start_col = cursor_col - max_x + 1;
                        cursor_col = max_x - 1;
                    }
                } else {
                    screen_start_line = num_lines - (max_y - 2);
                    if (screen_start_line < 0) screen_start_line = 0;
                }
                break;

            case KEY_RESIZE: // Handle screen resize
                getmaxyx(stdscr, max_y, max_x); // Update max_y and max_x
                wclear(content_win); // Clear the old window
                wrefresh(content_win);
                delwin(content_win); // Delete the old window
                content_win = newwin(max_y - 2, max_x, 1, 0); // Create a new window with the new dimensions
                wbkgd(content_win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
                wattron(content_win, COLOR_PAIR(COLOR_WHITE_ON_BLUE));
                break;

            case '\n': // Enter key
            {
                if (editor_mode) {
                    is_modified = 1;
                    // Split the current line into two at the cursor's position
                    char *first_half = malloc(absolute_cursor_col + 1);
                    char *second_half = malloc(current_line->line_length - absolute_cursor_col + 1);

                    memcpy(first_half, current_line->line, absolute_cursor_col);
                    first_half[absolute_cursor_col] = '\0';

                    memcpy(second_half, &current_line->line[absolute_cursor_col], current_line->line_length - absolute_cursor_col);
                    second_half[current_line->line_length - absolute_cursor_col] = '\0';

                    // Free the original line memory
                    free(current_line->line);

                    // Adjust the linked list to accommodate the new line
                    file_lines *new_line = malloc(sizeof(file_lines));
                    new_line->line = second_half;
                    new_line->line_length = current_line->line_length - absolute_cursor_col;
                    new_line->next = current_line->next;

                    current_line->next = new_line;
                    current_line->line = first_half;
                    current_line->line_length = absolute_cursor_col;

                    num_lines++;
                    num_bytes++;

                    // Move the cursor to the beginning of the next line
                    if (cursor_row < max_y - 3) {
                        cursor_row++;
                        cursor_col = 0;
                        screen_start_col = 0; // Reset horizontal scrolling when moving to a new line
                    } else {
                        screen_start_line++;
                        cursor_col = 0;
                        screen_start_col = 0; // Reset horizontal scrolling when moving to a new line
                    }
                }
            }
            break;


            case KEY_BACKSPACE: // Handle Backspace key
            {
                if (editor_mode) {
                    is_modified = 1;
                    if (absolute_cursor_col > 0) {
                        // Remove the character to the left of the cursor
                        memmove(&current_line->line[absolute_cursor_col - 1], &current_line->line[absolute_cursor_col], current_line->line_length - absolute_cursor_col);
                        current_line->line_length--;
                        char *new_line = realloc(current_line->line, current_line->line_length);
                        current_line->line = new_line;
                        num_bytes--;

                        // Move the cursor to the left
                        if (cursor_col > 0) {
                            cursor_col--;
                        } else {
                            screen_start_col--;
                        }
                    } else if (cursor_row > 0) {
                        // Merge the current line with the previous line
                        file_lines *prev_line = lines;
                        for (int i = 0; i < screen_start_line + cursor_row - 1; i++) {
                            prev_line = prev_line->next;
                        }

                        int original_prev_line_length = prev_line->line_length; // Store the original length before the merge

                        int new_length = prev_line->line_length + current_line->line_length;
                        char *merged_line = realloc(prev_line->line, new_length);
                        memcpy(&merged_line[prev_line->line_length], current_line->line, current_line->line_length);

                        prev_line->line = merged_line;
                        prev_line->line_length = new_length;
                        prev_line->next = current_line->next;
                        free(current_line->line);
                        free(current_line);
                        current_line = prev_line;
                        num_lines--;
                        num_bytes--;

                        cursor_row--;
                        cursor_col = original_prev_line_length - screen_start_col;
                    } else {
                        is_modified = 0;
                    }
                }
            }
            break;

            case KEY_DC: // Handle Delete key
            {
                if (editor_mode) {
                    is_modified = 1;
                    if (absolute_cursor_col < current_line->line_length) {
                        // Remove the character at the cursor position
                        memmove(&current_line->line[absolute_cursor_col], &current_line->line[absolute_cursor_col + 1], current_line->line_length - absolute_cursor_col - 1);
                        current_line->line_length--;
                        char *new_line = realloc(current_line->line, current_line->line_length);
                        current_line->line = new_line;
                        num_bytes--;
                    } else if (current_line->next) {
                        // Merge the current line with the next line
                        int new_length = current_line->line_length + current_line->next->line_length;
                        char *merged_line = realloc(current_line->line, new_length);
                        memcpy(&merged_line[current_line->line_length], current_line->next->line, current_line->next->line_length);
                        current_line->line = merged_line;
                        current_line->line_length = new_length;
                        file_lines *temp = current_line->next;
                        current_line->next = temp->next;
                        free(temp->line);
                        free(temp);
                        num_lines--;
                        num_bytes--;
                    } else {
                        is_modified = 0;
                    }
                }
            }
            break;
        }


        if (input == KEY_MOUSE && editor_mode) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                // Mouse clicked at event.y and event.x

                // Adjust based on screen start lines and columns due to scrolling
                int target_line = event.y - 1 + screen_start_line; // -1 to account for top row
                int target_col = event.x + screen_start_col;

                // Update cursor_row and cursor_col with the target values.
                cursor_row = event.y - 1;
                cursor_col = event.x;

                // Don't move cursor beyond the last line or beyond the line length
                if (target_line >= num_lines) {
                    cursor_row = num_lines - screen_start_line - 1;
                }
            }
        }


        // insert character where it belongs
        if (editor_mode && (isprint(input) || input == 9)) {
            is_modified = 1; num_bytes++;
            // Reallocate memory for the new character
            char *new_line = realloc(current_line->line, current_line->line_length + 1);
            current_line->line = new_line;
            memmove(&current_line->line[absolute_cursor_col + 1], &current_line->line[absolute_cursor_col], current_line->line_length - absolute_cursor_col);
            current_line->line[absolute_cursor_col] = input;
            current_line->line_length++;

            // Move the cursor to the right after inserting the character
            if (cursor_col < max_x - 1) {
                cursor_col++;
            } else {
                screen_start_col++;
            }
        }


        if (editor_mode) {

            // re-get current line, since it may have changed
            file_lines *current_line = lines;
            for (int i = 0; i < screen_start_line + cursor_row; i++) {
                current_line = current_line->next;
            }

            // recalculate absolutes
            absolute_cursor_col = cursor_col + screen_start_col;

            // Don't move cursor beyond the end of the line
            if (absolute_cursor_col > current_line->line_length) {
                cursor_col = current_line->line_length - screen_start_col;
                if (cursor_col < 0) {
                    screen_start_col = current_line->line_length;
                    cursor_col = 0;
                }
                absolute_cursor_col = cursor_col + screen_start_col;
                skip_refresh = 0; // Force a refresh to update cursor position
            }
        }

        if (!skip_refresh) {
            curs_set(0); // Hide cursor

            // Update the current pointer based on screen_start_line
            current = lines;
            for (int i = 0; i < screen_start_line; i++) {
                current = current->next;
            }

            // Redisplay the window content
            file_lines *temp = current;
            int i;
            for (i = 0; i < max_y - 2 && temp != NULL; i++) {
                wmove(content_win, i, 0);
                wclrtoeol(content_win);
                display_line(content_win, temp, max_x, screen_start_col, editor_mode, patterns, num_patterns);
                temp = temp->next;
            }

            // Clear any remaining lines on the screen
            for (; i < max_y - 2; i++) {
                wmove(content_win, i, 0);
                wclrtoeol(content_win);
            }
        }

        if (editor_mode) {
            wmove(content_win, cursor_row, cursor_col);
        }
    }

    return 0;
}

int view_file(char *filename) {
    return view_edit_file(filename, 0);
}

int edit_file(char *filename) {
    return view_edit_file(filename, 1);
}
