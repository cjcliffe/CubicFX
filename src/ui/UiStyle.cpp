#include <cstddef>

#include <raylib.h>

#include <raygui.h>

#include "ui/UiStyle.h"

void applyUiStyle() {
    GuiLoadStyleDefault();

    Font face = LoadFontEx("fonts/DejaVuSans.ttf", 32, NULL, 0);
    if (face.texture.id > 0) {
        SetTextureFilter(face.texture, TEXTURE_FILTER_BILINEAR);
        GuiSetFont(face);
    }

    int ink = ColorToInt(RAYWHITE);
    int faint = ColorToInt(GRAY);
    int line = ColorToInt(LIGHTGRAY);
    int row = ColorToInt(DARKGRAY);
    int hover = ColorToInt(GRAY);
    int accent = ColorToInt(SKYBLUE);
    int accentInk = ColorToInt(BLACK);
    int panel = ColorToInt(Color{ 64, 64, 64, 255 });
    int recess = ColorToInt(Color{ 32, 32, 32, 255 });

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, panel);

    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ink);
    GuiSetStyle(LABEL, TEXT_COLOR_FOCUSED, ink);
    GuiSetStyle(LABEL, TEXT_COLOR_PRESSED, ink);
    GuiSetStyle(LABEL, TEXT_COLOR_DISABLED, faint);

    int boxes[] = { BUTTON, TOGGLE };
    for (int control : boxes) {
        GuiSetStyle(control, BORDER_COLOR_NORMAL, line);
        GuiSetStyle(control, BASE_COLOR_NORMAL, row);
        GuiSetStyle(control, TEXT_COLOR_NORMAL, ink);
        GuiSetStyle(control, BORDER_COLOR_FOCUSED, line);
        GuiSetStyle(control, BASE_COLOR_FOCUSED, hover);
        GuiSetStyle(control, TEXT_COLOR_FOCUSED, ink);
        GuiSetStyle(control, BORDER_COLOR_PRESSED, accent);
        GuiSetStyle(control, BASE_COLOR_PRESSED, accent);
        GuiSetStyle(control, TEXT_COLOR_PRESSED, accentInk);
        GuiSetStyle(control, BORDER_COLOR_DISABLED, faint);
        GuiSetStyle(control, BASE_COLOR_DISABLED, recess);
        GuiSetStyle(control, TEXT_COLOR_DISABLED, faint);
        GuiSetStyle(control, BORDER_WIDTH, 1);
        GuiSetStyle(control, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(control, TEXT_PADDING, 8);
    }

    GuiSetStyle(TOGGLE, GROUP_PADDING, 8);

    GuiSetStyle(CHECKBOX, BORDER_COLOR_NORMAL, line);
    GuiSetStyle(CHECKBOX, BORDER_COLOR_FOCUSED, line);
    GuiSetStyle(CHECKBOX, BORDER_COLOR_PRESSED, accent);
    GuiSetStyle(CHECKBOX, TEXT_COLOR_NORMAL, accent);
    GuiSetStyle(CHECKBOX, TEXT_COLOR_FOCUSED, accent);
    GuiSetStyle(CHECKBOX, TEXT_COLOR_PRESSED, accent);
    GuiSetStyle(CHECKBOX, BORDER_WIDTH, 1);

    GuiSetStyle(LISTVIEW, BORDER_COLOR_NORMAL, line);
    GuiSetStyle(LISTVIEW, BORDER_COLOR_FOCUSED, line);
    GuiSetStyle(LISTVIEW, BORDER_COLOR_PRESSED, accent);
    GuiSetStyle(LISTVIEW, BASE_COLOR_FOCUSED, hover);
    GuiSetStyle(LISTVIEW, TEXT_COLOR_FOCUSED, ink);
    GuiSetStyle(LISTVIEW, BASE_COLOR_PRESSED, accent);
    GuiSetStyle(LISTVIEW, TEXT_COLOR_PRESSED, accentInk);
    GuiSetStyle(LISTVIEW, TEXT_COLOR_NORMAL, ink);
    GuiSetStyle(LISTVIEW, BORDER_WIDTH, 1);
    GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(LISTVIEW, TEXT_PADDING, 8);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 28);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_SPACING, 4);

    GuiSetStyle(SCROLLBAR, BORDER_COLOR_NORMAL, line);
    GuiSetStyle(SCROLLBAR, BASE_COLOR_NORMAL, recess);
    GuiSetStyle(SCROLLBAR, BASE_COLOR_FOCUSED, hover);
    GuiSetStyle(SCROLLBAR, BASE_COLOR_PRESSED, accent);
    GuiSetStyle(SCROLLBAR, TEXT_COLOR_NORMAL, ink);
    GuiSetStyle(SCROLLBAR, BORDER_WIDTH, 1);
}

