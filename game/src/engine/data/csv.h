// Shared helpers for reading the spreadsheets the team exports as CSV (Game Data, Audio Data).
//
// These used to live inside engine/assets/game_data.cpp. They are here so every importer
// handles quoting, blank rows, numbers and renamed headers the same way. No raylib, so
// importers built on them can be tested without a window or an audio device.
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rage::csv {
    // Splits CSV text into rows of cells. Handles the parts of the CSV format that
    // Google Sheets produces: quoted cells containing commas, quotes or line breaks,
    // Windows (CRLF) or Unix (LF) line endings, and an optional UTF-8 byte-order mark.
    std::vector<std::vector<std::string>> Parse(std::string_view text);

    // Reads a whole file, or std::nullopt when it cannot be opened.
    std::optional<std::string> ReadFile(const std::filesystem::path &path);

    // Removes spaces, tabs and line breaks from both ends of a cell.
    std::string Trim(std::string_view text);

    // Lowercase letters and digits only: "Frame Width (px)" -> "framewidthpx".
    // Used to compare headers and keywords ("Yes", " yes ", "YES" all become "yes").
    std::string Key(std::string_view text);

    // Text to number, or std::nullopt ("no value") when the cell is not a valid number.
    // The whole cell must be numeric: "12" works, "12fps" and "fast" do not.
    std::optional<double> Number(const std::string &text);

    // Like Number, but the value must be a whole number ("48" or "48.0", not "48.5").
    std::optional<int> Integer(const std::string &text);

    // The team's ID rule: lowercase letters, digits and underscores (player_walk).
    bool IsIdentifier(std::string_view id);

    // True when every cell is empty or whitespace. Google Sheets exports empty rows as ",,,,".
    bool IsBlank(const std::vector<std::string> &row);

    // One column an importer expects: its header as written in the sheet, plus any other
    // spellings to accept (compared with Key, so case and punctuation don't matter).
    struct ColumnName {
        const char *header;
        std::vector<const char *> aliases = {};
    };

    struct ColumnMap {
        std::vector<int> index;   // Cell position of each expected column, or -1 if missing.
        std::vector<int> renamed; // Expected columns found by position because their header was renamed.
    };

    // Works out which cell holds each expected column, by header text, so reordered columns
    // still work. If a header was renamed, the column's documented position (its index in
    // `columns`) is used instead, as long as that cell is not some other expected header.
    ColumnMap MapColumns(const std::vector<std::string> &header, const std::vector<ColumnName> &columns);

    // "A" for column 0, "B" for 1 ... for messages that point at a sheet column.
    std::string ColumnLetter(int column);
}
