#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>

enum class Align { Left, Right, Center };
enum class TableStyle { ASCII, Unicode };

template<typename T>
static std::string TtoString(const T& val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

// 判断码点是否为全角 (East Asian Wide / Ambiguous 简化处理为宽)
static bool isWideChar(uint32_t cp) {
    // 基于 Unicode 常见宽字符区间，可自行扩展
    return (cp >= 0x1100 && cp <= 0x115F)       // Hangul Jamo
        || (cp >= 0x2E80 && cp <= 0xA4CF)       // CJK Radicals ~ Yi
        || (cp >= 0xAC00 && cp <= 0xD7A3)       // Hangul Syllables
        || (cp >= 0xF900 && cp <= 0xFAFF)       // CJK Compatibility Ideographs
        || (cp >= 0xFE10 && cp <= 0xFE19)       // Vertical forms
        || (cp >= 0xFE30 && cp <= 0xFE6F)       // CJK Compatibility Forms
        || (cp >= 0xFF01 && cp <= 0xFF60)       // Fullwidth Forms
        || (cp >= 0xFFE0 && cp <= 0xFFE6)       // Fullwidth Signs
        || (cp >= 0x1F300 && cp <= 0x1F64F)     // Miscellaneous Symbols and Pictographs
        || (cp >= 0x1F900 && cp <= 0x1F9FF)     // Supplemental Symbols and Pictographs
        || (cp >= 0x20000 && cp <= 0x2FFFD)     // CJK Unified Ideographs Extension B~I
        || (cp >= 0x30000 && cp <= 0x3FFFD);    // CJK Unified Ideographs Extension G~H
}

static int displayWidth(const std::string& str) {
    int width = 0;
    size_t i = 0;
    while (i < str.size()) {
        uint32_t cp = 0;
        unsigned char c = str[i];

        // 解析 UTF-8 序列
        if (c < 0x80) {
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = ((c & 0x1F) << 6) | (str[i+1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = ((c & 0x0F) << 12) | ((str[i+1] & 0x3F) << 6) | (str[i+2] & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            cp = ((c & 0x07) << 18) | ((str[i+1] & 0x3F) << 12) | ((str[i+2] & 0x3F) << 6) | (str[i+3] & 0x3F);
            i += 4;
        } else {
            // 非法字节，跳过，宽度算1
            i += 1;
            width += 1;
            continue;
        }

        // 控制字符、零宽字符 -> 宽度0
        if (cp == 0 || (cp < 0x20 && cp != 0x09 && cp != 0x0A && cp != 0x0D)) {
            width += 0;
        } else if (isWideChar(cp)) {
            width += 2;
        } else {
            width += 1;
        }
    }
    return width;
}

class Tabulate {
public:
    std::string to_string() {
        std::stringstream ss;
        print(ss);
        return ss.str();
    }
    
    void setHeaders(const std::vector<std::string>& headerRow, const std::vector<Align>& aligns = {Align::Center, Align::Center, Align::Center}) {
        headers = headerRow;
        alignments = aligns;
        updateColumnWidths(headers);
    }
    
    
    template<typename... Args>
    void addRow(Args&&... args) {
        std::vector<std::string> row = { TtoString(std::forward<Args>(args))... };
        addRowv(row);
    }
    
    void addRowv(const std::vector<std::string>& row) {
        rows.push_back(row);
        updateColumnWidths(row);
    }
    
    void setStyle(TableStyle s) {
        style = s;
    }
    
    void print(std::ostream& os = std::cout) const {
        printBorder(os, true);
        if (!headers.empty()) {
            printRow(os, headers);
            printDivider(os);
        }
        for (const auto& row : rows)
        printRow(os, row);
        printBorder(os, false);
    }

private:
    std::string formatCell(const std::string& text, size_t width, Align align) const {
        int textWidth = displayWidth(text);
        if (textWidth >= (int)width) return text;

        int space = width - textWidth;
        switch (align) {
            case Align::Left:
                return text + std::string(space, ' ');
            case Align::Right:
                return std::string(space, ' ') + text;
            case Align::Center:
                return std::string(space / 2, ' ') + text + std::string((space + 1) / 2, ' ');
        }
        return text;
    }

    void updateColumnWidths(const std::vector<std::string>& row) {
        if (columnWidths.size() < row.size())
            columnWidths.resize(row.size(), 0);
        for (size_t i = 0; i < row.size(); ++i)
            columnWidths[i] = std::max(columnWidths[i], (size_t)displayWidth(row[i]));
    }

    void printDivider(std::ostream& os) const {
        std::string left, mid, right;
        char bar {'-'};
        if (style == TableStyle::Unicode) {
            left = "├"; mid = "┼"; right = "┤";
        } else {
            left = "+"; mid = "+"; right = "+";
        }

        os << left;
        for (size_t i = 0; i < columnWidths.size(); ++i) {
            os << std::setfill(bar) << std::setw(columnWidths[i] + 2) << "";
            os << std::setfill(' ') << (i + 1 < columnWidths.size() ? mid : right);
        }
        os << "\n";
    }

    void printBorder(std::ostream& os, bool top) const {
        std::string left, mid, right;
        char bar {'-'};
        if (style == TableStyle::Unicode) {
            left = top ? "┌" : "└";
            mid  = top ? "┬" : "┴";
            right = top ? "┐" : "┘";
        } else {
            left = "+"; mid = "+"; right = "+";
        }

        os << left;
        for (size_t i = 0; i < columnWidths.size(); ++i) {
            os << std::setfill(bar) << std::setw(columnWidths[i] + 2) << "";
            os << std::setfill(' ') << (i + 1 < columnWidths.size() ? mid : right);
        }
        os << "\n";
    }

    void printRow(std::ostream& os, const std::vector<std::string>& row) const {
        os << (style == TableStyle::Unicode ? "│" : "|");
        for (size_t i = 0; i < columnWidths.size(); ++i) {
            std::string cell = (i < row.size()) ? row[i] : "";
            Align a = (i < alignments.size()) ? alignments[i] : Align::Left;
            os << " " << formatCell(cell, columnWidths[i], a) << " ";
            os << (style == TableStyle::Unicode ? "│" : "|");
        }
        os << "\n";
    }

private:
    std::vector<std::string> headers;
    std::vector<Align> alignments;
    std::vector<std::vector<std::string>> rows;
    std::vector<size_t> columnWidths;
    TableStyle style = TableStyle::ASCII;
};