#pragma once

#include <cstddef>
#include <string>

namespace utf16_to_utf8 {

/**
 * Converts a UTF-16 code unit offset to a UTF-8 byte offset within a line.
 *
 * @param lineText The UTF-8 encoded line text
 * @param utf16CodeUnits The number of UTF-16 code units to advance
 * @return The UTF-8 byte offset corresponding to the UTF-16 code unit position
 */
size_t utf16CodeUnitsToUtf8Offset(const std::string &lineText,
                                  size_t utf16CodeUnits);

/**
 * Converts a UTF-16 code unit offset to a UTF-8 byte offset within a string.
 * This version works on the entire string, not just a line.
 *
 * @param text The UTF-8 encoded text
 * @param utf16CodeUnits The number of UTF-16 code units to advance
 * @return The UTF-8 byte offset corresponding to the UTF-16 code unit position
 */
size_t utf16CodeUnitsToUtf8OffsetInString(const std::string &text,
                                          size_t utf16CodeUnits);

/**
 * Gets the UTF-16 code unit count for a given UTF-8 string.
 *
 * @param text The UTF-8 encoded text
 * @return The number of UTF-16 code units needed to represent the text
 */
size_t getUtf16CodeUnitCount(const std::string &text);

/**
 * Gets the UTF-16 code unit count for a UTF-8 string up to a given byte offset.
 *
 * @param text The UTF-8 encoded text
 * @param utf8ByteOffset The byte offset in the UTF-8 string
 * @return The number of UTF-16 code units up to that byte offset
 */
size_t getUtf16CodeUnitCountUpToOffset(const std::string &text,
                                       size_t utf8ByteOffset);

} // namespace utf16_to_utf8
