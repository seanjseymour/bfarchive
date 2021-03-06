/*
  Copyright 2015 International Business Machines, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "UnixTestSeams.h"
#include <sys/time.h>
#include <unistd.h>

// Wrap 'utimes'.

static Type_utimes wrap_utimes = Real_utimes;

int Real_utimes( const char* filename, const struct timeval* times )
{
  return utimes( filename, times );
}

int Wrap_utimes( const char* filename, const struct timeval* times )
{
  return wrap_utimes( filename, times );
}

void Set_utimes( Type_utimes wrapFunction )
{
  wrap_utimes = wrapFunction;
}

// Wrap 'read'.

static Type_read wrap_read = Real_read;

int Real_read( int fd, void* buf, size_t count )
{
  return read( fd, buf, count );
}

int Wrap_read( int fd, void* buf, size_t count )
{
  return wrap_read( fd, buf, count );
}

void Set_read( Type_read wrapFunction )
{
  wrap_read = wrapFunction;
}

// Wrap 'write'.

static Type_write wrap_write = Real_write;

int Real_write( int fd, const void* buf, size_t count )
{
  return write( fd, buf, count );
}

int Wrap_write( int fd, const void* buf, size_t count )
{
  return wrap_write( fd, buf, count );
}

void Set_write( Type_write wrapFunction )
{
  wrap_write = wrapFunction;
}

// Wrap 'readdir_r'.

static Type_readdir_r wrap_readdir_r = Real_readdir_r;

int Real_readdir_r( DIR* dirp, struct dirent* entry, struct dirent** result )
{
  return readdir_r( dirp, entry, result );
}

int Wrap_readdir_r( DIR* dirp, struct dirent* entry, struct dirent** result )
{
  return wrap_readdir_r( dirp, entry, result );
}

void Set_readdir_r( Type_readdir_r wrapFunction )
{
  wrap_readdir_r = wrapFunction;
}

// Wrap 'gmtime_r'

static Type_gmtime_r wrap_gmtime_r = Real_gmtime_r;

struct tm* Real_gmtime_r( const time_t* timep, struct tm* result )
{
  return gmtime_r( timep, result );
}

struct tm* Wrap_gmtime_r( const time_t* timep, struct tm* result )
{
  return wrap_gmtime_r( timep, result );
}

void Set_gmtime_r( Type_gmtime_r wrapFunction )
{
  wrap_gmtime_r = wrapFunction;
}
