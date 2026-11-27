// EN-only launcher: the Othello ("Osero") move rules and search: playing a move on the packed board (Ose_PlayMove) and
// the computer's move searches (Ose_SearchMove, Ose_SearchMovePacked).
// Draft, not matched yet (open.tsv): the original keeps this out of line from COseDlg::OnTimer although both live in
// the same object; a definition next to OnTimer gets inlined there (also true of the two searches), so they live in their own file.
#include "launcher_dialog.h"

// Column steps (EN 0x00908990) and row steps (EN 0x009089b0, the next 32 bytes) of the 8 directions, starting at the
// left and turning clockwise.
int g_oseDirectionColumn[OSE_DIRECTIONS] = { -1, -1, 0, 1, 1, 1, 0, -1 };
int g_oseDirectionRow[OSE_DIRECTIONS] = { 0, -1, -1, -1, 0, 1, 1, 1 };

// Plays a move of `color` at (column, row) on the packed board `board`: unpacks it, and when the cell is empty
// flips the opponent pieces enclosed in each of the 8 directions, then repacks the board. Returns 1 when something was
// flipped (legal move), otherwise 0 and the board is left alone. Called by OnTimer (computer) and OnLButtonDown.
int Ose_PlayMove(int *board, unsigned int column, unsigned int row, int color)
{
    unsigned char cells[OSE_CELL_COUNT];

    int cell = 0;
    for (int word = 0; word < OSE_BOARD_WORDS; word++) {
        for (int shift = 0; shift < 32; shift += 2) {
            cells[cell++] = (board[word] >> shift) & 3;
        }
    }
    int flipped = 0;
    int index = row + column * OSE_BOARD_SIZE;
    unsigned char *target = &cells[index];
    if (cells[index] == OSE_CELL_EMPTY) {
        for (int direction = 0; direction < OSE_DIRECTIONS; direction++) {
            int columnStep = g_oseDirectionColumn[direction];
            int rowStep = g_oseDirectionRow[direction];
            int adjacent = 1;   // the first cell met must be an opponent piece
            unsigned int r = row + rowStep;
            unsigned int c = column + columnStep;
            for (int distance = 1; distance < OSE_BOARD_SIZE; distance++) {
                // Cells past the edge (the unsigned test also catches -1) are skipped, not a stop.
                if (c <= OSE_BOARD_SIZE && r <= OSE_BOARD_SIZE) {
                    unsigned char piece = cells[r + c * OSE_BOARD_SIZE];
                    if (!adjacent) {
                        if (piece == OSE_CELL_EMPTY) {
                            break;
                        }
                        if (piece == color) {
                            flipped = 1;
                            if (distance > 0) {
                                unsigned char *flip = target;
                                for (int n = 0; n < distance; n++) {
                                    *flip = (unsigned char)color;
                                    flip += rowStep + columnStep * OSE_BOARD_SIZE;
                                }
                            }
                        }
                    } else {
                        if (piece == OSE_CELL_EMPTY || piece == color) {
                            break;
                        }
                    }
                    adjacent = 0;
                }
                c += columnStep;
                r += rowStep;
            }
        }
        if (flipped) {
            cell = 0;
            for (int word = 0; word < OSE_BOARD_WORDS; word++) {
                *board = 0;
                int packed = 0;
                for (int slot = 0; slot < 16; slot += 4) {
                    packed |= cells[cell] << (slot * 2);
                    packed |= cells[cell + 1] << (slot * 2 + 2);
                    packed |= cells[cell + 2] << (slot * 2 + 4);
                    packed |= cells[cell + 3] << (slot * 2 + 6);
                    cell += 4;
                }
                *board = packed;
                board++;
            }
            return 1;
        }
    }
    return 0;
}

// Move search of the computer (see the header): exhaustive search of OSE_SEARCH_DEPTH plies that keeps the first move
// of the line ending with the most pieces of countColor. Calls itself for every move found and for a pass.
// Quirk kept from the original: the "cell is empty" test reads the work board before it is copied from `cells`
// (it holds the previous candidate's flips, uninitialised for the first cell).
int Ose_SearchMove(unsigned char *cells, int depth, int color, int countColor)
{
    unsigned char work[OSE_CELL_COUNT];
    unsigned char child[OSE_CELL_COUNT];

    if (depth == 0) {
        g_oseBestCount = 0;
    }
    int flipped = 1;
    int hasEmpty = 0;
    int hasBlack = 0;
    int hasWhite = 0;
    for (int column = 0; column < OSE_BOARD_SIZE; column++) {
        for (int row = 0; row < OSE_BOARD_SIZE; row++) {
            unsigned char piece = cells[column * OSE_BOARD_SIZE + row];
            hasEmpty = piece == OSE_CELL_EMPTY ? 1 : hasEmpty;
            hasBlack = piece == OSE_CELL_BLACK ? 1 : hasBlack;
            hasWhite = piece == OSE_CELL_WHITE ? 1 : hasWhite;
        }
    }
    if (hasWhite && hasBlack && hasEmpty && depth < OSE_SEARCH_DEPTH) {
        bool moved = false;
        for (int column = 0; column < OSE_BOARD_SIZE; column++) {
            for (int row = 0; row < OSE_BOARD_SIZE; row++) {
                flipped = 0;
                unsigned char *target = &work[column * OSE_BOARD_SIZE + row];
                if (*target == OSE_CELL_EMPTY) {
                    memcpy(work, cells, sizeof(work));
                    for (int direction = 0; direction < OSE_DIRECTIONS; direction++) {
                        int columnStep = g_oseDirectionColumn[direction];
                        int adjacent = 1;   // the first cell met must be an opponent piece
                        unsigned int c = column + columnStep;
                        unsigned int r = row + g_oseDirectionRow[direction];
                        for (int distance = 1; distance < OSE_BOARD_SIZE; distance++) {
                            if (c <= OSE_BOARD_SIZE - 1 && r <= OSE_BOARD_SIZE - 1) {
                                unsigned char piece = work[r + c * OSE_BOARD_SIZE];
                                if (adjacent) {
                                    if (piece == OSE_CELL_EMPTY || piece == color) {
                                        break;
                                    }
                                } else {
                                    if (piece == OSE_CELL_EMPTY) {
                                        break;
                                    }
                                    if (piece == color) {
                                        flipped = 1;
                                        unsigned char *flip = target;
                                        for (; distance > 0; distance--) {
                                            *flip = (unsigned char)color;
                                            flip += g_oseDirectionRow[direction] + columnStep * OSE_BOARD_SIZE;
                                        }
                                        break;
                                    }
                                }
                                adjacent = 0;
                            }
                            c += columnStep;
                            r += g_oseDirectionRow[direction];
                        }
                    }
                    if (flipped) {
                        memcpy(child, work, sizeof(child));
                        int nextColor = color + 1;
                        if (nextColor > OSE_CELL_WHITE) {
                            nextColor = OSE_CELL_BLACK;
                        }
                        g_oseMoveColumn[depth] = column;
                        g_oseMoveRow[depth] = row;
                        Ose_SearchMove(child, depth + 1, nextColor, countColor);
                        moved = true;
                    }
                }
            }
        }
        if (!moved) {
            int nextColor = color + 1;
            if (nextColor > OSE_CELL_WHITE) {
                nextColor = OSE_CELL_BLACK;
            }
            Ose_SearchMove(cells, depth + 1, nextColor, countColor);
            return 0;
        }
    } else if (depth != 0) {
        int count = 0;
        for (int column = 0; column < OSE_BOARD_SIZE; column++) {
            for (int row = 0; row < OSE_BOARD_SIZE; row++) {
                if (cells[column * OSE_BOARD_SIZE + row] == countColor) {
                    count++;
                }
            }
        }
        if (g_oseBestCount < count) {
            g_oseChosenColumn = g_oseMoveColumn[0];
            g_oseChosenRow = g_oseMoveRow[0];
            g_oseBestCount = count;
        }
    }
    return 0;
}

// Packed-board twin of Ose_SearchMove (dead code in the exe, only calls itself): the same search, but the board is
// unpacked here and every child board is packed again before the recursive call. Its bounds test accepts one cell
// more than the board has (column/row 8), unlike Ose_SearchMove.
int Ose_SearchMovePacked(int *board, int depth, int color, int countColor)
{
    unsigned char cells[OSE_CELL_COUNT];
    unsigned char work[OSE_CELL_COUNT];
    int childBoard[OSE_BOARD_WORDS];

    if (depth == 0) {
        g_oseBestCount = 0;
    }
    int cell = 0;
    for (int word = 0; word < OSE_BOARD_WORDS; word++) {
        for (int shift = 0; shift < 32; shift += 2) {
            cells[cell++] = (board[word] >> shift) & 3;
        }
    }
    int flipped = 1;
    int hasEmpty = 0;
    int hasBlack = 0;
    int hasWhite = 0;
    for (int column = 0; column < OSE_BOARD_SIZE; column++) {
        for (int row = 0; row < OSE_BOARD_SIZE; row++) {
            unsigned char piece = cells[column * OSE_BOARD_SIZE + row];
            hasEmpty = piece == OSE_CELL_EMPTY ? 1 : hasEmpty;
            hasBlack = piece == OSE_CELL_BLACK ? 1 : hasBlack;
            hasWhite = piece == OSE_CELL_WHITE ? 1 : hasWhite;
        }
    }
    if (hasWhite && hasBlack && hasEmpty && depth < OSE_SEARCH_DEPTH) {
        bool moved = false;
        for (int column = 0; column < OSE_BOARD_SIZE; column++) {
            for (int row = 0; row < OSE_BOARD_SIZE; row++) {
                flipped = 0;
                memcpy(work, cells, sizeof(work));
                unsigned char *target = &work[column * OSE_BOARD_SIZE + row];
                if (*target == OSE_CELL_EMPTY) {
                    for (int direction = 0; direction < OSE_DIRECTIONS; direction++) {
                        int columnStep = g_oseDirectionColumn[direction];
                        int adjacent = 1;   // the first cell met must be an opponent piece
                        unsigned int c = column + columnStep;
                        unsigned int r = row + g_oseDirectionRow[direction];
                        for (int distance = 1; distance < OSE_BOARD_SIZE; distance++) {
                            if (c <= OSE_BOARD_SIZE && r <= OSE_BOARD_SIZE) {
                                unsigned char piece = work[r + c * OSE_BOARD_SIZE];
                                if (adjacent) {
                                    if (piece == OSE_CELL_EMPTY || piece == color) {
                                        break;
                                    }
                                } else {
                                    if (piece == OSE_CELL_EMPTY) {
                                        break;
                                    }
                                    if (piece == color) {
                                        flipped = 1;
                                        unsigned char *flip = target;
                                        for (; distance > 0; distance--) {
                                            *flip = (unsigned char)color;
                                            flip += g_oseDirectionRow[direction] + columnStep * OSE_BOARD_SIZE;
                                        }
                                        break;
                                    }
                                }
                                adjacent = 0;
                            }
                            c += columnStep;
                            r += g_oseDirectionRow[direction];
                        }
                    }
                    if (flipped) {
                        cell = 0;
                        for (int word = 0; word < OSE_BOARD_WORDS; word++) {
                            int packed = 0;
                            for (int slot = 0; slot < 16; slot += 4) {
                                packed |= work[cell] << (slot * 2);
                                packed |= work[cell + 1] << (slot * 2 + 2);
                                packed |= work[cell + 2] << (slot * 2 + 4);
                                packed |= work[cell + 3] << (slot * 2 + 6);
                                cell += 4;
                            }
                            childBoard[word] = packed;
                        }
                        int nextColor = color + 1;
                        if (nextColor > OSE_CELL_WHITE) {
                            nextColor = OSE_CELL_BLACK;
                        }
                        g_oseMoveColumn[depth] = column;
                        g_oseMoveRow[depth] = row;
                        Ose_SearchMovePacked(childBoard, depth + 1, nextColor, countColor);
                        moved = true;
                    }
                }
            }
        }
        if (!moved) {
            int nextColor = color + 1;
            if (nextColor > OSE_CELL_WHITE) {
                nextColor = OSE_CELL_BLACK;
            }
            Ose_SearchMovePacked(board, depth + 1, nextColor, countColor);
            return 0;
        }
    } else if (depth != 0) {
        int count = 0;
        for (int column = 0; column < OSE_BOARD_SIZE; column++) {
            for (int row = 0; row < OSE_BOARD_SIZE; row++) {
                if (cells[column * OSE_BOARD_SIZE + row] == countColor) {
                    count++;
                }
            }
        }
        if (g_oseBestCount < count) {
            g_oseChosenColumn = g_oseMoveColumn[0];
            g_oseChosenRow = g_oseMoveRow[0];
            g_oseBestCount = count;
        }
    }
    return 0;
}
