#include "IO.h"

namespace IO {


  int16_t readInt16LE(const uint8_t* bytes, size_t byteOffset) {
    return static_cast<int16_t>(readUInt16LE(bytes, byteOffset));
  }

  int16_t readInt16BE(const uint8_t* bytes, size_t byteOffset) {
    return static_cast<int16_t>(readUInt16BE(bytes, byteOffset));
  }

  int16_t readInt16(Endian endian, const uint8_t* bytes, size_t byteOffset) {
    return (endian == Endian::Little ?
      readInt16LE(bytes, byteOffset) :
      readInt16BE(bytes, byteOffset));
  }

  uint16_t readUInt16LE(const uint8_t* bytes, size_t byteOffset) {
      uint16_t result = 0;
      result |= static_cast<uint16_t>(bytes[0+byteOffset]) << 0;
      result |= static_cast<uint16_t>(bytes[1+byteOffset]) << 8;
      return result;
  }

  uint16_t readUInt16BE(const uint8_t* bytes, size_t byteOffset) {
      uint16_t result = 0;
      result |= static_cast<uint16_t>(bytes[0]+byteOffset) << 8;
      result |= static_cast<uint16_t>(bytes[1]+byteOffset) << 0;
      return result;
  }

  uint16_t readUInt16(Endian endian, const uint8_t* bytes, size_t byteOffset) {
    return (endian == Endian::Little ?
      readUInt16LE(bytes, byteOffset) :
      readUInt16BE(bytes, byteOffset));
  }

  uint32_t readUInt32LE(const uint8_t* bytes, size_t byteOffset) {
      uint32_t result = 0;
      result |= static_cast<uint32_t>(bytes[0+byteOffset]) << 0;
      result |= static_cast<uint32_t>(bytes[1+byteOffset]) << 8;
      result |= static_cast<uint32_t>(bytes[2+byteOffset]) << 16;
      result |= static_cast<uint32_t>(bytes[3+byteOffset]) << 24;
      return result;
  }

  uint32_t readUInt32BE(const uint8_t* bytes, size_t byteOffset) {
      uint32_t result = 0;
      result |= static_cast<uint32_t>(bytes[0]+byteOffset) << 24;
      result |= static_cast<uint32_t>(bytes[1]+byteOffset) << 16;
      result |= static_cast<uint32_t>(bytes[2]+byteOffset) << 8;
      result |= static_cast<uint32_t>(bytes[3]+byteOffset) << 0;
      return result;
  }

  uint32_t readUInt32(Endian endian, const uint8_t* bytes, size_t byteOffset) {
    return (endian == Endian::Little ?
      readUInt32LE(bytes, byteOffset) :
      readUInt32BE(bytes, byteOffset));
  }

  uint8_t* writeInt16LE(uint8_t* dest, int16_t value) {
    dest[0] = static_cast<uint8_t>(value);
    dest[1] = static_cast<uint8_t>(value >> 8);
    //return WriteUInt16LE(dest, static_cast<uint16_t>(value));
    return dest+2;
  }
  uint8_t* writeInt16BE(uint8_t* dest, int16_t value) {
    dest[1] = static_cast<uint8_t>(value);
    dest[0] = static_cast<uint8_t>(value >> 8);
    //return WriteUInt16LE(dest, static_cast<uint16_t>(value));
    return dest+2;
    //return WriteUInt16BE(dest, static_cast<uint16_t>(value));
  }
  uint8_t* writeInt16(uint8_t* dest, Endian endian, int16_t value) {
    //return WriteUInt16(dest, endian, static_cast<uint16_t>(value));
    return (endian == Endian::Little ?
      writeInt16LE(dest, value) :
      writeInt16BE(dest, value));
  }

  uint8_t* writeUInt16LE(uint8_t* dest, uint16_t value) {
    dest[0] = value & 0xff;
    dest[1] = (value >> 8) & 0xff;
    return dest+2;
  }
  uint8_t* writeUInt16BE(uint8_t* dest, uint16_t value) {
    dest[0] = (value >> 8) & 0xff;
    dest[1] = value & 0xff;
    return dest+2;
  }
  uint8_t* writeUInt16(uint8_t* dest, Endian endian, uint16_t value) {
    return (endian == Endian::Little ?
      writeUInt16LE(dest, value) :
      writeUInt16BE(dest, value));
  }

  uint8_t* writeUInt32LE(uint8_t* dest, uint32_t value) {
    dest[0] = value & 0xff;
    dest[1] = (value >> 8) & 0xff;
    dest[2] = (value >> 16) & 0xff;
    dest[3] = (value >> 24) & 0xff;
    return dest+4;
  }
  uint8_t* writeUInt32BE(uint8_t* dest, uint32_t value) {
    dest[0] = (value >> 24) & 0xff;
    dest[1] = (value >> 16) & 0xff;
    dest[2] = (value >> 8) & 0xff;
    dest[3] = value & 0xff;
    return dest+4;
  }
  uint8_t* writeUInt32(uint8_t* dest, Endian endian, uint32_t value) {
    return (endian == Endian::Little ?
      writeUInt32LE(dest, value) :
      writeUInt32BE(dest, value));
  }

  bool readFileContents(const std::string& filename, std::vector<uint8_t>& result) {
    // open the file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
      file.close();
      return false;
    }

    // determine file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data
    result.resize(fileSize);
    file.read(reinterpret_cast<char*>(result.data()), fileSize);
    file.close();
    return true;
  }

  bool writeFileContents(const std::string& filename, const std::vector<uint8_t>& fileContents) {
    // open the file
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
      file.close();
      return false;
    }
    // write the data
    file.write(reinterpret_cast<const char*>(fileContents.data()), fileContents.size());
    file.close();
    return true;
  }

  FileWriter::FileWriter(const std::string& filename) {
    open(filename);
  }

  FileWriter::~FileWriter() {
    close();
  }

  bool FileWriter::open(const std::string& filename) {
    if (isOpen()) {
      return false;
    }
    _file = new std::ofstream(filename, std::ios::binary);
    if (!_file->is_open()) {
      close();
      return false;
    }
    return true;
  }

  bool FileWriter::isOpen() const {
    return (_file != nullptr);
  }

  bool FileWriter::close() {
    if (_file == nullptr) {
      return false;
    }
    _file->close();
    delete _file;
    _file = nullptr;
    return true;
  }

  size_t FileWriter::getSize() const {
    if (_file) {
      return _file->tellp();
    }
    return 0;
  }

  bool FileWriter::rewrite(size_t offset, const uint8_t* data, size_t numBytes) {
    if (!_file) {
      return false;
    }
    if (offset + numBytes > _size) {
      return false;
    }
    _file->seekp(offset);
    _file->write(reinterpret_cast<const char*>(data), numBytes);
    _file->seekp(0, std::ios_base::end);
    return true;
  }

  bool FileWriter::append(const std::vector<uint8_t>& data) {
    return append(data.data(), data.size());
  }

  bool FileWriter::append(const uint8_t* data, size_t numBytes) {
    if (!_file) {
      return false;
    }
    if (numBytes == 0) {
      return true;
    }
    _file->write(reinterpret_cast<const char*>(data), numBytes);
    _size += numBytes;
    return true;
  }

  FileReader::FileReader(const std::string& filename) {
    open(filename);
  }

  FileReader::~FileReader() {
    close();
  }

  bool FileReader::open(const std::string& filename) {
    _file.close();
    _file.open(filename, std::ios::binary);
    if (!_file.is_open()) {
      _file.close();
      _isOpen = false;
      _fileSize = 0;
      _readOffset = 0;
      return false;
    }

    // determine file size
    _file.seekg(0, std::ios::end);
    _fileSize = _file.tellg();
    _file.seekg(0, std::ios::beg);

    _isOpen = true;
    _readOffset = 0;
    return true;
  }

  bool FileReader::isOpen() const {
    return _isOpen;
  }

  bool FileReader::close() {
    if (!_isOpen) {
      return false;
    }
    _file.close();
    _isOpen = false;
    _fileSize = 0;
    _readOffset = 0;
    return true;
  }

  size_t FileReader::getSize() const {
    return _fileSize;
  }

  bool FileReader::seekTo(size_t offset) {
    if (!_isOpen) {
      return false;
    }
    if (offset > _fileSize) {
      return false;
    }
    _file.seekg(offset, std::ios::beg);
    _readOffset = offset;
    return true;
  }

  size_t FileReader::getReadOffset() const {
    return _readOffset;
  }

  size_t FileReader::readNext(size_t numBytes, std::vector<uint8_t>& result, bool append) {
    if (!_isOpen) {
      return 0;
    }
    if (!append) {
      result.clear();
    }
    size_t remainingBytes = (_fileSize - _readOffset);
    size_t bytesToRead = std::min(remainingBytes, numBytes);
    if (bytesToRead == 0) {
      return 0;
    }
    size_t writeOffset = result.size();
    result.resize(writeOffset + bytesToRead);
    _file.read(reinterpret_cast<char*>(&result[writeOffset]), bytesToRead);
    _readOffset += bytesToRead;
    return bytesToRead;
  }

  size_t FileReader::readRange(size_t startOffset, size_t numBytes, std::vector<uint8_t>& result, bool append) {
    if (!seekTo(startOffset)) {
      return 0;
    }
    return readNext(numBytes, result, append);
  }
}
