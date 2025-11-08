#include "utf16_to_utf8.hpp"

#include <cstddef>
#include <cstdint>

namespace utf16_to_utf8 {

// Helper function to check if a byte is a continuation byte (10xxxxxx)
inline bool isContinuationByte(uint8_t byte) { return (byte & 0xC0) == 0x80; }

// Helper function to get the number of bytes in a UTF-8 sequence
inline size_t getUtf8SequenceLength(uint8_t firstByte) {
  if ((firstByte & 0x80) == 0) {
    return 1; // ASCII
  } else if ((firstByte & 0xE0) == 0xC0) {
    return 2;
  } else if ((firstByte & 0xF0) == 0xE0) {
    return 3;
  } else if ((firstByte & 0xF8) == 0xF0) {
    return 4;
  }
  return 1; // Invalid, treat as single byte
}

// Helper function to decode a UTF-8 sequence and get the code point
inline uint32_t decodeUtf8CodePoint(const std::string &text, size_t offset,
                                    size_t &bytesRead) {
  if (offset >= text.size()) {
    bytesRead = 0;
    return 0;
  }

  uint8_t firstByte = static_cast<uint8_t>(text[offset]);
  size_t seqLen = getUtf8SequenceLength(firstByte);

  if (offset + seqLen > text.size()) {
    bytesRead = 0;
    return 0;
  }

  uint32_t codePoint = 0;
  bytesRead = seqLen;

  if (seqLen == 1) {
    codePoint = firstByte;
  } else if (seqLen == 2) {
    codePoint = ((firstByte & 0x1F) << 6) | (text[offset + 1] & 0x3F);
  } else if (seqLen == 3) {
    codePoint = ((firstByte & 0x0F) << 12) |
                ((static_cast<uint8_t>(text[offset + 1]) & 0x3F) << 6) |
                (static_cast<uint8_t>(text[offset + 2]) & 0x3F);
  } else if (seqLen == 4) {
    codePoint = ((firstByte & 0x07) << 18) |
                ((static_cast<uint8_t>(text[offset + 1]) & 0x3F) << 12) |
                ((static_cast<uint8_t>(text[offset + 2]) & 0x3F) << 6) |
                (static_cast<uint8_t>(text[offset + 3]) & 0x3F);
  }

  return codePoint;
}

size_t utf16CodeUnitsToUtf8Offset(const std::string &lineText,
                                  size_t utf16CodeUnits) {
  size_t utf8Offset = 0;
  size_t utf16Count = 0;

  while (utf8Offset < lineText.size() && utf16Count < utf16CodeUnits) {
    size_t bytesRead = 0;
    uint32_t codePoint = decodeUtf8CodePoint(lineText, utf8Offset, bytesRead);

    if (bytesRead == 0) {
      // Invalid UTF-8, advance by 1 byte
      utf8Offset++;
      utf16Count++;
      continue;
    }

    // UTF-16 encoding: code points <= U+FFFF use 1 code unit, > U+FFFF use 2
    // (surrogate pair)
    if (codePoint <= 0xFFFF) {
      utf16Count++;
    } else {
      // Surrogate pair: 2 UTF-16 code units
      utf16Count += 2;
    }

    if (utf16Count <= utf16CodeUnits) {
      utf8Offset += bytesRead;
    }
  }

  return utf8Offset;
}

size_t utf16CodeUnitsToUtf8OffsetInString(const std::string &text,
                                          size_t utf16CodeUnits) {
  return utf16CodeUnitsToUtf8Offset(text, utf16CodeUnits);
}

size_t getUtf16CodeUnitCount(const std::string &text) {
  size_t utf8Offset = 0;
  size_t utf16Count = 0;

  while (utf8Offset < text.size()) {
    size_t bytesRead = 0;
    uint32_t codePoint = decodeUtf8CodePoint(text, utf8Offset, bytesRead);

    if (bytesRead == 0) {
      // Invalid UTF-8, treat as 1 code unit
      utf8Offset++;
      utf16Count++;
      continue;
    }

    // UTF-16 encoding: code points <= U+FFFF use 1 code unit, > U+FFFF use 2
    // (surrogate pair)
    if (codePoint <= 0xFFFF) {
      utf16Count++;
    } else {
      // Surrogate pair: 2 UTF-16 code units
      utf16Count += 2;
    }

    utf8Offset += bytesRead;
  }

  return utf16Count;
}

size_t getUtf16CodeUnitCountUpToOffset(const std::string &text,
                                       size_t utf8ByteOffset) {
  size_t utf8Offset = 0;
  size_t utf16Count = 0;

  while (utf8Offset < text.size() && utf8Offset < utf8ByteOffset) {
    size_t bytesRead = 0;
    uint32_t codePoint = decodeUtf8CodePoint(text, utf8Offset, bytesRead);

    if (bytesRead == 0) {
      // Invalid UTF-8, treat as 1 code unit
      utf8Offset++;
      utf16Count++;
      continue;
    }

    // Don't count code points that extend beyond the target offset
    if (utf8Offset + bytesRead > utf8ByteOffset) {
      break;
    }

    // UTF-16 encoding: code points <= U+FFFF use 1 code unit, > U+FFFF use 2
    // (surrogate pair)
    if (codePoint <= 0xFFFF) {
      utf16Count++;
    } else {
      // Surrogate pair: 2 UTF-16 code units
      utf16Count += 2;
    }

    utf8Offset += bytesRead;
  }

  return utf16Count;
}

} // namespace utf16_to_utf8
