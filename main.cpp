#include <ncurses.h>
#include <fstream>
#include <vector>
#include <string>

class Editor {
public:
    Editor(const std::string &filename) 
        : filename(filename), cursor_x(0), cursor_y(0), offset_x(0), offset_y(0), copy_buffer(""), mode(NORMAL) {
        initscr();
        raw();
        keypad(stdscr, TRUE);
        noecho();
        loadFile();
        draw();
    }

    ~Editor() {
        endwin();
    }

    void run() {
        int ch;
        while ((ch = getch()) != 'q') {
            switch (mode) {
                case NORMAL:
                    handleNormalMode(ch);
                    break;
                case INSERT:
                    handleInsertMode(ch);
                    break;
            }
            draw();
        }
        saveFile();
    }

private:
    enum Mode { NORMAL, INSERT };
    
    std::string filename;
    std::vector<std::string> content;
    std::string copy_buffer; // Stores copied line
    int cursor_x, cursor_y;   // Actual cursor position in the file
    int offset_x, offset_y;   // Screen offset for scrolling
    Mode mode;

    void loadFile() {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            while (getline(file, line)) {
                content.push_back(line);
            }
            if (content.empty()) {
                content.push_back(""); // Ensure there's at least one empty line
            }
            file.close();
        }
    }

    void saveFile() {
        std::ofstream file(filename);
        if (file.is_open()) {
            for (const auto &line : content) {
                file << line << "\n";
            }
            file.close();
        }
    }

    void handleNormalMode(int ch) {
        switch (ch) {
            case KEY_UP:
                if (cursor_y > 0) cursor_y--;
                break;
            case KEY_DOWN:
                if (cursor_y < content.size() - 1) cursor_y++;
                break;
            case KEY_LEFT:
                if (cursor_x > 0) cursor_x--;
                break;
            case KEY_RIGHT:
                if (cursor_x < content[cursor_y].length()) cursor_x++;
                break;
            case 'i': // Switch to insert mode
                mode = INSERT;
                break;
            case 'd': // Delete current line
                if (content.size() > 1) {
                    content.erase(content.begin() + cursor_y);
                    if (cursor_y >= content.size()) cursor_y--;
                } else {
                    content[0] = "";
                }
                cursor_x = 0;
                break;
            case 'y': // Copy current line
                copy_buffer = content[cursor_y];
                break;
            case 'p': // Paste copied line below current
                content.insert(content.begin() + cursor_y + 1, copy_buffer);
                cursor_y++;
                break;
            case 'o': // Insert a new empty line below current
                content.insert(content.begin() + cursor_y + 1, "");
                cursor_y++;
                cursor_x = 0;
                break;
        }
    }

    void handleInsertMode(int ch) {
        switch (ch) {
            case 27: // ESC key to exit insert mode
                mode = NORMAL;
                break;
            case KEY_BACKSPACE:
            case 127:
                if (cursor_x > 0) {
                    content[cursor_y].erase(cursor_x - 1, 1);
                    cursor_x--;
                }
                break;
            case '\n': // Enter key to insert a new line
                content.insert(content.begin() + cursor_y + 1, content[cursor_y].substr(cursor_x));
                content[cursor_y] = content[cursor_y].substr(0, cursor_x);
                cursor_y++;
                cursor_x = 0;
                break;
            default:
                if (ch >= 32 && ch <= 126) { // Printable characters
                    content[cursor_y].insert(cursor_x, 1, ch);
                    cursor_x++;
                }
                break;
        }
    }

    void draw() {
        clear();
        // Calculate visible area
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        // Scrolling logic
        if (cursor_y < offset_y) {
            offset_y = cursor_y;
        } else if (cursor_y >= offset_y + max_y - 1) {
            offset_y = cursor_y - max_y + 1;
        }

        if (cursor_x < offset_x) {
            offset_x = cursor_x;
        } else if (cursor_x >= offset_x + max_x - 1) {
            offset_x = cursor_x - max_x + 1;
        }

        // Draw content
        for (size_t i = offset_y; i < content.size() && i - offset_y < max_y - 1; ++i) {
            mvprintw(i - offset_y, 0, content[i].substr(offset_x, max_x).c_str());
        }

        // Draw status bar
        attron(A_REVERSE);
        mvprintw(max_y - 1, 0, "%s - Line %d, Col %d  -- %s", filename.c_str(), cursor_y + 1, cursor_x + 1, mode == INSERT ? "INSERT" : "NORMAL");
        attroff(A_REVERSE);

        // Move cursor
        move(cursor_y - offset_y, cursor_x - offset_x);
        refresh();
    }
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    Editor editor(argv[1]);
    editor.run();
    return 0;
}

