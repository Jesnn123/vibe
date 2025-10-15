/*
 * VIBE Parser Tool - Interactive TUI Dashboard
 *
 * This is a full-featured terminal UI for watching the VIBE parser work in real-time.
 * You can step through parsing line-by-line, see tokens being generated, watch API calls,
 * and learn the VIBE spec as you go. Pretty handy for debugging configs or just understanding
 * how the parser thinks.
 *
 * Built with ncurses because sometimes a GUI is overkill - terminals are everywhere.
 */

#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 600
#include <ncurses.h>
#include <panel.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include "vibe.h"

// Color scheme for the UI - we use 15 different color pairs
// Each panel and element type gets its own color for clarity
#define COLOR_TITLE 1
#define COLOR_BORDER 2
#define COLOR_HIGHLIGHT 3
#define COLOR_SUCCESS 4
#define COLOR_ERROR 5
#define COLOR_INFO 6
#define COLOR_WARNING 7
#define COLOR_CURRENT_LINE 8
#define COLOR_API_CALL 9
#define COLOR_DATA 10
#define COLOR_COMMENT 11
#define COLOR_KEY 12
#define COLOR_VALUE 13
#define COLOR_HEADER 14
#define COLOR_ACCENT 15

// Each panel is a window in the UI - we use ncurses for this
// The panel library lets us layer them and manage z-order
typedef struct {
    WINDOW *win;              // The actual ncurses window
    PANEL *panel;             // Panel wrapper (for stacking)
    const char *title;        // What shows at the top
    const char *description;  // Subtitle text
    int height;               // Size and position - calculated
    int width;                // based on terminal dimensions
    int starty;
    int startx;
} DashboardPanel;

// The whole dashboard is 8 panels arranged in a grid
// Left column: config source, parser state, memory stats
// Right column: API calls, spec tutorial, token stream
// Bottom: help panel with keyboard shortcuts
typedef struct {
    DashboardPanel config_panel;           // Shows the VIBE config being parsed
    DashboardPanel parser_state_panel;     // Parser internals (position, progress, etc.)
    DashboardPanel api_calls_panel;        // Every API function call we make
    DashboardPanel data_structure_panel;   // The resulting data tree (not used much)
    DashboardPanel token_panel;            // Tokens as they're recognized
    DashboardPanel memory_panel;           // Memory usage and stats
    DashboardPanel spec_panel;             // Interactive VIBE spec tutorial
    DashboardPanel help_panel;             // Keyboard shortcuts
} Dashboard;

// We log every API call so you can see exactly what the parser is doing
// This makes it super clear how VIBE maps text to function calls
typedef struct {
    char operation[256];      // What are we trying to do?
    char api_function[128];   // Which function did we call?
    char parameters[256];     // What did we pass to it?
    char result[256];         // What did it return?
    char explanation[256];    // Human-readable explanation
    int timestamp;            // When did this happen?
} APICallLog;

// Track every token we recognize during lexing
// Helps you understand how VIBE breaks text into pieces
typedef struct {
    char token_type[64];      // IDENTIFIER, STRING, LBRACE, etc.
    char token_value[256];    // The actual text (if applicable)
    int line;                 // Position in the source
    int column;
    char context[128];        // What's this token for?
    char spec_reference[128]; // Which part of the spec does this relate to?
} TokenInfo;

// Track VIBE spec compliance - are we following the rules?
// (Not heavily used currently but the infrastructure is there)
typedef struct {
    char rule[128];           // Which grammar rule?
    char description[256];    // What does it mean?
    bool satisfied;           // Did we pass?
} SpecCheck;

// Ridiculous limits because we're using static arrays for simplicity
// In reality you'll never hit these - most configs have < 100 tokens
#define MAX_API_LOGS    900000
#define MAX_TOKENS      900000
#define MAX_SPEC_CHECKS 200

// Global logs - yes, global state, but this is a demo tool not production code
APICallLog api_logs[MAX_API_LOGS];
int api_log_count = 0;
TokenInfo tokens[MAX_TOKENS];
int token_count = 0;
SpecCheck spec_checks[MAX_SPEC_CHECKS];
int spec_check_count = 0;

// History for the rewind feature - we save state before each step
// so you can go backward through the parsing process
#define MAX_HISTORY 1000
typedef struct {
    int config_line;       // Which line were we on?
    int step;              // Step number
    int token_count;       // How many tokens had we seen?
    int api_log_count;     // How many API calls had we made?
} ParserState;

ParserState state_history[MAX_HISTORY];
int history_count = 0;

// Terminal resize optimization - only redraw if size actually changed
// Prevents flickering when the terminal fires multiple resize events
int last_resize_y = 0;
int last_resize_x = 0;
bool needs_full_redraw = false;

// Parsing visualization state - where are we in the process?
int current_step = 0;           // Which step of parsing?
int total_steps = 0;            // How many total steps?
bool paused = true;             // Are we in step-by-step mode?
int current_config_line = 0;    // Which line of config are we on?
char **config_lines = NULL;     // The config file split into lines
int config_line_count = 0;      // How many lines in the file?
time_t start_time;              // When did we start? (for uptime)

// You can load VIBE configs from multiple sources
// Files are most common, but stdin/sockets are handy for piping/streaming
typedef enum {
    INPUT_FILE,      // Load from a file path
    INPUT_STDIN,     // Read from standard input
    INPUT_SOCKET,    // Listen on a TCP socket
    INPUT_PASTE      // Interactive paste mode
} InputMethod;

// Function prototypes - lots of them, this is a big program!
// Organized by: UI setup, panel updates, data logging, input handling, parsing simulation
void init_colors(void);
void create_panel(DashboardPanel *panel, int height, int width, int starty, int startx,
                  const char *title, const char *description);
void draw_fancy_border(DashboardPanel *panel);
void init_dashboard(Dashboard *dash);
void cleanup_dashboard(Dashboard *dash);
void update_config_panel(DashboardPanel *panel, int current_line);
void update_parser_state_panel(DashboardPanel *panel, VibeParser *parser);
void update_api_calls_panel(DashboardPanel *panel);
void update_data_structure_panel(DashboardPanel *panel, VibeValue *value);
void update_token_panel(DashboardPanel *panel);
void update_memory_panel(DashboardPanel *panel, size_t allocated, size_t freed);
void update_spec_panel(DashboardPanel *panel);
void update_help_panel(DashboardPanel *panel);
void add_api_log(const char *operation, const char *api_function, const char *params,
                 const char *result, const char *explanation);
void add_token(const char *type, const char *value, int line, int col,
               const char *context, const char *spec_ref);
void add_spec_check(const char *rule, const char *description, bool satisfied);
void load_config_file(const char *filename);
void load_config_from_string(const char *content);
void simulate_parsing_step(VibeParser *parser, Dashboard *dash);
void render_value_tree(WINDOW *win, VibeValue *value, int *y, int x, int max_y, int indent);
const char *type_to_string(VibeType type);
char* read_from_stdin(void);
char* read_from_socket(int port);
char* read_paste_input(void);
char* prompt_file_input(void);
void show_input_menu(void);
int create_socket_listener(int port);
char* receive_socket_data(int socket_fd);
void draw_ascii_box(WINDOW *win, int y, int x, int width, const char *content);
void draw_progress_bar(WINDOW *win, int y, int x, int width, float percent);

// Set up the color scheme - ncurses needs this before we can use colors
// We're using a classic terminal color palette: bright but not garish
void init_colors(void) {
    start_color();
    init_pair(COLOR_TITLE, COLOR_CYAN, COLOR_BLACK);           // Panel titles - cyan stands out
    init_pair(COLOR_BORDER, COLOR_BLUE, COLOR_BLACK);          // Borders and separators
    init_pair(COLOR_HIGHLIGHT, COLOR_YELLOW, COLOR_BLACK);     // Important stuff
    init_pair(COLOR_SUCCESS, COLOR_GREEN, COLOR_BLACK);        // Things that worked
    init_pair(COLOR_ERROR, COLOR_RED, COLOR_BLACK);            // Things that broke
    init_pair(COLOR_INFO, COLOR_CYAN, COLOR_BLACK);            // Informational text
    init_pair(COLOR_WARNING, COLOR_MAGENTA, COLOR_BLACK);      // Pay attention!
    init_pair(COLOR_CURRENT_LINE, COLOR_BLACK, COLOR_YELLOW);  // Current line - inverted for emphasis
    init_pair(COLOR_API_CALL, COLOR_GREEN, COLOR_BLACK);       // API function names
    init_pair(COLOR_DATA, COLOR_MAGENTA, COLOR_BLACK);         // Data values
    init_pair(COLOR_COMMENT, COLOR_BLUE, COLOR_BLACK);         // Comments - subtle blue
    init_pair(COLOR_KEY, COLOR_YELLOW, COLOR_BLACK);           // Config keys
    init_pair(COLOR_VALUE, COLOR_GREEN, COLOR_BLACK);          // Config values
    init_pair(COLOR_HEADER, COLOR_WHITE, COLOR_BLUE);          // Section headers - bold
    init_pair(COLOR_ACCENT, COLOR_MAGENTA, COLOR_BLACK);       // Decorative elements
}

void create_panel(DashboardPanel *panel, int height, int width, int starty, int startx,
                  const char *title, const char *description) {
    if (!panel) return;

    // Ensure positive dimensions
    if (height < 3) height = 3;
    if (width < 10) width = 10;
    if (starty < 0) starty = 0;
    if (startx < 0) startx = 0;

    panel->height = height;
    panel->width = width;
    panel->starty = starty;
    panel->startx = startx;
    panel->title = title ? title : "Panel";
    panel->description = description ? description : "";
    panel->win = newwin(height, width, starty, startx);

    if (!panel->win) {
        fprintf(stderr, "Error: Failed to create window\n");
        return;
    }

    panel->panel = new_panel(panel->win);
}

void draw_fancy_border(DashboardPanel *panel) {
    if (!panel || !panel->win) return;

    /* Fancy double-line border */
    wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_BOLD);

    /* Corners and edges */
    mvwaddch(panel->win, 0, 0, ACS_ULCORNER);
    mvwaddch(panel->win, 0, panel->width - 1, ACS_URCORNER);
    mvwaddch(panel->win, panel->height - 1, 0, ACS_LLCORNER);
    mvwaddch(panel->win, panel->height - 1, panel->width - 1, ACS_LRCORNER);

    /* Top and bottom */
    for (int i = 1; i < panel->width - 1; i++) {
        mvwaddch(panel->win, 0, i, ACS_HLINE);
        mvwaddch(panel->win, panel->height - 1, i, ACS_HLINE);
    }

    /* Sides */
    for (int i = 1; i < panel->height - 1; i++) {
        mvwaddch(panel->win, i, 0, ACS_VLINE);
        mvwaddch(panel->win, i, panel->width - 1, ACS_VLINE);
    }

    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_BOLD);

    /* Title with fancy background */
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, 0, 3, "[ %s ]", panel->title);
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    /* Description if available */
    if (panel->description && strlen(panel->description) > 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_INFO));
        mvwprintw(panel->win, 1, 2, "%.*s", panel->width - 4, panel->description);
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
    }
}

void draw_progress_bar(WINDOW *win, int y, int x, int width, float percent) {
    if (!win || width <= 0) return;

    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;

    int filled = (int)(percent / 100.0f * width);
    if (filled > width) filled = width;

    mvwaddch(win, y, x, '[');
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            wattron(win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
            waddch(win, '=');
            wattroff(win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
        } else if (i == filled) {
            wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT));
            waddch(win, '>');
            wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT));
        } else {
            wattron(win, COLOR_PAIR(COLOR_BORDER));
            waddch(win, '-');
            wattroff(win, COLOR_PAIR(COLOR_BORDER));
        }
    }
    waddch(win, ']');
}

void draw_ascii_box(WINDOW *win, int y, int x, int width, const char *content) {
    if (!win || width <= 0) return;

    wattron(win, COLOR_PAIR(COLOR_ACCENT));
    mvwaddch(win, y, x, ACS_ULCORNER);
    for (int i = 0; i < width - 2; i++) waddch(win, ACS_HLINE);
    waddch(win, ACS_URCORNER);
    mvwaddch(win, y + 1, x, ACS_VLINE);
    wprintw(win, " %-*.*s ", width - 4, width - 4, content);
    waddch(win, ACS_VLINE);
    mvwaddch(win, y + 2, x, ACS_LLCORNER);
    for (int i = 0; i < width - 2; i++) waddch(win, ACS_HLINE);
    waddch(win, ACS_LRCORNER);
    wattroff(win, COLOR_PAIR(COLOR_ACCENT));
}

// Initialize the whole dashboard - create and position all 8 panels
// This calculates the layout based on current terminal size
void init_dashboard(Dashboard *dash) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // We need at least 80x24 to fit everything
    // Below that and panels start overlapping - not pretty
    if (max_y < 24 || max_x < 80) {
        // Don't crash, just show a friendly message
        clear();
        mvprintw(max_y / 2, (max_x - 40) / 2, "Terminal too small: %dx%d", max_x, max_y);
        mvprintw(max_y / 2 + 1, (max_x - 40) / 2, "Please resize to at least 80x24");
        refresh();
        return;
    }

    int col1_width = max_x / 2;
    int col2_width = max_x - col1_width;
    int row_height = (max_y - 4) / 3;

    // Ensure positive dimensions
    if (row_height < 5) row_height = 5;
    if (col1_width < 20) col1_width = 20;
    if (col2_width < 20) col2_width = 20;

    /* Left column */
    create_panel(&dash->config_panel, row_height, col1_width, 0, 0,
                 "VIBE CONFIG SOURCE", "Raw configuration being parsed line-by-line");
    create_panel(&dash->parser_state_panel, row_height, col1_width, row_height, 0,
                 "PARSER INTERNALS", "Real-time parser state: position, line, column, progress");
    create_panel(&dash->memory_panel, row_height, col1_width, row_height * 2, 0,
                 "STATISTICS & MEMORY", "Performance metrics and resource usage tracking");

    /* Right column */
    create_panel(&dash->api_calls_panel, row_height - 3, col2_width, 0, col1_width,
                 "API CALL TRACE", "Every VIBE API function call with parameters & results");
    create_panel(&dash->spec_panel, row_height + 3, col2_width, row_height - 3, col1_width,
                 "VIBE SPEC COMPLIANCE", "Grammar rules and specification requirements");
    create_panel(&dash->token_panel, row_height, col2_width, row_height * 2, col1_width,
                 "TOKEN STREAM", "Lexical analysis: breaking text into meaningful tokens");

    /* Bottom bar */
    create_panel(&dash->help_panel, 4, max_x, max_y - 4, 0,
                 "CONTROLS", "");
}

// When the terminal is resized, we need to rebuild everything
// Can't just move panels around - ncurses doesn't work that way
void reinit_dashboard(Dashboard *dash) {
    if (!dash) return;

    // First, tear down the old layout completely
    cleanup_dashboard(dash);

    // Force ncurses to redraw everything from scratch
    erase();
    clearok(stdscr, TRUE);
    refresh();

    // Now rebuild with the new dimensions
    init_dashboard(dash);
}

void cleanup_dashboard(Dashboard *dash) {
    if (!dash) return;

    if (dash->config_panel.panel) del_panel(dash->config_panel.panel);
    if (dash->config_panel.win) delwin(dash->config_panel.win);

    if (dash->parser_state_panel.panel) del_panel(dash->parser_state_panel.panel);
    if (dash->parser_state_panel.win) delwin(dash->parser_state_panel.win);

    if (dash->api_calls_panel.panel) del_panel(dash->api_calls_panel.panel);
    if (dash->api_calls_panel.win) delwin(dash->api_calls_panel.win);

    if (dash->data_structure_panel.panel) del_panel(dash->data_structure_panel.panel);
    if (dash->data_structure_panel.win) delwin(dash->data_structure_panel.win);

    if (dash->token_panel.panel) del_panel(dash->token_panel.panel);
    if (dash->token_panel.win) delwin(dash->token_panel.win);

    if (dash->memory_panel.panel) del_panel(dash->memory_panel.panel);
    if (dash->memory_panel.win) delwin(dash->memory_panel.win);

    if (dash->spec_panel.panel) del_panel(dash->spec_panel.panel);
    if (dash->spec_panel.win) delwin(dash->spec_panel.win);

    if (dash->help_panel.panel) del_panel(dash->help_panel.panel);
    if (dash->help_panel.win) delwin(dash->help_panel.win);
}

void update_config_panel(DashboardPanel *panel, int current_line) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    int y = 2;
    int max_y = panel->height - 2;


    // Safety checks
    if (!config_lines || config_line_count == 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
        mvwprintw(panel->win, y + 2, 4, "! No configuration loaded");
        wattroff(panel->win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);

        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y + 3, 4, "  -> Use one of the input methods to load config");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        wrefresh(panel->win);
        return;
    }

    if (current_line >= config_line_count) current_line = config_line_count - 1;
    if (current_line < 0) current_line = 0;

    // Calculate progress
    int progress = config_line_count > 0 ? (current_line * 100 / config_line_count) : 0;

    // Calculate nesting depth and build breadcrumb
    int depth = 0;
    char breadcrumb_parts[32][32];  // Stack of path parts
    int breadcrumb_count = 0;
    strcpy(breadcrumb_parts[breadcrumb_count++], "root");

    for (int i = 0; i <= current_line && i < config_line_count; i++) {
        if (config_lines[i]) {
            if (strchr(config_lines[i], '{')) {
                depth++;
                if (breadcrumb_count < 31) {
                    char *space = strchr(config_lines[i], ' ');
                    if (space && space > config_lines[i]) {
                        int len = (space - config_lines[i]) < 30 ? (space - config_lines[i]) : 30;
                        strncpy(breadcrumb_parts[breadcrumb_count], config_lines[i], len);
                        breadcrumb_parts[breadcrumb_count][len] = '\0';
                        breadcrumb_count++;
                    } else {
                        strcpy(breadcrumb_parts[breadcrumb_count++], "obj");
                    }
                }
            }
            if (strchr(config_lines[i], '}')) {
                depth--;
                if (breadcrumb_count > 1) {
                    breadcrumb_count--;
                }
            }
        }
    }
    if (depth < 0) depth = 0;

    // Build breadcrumb string from parts
    char breadcrumb[256] = "";
    for (int i = 0; i < breadcrumb_count; i++) {
        if (i > 0) strcat(breadcrumb, " > ");
        strcat(breadcrumb, breadcrumb_parts[i]);
    }

    // Analyze current line type
    char *curr_line_text = "";
    const char *line_type = "Empty";
    const char *line_icon = " ";
    if (current_line < config_line_count && config_lines[current_line]) {
        curr_line_text = config_lines[current_line];
        char *trimmed = curr_line_text;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        if (trimmed[0] == '#') {
            line_type = "Comment";
            line_icon = "#";
        } else if (strchr(trimmed, '{')) {
            line_type = "Object";
            line_icon = "{";
        } else if (strchr(trimmed, '}')) {
            line_type = "Close";
            line_icon = "}";
        } else if (strchr(trimmed, '[')) {
            line_type = "Array";
            line_icon = "[";
        } else if (strchr(trimmed, ']')) {
            line_type = "Close";
            line_icon = "]";
        } else if (strchr(trimmed, ' ')) {
            line_type = "KeyVal";
            line_icon = "=";
        } else if (strlen(trimmed) > 0) {
            line_type = "Value";
            line_icon = "*";
        }
    }

    // Status bar at top
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, " ");

    // Line info with type indicator
    mvwprintw(panel->win, y, 4, "[%s] %s", line_icon, line_type);
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    wattron(panel->win, COLOR_PAIR(COLOR_INFO));
    wprintw(panel->win, " Line %d/%d", current_line + 1, config_line_count);
    wattroff(panel->win, COLOR_PAIR(COLOR_INFO));

    // Depth indicator
    wattron(panel->win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
    wprintw(panel->win, " | Depth:%d", depth);
    wattroff(panel->win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);

    // Progress bar
    int bar_start = panel->width - 30;
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, y, bar_start, "[");
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    int bar_width = 20;
    int filled = (progress * bar_width) / 100;
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
            wprintw(panel->win, "=");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
        } else {
            wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
            wprintw(panel->win, "-");
            wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
        }
    }

    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    wprintw(panel->win, "] %3d%%", progress);
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    y++;

    // Breadcrumb trail
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT));
    mvwprintw(panel->win, y, 4, "Path: ");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT));

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
    wprintw(panel->win, "%-.*s", panel->width - 15, breadcrumb);
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
    y++;

    // Separator line
    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, "=");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    // Find matching brace for current line (if applicable)
    int matching_brace_line = -1;
    if (current_line < config_line_count && config_lines[current_line]) {
        char *curr = config_lines[current_line];
        if (strchr(curr, '{')) {
            // Find closing }
            int brace_count = 1;
            for (int i = current_line + 1; i < config_line_count && brace_count > 0; i++) {
                if (config_lines[i]) {
                    if (strchr(config_lines[i], '{')) brace_count++;
                    if (strchr(config_lines[i], '}')) brace_count--;
                    if (brace_count == 0) {
                        matching_brace_line = i;
                        break;
                    }
                }
            }
        } else if (strchr(curr, '}')) {
            // Find opening {
            int brace_count = 1;
            for (int i = current_line - 1; i >= 0 && brace_count > 0; i--) {
                if (config_lines[i]) {
                    if (strchr(config_lines[i], '}')) brace_count++;
                    if (strchr(config_lines[i], '{')) brace_count--;
                    if (brace_count == 0) {
                        matching_brace_line = i;
                        break;
                    }
                }
            }
        }
    }

    // Content area
    int start_line = (current_line > (max_y - y) / 2) ? current_line - (max_y - y) / 2 : 0;
    if (start_line >= config_line_count) start_line = config_line_count - 1;
    if (start_line < 0) start_line = 0;

    // Scroll indicator at top
    if (start_line > 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y, panel->width / 2 - 6, "  ... more ...  ");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        y++;
    }

    for (int i = start_line; i < config_line_count && y < max_y - 1; i++) {
        if (!config_lines[i]) continue;

        char *line = config_lines[i];
        int indent = 0;
        while (line[indent] == ' ' || line[indent] == '\t') indent++;
        if (line[indent] == '\t') indent *= 2;

        if (i == current_line) {
            /* ========== CURRENT LINE - AWESOME HIGHLIGHT ========== */

            // Depth indicator bars on the left
            wattron(panel->win, COLOR_PAIR(COLOR_WARNING));
            for (int d = 0; d < depth && d < 3; d++) {
                mvwprintw(panel->win, y, 2 + d, "|");
            }
            wattroff(panel->win, COLOR_PAIR(COLOR_WARNING));

            // Left bracket with emphasis
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
            mvwprintw(panel->win, y, 2 + (depth < 3 ? depth : 3), ">>>");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);

            // Line number in yellow background
            wattron(panel->win, COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD);
            wprintw(panel->win, " %4d ", i + 1);
            wattroff(panel->win, COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD);

            // Right bracket
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
            wprintw(panel->win, "<<<");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);

            wprintw(panel->win, " ");

            // Content with yellow background
            char *trimmed = line;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

            // Display with full yellow background
            wattron(panel->win, COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD);

            // Add indentation
            for (int j = 0; j < indent; j++) wprintw(panel->win, " ");

            if (trimmed[0] == '#') {
                wprintw(panel->win, "# %s", trimmed + 1);
            } else if (strchr(trimmed, '{') || strchr(trimmed, '}')) {
                wprintw(panel->win, "%s", trimmed);
            } else if (strchr(trimmed, '[') || strchr(trimmed, ']')) {
                wprintw(panel->win, "%s", trimmed);
            } else {
                char *space = strchr(trimmed, ' ');
                if (space) {
                    char key[256] = {0};
                    strncpy(key, trimmed, space - trimmed);
                    wprintw(panel->win, "%s  %s", key, space + 1);
                } else {
                    wprintw(panel->win, "%s", trimmed);
                }
            }

            // Pad to end with yellow background
            int curr_x = getcurx(panel->win);
            for (int j = curr_x; j < panel->width - 2; j++) wprintw(panel->win, " ");

            wattroff(panel->win, COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD);

        } else {
            /* Context lines with nice styling */
            int distance = abs(i - current_line);
            bool is_very_close = distance == 1;
            bool is_close = distance <= 3;
            bool is_matching_brace = (i == matching_brace_line);

            // Calculate this line's depth for visual guide
            int line_depth = 0;
            for (int j = 0; j < i && j < config_line_count; j++) {
                if (config_lines[j]) {
                    if (strchr(config_lines[j], '{')) line_depth++;
                    if (strchr(config_lines[j], '}')) line_depth--;
                }
            }
            if (line_depth < 0) line_depth = 0;

            // Depth bars
            if (line_depth > 0) {
                wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
                for (int d = 0; d < line_depth && d < 3; d++) {
                    mvwprintw(panel->win, y, 2 + d, "|");
                }
                wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            }

            // Line indicator
            if (is_matching_brace) {
                wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
                mvwprintw(panel->win, y, 2 + (line_depth < 3 ? line_depth : 3), "<->");
                wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            } else if (!is_close) {
                wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
                mvwprintw(panel->win, y, 2 + (line_depth < 3 ? line_depth : 3), "   ");
                wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            } else if (is_very_close) {
                wattron(panel->win, COLOR_PAIR(COLOR_INFO));
                mvwprintw(panel->win, y, 2 + (line_depth < 3 ? line_depth : 3), " > ");
                wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
            } else {
                wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
                mvwprintw(panel->win, y, 2 + (line_depth < 3 ? line_depth : 3), "   ");
                wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
            }

            // Line number with varying emphasis
            if (is_very_close) {
                wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            } else if (is_close) {
                wattron(panel->win, COLOR_PAIR(COLOR_INFO));
            } else {
                wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            }
            wprintw(panel->win, " %4d ", i + 1);

            if (is_very_close) {
                wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            } else if (is_close) {
                wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
            } else {
                wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            }

            wprintw(panel->win, "    ");

            // Indentation guides for nested content
            if (indent > 0 && is_close) {
                wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
                for (int j = 0; j < indent / 2 && j < 4; j++) {
                    wprintw(panel->win, "| ");
                }
                wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
            } else {
                for (int j = 0; j < indent; j++) wprintw(panel->win, " ");
            }

            // Line content with syntax highlighting
            char *trimmed = line;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

            if (strlen(trimmed) == 0) {
                // Empty line
                wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
                wprintw(panel->win, "(empty)");
                wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            } else if (trimmed[0] == '#') {
                // Comment
                int color_attr = is_very_close ? (COLOR_PAIR(COLOR_COMMENT) | A_BOLD) :
                                is_close ? COLOR_PAIR(COLOR_COMMENT) :
                                (COLOR_PAIR(COLOR_COMMENT) | A_DIM);
                wattron(panel->win, color_attr);
                wprintw(panel->win, "# %s", trimmed + 1);
                wattroff(panel->win, color_attr);
            } else if (strchr(trimmed, '{')) {
                // Object start
                int color_attr = is_very_close ? (COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD) :
                                is_close ? COLOR_PAIR(COLOR_HIGHLIGHT) :
                                (COLOR_PAIR(COLOR_HIGHLIGHT) | A_DIM);
                wattron(panel->win, color_attr);
                wprintw(panel->win, "%s", trimmed);
                wattroff(panel->win, color_attr);
            } else if (strchr(trimmed, '}')) {
                // Object end
                int color_attr = is_very_close ? (COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD) :
                                is_close ? COLOR_PAIR(COLOR_HIGHLIGHT) :
                                (COLOR_PAIR(COLOR_HIGHLIGHT) | A_DIM);
                wattron(panel->win, color_attr);
                wprintw(panel->win, "%s", trimmed);
                wattroff(panel->win, color_attr);
            } else if (strchr(trimmed, '[') || strchr(trimmed, ']')) {
                // Array
                int color_attr = is_very_close ? (COLOR_PAIR(COLOR_WARNING) | A_BOLD) :
                                is_close ? COLOR_PAIR(COLOR_WARNING) :
                                (COLOR_PAIR(COLOR_WARNING) | A_DIM);
                wattron(panel->win, color_attr);
                wprintw(panel->win, "%s", trimmed);
                wattroff(panel->win, color_attr);
            } else {
                // Key-value pair
                char *space = strchr(trimmed, ' ');
                if (space) {
                    char key[256] = {0};
                    strncpy(key, trimmed, space - trimmed);

                    int key_attr = is_very_close ? (COLOR_PAIR(COLOR_KEY) | A_BOLD) :
                                  is_close ? COLOR_PAIR(COLOR_KEY) :
                                  (COLOR_PAIR(COLOR_KEY) | A_DIM);
                    wattron(panel->win, key_attr);
                    wprintw(panel->win, "%s", key);
                    wattroff(panel->win, key_attr);

                    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT));
                    wprintw(panel->win, "  ");
                    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT));

                    int val_attr = is_very_close ? COLOR_PAIR(COLOR_VALUE) :
                                  is_close ? (COLOR_PAIR(COLOR_VALUE) | A_DIM) :
                                  (COLOR_PAIR(COLOR_VALUE) | A_DIM);
                    wattron(panel->win, val_attr);
                    wprintw(panel->win, "%s", space + 1);
                    wattroff(panel->win, val_attr);
                } else {
                    wattron(panel->win, is_close ? COLOR_PAIR(COLOR_BORDER) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
                    wprintw(panel->win, "%s", trimmed);
                    wattroff(panel->win, is_close ? COLOR_PAIR(COLOR_BORDER) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
                }
            }
        }

        y++;
    }

    // Scroll indicator at bottom
    if (current_line < config_line_count - 1 && y < max_y) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y, panel->width / 2 - 6, "  ... more ...  ");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
    }

    wrefresh(panel->win);
}

void update_parser_state_panel(DashboardPanel *panel, VibeParser *parser) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    int y = 2;

    if (!parser) {
        wattron(panel->win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
        mvwprintw(panel->win, y + 2, 4, "! Parser not initialized");
        wattroff(panel->win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
        wrefresh(panel->win);
        return;
    }

    // Calculate runtime statistics
    time_t elapsed = time(NULL) - start_time;
    float progress = (parser->length > 0) ? (float)parser->pos / parser->length * 100.0f : 0.0f;
    float bytes_per_sec = elapsed > 0 ? (float)parser->pos / elapsed : 0.0f;
    size_t remaining_bytes = parser->length - parser->pos;
    int eta_seconds = bytes_per_sec > 0 ? (int)(remaining_bytes / bytes_per_sec) : 0;

    // Header with status indicator
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, " ");
    mvwprintw(panel->win, y, 4, "PARSER INTERNALS");
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    // Status indicator
    if (parser->error.has_error) {
        wattron(panel->win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
        mvwprintw(panel->win, y, panel->width - 15, "[ERROR]");
        wattroff(panel->win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
    } else {
        wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
        mvwprintw(panel->win, y, panel->width - 15, "[ACTIVE]");
        wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    }
    y++;

    // Separator
    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, "=");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y += 2;

    // Position metrics in a grid layout
    int col1 = 3;
    int col2 = panel->width / 2 + 2;

    // Left column - Position info
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y, col1, "POSITION");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "+-----------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| Line:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    wprintw(panel->win, " %-9d|", parser->line);
    wattroff(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| Column:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    wprintw(panel->win, " %-6d|", parser->column);
    wattroff(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| Offset:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    wprintw(panel->win, " %-7zu|", parser->pos);
    wattroff(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "+-----------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));

    // Right column - File info
    int y2 = y - 5;
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y2, col2, "FILE INFO");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y2, col2, "+-----------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y2, col2, "| Total:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_INFO));
    wprintw(panel->win, " %-7zu|", parser->length);
    wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y2, col2, "| Read:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    wprintw(panel->win, " %-8zu|", parser->pos);
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y2, col2, "| Left:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_WARNING));
    wprintw(panel->win, " %-8zu|", remaining_bytes);
    wattroff(panel->win, COLOR_PAIR(COLOR_WARNING));
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y2, col2, "+-----------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));

    y += 2;

    // Performance metrics
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y, col1, "PERFORMANCE");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "+---------------------------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| Runtime:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_INFO));
    wprintw(panel->win, " %02ld:%02ld            |", elapsed / 60, elapsed % 60);
    wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| Speed:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    if (bytes_per_sec > 1024) {
        wprintw(panel->win, " %.1f KB/s          |", bytes_per_sec / 1024);
    } else {
        wprintw(panel->win, " %.0f B/s            |", bytes_per_sec);
    }
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| ETA:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_WARNING));
    if (eta_seconds > 0) {
        wprintw(panel->win, " %02d:%02d              |", eta_seconds / 60, eta_seconds % 60);
    } else {
        wprintw(panel->win, " --:--              |");
    }
    wattroff(panel->win, COLOR_PAIR(COLOR_WARNING));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "+---------------------------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y += 2;

    // Visual progress section
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y, col1, "PARSING PROGRESS");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y++;

    // Large progress bar
    draw_progress_bar(panel->win, y, col1, panel->width - 8, progress);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    mvwprintw(panel->win, y, col1, "%.2f%% Complete", progress);
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);

    // Show detailed byte info
    wattron(panel->win, COLOR_PAIR(COLOR_INFO));
    mvwprintw(panel->win, y, panel->width - 28, "(%zu / %zu bytes)", parser->pos, parser->length);
    wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
    y++;

    // Visual percentage blocks
    wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
    mvwprintw(panel->win, y, col1, "Blocks: ");
    wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));

    int blocks = 20;
    int filled_blocks = (int)(progress / 100.0 * blocks);
    for (int i = 0; i < blocks; i++) {
        if (i < filled_blocks) {
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
            wprintw(panel->win, "#");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
        } else {
            wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            wprintw(panel->win, ".");
            wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
        }
    }
    y += 2;

    // Error section (if applicable)
    if (parser->error.has_error) {
        wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
        mvwprintw(panel->win, y, col1, " ");
        for (int i = 0; i < panel->width - 6; i++) wprintw(panel->win, "-");
        wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
        mvwprintw(panel->win, y, col1, "!!! PARSE ERROR !!!");
        wattroff(panel->win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_KEY));
        mvwprintw(panel->win, y, col1, "Location:");
        wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
        wattron(panel->win, COLOR_PAIR(COLOR_ERROR));
        wprintw(panel->win, " Line %d, Column %d", parser->error.line, parser->error.column);
        wattroff(panel->win, COLOR_PAIR(COLOR_ERROR));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_KEY));
        mvwprintw(panel->win, y, col1, "Message:");
        wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_ERROR));
        mvwprintw(panel->win, y, col1 + 2, "%.*s", panel->width - col1 - 4, parser->error.message);
        wattroff(panel->win, COLOR_PAIR(COLOR_ERROR));
    } else {
        wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
        mvwprintw(panel->win, y, col1, " ");
        for (int i = 0; i < panel->width - 6; i++) wprintw(panel->win, "-");
        wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
        mvwprintw(panel->win, y, col1, "[OK]");
        wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_INFO));
        wprintw(panel->win, " No errors detected");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
    }

    wrefresh(panel->win);
}

void update_api_calls_panel(DashboardPanel *panel) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    int y = 2;
    int max_y = panel->height - 2;

    // Header with call count
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, " ");
    mvwprintw(panel->win, y, 4, "API CALL TRACE - Live Stream");
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    // Call counter with icon
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    mvwprintw(panel->win, y, panel->width - 22, "[");
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    wattron(panel->win, COLOR_PAIR(COLOR_API_CALL) | A_BOLD);
    wprintw(panel->win, " %d calls ", api_log_count);
    wattroff(panel->win, COLOR_PAIR(COLOR_API_CALL) | A_BOLD);
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    wprintw(panel->win, "]");
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    y++;

    // Separator
    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, "=");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    if (api_log_count == 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y + 2, 4, "No API calls yet...");
        mvwprintw(panel->win, y + 3, 4, "Waiting for parser activity");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        wrefresh(panel->win);
        return;
    }

    y++;

    // Calculate how many calls we can show
    int lines_per_call = 6;  // Increased for more detail
    int visible_calls = (max_y - y) / lines_per_call;
    if (visible_calls < 1) visible_calls = 1;

    int start = (api_log_count > visible_calls) ? api_log_count - visible_calls : 0;

    // Show scroll indicator if needed
    if (start > 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y, panel->width / 2 - 8, "... %d earlier ...", start);
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        y++;
    }

    for (int i = start; i < api_log_count && y < max_y - 1; i++) {
        bool is_latest = (i == api_log_count - 1);
        int indent = 2;
        int card_width = panel->width - 6;

        // Draw card top border
        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "+");
        for (int j = 0; j < card_width; j++) wprintw(panel->win, "-");
        wprintw(panel->win, "+");
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Call header line with ID and timestamp
        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "|");
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

        // Call badge
        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD) : (COLOR_PAIR(COLOR_HEADER) | A_BOLD));
        wprintw(panel->win, " #%-3d ", i + 1);
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD) : (COLOR_PAIR(COLOR_HEADER) | A_BOLD));

        // Timestamp
        time_t call_time = start_time + api_logs[i].timestamp;
        struct tm *tm_info = localtime(&call_time);
        wattron(panel->win, COLOR_PAIR(COLOR_INFO));
        wprintw(panel->win, " @ %02d:%02d:%02d ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO));

        // Right border
        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent + card_width + 1, "|");
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Function name line
        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "|");
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_API_CALL) | A_BOLD) : COLOR_PAIR(COLOR_API_CALL));
        wprintw(panel->win, " CALL: %s()", api_logs[i].api_function);
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_API_CALL) | A_BOLD) : COLOR_PAIR(COLOR_API_CALL));

        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent + card_width + 1, "|");
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Parameters line
        if (strlen(api_logs[i].parameters) > 0 && y < max_y) {
            wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent, "|");
            wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

            wattron(panel->win, COLOR_PAIR(COLOR_KEY));
            wprintw(panel->win, "  IN:");
            wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
            wattron(panel->win, is_latest ? COLOR_PAIR(COLOR_VALUE) : (COLOR_PAIR(COLOR_VALUE) | A_DIM));
            wprintw(panel->win, " %.*s", card_width - 10, api_logs[i].parameters);
            wattroff(panel->win, is_latest ? COLOR_PAIR(COLOR_VALUE) : (COLOR_PAIR(COLOR_VALUE) | A_DIM));

            wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent + card_width + 1, "|");
            wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            y++;
        }

        // Result line
        if (strlen(api_logs[i].result) > 0 && y < max_y) {
            wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent, "|");
            wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

            wattron(panel->win, COLOR_PAIR(COLOR_KEY));
            wprintw(panel->win, " OUT:");
            wattroff(panel->win, COLOR_PAIR(COLOR_KEY));

            // Check for success/failure keywords
            bool is_error = (strstr(api_logs[i].result, "FAIL") != NULL ||
                           strstr(api_logs[i].result, "ERROR") != NULL ||
                           strstr(api_logs[i].result, "NULL") != NULL);
            bool is_success = (strstr(api_logs[i].result, "SUCCESS") != NULL ||
                             strstr(api_logs[i].result, "OK") != NULL ||
                             strstr(api_logs[i].result, "VibeValue") != NULL ||
                             strstr(api_logs[i].result, "VibeParser") != NULL);

            int result_color = is_error ? COLOR_ERROR : (is_success ? COLOR_SUCCESS : COLOR_VALUE);
            wattron(panel->win, COLOR_PAIR(result_color) | (is_latest ? A_BOLD : 0));
            wprintw(panel->win, " %.*s", card_width - 11, api_logs[i].result);
            wattroff(panel->win, COLOR_PAIR(result_color) | (is_latest ? A_BOLD : 0));

            wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent + card_width + 1, "|");
            wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            y++;
        }

        // Explanation line
        if (strlen(api_logs[i].explanation) > 0 && y < max_y) {
            wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent, "|");
            wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

            wattron(panel->win, COLOR_PAIR(COLOR_COMMENT) | (is_latest ? 0 : A_DIM));
            wprintw(panel->win, " -> %.*s", card_width - 5, api_logs[i].explanation);
            wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT) | (is_latest ? 0 : A_DIM));

            wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent + card_width + 1, "|");
            wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            y++;
        }

        // Draw card bottom border
        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "+");
        for (int j = 0; j < card_width; j++) wprintw(panel->win, "-");
        wprintw(panel->win, "+");
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Add spacing between cards
        if (i < api_log_count - 1 && y < max_y - 1) {
            y++;
        }
    }

    // Show if there are more calls below
    if (y >= max_y && api_log_count > start + visible_calls) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, max_y, panel->width / 2 - 6, "... more ...");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
    }

    wrefresh(panel->win);
}

void update_spec_panel(DashboardPanel *panel) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    int y = 2;
    int max_y = panel->height - 2;

    // Header with lesson counter
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, " ");
    mvwprintw(panel->win, y, 4, "VIBE SPEC TUTORIAL");
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    // Lesson indicator
    wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
    mvwprintw(panel->win, y, panel->width - 25, "Lesson %d/%d", current_config_line + 1, config_line_count);
    wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
    y++;

    // Separator
    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, "=");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    // Get current line for analysis
    const char *curr_line = "";
    if (current_config_line < config_line_count && config_lines && config_lines[current_config_line]) {
        curr_line = config_lines[current_config_line];
    }

    // Calculate statistics
    int obj_cnt = 0, arr_cnt = 0, cmt_cnt = 0, kv_cnt = 0;
    for (int i = 0; i <= current_config_line && i < config_line_count; i++) {
        if (config_lines[i]) {
            if (strchr(config_lines[i], '{')) obj_cnt++;
            if (strchr(config_lines[i], '[')) arr_cnt++;
            if (config_lines[i][0] == '#') cmt_cnt++;
            if (strchr(config_lines[i], ' ') && config_lines[i][0] != '#') kv_cnt++;
        }
    }

    if (strlen(curr_line) == 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y + 2, 4, "Empty line - whitespace is ignored in VIBE");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        wrefresh(panel->win);
        return;
    }

    y++;

    // Current line display card
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    mvwprintw(panel->win, y, 3, "+");
    for (int i = 0; i < panel->width - 8; i++) wprintw(panel->win, "=");
    wprintw(panel->win, "+");
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    mvwprintw(panel->win, y, 3, "|");
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    wattron(panel->win, COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD);
    wprintw(panel->win, " CURRENT: %-*.*s ", panel->width - 16, panel->width - 16, curr_line);
    wattroff(panel->win, COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD);
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    mvwprintw(panel->win, y, panel->width - 5, "|");
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    mvwprintw(panel->win, y, 3, "+");
    for (int i = 0; i < panel->width - 8; i++) wprintw(panel->win, "=");
    wprintw(panel->win, "+");
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    y += 2;

    // Trim whitespace from line
    char *trimmed = (char*)curr_line;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

    // Teach based on line type
    if (trimmed[0] == '#') {
        // COMMENT LESSON
        wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        mvwprintw(panel->win, y, 3, "[LESSON] COMMENTS");
        wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        y += 2;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "SYNTAX:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y++, 5, "# <any text here>");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "PURPOSE:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
        mvwprintw(panel->win, y++, 5, "* Document your configuration");
        mvwprintw(panel->win, y++, 5, "* Add notes for other developers");
        mvwprintw(panel->win, y++, 5, "* Temporarily disable config lines");
        wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "PARSER BEHAVIOR:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_KEY));
        mvwprintw(panel->win, y++, 5, "> Completely ignored during parsing");
        mvwprintw(panel->win, y++, 5, "> No memory allocated");
        mvwprintw(panel->win, y++, 5, "> No API calls generated");
        wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "EXAMPLES:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y++, 5, "# Database configuration");
        mvwprintw(panel->win, y++, 5, "# TODO: Add SSL support");
        mvwprintw(panel->win, y++, 5, "# host localhost  <- disabled");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));

    } else if (strchr(trimmed, '{')) {
        // OBJECT LESSON
        wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        mvwprintw(panel->win, y, 3, "[LESSON] OBJECTS / GROUPS");
        wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        y += 2;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "SYNTAX:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
        mvwprintw(panel->win, y++, 5, "key {");
        mvwprintw(panel->win, y++, 7, "nested_key value");
        mvwprintw(panel->win, y++, 5, "}");
        wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "PURPOSE:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
        mvwprintw(panel->win, y++, 5, "* Group related configuration");
        mvwprintw(panel->win, y++, 5, "* Create hierarchical structure");
        mvwprintw(panel->win, y++, 5, "* Organize complex configs");
        wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "MEMORY STRUCTURE:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_KEY));
        mvwprintw(panel->win, y++, 5, "Type: VibeObject");
        mvwprintw(panel->win, y++, 5, "Storage: key-value pairs in entries[]");
        mvwprintw(panel->win, y++, 5, "API: vibe_value_new_object()");
        mvwprintw(panel->win, y++, 5, "Access: vibe_object_get(obj, \"key\")");
        wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "SPEC RULES:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
        mvwprintw(panel->win, y++, 5, "+ Unlimited nesting depth");
        mvwprintw(panel->win, y++, 5, "+ Must have matching closing }");
        mvwprintw(panel->win, y++, 5, "+ Keys must be unique within object");
        wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));

    } else if (strchr(trimmed, '[')) {
        // ARRAY LESSON
        wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        mvwprintw(panel->win, y, 3, "[LESSON] ARRAYS / LISTS");
        wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        y += 2;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "SYNTAX:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_WARNING));
        mvwprintw(panel->win, y++, 5, "key [ item1 item2 item3 ]");
        wattroff(panel->win, COLOR_PAIR(COLOR_WARNING));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "PURPOSE:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
        mvwprintw(panel->win, y++, 5, "* Store multiple values in order");
        mvwprintw(panel->win, y++, 5, "* Create lists of items");
        mvwprintw(panel->win, y++, 5, "* Support mixed types");
        wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "MEMORY STRUCTURE:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_KEY));
        mvwprintw(panel->win, y++, 5, "Type: VibeArray");
        mvwprintw(panel->win, y++, 5, "Storage: VibeValue** array");
        mvwprintw(panel->win, y++, 5, "API: vibe_value_new_array()");
        mvwprintw(panel->win, y++, 5, "Access: vibe_array_get(arr, index)");
        wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "SPEC RULES:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
        mvwprintw(panel->win, y++, 5, "+ Items separated by whitespace");
        mvwprintw(panel->win, y++, 5, "+ Order is preserved");
        mvwprintw(panel->win, y++, 5, "+ Zero-indexed access");
        wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));

    } else if (strchr(trimmed, ' ')) {
        // KEY-VALUE LESSON
        char *space = strchr(trimmed, ' ');
        char val[64] = {0};
        strncpy(val, space + 1, 63);
        char *v = val;
        while (*v == ' ') v++;

        wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        mvwprintw(panel->win, y, 3, "[LESSON] KEY-VALUE PAIRS");
        wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
        y += 2;

        wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "SYNTAX:");
        wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
        wattron(panel->win, COLOR_PAIR(COLOR_KEY));
        mvwprintw(panel->win, y, 5, "key");
        wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
        wattron(panel->win, COLOR_PAIR(COLOR_ACCENT));
        wprintw(panel->win, " ");
        wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT));
        wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
        wprintw(panel->win, "value");
        wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
        y += 2;

        // Type detection
        if (strcmp(v, "true") == 0 || strcmp(v, "false") == 0) {
            wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            mvwprintw(panel->win, y++, 3, "TYPE: BOOLEAN");
            wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(panel->win, y++, 5, "Value: %s", v);
            mvwprintw(panel->win, y++, 5, "Size: 1 byte (bool)");
            mvwprintw(panel->win, y++, 5, "Valid: 'true' or 'false' only");
            wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_KEY));
            mvwprintw(panel->win, y++, 5, "API: vibe_value_new_boolean(%s)", v);
            mvwprintw(panel->win, y++, 5, "Get: vibe_value_as_boolean(val)");
            wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
            mvwprintw(panel->win, y++, 5, "Example: enabled true");
            mvwprintw(panel->win, y++, 5, "Example: debug false");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));

        } else if (strchr(v, '.')) {
            wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            mvwprintw(panel->win, y++, 3, "TYPE: FLOAT");
            wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(panel->win, y++, 5, "Value: %s", v);
            mvwprintw(panel->win, y++, 5, "Size: 8 bytes (double)");
            mvwprintw(panel->win, y++, 5, "Precision: 15-17 digits");
            wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_KEY));
            mvwprintw(panel->win, y++, 5, "API: vibe_value_new_float(%.2f)", atof(v));
            mvwprintw(panel->win, y++, 5, "Get: vibe_value_as_float(val)");
            wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
            mvwprintw(panel->win, y++, 5, "Example: timeout 3.5");
            mvwprintw(panel->win, y++, 5, "Example: rate 0.75");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));

        } else if (v[0] >= '0' && v[0] <= '9') {
            wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            mvwprintw(panel->win, y++, 3, "TYPE: INTEGER");
            wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(panel->win, y++, 5, "Value: %s", v);
            mvwprintw(panel->win, y++, 5, "Size: 8 bytes (int64_t)");
            mvwprintw(panel->win, y++, 5, "Range: -9.2e18 to 9.2e18");
            wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_KEY));
            mvwprintw(panel->win, y++, 5, "API: vibe_value_new_integer(%s)", v);
            mvwprintw(panel->win, y++, 5, "Get: vibe_value_as_integer(val)");
            wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
            mvwprintw(panel->win, y++, 5, "Example: port 8080");
            mvwprintw(panel->win, y++, 5, "Example: max_conn 100");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));

        } else {
            wattron(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            mvwprintw(panel->win, y++, 3, "TYPE: STRING");
            wattroff(panel->win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
            wattron(panel->win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(panel->win, y++, 5, "Value: \"%.*s\"", 30, v);
            mvwprintw(panel->win, y++, 5, "Size: strlen + 1 (heap)");
            mvwprintw(panel->win, y++, 5, "Quotes: Optional");
            wattroff(panel->win, COLOR_PAIR(COLOR_VALUE));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_KEY));
            mvwprintw(panel->win, y++, 5, "API: vibe_value_new_string(\"%s\")", v);
            mvwprintw(panel->win, y++, 5, "Get: vibe_value_as_string(val)");
            wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
            y++;
            wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
            mvwprintw(panel->win, y++, 5, "Example: host localhost");
            mvwprintw(panel->win, y++, 5, "Example: name \"My App\"");
            wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));
        }
    }

    // Progress footer
    if (y < max_y - 2) {
        y = max_y - 5;
        wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
        mvwprintw(panel->win, y, 3, " ");
        for (int i = 0; i < panel->width - 8; i++) wprintw(panel->win, "-");
        wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
        y++;

        wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
        mvwprintw(panel->win, y++, 3, "PROGRESS:");
        wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

        wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
        mvwprintw(panel->win, y, 5, "{} Objects: %d", obj_cnt);
        wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));

        wattron(panel->win, COLOR_PAIR(COLOR_WARNING));
        mvwprintw(panel->win, y, 25, "[] Arrays: %d", arr_cnt);
        wattroff(panel->win, COLOR_PAIR(COLOR_WARNING));

        wattron(panel->win, COLOR_PAIR(COLOR_KEY));
        mvwprintw(panel->win, y, 43, "K:V Pairs: %d", kv_cnt);
        wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    }

    wrefresh(panel->win);
}

const char *type_to_string(VibeType type) {
    switch (type) {
        case VIBE_TYPE_NULL: return "NULL";
        case VIBE_TYPE_INTEGER: return "INTEGER";
        case VIBE_TYPE_FLOAT: return "FLOAT";
        case VIBE_TYPE_BOOLEAN: return "BOOLEAN";
        case VIBE_TYPE_STRING: return "STRING";
        case VIBE_TYPE_ARRAY: return "ARRAY";
        case VIBE_TYPE_OBJECT: return "OBJECT";
        default: return "UNKNOWN";
    }
}

void render_value_tree(WINDOW *win, VibeValue *value, int *y, int x, int max_y, int indent) {
    if (!value || *y >= max_y) return;

    char indent_str[128] = "";
    for (int i = 0; i < indent && i < 30; i++) {
        strcat(indent_str, "  ");
    }

    switch (value->type) {
        case VIBE_TYPE_INTEGER:
            wattron(win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(win, *y, x, "%s+- INT: %lld", indent_str, (long long)value->as_integer);
            wattroff(win, COLOR_PAIR(COLOR_VALUE));
            (*y)++;
            break;

        case VIBE_TYPE_FLOAT:
            wattron(win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(win, *y, x, "%s+- FLOAT: %.2f", indent_str, value->as_float);
            wattroff(win, COLOR_PAIR(COLOR_VALUE));
            (*y)++;
            break;

        case VIBE_TYPE_BOOLEAN:
            wattron(win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(win, *y, x, "%s+- BOOL: %s", indent_str,
                     value->as_boolean ? "true" : "false");
            wattroff(win, COLOR_PAIR(COLOR_VALUE));
            (*y)++;
            break;

        case VIBE_TYPE_STRING:
            wattron(win, COLOR_PAIR(COLOR_VALUE));
            mvwprintw(win, *y, x, "%s+- STR: \"%.*s\"", indent_str, 40,
                     value->as_string ? value->as_string : "");
            wattroff(win, COLOR_PAIR(COLOR_VALUE));
            (*y)++;
            break;

        case VIBE_TYPE_OBJECT:
            wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            mvwprintw(win, *y, x, "%s+- OBJECT {", indent_str);
            wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            (*y)++;

            if (value->as_object) {
                for (size_t i = 0; i < value->as_object->count && *y < max_y; i++) {
                    wattron(win, COLOR_PAIR(COLOR_KEY));
                    mvwprintw(win, *y, x, "%s| %s:", indent_str, value->as_object->entries[i].key);
                    wattroff(win, COLOR_PAIR(COLOR_KEY));
                    (*y)++;
                    render_value_tree(win, value->as_object->entries[i].value, y, x, max_y, indent + 1);
                }
            }

            wattron(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            mvwprintw(win, *y, x, "%s+- }", indent_str);
            wattroff(win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            (*y)++;
            break;

        case VIBE_TYPE_ARRAY:
            wattron(win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
            mvwprintw(win, *y, x, "%s+- ARRAY [", indent_str);
            wattroff(win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
            (*y)++;

            if (value->as_array) {
                for (size_t i = 0; i < value->as_array->count && *y < max_y; i++) {
                    wattron(win, COLOR_PAIR(COLOR_COMMENT));
                    mvwprintw(win, *y, x, "%s| [%zu]", indent_str, i);
                    wattroff(win, COLOR_PAIR(COLOR_COMMENT));
                    (*y)++;
                    render_value_tree(win, value->as_array->values[i], y, x, max_y, indent + 1);
                }
            }

            wattron(win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
            mvwprintw(win, *y, x, "%s+- ]", indent_str);
            wattroff(win, COLOR_PAIR(COLOR_WARNING) | A_BOLD);
            (*y)++;
            break;

        default:
            wattron(win, COLOR_PAIR(COLOR_ERROR));
            mvwprintw(win, *y, x, "%s%s", indent_str, type_to_string(value->type));
            wattroff(win, COLOR_PAIR(COLOR_ERROR));
            (*y)++;
            break;
    }
}

void update_data_structure_panel(DashboardPanel *panel, VibeValue *value) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    int y = 3;
    int max_y = panel->height - 2;

    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y++, 2, "+-- DATA STRUCTURE TREE --------------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
    mvwprintw(panel->win, y++, 2, "Visual representation of parsed config:");
    wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
    y++;

    if (value) {
        render_value_tree(panel->win, value, &y, 2, max_y, 0);
    } else {
        wattron(panel->win, COLOR_PAIR(COLOR_WARNING));
        mvwprintw(panel->win, y++, 4, "! No data parsed yet");
        wattroff(panel->win, COLOR_PAIR(COLOR_WARNING));

        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y++, 6, "  -> Data will appear as parsing progresses");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
    }

    wrefresh(panel->win);
}

void update_token_panel(DashboardPanel *panel) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    int y = 2;
    int max_y = panel->height - 2;

    // Header with token count
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, " ");
    mvwprintw(panel->win, y, 4, "TOKEN STREAM - Lexical Analysis");
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    // Token counter
    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
    mvwprintw(panel->win, y, panel->width - 22, "[Tokens: %d]", token_count);
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
    y++;

    // Separator
    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, "=");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    if (token_count == 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y + 2, 4, "No tokens generated yet...");
        mvwprintw(panel->win, y + 3, 4, "Tokens appear as lexer processes input");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        wrefresh(panel->win);
        return;
    }

    y++;

    // Calculate how many tokens to show
    int lines_per_token = 5;
    int visible_tokens = (max_y - y) / lines_per_token;
    if (visible_tokens < 1) visible_tokens = 1;

    int start = (token_count > visible_tokens) ? token_count - visible_tokens : 0;

    // Show scroll indicator
    if (start > 0) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, y, panel->width / 2 - 8, "... %d earlier ...", start);
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
        y++;
    }

    for (int i = start; i < token_count && y < max_y - 1; i++) {
        bool is_latest = (i == token_count - 1);
        int indent = 2;
        int card_width = panel->width - 6;

        // Determine token type color
        int type_color = COLOR_PAIR(COLOR_HIGHLIGHT);
        const char *type_icon = "[T]";

        if (strstr(tokens[i].token_type, "COMMENT")) {
            type_color = COLOR_PAIR(COLOR_COMMENT);
            type_icon = "[#]";
        } else if (strstr(tokens[i].token_type, "IDENTIFIER")) {
            type_color = COLOR_PAIR(COLOR_KEY);
            type_icon = "[K]";
        } else if (strstr(tokens[i].token_type, "BOOLEAN")) {
            type_color = COLOR_PAIR(COLOR_SUCCESS);
            type_icon = "[B]";
        } else if (strstr(tokens[i].token_type, "INTEGER")) {
            type_color = COLOR_PAIR(COLOR_VALUE);
            type_icon = "[I]";
        } else if (strstr(tokens[i].token_type, "FLOAT")) {
            type_color = COLOR_PAIR(COLOR_VALUE);
            type_icon = "[F]";
        } else if (strstr(tokens[i].token_type, "STRING")) {
            type_color = COLOR_PAIR(COLOR_VALUE);
            type_icon = "[S]";
        } else if (strstr(tokens[i].token_type, "LBRACE") || strstr(tokens[i].token_type, "RBRACE")) {
            type_color = COLOR_PAIR(COLOR_HIGHLIGHT);
            type_icon = "[{}]";
        } else if (strstr(tokens[i].token_type, "LBRACKET") || strstr(tokens[i].token_type, "RBRACKET")) {
            type_color = COLOR_PAIR(COLOR_WARNING);
            type_icon = "[[]";
        }

        // Draw token card top border
        wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "+");
        for (int j = 0; j < card_width; j++) wprintw(panel->win, "-");
        wprintw(panel->win, "+");
        wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Token header with position
        wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "|");
        wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

        wattron(panel->win, is_latest ? (COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD) : (COLOR_PAIR(COLOR_INFO) | A_BOLD));
        wprintw(panel->win, " #%-3d ", i + 1);
        wattroff(panel->win, is_latest ? (COLOR_PAIR(COLOR_CURRENT_LINE) | A_REVERSE | A_BOLD) : (COLOR_PAIR(COLOR_INFO) | A_BOLD));

        wattron(panel->win, type_color | (is_latest ? A_BOLD : 0));
        wprintw(panel->win, "%s %s", type_icon, tokens[i].token_type);
        wattroff(panel->win, type_color | (is_latest ? A_BOLD : 0));

        wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent + card_width + 1, "|");
        wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Position information line
        wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "|");
        wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        wprintw(panel->win, " Position: Line %d, Col %d", tokens[i].line, tokens[i].column);
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));

        wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent + card_width + 1, "|");
        wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Value line
        if (strlen(tokens[i].token_value) > 0 && y < max_y) {
            wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent, "|");
            wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

            wattron(panel->win, COLOR_PAIR(COLOR_KEY));
            wprintw(panel->win, " Value: ");
            wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
            wattron(panel->win, COLOR_PAIR(COLOR_VALUE) | (is_latest ? A_BOLD : 0));
            wprintw(panel->win, "\"%.*s\"", card_width - 12, tokens[i].token_value);
            wattroff(panel->win, COLOR_PAIR(COLOR_VALUE) | (is_latest ? A_BOLD : 0));

            wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent + card_width + 1, "|");
            wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            y++;
        }

        // Context/description line
        if (strlen(tokens[i].context) > 0 && y < max_y) {
            wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent, "|");
            wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));

            wattron(panel->win, COLOR_PAIR(COLOR_COMMENT) | (is_latest ? 0 : A_DIM));
            wprintw(panel->win, " -> %.*s", card_width - 5, tokens[i].context);
            wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT) | (is_latest ? 0 : A_DIM));

            wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            mvwprintw(panel->win, y, indent + card_width + 1, "|");
            wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
            y++;
        }

        // Draw token card bottom border
        wattron(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        mvwprintw(panel->win, y, indent, "+");
        for (int j = 0; j < card_width; j++) wprintw(panel->win, "-");
        wprintw(panel->win, "+");
        wattroff(panel->win, is_latest ? (type_color | A_BOLD) : (COLOR_PAIR(COLOR_BORDER) | A_DIM));
        y++;

        // Add spacing between tokens
        if (i < token_count - 1 && y < max_y - 1) {
            y++;
        }
    }

    // Show if there are more tokens
    if (y >= max_y && token_count > start + visible_tokens) {
        wattron(panel->win, COLOR_PAIR(COLOR_COMMENT));
        mvwprintw(panel->win, max_y, panel->width / 2 - 6, "... more ...");
        wattroff(panel->win, COLOR_PAIR(COLOR_COMMENT));
    }

    wrefresh(panel->win);
}

void update_memory_panel(DashboardPanel *panel, size_t allocated, size_t freed) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    int y = 2;

    // Calculate statistics
    time_t elapsed = time(NULL) - start_time;
    size_t current = allocated - freed;
    float mem_usage_percent = allocated > 0 ? (float)current / allocated * 100.0f : 0.0f;
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;

    // Header
    wattron(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, " ");
    mvwprintw(panel->win, y, 4, "STATISTICS & MEMORY");
    wattroff(panel->win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

    // Uptime indicator
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    mvwprintw(panel->win, y, panel->width - 20, "Up: %02d:%02d:%02d", hours, minutes, seconds);
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    y++;

    // Separator
    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, 2, " ");
    for (int i = 0; i < panel->width - 4; i++) wprintw(panel->win, "=");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y += 2;

    int col1 = 3;
    int col2 = panel->width / 2 + 2;

    // Left column - Memory stats
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y, col1, "MEMORY USAGE");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "+------------------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| Allocated:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_INFO));
    if (allocated > 1024) {
        wprintw(panel->win, " %6.2f KB |", allocated / 1024.0);
    } else {
        wprintw(panel->win, " %6zu B  |", allocated);
    }
    wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| Freed:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    if (freed > 1024) {
        wprintw(panel->win, " %9.2f KB |", freed / 1024.0);
    } else {
        wprintw(panel->win, " %9zu B  |", freed);
    }
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y, col1, "| In Use:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    int mem_color = current > 10000 ? COLOR_WARNING : COLOR_SUCCESS;
    wattron(panel->win, COLOR_PAIR(mem_color) | A_BOLD);
    if (current > 1024) {
        wprintw(panel->win, " %9.2f KB |", current / 1024.0);
    } else {
        wprintw(panel->win, " %9zu B  |", current);
    }
    wattroff(panel->win, COLOR_PAIR(mem_color) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "+------------------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));

    // Right column - Parsing stats
    int y2 = y - 5;
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y2, col2, "PARSING STATS");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y2, col2, "+------------------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y2, col2, "| Lines:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    wprintw(panel->win, " %-14d|", config_line_count);
    wattroff(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y2, col2, "| Tokens:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " %-13d|", token_count);
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_KEY));
    mvwprintw(panel->win, y2, col2, "| API Calls:");
    wattroff(panel->win, COLOR_PAIR(COLOR_KEY));
    wattron(panel->win, COLOR_PAIR(COLOR_API_CALL));
    wprintw(panel->win, " %-10d|", api_log_count);
    wattroff(panel->win, COLOR_PAIR(COLOR_API_CALL));
    y2++;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y2, col2, "+------------------------+");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));

    y += 2;

    // Memory usage visualization
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y, col1, "MEMORY GRAPH");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y++;

    // Bar chart
    int bar_width = panel->width - 10;
    int filled = (int)(mem_usage_percent / 100.0 * bar_width);
    if (filled > bar_width) filled = bar_width;

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "[");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));

    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            if (mem_usage_percent > 80) {
                wattron(panel->win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
            } else if (mem_usage_percent > 50) {
                wattron(panel->win, COLOR_PAIR(COLOR_WARNING));
            } else {
                wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
            }
            wprintw(panel->win, "#");
            if (mem_usage_percent > 80) {
                wattroff(panel->win, COLOR_PAIR(COLOR_ERROR) | A_BOLD);
            } else if (mem_usage_percent > 50) {
                wattroff(panel->win, COLOR_PAIR(COLOR_WARNING));
            } else {
                wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));
            }
        } else {
            wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            wprintw(panel->win, ".");
            wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
        }
    }

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    wprintw(panel->win, "]");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_INFO));
    mvwprintw(panel->win, y, col1, "Usage: %.1f%% of allocated memory", mem_usage_percent);
    wattroff(panel->win, COLOR_PAIR(COLOR_INFO));
    y += 2;

    // Step progress
    wattron(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    mvwprintw(panel->win, y, col1, "STEP PROGRESS");
    wattroff(panel->win, COLOR_PAIR(COLOR_ACCENT) | A_BOLD);
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    mvwprintw(panel->win, y, col1, "Step %d / %d", current_step, total_steps);
    wattroff(panel->win, COLOR_PAIR(COLOR_VALUE) | A_BOLD);
    y++;

    // Step progress bar
    int step_bar_width = panel->width - 10;
    float step_progress = total_steps > 0 ? (float)current_step / total_steps * 100.0f : 0.0f;
    int step_filled = (int)(step_progress / 100.0 * step_bar_width);

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    mvwprintw(panel->win, y, col1, "[");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));

    for (int i = 0; i < step_bar_width; i++) {
        if (i < step_filled) {
            wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
            wprintw(panel->win, "=");
            wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
        } else {
            wattron(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
            wprintw(panel->win, "-");
            wattroff(panel->win, COLOR_PAIR(COLOR_BORDER) | A_DIM);
        }
    }

    wattron(panel->win, COLOR_PAIR(COLOR_BORDER));
    wprintw(panel->win, "]");
    wattroff(panel->win, COLOR_PAIR(COLOR_BORDER));
    y++;

    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS));
    mvwprintw(panel->win, y, col1, "%.1f%% Complete", step_progress);
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS));

    wrefresh(panel->win);
}

void update_help_panel(DashboardPanel *panel) {
    if (!panel || !panel->win) return;

    werase(panel->win);
    draw_fancy_border(panel);

    wattron(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);
    mvwprintw(panel->win, 2, 2, "KEYBOARD CONTROLS:");
    wattroff(panel->win, COLOR_PAIR(COLOR_SUCCESS) | A_BOLD);

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    mvwprintw(panel->win, 2, 24, " [SPACE/N] ");
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " Step Forward ");

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " [F] ");
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " Fast Forward ");

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " [B] ");
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " Step Back ");

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " [Shift+B] ");
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " Fast Back ");

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " [P] ");
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " Play/Pause ");

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " [R] ");
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " Reset ");

    wattron(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " [Q] ");
    wattroff(panel->win, COLOR_PAIR(COLOR_HIGHLIGHT));
    wprintw(panel->win, " Quit");

    wrefresh(panel->win);
}

void add_api_log(const char *operation, const char *api_function, const char *params,
                 const char *result, const char *explanation) {
    if (api_log_count < MAX_API_LOGS) {
        strncpy(api_logs[api_log_count].operation, operation, 255);
        strncpy(api_logs[api_log_count].api_function, api_function, 127);
        strncpy(api_logs[api_log_count].parameters, params, 255);
        strncpy(api_logs[api_log_count].result, result, 255);
        strncpy(api_logs[api_log_count].explanation, explanation, 255);
        api_logs[api_log_count].timestamp = api_log_count;
        api_log_count++;
    }
}

void add_token(const char *type, const char *value, int line, int col,
               const char *context, const char *spec_ref) {
    if (token_count < MAX_TOKENS) {
        strncpy(tokens[token_count].token_type, type, 63);
        strncpy(tokens[token_count].token_value, value, 255);
        tokens[token_count].line = line;
        tokens[token_count].column = col;
        strncpy(tokens[token_count].context, context, 127);
        strncpy(tokens[token_count].spec_reference, spec_ref, 127);
        token_count++;
    }
}

void add_spec_check(const char *rule, const char *description, bool satisfied) {
    if (spec_check_count < MAX_SPEC_CHECKS) {
        strncpy(spec_checks[spec_check_count].rule, rule, 127);
        strncpy(spec_checks[spec_check_count].description, description, 255);
        spec_checks[spec_check_count].satisfied = satisfied;
        spec_check_count++;
    }
}

void load_config_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;

    char line[512];
    config_line_count = 0;

    while (fgets(line, sizeof(line), f)) {
        config_line_count++;
    }

    config_lines = malloc(sizeof(char*) * config_line_count);

    rewind(f);
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < config_line_count) {
        line[strcspn(line, "\n")] = 0;
        config_lines[i] = strdup(line);
        i++;
    }

    fclose(f);
}

void load_config_from_string(const char *content) {
    if (!content) return;

    config_line_count = 1;
    for (const char *p = content; *p; p++) {
        if (*p == '\n') config_line_count++;
    }

    config_lines = malloc(sizeof(char*) * config_line_count);

    int i = 0;
    const char *line_start = content;
    const char *p = content;

    while (*p) {
        if (*p == '\n' || *(p + 1) == '\0') {
            size_t len = (*p == '\n') ? (p - line_start) : (p - line_start + 1);
            config_lines[i] = malloc(len + 1);
            strncpy(config_lines[i], line_start, len);
            config_lines[i][len] = '\0';
            i++;
            line_start = p + 1;
        }
        p++;
    }
}

char* read_from_stdin(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║           VIBE Dashboard - Read from STDIN               ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    printf("Enter your VIBE configuration (end with Ctrl+D):\n\n");

    size_t buffer_size = 4096;
    size_t current_size = 0;
    char *buffer = malloc(buffer_size);
    char line[512];

    while (fgets(line, sizeof(line), stdin)) {
        size_t line_len = strlen(line);

        if (current_size + line_len >= buffer_size) {
            buffer_size *= 2;
            buffer = realloc(buffer, buffer_size);
        }

        strcpy(buffer + current_size, line);
        current_size += line_len;
    }

    buffer[current_size] = '\0';

    if (current_size == 0) {
        free(buffer);
        return NULL;
    }

    printf("\n✓ Read %zu bytes from stdin\n", current_size);
    return buffer;
}

int create_socket_listener(int port) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(socket_fd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(socket_fd);
        return -1;
    }

    if (listen(socket_fd, 5) < 0) {
        perror("Listen failed");
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

char* receive_socket_data(int socket_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    printf("Waiting for connection...\n");

    int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed");
        return NULL;
    }

    printf("✓ Client connected from %s\n", inet_ntoa(client_addr.sin_addr));

    size_t buffer_size = 4096;
    size_t current_size = 0;
    char *buffer = malloc(buffer_size);
    char temp[1024];
    ssize_t bytes_read;

    while ((bytes_read = recv(client_fd, temp, sizeof(temp) - 1, 0)) > 0) {
        if (current_size + bytes_read >= buffer_size) {
            buffer_size *= 2;
            buffer = realloc(buffer, buffer_size);
        }

        memcpy(buffer + current_size, temp, bytes_read);
        current_size += bytes_read;

        if (current_size >= 4 &&
            buffer[current_size - 4] == '\n' &&
            buffer[current_size - 3] == 'E' &&
            buffer[current_size - 2] == 'N' &&
            buffer[current_size - 1] == 'D') {
            current_size -= 4;
            break;
        }
    }

    buffer[current_size] = '\0';
    close(client_fd);

    printf("✓ Received %zu bytes\n", current_size);

    return buffer;
}

char* read_from_socket(int port) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║        VIBE Dashboard - Network Socket Listener          ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    printf("Creating socket listener on port %d...\n", port);

    int socket_fd = create_socket_listener(port);
    if (socket_fd < 0) {
        return NULL;
    }

    printf("✓ Listening on 0.0.0.0:%d\n", port);
    printf("\nSend config: cat file.vibe | nc localhost %d\n\n", port);

    char *data = receive_socket_data(socket_fd);
    close(socket_fd);

    return data;
}

char* read_paste_input(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║          VIBE Dashboard - Multi-line Paste Mode          ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    printf("Paste your VIBE configuration.\n");
    printf("Type 'END' on a new line when done.\n\n");

    size_t buffer_size = 4096;
    size_t current_size = 0;
    char *buffer = malloc(buffer_size);
    char line[512];

    while (fgets(line, sizeof(line), stdin)) {
        if (strcmp(line, "END\n") == 0 || strcmp(line, "END") == 0) {
            break;
        }

        size_t line_len = strlen(line);

        if (current_size + line_len >= buffer_size) {
            buffer_size *= 2;
            buffer = realloc(buffer, buffer_size);
        }

        strcpy(buffer + current_size, line);
        current_size += line_len;
    }

    buffer[current_size] = '\0';

    if (current_size == 0) {
        free(buffer);
        printf("\n⚠ No input received\n");
        return NULL;
    }

    printf("\n✓ Received %zu bytes\n", current_size);
    return buffer;
}

char* prompt_file_input(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║           VIBE Dashboard - File Path Input               ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    printf("Enter file path:\n> ");

    char filename[512];
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        return NULL;
    }

    filename[strcspn(filename, "\n")] = 0;

    if (strlen(filename) == 0) {
        printf("⚠ No filename entered\n");
        return NULL;
    }

    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("✗ Could not open '%s'\n", filename);
        perror("  ");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    size_t bytes_read = fread(buffer, 1, file_size, f);
    buffer[bytes_read] = '\0';

    fclose(f);

    printf("✓ Loaded %zu bytes from '%s'\n", bytes_read, filename);
    return buffer;
}

void show_input_menu(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║        VIBE Parser Dashboard - Input Selection           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    printf("Choose input method:\n\n");
    printf("  [1] Load from file path\n");
    printf("  [2] Read from stdin\n");
    printf("  [3] Paste multi-line config\n");
    printf("  [4] Listen on network socket\n");
    printf("  [5] Exit\n\n");
    printf("Choice (1-5): ");
}

// Save the current state before taking a parsing step
// This lets us rewind later with the 'b' key
void save_state(void) {
    if (history_count < MAX_HISTORY) {
        state_history[history_count].config_line = current_config_line;
        state_history[history_count].step = current_step;
        state_history[history_count].token_count = token_count;
        state_history[history_count].api_log_count = api_log_count;
        history_count++;
    }
    // If we hit MAX_HISTORY, oldest states get lost - acceptable tradeoff
}

// Go back one step - restore the previous state from history
// Super useful for understanding what the parser just did
void restore_previous_state(void) {
    if (history_count > 0) {
        history_count--;
        current_config_line = state_history[history_count].config_line;
        current_step = state_history[history_count].step;
        token_count = state_history[history_count].token_count;
        api_log_count = state_history[history_count].api_log_count;
    }
}

// Simulate one step of parsing - process the current line
// This doesn't actually parse (that already happened), it just walks through
// the config line-by-line and logs what *would* happen during real parsing
void simulate_parsing_step(VibeParser *parser, Dashboard *dash __attribute__((unused))) {
    if (!parser || current_config_line >= config_line_count ||
        !config_lines || current_config_line < 0) {
        return;  // Nothing to do
    }

    // Save state first so we can rewind
    save_state();

    char *line = config_lines[current_config_line];

    if (strlen(line) == 0 || line[0] == '#') {
        add_token("COMMENT", line, current_config_line + 1, 1,
                  "Comment or empty line", "VIBE Spec: Comments");
        current_config_line++;
        current_step++;
        return;
    }

    if (strchr(line, '{')) {
        add_token("LBRACE", "{", current_config_line + 1, strchr(line, '{') - line,
                  "Object start", "Grammar: object-decl");
        add_api_log("Object parsing", "vibe_value_new_object()", "", "VibeValue* (OBJECT)",
                    "Creates a new object to hold key-value pairs");
    } else if (strchr(line, '}')) {
        add_token("RBRACE", "}", current_config_line + 1, strchr(line, '}') - line,
                  "Object end", "Grammar: object-end");
        add_api_log("Close object", "vibe_object_set()", "key, value", "void",
                    "Adds property to parent object");
    } else if (strchr(line, '[')) {
        add_token("LBRACKET", "[", current_config_line + 1, strchr(line, '[') - line,
                  "Array start", "Grammar: array-decl");
        add_api_log("Array parsing", "vibe_value_new_array()", "", "VibeValue* (ARRAY)",
                    "Creates a new array to hold multiple values");
    } else if (strchr(line, ']')) {
        add_token("RBRACKET", "]", current_config_line + 1, strchr(line, ']') - line,
                  "Array end", "Grammar: array-end");
        add_api_log("Close array", "vibe_array_push()", "array, value", "void",
                    "Adds element to array");
    } else {
        char *space = strchr(line, ' ');
        if (space) {
            char key[128] = {0};
            char value[256] = {0};
            strncpy(key, line, space - line);
            strcpy(value, space + 1);

            char *v = value;
            while (*v == ' ') v++;

            add_token("IDENTIFIER", key, current_config_line + 1, 1,
                      "Key name", "Grammar: identifier");

            if (strcmp(v, "true") == 0 || strcmp(v, "false") == 0) {
                add_token("BOOLEAN", v, current_config_line + 1, space - line + 1,
                          "Boolean value", "Type: boolean");
                add_api_log("Boolean value", "vibe_value_new_boolean()", v, "VibeValue* (BOOLEAN)",
                            "true/false keyword parsed as boolean type");
            } else if (strchr(v, '.')) {
                add_token("FLOAT", v, current_config_line + 1, space - line + 1,
                          "Float value", "Type: float");
                add_api_log("Float value", "vibe_value_new_float()", v, "VibeValue* (FLOAT)",
                            "Number with decimal point parsed as float");
            } else if (v[0] >= '0' && v[0] <= '9') {
                add_token("INTEGER", v, current_config_line + 1, space - line + 1,
                          "Integer value", "Type: integer");
                add_api_log("Integer value", "vibe_value_new_integer()", v, "VibeValue* (INTEGER)",
                            "Whole number parsed as 64-bit integer");
            } else {
                add_token("STRING", v, current_config_line + 1, space - line + 1,
                          "String value", "Type: string");
                add_api_log("String value", "vibe_value_new_string()", v, "VibeValue* (STRING)",
                            "Text value parsed as string type");
            }

            add_api_log("Set property", "vibe_object_set()", key, "Added to object",
                        "Stores key-value pair in parent object");
        }
    }

    current_config_line++;
    current_step++;
}

int main(int argc, char *argv[]) {
    char *config_content = NULL;
    char *temp_file = NULL;
    InputMethod input_method = INPUT_FILE;

    start_time = time(NULL);

    if (argc >= 2) {
        if (strcmp(argv[1], "--stdin") == 0 || strcmp(argv[1], "-s") == 0) {
            input_method = INPUT_STDIN;
        } else if (strcmp(argv[1], "--socket") == 0 || strcmp(argv[1], "-n") == 0) {
            input_method = INPUT_SOCKET;
        } else if (strcmp(argv[1], "--paste") == 0 || strcmp(argv[1], "-p") == 0) {
            input_method = INPUT_PASTE;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("\n╔═══════════════════════════════════════════════════════════╗\n");
            printf("║              VIBE Parser Dashboard - Help                 ║\n");
            printf("╚═══════════════════════════════════════════════════════════╝\n\n");
            printf("Usage: %s [OPTIONS] [FILE]\n\n", argv[0]);
            printf("Options:\n");
            printf("  <file>              Load VIBE config from file\n");
            printf("  --stdin,  -s        Read from stdin\n");
            printf("  --paste,  -p        Paste mode\n");
            printf("  --socket, -n [port] Network socket (default: 9999)\n");
            printf("  --help,   -h        Show this help\n\n");
            printf("Examples:\n");
            printf("  %s config.vibe\n", argv[0]);
            printf("  %s --stdin < config.vibe\n", argv[0]);
            printf("  %s --paste\n", argv[0]);
            printf("  %s --socket 8080\n", argv[0]);
            printf("  cat config.vibe | %s -s\n\n", argv[0]);
            return 0;
        } else {
            load_config_file(argv[1]);
            if (config_line_count == 0) {
                printf("✗ Could not load file: %s\n", argv[1]);
                return 1;
            }
            goto parse_config;
        }
    } else {
        show_input_menu();

        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("✗ Invalid input\n");
            return 1;
        }
        getchar();

        switch (choice) {
            case 1:
                config_content = prompt_file_input();
                break;
            case 2:
                config_content = read_from_stdin();
                break;
            case 3:
                config_content = read_paste_input();
                break;
            case 4: {
                printf("\nPort number (default 9999): ");
                char port_str[10];
                int port = 9999;
                if (fgets(port_str, sizeof(port_str), stdin)) {
                    int p = atoi(port_str);
                    if (p > 0 && p < 65536) port = p;
                }
                config_content = read_from_socket(port);
                break;
            }
            case 5:
                printf("Goodbye!\n");
                return 0;
            default:
                printf("✗ Invalid choice\n");
                return 1;
        }

        if (!config_content) {
            printf("✗ Failed to read configuration\n");
            return 1;
        }

        load_config_from_string(config_content);

        temp_file = strdup("/tmp/vibe_dashboard_XXXXXX");
        int fd = mkstemp(temp_file);
        if (fd < 0) {
            printf("✗ Failed to create temp file\n");
            free(config_content);
            return 1;
        }
        write(fd, config_content, strlen(config_content));
        close(fd);

        goto parse_config;
    }

    if (input_method == INPUT_STDIN) {
        config_content = read_from_stdin();
    } else if (input_method == INPUT_SOCKET) {
        int port = 9999;
        if (argc >= 3) {
            port = atoi(argv[2]);
            if (port <= 0 || port >= 65536) port = 9999;
        }
        config_content = read_from_socket(port);
    } else if (input_method == INPUT_PASTE) {
        config_content = read_paste_input();
    }

    if (!config_content) {
        printf("✗ Failed to read configuration\n");
        return 1;
    }

    load_config_from_string(config_content);

    temp_file = strdup("/tmp/vibe_dashboard_XXXXXX");
    int fd = mkstemp(temp_file);
    if (fd < 0) {
        printf("✗ Failed to create temp file\n");
        free(config_content);
        return 1;
    }
    write(fd, config_content, strlen(config_content));
    close(fd);

parse_config:
    if (config_line_count == 0) {
        printf("✗ No configuration data\n");
        if (temp_file) {
            unlink(temp_file);
            free(temp_file);
        }
        if (config_content) free(config_content);
        return 1;
    }

    printf("\n✓ Loaded: %d lines\n", config_line_count);
    printf("Press Enter to start...\n");
    getchar();

    total_steps = config_line_count;
    const char *config_file_path = temp_file ? temp_file : (argc >= 2 ? argv[1] : "memory");

    // Initialize ncurses
    initscr();

    // Check if initialization succeeded
    if (stdscr == NULL) {
        fprintf(stderr, "Error: Failed to initialize ncurses\n");
        return 1;
    }
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        init_colors();
    }

    Dashboard dash;
    init_dashboard(&dash);

    VibeParser *parser = vibe_parser_new();
    VibeValue *root_value = NULL;

    add_api_log("Init parser", "vibe_parser_new()", "", "VibeParser*",
                "Creates new parser instance");
    root_value = vibe_parse_file(parser, config_file_path);
    add_api_log("Parse file", "vibe_parse_file()", config_file_path,
                root_value ? "SUCCESS" : "FAILED",
                "Parses entire config file into data structure");

    current_config_line = 0;
    current_step = 0;
    token_count = 0;
    api_log_count = 0;
    history_count = 0;

    add_api_log("Start viz", "vibe_parser_new()", "", "VibeParser*",
                "Beginning step-by-step visualization");

    bool running = true;
    bool auto_play = false;
    int delay = 500;
    size_t memory_allocated = 1024;
    size_t memory_freed = 0;

    // Initial render with safety
    if (dash.config_panel.win) update_config_panel(&dash.config_panel, current_config_line);
    if (dash.parser_state_panel.win) update_parser_state_panel(&dash.parser_state_panel, parser);
    if (dash.api_calls_panel.win) update_api_calls_panel(&dash.api_calls_panel);
    if (dash.data_structure_panel.win) update_data_structure_panel(&dash.data_structure_panel, root_value);
    if (dash.token_panel.win) update_token_panel(&dash.token_panel);
    if (dash.memory_panel.win) update_memory_panel(&dash.memory_panel, memory_allocated, memory_freed);
    if (dash.spec_panel.win) update_spec_panel(&dash.spec_panel);
    if (dash.help_panel.win) update_help_panel(&dash.help_panel);
    update_panels();
    doupdate();

    while (running) {
        int ch = getch();

        // Handle terminal resize with debouncing
        if (ch == KEY_RESIZE) {
            int new_y, new_x;
            getmaxyx(stdscr, new_y, new_x);

            // Only reinit if size actually changed (debounce)
            if (new_y != last_resize_y || new_x != last_resize_x) {
                last_resize_y = new_y;
                last_resize_x = new_x;

                if (new_y >= 24 && new_x >= 80) {
                    // Reinitialize dashboard with new size
                    reinit_dashboard(&dash);
                    needs_full_redraw = true;
                } else {
                    // Show resize message efficiently
                    erase();
                    attron(COLOR_PAIR(COLOR_WARNING) | A_BOLD);
                    mvprintw(new_y / 2, (new_x > 40 ? (new_x - 40) / 2 : 0), "Terminal too small: %dx%d", new_x, new_y);
                    mvprintw(new_y / 2 + 1, (new_x > 40 ? (new_x - 40) / 2 : 0), "Please resize to at least 80x24");
                    mvprintw(new_y / 2 + 2, (new_x > 40 ? (new_x - 40) / 2 : 0), "Press Q to quit");
                    attroff(COLOR_PAIR(COLOR_WARNING) | A_BOLD);
                    refresh();
                }
            }
            continue;
        }

        switch (ch) {
            case 'q':
            case 'Q':
                running = false;
                break;

            case ' ':
            case 'n':
            case 'N':
                if (current_config_line < config_line_count) {
                    simulate_parsing_step(parser, &dash);
                    memory_allocated += 64;

                    if (parser) {
                        parser->line = current_config_line + 1;
                        parser->column = 1;
                        parser->pos = (current_config_line * 50);
                        parser->length = (config_line_count * 50);
                    }
                }
                break;

            case 'b':
                // Step back one
                if (history_count > 0) {
                    restore_previous_state();
                    auto_play = false;
                    if (parser) {
                        parser->line = current_config_line + 1;
                        parser->column = 1;
                        parser->pos = (current_config_line * 50);
                        parser->length = (config_line_count * 50);
                    }
                }
                break;

            case 'B':
                // Fast backward (rewind to beginning)
                if (history_count > 0) {
                    current_config_line = 0;
                    current_step = 0;
                    token_count = 0;
                    api_log_count = 0;
                    history_count = 0;
                    auto_play = false;
                    if (parser) {
                        parser->line = 1;
                        parser->column = 1;
                        parser->pos = 0;
                        parser->length = (config_line_count * 50);
                    }
                }
                break;

            case 'r':
            case 'R':
                current_config_line = 0;
                current_step = 0;
                token_count = 0;
                api_log_count = 0;
                history_count = 0;
                memory_allocated = 1024;
                memory_freed = 0;
                add_api_log("Reset", "vibe_parser_new()", "", "VibeParser*",
                            "Restarting visualization from beginning");
                auto_play = false;
                break;

            case 'p':
            case 'P':
                auto_play = !auto_play;
                break;

            case 'f':
            case 'F':
                while (current_config_line < config_line_count) {
                    simulate_parsing_step(parser, &dash);
                    memory_allocated += 64;
                    if (parser) {
                        parser->line = current_config_line + 1;
                        parser->column = 1;
                        parser->pos = (current_config_line * 50);
                        parser->length = (config_line_count * 50);
                    }
                }
                break;
        }

        if (auto_play && current_config_line < config_line_count) {
            simulate_parsing_step(parser, &dash);
            memory_allocated += 64;
            if (parser) {
                parser->line = current_config_line + 1;
                parser->column = 1;
                parser->pos = (current_config_line * 50);
                parser->length = (config_line_count * 50);
            }
            usleep(delay * 1000);
        }

        // Update all panels (full redraw if needed, otherwise normal update)
        if (needs_full_redraw || ch != ERR) {
            if (dash.config_panel.win) update_config_panel(&dash.config_panel, current_config_line);
            if (dash.parser_state_panel.win) update_parser_state_panel(&dash.parser_state_panel, parser);
            if (dash.api_calls_panel.win) update_api_calls_panel(&dash.api_calls_panel);
            if (dash.data_structure_panel.win) update_data_structure_panel(&dash.data_structure_panel, root_value);
            if (dash.token_panel.win) update_token_panel(&dash.token_panel);
            if (dash.memory_panel.win) update_memory_panel(&dash.memory_panel, memory_allocated, memory_freed);
            if (dash.spec_panel.win) update_spec_panel(&dash.spec_panel);
            if (dash.help_panel.win) update_help_panel(&dash.help_panel);

            update_panels();
            doupdate();

            needs_full_redraw = false;
        }

        usleep(50000);
    }

    if (root_value) {
        vibe_value_free(root_value);
    }
    vibe_parser_free(parser);

    cleanup_dashboard(&dash);

    for (int i = 0; i < config_line_count; i++) {
        free(config_lines[i]);
    }
    free(config_lines);

    if (temp_file) {
        unlink(temp_file);
        free(temp_file);
    }

    if (config_content) {
        free(config_content);
    }

    endwin();

    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        VIBE Parser Dashboard - Session Complete           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\nStatistics:\n");
    printf("  • Lines processed: %d\n", current_config_line);
    printf("  • API calls: %d\n", api_log_count);
    printf("  • Tokens: %d\n", token_count);
    printf("  • Memory allocated: %zu bytes\n", memory_allocated);
    printf("\nThank you for using VIBE Parser Dashboard!\n\n");

    return 0;
}
