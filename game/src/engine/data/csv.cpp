#include "engine/data/csv.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace rage::csv {
    // Reads the text one character at a time. Outside quotes, a comma ends a cell and a
    // line break ends a row. Inside quotes, everything is literal except "" (an escaped ").
    std::vector<std::vector<std::string>> Parse(std::string_view text) {
        // Skip the invisible UTF-8 "byte-order mark" some programs put at the start of a file.
        if (text.size() >= 3 && text.substr(0, 3) == "\xEF\xBB\xBF") text.remove_prefix(3);
        std::vector<std::vector<std::string>> rows;
        std::vector<std::string> row;
        std::string cell;
        bool quoted = false;
        bool rowHasContent = false;
        for (std::size_t i = 0; i < text.size(); ++i) {
            const char c = text[i];
            if (quoted) {
                if (c == '"' && i + 1 < text.size() && text[i + 1] == '"') {
                    cell += '"';
                    ++i;
                } else if (c == '"') quoted = false;
                else cell += c;
            } else if (c == '"') {
                quoted = true;
                rowHasContent = true;
            } else if (c == ',') {
                row.push_back(std::move(cell));
                cell.clear();
                rowHasContent = true;
            } else if (c == '\n' || c == '\r') {
                // Treat \r\n (Windows) as a single line break.
                if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;
                row.push_back(std::move(cell));
                cell.clear();
                rows.push_back(std::move(row));
                row.clear();
                rowHasContent = false;
            } else {
                cell += c;
                rowHasContent = true;
            }
        }
        // The last row may not end with a line break.
        if (rowHasContent || !cell.empty()) {
            row.push_back(std::move(cell));
            rows.push_back(std::move(row));
        }
        return rows;
    }

    std::optional<std::string> ReadFile(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return std::nullopt;
        std::ostringstream text;
        text << file.rdbuf();
        return text.str();
    }

    std::string Trim(std::string_view text) {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) return {};
        const auto last = text.find_last_not_of(" \t\r\n");
        return std::string(text.substr(first, last - first + 1));
    }

    std::string Key(std::string_view text) {
        std::string key;
        for (const char c: text) {
            const auto u = static_cast<unsigned char>(c);
            if (std::isalnum(u)) key += static_cast<char>(std::tolower(u));
        }
        return key;
    }

    std::optional<double> Number(const std::string &text) {
        if (text.empty()) return std::nullopt;
        char *end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end != text.c_str() + text.size() || !std::isfinite(value)) return std::nullopt;
        return value;
    }

    std::optional<int> Integer(const std::string &text) {
        const auto value = Number(text);
        if (!value || *value != std::floor(*value) || std::abs(*value) > 1'000'000) return std::nullopt;
        return static_cast<int>(*value);
    }

    bool IsIdentifier(std::string_view id) {
        return !id.empty() && std::all_of(id.begin(), id.end(), [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        });
    }

    bool IsBlank(const std::vector<std::string> &row) {
        return std::all_of(row.begin(), row.end(), [](const std::string &cell) { return Trim(cell).empty(); });
    }

    namespace {
        bool Matches(const ColumnName &column, const std::string &key) {
            if (key == Key(column.header)) return true;
            return std::any_of(column.aliases.begin(), column.aliases.end(),
                               [&](const char *alias) { return key == Key(alias); });
        }
    }

    ColumnMap MapColumns(const std::vector<std::string> &header, const std::vector<ColumnName> &columns) {
        const auto isKnownHeader = [&](const std::string &key) {
            return std::any_of(columns.begin(), columns.end(), [&](const ColumnName &column) { return Matches(column, key); });
        };
        ColumnMap map;
        map.index.assign(columns.size(), -1);
        for (std::size_t c = 0; c < columns.size(); ++c) {
            for (std::size_t i = 0; i < header.size(); ++i) {
                if (Matches(columns[c], Key(header[i]))) {
                    map.index[c] = static_cast<int>(i);
                    break;
                }
            }
            // Not found by name: fall back to the documented position, unless that cell is
            // another expected column (then this column really is missing).
            if (map.index[c] >= 0 || c >= header.size()) continue;
            if (!isKnownHeader(Key(header[c]))) {
                map.index[c] = static_cast<int>(c);
                map.renamed.push_back(static_cast<int>(c));
            }
        }
        return map;
    }

    std::string ColumnLetter(int column) {
        return column >= 0 && column < 26 ? std::string(1, static_cast<char>('A' + column)) : std::to_string(column + 1);
    }
}
