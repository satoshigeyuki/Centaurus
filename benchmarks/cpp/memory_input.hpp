#pragma once

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class MemoryInput
{
  void *_buffer;
  std::size_t _size;
public:
  MemoryInput(const char *filename) {
    int fd = open(filename, O_RDONLY);
    struct stat sb;
    fstat(fd, &sb);
    _size = sb.st_size;
    _buffer = mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
  }
  virtual ~MemoryInput() {
    if (_buffer != nullptr) munmap(_buffer, _size);
  }
  const char *buffer() const { return reinterpret_cast<char*>(_buffer); }
  std::size_t size() const { return _size; }
};
