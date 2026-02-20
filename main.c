#include <genesis.h>
#include "bible_data.h"
#include "resources.h"

#define MAX_LINES 22
#define MAX_CHARS_PER_LINE 38
#define BOOKS_PER_PAGE 20
#define CHAPTERS_PER_ROW 8
#define CHAPTERS_PER_PAGE 48

static u16 currentBook = 0;
static u16 currentChapter = 0;
static u16 bookScroll = 0;
static s16 scrollOffset = 0;
static u16 maxScrollOffset = 0;

typedef enum {
    STATE_BOOK_SELECT,
    STATE_CHAPTER_SELECT,
    STATE_READING
} AppState;

static AppState currentState = STATE_BOOK_SELECT;

static void initPalette() {
    PAL_setColor(0, RGB8_8_8_TO_VDPCOLOR(240, 230, 210));
    PAL_setColor(15, RGB8_8_8_TO_VDPCOLOR(40, 30, 20));

    PAL_setColor(16 + 0, RGB8_8_8_TO_VDPCOLOR(240, 230, 210));
    PAL_setColor(16 + 15, RGB8_8_8_TO_VDPCOLOR(20, 60, 120));

    PAL_setColor(32 + 0, RGB8_8_8_TO_VDPCOLOR(240, 230, 210));
    PAL_setColor(32 + 15, RGB8_8_8_TO_VDPCOLOR(140, 80, 20));

    PAL_setColor(48 + 0, RGB8_8_8_TO_VDPCOLOR(240, 230, 210));
    PAL_setColor(48 + 15, RGB8_8_8_TO_VDPCOLOR(10, 90, 40));

    VDP_setBackgroundColor(0);
    VDP_setTextPalette(PAL0);
}

static void showSplash() {
    SYS_disableInts();
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    u16 ind = TILE_USER_INDEX;
    VDP_loadTileSet(splash.tileset, ind, DMA);
    VDP_setTileMapEx(BG_A, splash.tilemap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind),
                     0, 0, 0, 0, splash.tilemap->w, splash.tilemap->h, CPU);
    PAL_setColors(0, (u16*) splash.palette->data, 16, DMA);

    SYS_enableInts();

    while(!(JOY_readJoypad(JOY_1) & BUTTON_START)) {
        SYS_doVBlankProcess();
    }
    while(JOY_readJoypad(JOY_1) & BUTTON_START) {
        SYS_doVBlankProcess();
    }
}

static void drawCentered(const char *text, u16 y, u16 palette) {
    u16 len = strlen(text);
    if (len > 40) len = 40;
    VDP_setTextPalette(palette);
    VDP_drawText(text, (40 - len) / 2, y);
    VDP_setTextPalette(PAL0);
}

static u16 wrapText(const char* input, char lines[][MAX_CHARS_PER_LINE + 1], u16 maxLines) {
    u16 lineCount = 0;
    u16 inputLen = strlen(input);
    u16 pos = 0;

    while (pos < inputLen && lineCount < maxLines) {
        u16 endPos = pos + MAX_CHARS_PER_LINE;
        if (endPos > inputLen) {
            endPos = inputLen;
        } else {
            while (endPos > pos && input[endPos] != ' ') endPos--;
            if (endPos == pos) endPos = pos + MAX_CHARS_PER_LINE;
        }

        strncpy(lines[lineCount], input + pos, endPos - pos);
        lines[lineCount][endPos - pos] = '\0';
        lineCount++;

        pos = endPos;
        while (pos < inputLen && input[pos] == ' ') pos++;
    }

    return lineCount;
}

static void displayBookSelect() {
    VDP_clearPlane(BG_A, TRUE);

    drawCentered("KING JAMES BIBLE", 1, PAL1);
    drawCentered("SELECT BOOK", 3, PAL1);
    VDP_setTextPalette(PAL1);
    VDP_drawText("========================================", 0, 4);
    VDP_setTextPalette(PAL0);

    u16 end = bookScroll + BOOKS_PER_PAGE;
    if (end > NUM_BIBLE_BOOKS) end = NUM_BIBLE_BOOKS;

    for (u16 i = bookScroll; i < end; i++) {
        u16 row = 6 + (i - bookScroll);
        char nameBuf[28];
        strncpy(nameBuf, bible_books[i].name, sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';

        if (i == currentBook) {
            VDP_setTextPalette(PAL2);
            VDP_drawText(">", 1, row);
            VDP_drawText(nameBuf, 3, row);
            VDP_setTextPalette(PAL0);
        } else {
            VDP_drawText(" ", 1, row);
            VDP_drawText(nameBuf, 3, row);
        }
    }

    VDP_setTextPalette(PAL1);
    VDP_drawText("========================================", 0, 26);
    VDP_setTextPalette(PAL3);
    VDP_drawText("UP/DOWN: SELECT  A/START: CHAPTERS", 1, 27);
    VDP_setTextPalette(PAL0);
}

static void displayChapterSelect() {
    VDP_clearPlane(BG_A, TRUE);

    drawCentered(bible_books[currentBook].name, 1, PAL1);
    drawCentered("SELECT CHAPTER", 3, PAL1);
    VDP_setTextPalette(PAL1);
    VDP_drawText("========================================", 0, 4);
    VDP_setTextPalette(PAL0);

    u16 numChapters = bible_books[currentBook].numChapters;
    u16 pageStart = (currentChapter / CHAPTERS_PER_PAGE) * CHAPTERS_PER_PAGE;
    u16 pageEnd = pageStart + CHAPTERS_PER_PAGE;
    if (pageEnd > numChapters) pageEnd = numChapters;

    for (u16 i = pageStart; i < pageEnd; i++) {
        char chap[4];
        sprintf(chap, "%2d", i + 1);

        u16 idx = i - pageStart;
        u16 row = idx / CHAPTERS_PER_ROW;
        u16 col = idx % CHAPTERS_PER_ROW;

        u16 x = 3 + col * 4;
        u16 y = 6 + row;

        if (i == currentChapter) {
            VDP_setTextPalette(PAL2);
            VDP_drawText(">", x - 2, y);
            VDP_drawText(chap, x, y);
            VDP_setTextPalette(PAL0);
        } else {
            VDP_drawText(chap, x, y);
        }
    }

    VDP_setTextPalette(PAL1);
    VDP_drawText("========================================", 0, 26);
    VDP_setTextPalette(PAL3);
    VDP_drawText("ARROWS: SELECT  A: READ  B: BOOKS", 1, 27);
    VDP_setTextPalette(PAL0);
}

static void displayReading() {
    VDP_clearPlane(BG_A, TRUE);

    char header[40];
    sprintf(header, "%s %d", bible_books[currentBook].name, currentChapter + 1);
    drawCentered(header, 1, PAL1);

    VDP_setTextPalette(PAL1);
    VDP_drawText("========================================", 0, 2);
    VDP_setTextPalette(PAL0);

    const char * const * verses = bible_books[currentBook].chapters[currentChapter];

    if (verses == NULL) {
        drawCentered("Chapter not available", 12, PAL2);
        VDP_setTextPalette(PAL1);
        VDP_drawText("========================================", 0, 26);
        VDP_setTextPalette(PAL3);
        VDP_drawText("B: CHAPTERS  C: BOOKS", 9, 27);
        VDP_setTextPalette(PAL0);
        return;
    }

    u16 displayLine = 4;
    u16 currentLine = 0;

    for (u16 i = 0; verses[i] != NULL; i++) {
        char verseBuf[768];
        char wrappedLines[24][MAX_CHARS_PER_LINE + 1];
        sprintf(verseBuf, "%d %s", i + 1, verses[i]);

        u16 lineCount = wrapText(verseBuf, wrappedLines, 24);

        for (u16 j = 0; j < lineCount; j++) {
            if (currentLine >= scrollOffset && displayLine < 25) {
                VDP_drawText(wrappedLines[j], 1, displayLine++);
            }
            currentLine++;
        }

        if (currentLine >= scrollOffset && displayLine < 25) displayLine++;
        currentLine++;
    }

    maxScrollOffset = (currentLine > MAX_LINES) ? (currentLine - MAX_LINES) : 0;

    VDP_setTextPalette(PAL1);
    VDP_drawText("========================================", 0, 26);
    VDP_setTextPalette(PAL3);

    if (maxScrollOffset > 0) {
        char status[40];
        sprintf(status, "UP/DOWN: SCROLL (%d/%d)", scrollOffset, maxScrollOffset);
        VDP_drawText(status, 1, 27);
    } else {
        VDP_drawText("B: CHAPTERS  C: BOOKS", 9, 27);
    }
    VDP_setTextPalette(PAL0);
}

static void clampChapter() {
    if (currentChapter >= bible_books[currentBook].numChapters) currentChapter = 0;
}

static void handleInputBookSelect() {
    u16 joy = JOY_readJoypad(JOY_1);
    static u16 lastJoy = 0;
    u16 pressed = joy & ~lastJoy;

    if (pressed & BUTTON_UP) {
        if (currentBook > 0) currentBook--;
    } else if (pressed & BUTTON_DOWN) {
        if (currentBook + 1 < NUM_BIBLE_BOOKS) currentBook++;
    } else if (pressed & BUTTON_LEFT) {
        if (currentBook >= 5) currentBook -= 5;
        else currentBook = 0;
    } else if (pressed & BUTTON_RIGHT) {
        if (currentBook + 5 < NUM_BIBLE_BOOKS) currentBook += 5;
        else currentBook = NUM_BIBLE_BOOKS - 1;
    } else if (pressed & BUTTON_A || pressed & BUTTON_START) {
        clampChapter();
        currentState = STATE_CHAPTER_SELECT;
        displayChapterSelect();
    }

    if (currentBook < bookScroll) bookScroll = currentBook;
    if (currentBook >= bookScroll + BOOKS_PER_PAGE) bookScroll = currentBook - BOOKS_PER_PAGE + 1;

    if (pressed) displayBookSelect();

    lastJoy = joy;
}

static void handleInputChapterSelect() {
    u16 joy = JOY_readJoypad(JOY_1);
    static u16 lastJoy = 0;
    u16 pressed = joy & ~lastJoy;

    u16 numChapters = bible_books[currentBook].numChapters;

    if (pressed & BUTTON_UP) {
        if (currentChapter >= CHAPTERS_PER_ROW) currentChapter -= CHAPTERS_PER_ROW;
    } else if (pressed & BUTTON_DOWN) {
        if (currentChapter + CHAPTERS_PER_ROW < numChapters) currentChapter += CHAPTERS_PER_ROW;
    } else if (pressed & BUTTON_LEFT) {
        if (currentChapter > 0) currentChapter--;
    } else if (pressed & BUTTON_RIGHT) {
        if (currentChapter + 1 < numChapters) currentChapter++;
    } else if (pressed & BUTTON_B) {
        currentState = STATE_BOOK_SELECT;
        displayBookSelect();
    } else if (pressed & BUTTON_A || pressed & BUTTON_START) {
        scrollOffset = 0;
        currentState = STATE_READING;
        displayReading();
    }

    if (pressed && currentState == STATE_CHAPTER_SELECT) displayChapterSelect();

    lastJoy = joy;
}

static void handleInputReading() {
    u16 joy = JOY_readJoypad(JOY_1);
    static u16 lastJoy = 0;
    u16 pressed = joy & ~lastJoy;
    u16 numChapters = bible_books[currentBook].numChapters;

    if (pressed & BUTTON_UP && scrollOffset > 0) {
        scrollOffset--;
        displayReading();
    } else if (pressed & BUTTON_DOWN && scrollOffset < maxScrollOffset) {
        scrollOffset++;
        displayReading();
    } else if (pressed & BUTTON_LEFT) {
        if (currentChapter > 0) {
            currentChapter--;
            scrollOffset = 0;
            displayReading();
        }
    } else if (pressed & BUTTON_RIGHT) {
        if (currentChapter + 1 < numChapters) {
            currentChapter++;
            scrollOffset = 0;
            displayReading();
        }
    } else if (pressed & BUTTON_B || pressed & BUTTON_START) {
        currentState = STATE_CHAPTER_SELECT;
        displayChapterSelect();
    } else if (pressed & BUTTON_C) {
        currentState = STATE_BOOK_SELECT;
        displayBookSelect();
    }

    lastJoy = joy;
}

int main() {
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    showSplash();
    initPalette();

    displayBookSelect();

    while (1) {
        if (currentState == STATE_BOOK_SELECT) {
            handleInputBookSelect();
        } else if (currentState == STATE_CHAPTER_SELECT) {
            handleInputChapterSelect();
        } else {
            handleInputReading();
        }
        SYS_doVBlankProcess();
    }

    return 0;
}
