/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include "ink_unused.h"  /* MAGIC_EDITING_TAG */

#include "libts.h"
#include "TextBuffer.h"

/****************************************************************************
 *
 *  TextBuffer.cc - A self-expanding buffer, primarly meant for strings
 *
 *
 *
 ****************************************************************************/

textBuffer::textBuffer(int size)
{
  bufferStart = NULL;
  nextAdd = NULL;
  currentSize = spaceLeft = 0;
  if (size > 0) {

    // Insitute a minimum size
    if (size < 1024) {
      size = 1024;
    }

    bufferStart = (char *)ats_malloc(size);
    nextAdd = bufferStart;
    currentSize = size;
    spaceLeft = size - 1;     // Leave room for a terminator;
    nextAdd[0] = '\0';
  }
}

textBuffer::~textBuffer()
{
  ats_free(bufferStart);
}

// void textBuffer::reUse()
//
//   Sets the text buffer for reuse by repositioning the
//     ptrs to beginning of buffer.  The buffer space is
//     reused
void
textBuffer::reUse()
{
  if (bufferStart != NULL) {
    nextAdd = bufferStart;
    spaceLeft = currentSize - 1;
    nextAdd[0] = '\0';
  }
}

// int textBuffer::copyFrom(void*,int num_bytes)
//
//
//  Copy N bytes (determined by num_bytes) on to the
//  end of the buffer.
//
//  Returns the number of bytes copies or
//  -1 if there was insufficient memory
int
textBuffer::copyFrom(const void *source, int num_bytes)
{

  // Get more space if necessary
  if (spaceLeft < num_bytes) {
    if (enlargeBuffer(num_bytes) == -1) {
      return -1;
    }
  }

  memcpy(nextAdd, source, num_bytes);
  spaceLeft -= num_bytes;

  nextAdd += num_bytes;
  nextAdd[0] = '\0';

  return num_bytes;
}

//  textBuffer::enlargeBuffer(int n)
//
//  Enlarge the buffer so at least at N
//    bytes are free in the buffer.
//
//  Always enlarges by a power of two.
//
//  Returns -1 if insufficient memory,
//    zero otherwise
int
textBuffer::enlargeBuffer(int N)
{
  int addedSize = currentSize;
  int newSize = currentSize * 2;
  char *newSpace;

  if (spaceLeft < N) {

    while (addedSize < N) {
      addedSize += newSize;
      newSize *= 2;
    }

    newSpace = (char *)ats_realloc(bufferStart, newSize);
    if (newSpace != NULL) {
      nextAdd = newSpace + (unsigned int) (nextAdd - bufferStart);
      bufferStart = newSpace;
      spaceLeft += addedSize;
      currentSize = newSize;
    } else {
      // Out of Memory, Sigh
      return -1;
    }
  }

  return 0;
}

// int textBuffer::rawReadFromFile
//
// - Issues a single read command on the file descriptor or handle
//   passed in and reads in raw data (not assumed to be text, no
//   string terminators added).
// - Cannot read from file descriptor on win32 because the win32
//   read() function replaces CR-LF with LF if the file is not
//   opened in binary mode.
int
textBuffer::rawReadFromFile(int fd)
{
  int readSize;

  // Check to see if we have got a resonable amount of space left in our
  //   buffer, if not try to get somemore
  if (spaceLeft < 4096) {
    if (enlargeBuffer(4096) == -1) {
      return -1;
    }
  }

  readSize = read(fd, nextAdd, spaceLeft - 1);

  if (readSize == 0) {          //EOF
    return 0;
  } else if (readSize < 0) {
    // Error on read
    return readSize;
  } else {
    nextAdd = nextAdd + readSize;
    spaceLeft -= readSize;
    return readSize;
  }
}

// int textBuffer::readFromFD(int fd)
//
// Issues a single read command on the file
// descritor passed in.  Attempts to read a minimum of
// 512 bytes from file descriptor passed.
int
textBuffer::readFromFD(int fd)
{
  int readSize;

  // Check to see if we have got a resonable amount of space left in our
  //   buffer, if not try to get somemore
  if (spaceLeft < 512) {
    if (enlargeBuffer(512) == -1) {
      return -1;
    }
  }

  readSize = read(fd, nextAdd, spaceLeft - 1);

  if (readSize == 0) {
    // Socket is empty so we are done
    return 0;
  } else if (readSize < 0) {
    // Error on read
    return readSize;
  } else {
    nextAdd = nextAdd + readSize;
    nextAdd[0] = '\0';
    spaceLeft -= readSize + 1;
    return readSize;
  }
}

char *
textBuffer::bufPtr()
{
  return bufferStart;
}
