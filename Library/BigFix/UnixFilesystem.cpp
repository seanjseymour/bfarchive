/*
  Copyright 2014 International Business Machines, Inc.

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

#include "UnixFilesystem.h"
#include "BigFix/DataRef.h"
#include "BigFix/Error.h"
#include "BigFix/UnixTestSeams.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef ENABLE_TEST_SEAMS
#define utimes Wrap_utimes
#define read Wrap_read
#define write Wrap_write
#define readdir_r Wrap_readdir_r
#define gmtime_r Wrap_gmtime_r
#endif

static std::string StringError( int errnum )
{
  char buffer[4096];

#ifdef __APPLE__
  int status = strerror_r( errnum, buffer, sizeof( buffer ) );
  if ( status )
    return "Unknown error";

  return buffer;
#else
  return strerror_r( errnum, buffer, sizeof( buffer ) );
#endif
}

static std::string FileErrorString( const std::string& what,
                                    const std::string& path,
                                    int errnum )
{
  return what + " " + path + ": " + StringError( errnum );
}

namespace BigFix
{

UnixFile::UnixFile( const std::string& path )
  : m_fd( -1 ), m_path( path )
{
}

void UnixFile::SetFile( int fd )
{
  m_fd = fd;
}

UnixFile::~UnixFile()
{
  if ( m_fd != -1 )
    close( m_fd );
}

void UnixFile::SetModificationTime( const DateTime& mtime )
{
  struct tm systemTime;
  memset( &systemTime, 0, sizeof( systemTime ) );

  systemTime.tm_year = mtime.Year() - 1900;
  systemTime.tm_mon = mtime.Month() - 1;
  systemTime.tm_mday = mtime.Day();
  systemTime.tm_wday = mtime.DayOfWeek() - 1;
  systemTime.tm_hour = mtime.Hour();
  systemTime.tm_min = mtime.Minute();
  systemTime.tm_sec = mtime.Second();

  time_t unixTime = timegm( &systemTime );

  struct timeval fileTimes[2];
  memset( &fileTimes, 0, sizeof( fileTimes ) );

  fileTimes[0].tv_sec = unixTime;
  fileTimes[1].tv_sec = unixTime;

  if ( utimes( m_path.c_str(), fileTimes ) )
    throw Error(
      FileErrorString( "Failed to set modification time on", m_path, errno ) );
}

size_t UnixFile::Read( uint8_t* buffer, size_t length )
{
  ssize_t nread = read( m_fd, buffer, length );

  if ( nread < 0 )
    throw Error( FileErrorString( "Failed to read file", m_path, errno ) );

  return nread;
}

void UnixFile::Write( DataRef data )
{
  const uint8_t* start = data.Start();
  const uint8_t* end = data.End();

  while ( start != end )
  {
    ssize_t nwritten = write( m_fd, start, end - start );

    if ( nwritten < 0 )
      throw Error( FileErrorString( "Failed to write file", m_path, errno ) );

    start += nwritten;
  }
}

static void ThrowOpenError( const char* path, int fd )
{
  if ( fd < 0 )
    throw Error( FileErrorString( "Failed to open", path, errno ) );
}

std::auto_ptr<File> OpenAsNewFile( const char* path )
{
  std::auto_ptr<UnixFile> file( new UnixFile( path ) );

  int fd = open( path,
                 O_RDWR | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );

  ThrowOpenError( path, fd );
  file->SetFile( fd );

  std::auto_ptr<File> base( file.release() );
  return base;
}

std::auto_ptr<File> OpenExistingFile( const char* path )
{
  std::auto_ptr<UnixFile> file( new UnixFile( path ) );

  int fd = open( path, O_RDONLY );

  ThrowOpenError( path, fd );
  file->SetFile( fd );

  std::auto_ptr<File> base( file.release() );
  return base;
}

void MakeDir( const char* path )
{
  if ( mkdir( path, S_IRWXU ) == 0 )
    return;

  int errnum = errno;

  if ( errnum == EEXIST && Stat( path ).IsDirectory() )
    return;

  throw Error( FileErrorString( "Failed to create directory", path, errnum ) );
}

FileStatus Stat( const char* path )
{
  struct stat stats;

  if ( stat( path, &stats ) )
    throw Error( FileErrorString( "Failed to stat file", path, errno ) );

  struct tm result;
  if ( gmtime_r( &stats.st_mtime, &result ) == 0 )
    throw Error( FileErrorString(
      "Failed to convert file time to DateTime", path, errno ) );

  DateTime mtime( result.tm_year + 1900,
                  result.tm_mon + 1,
                  result.tm_mday,
                  result.tm_wday + 1,
                  result.tm_hour,
                  result.tm_min,
                  result.tm_sec );

  return FileStatus(
    stats.st_size, mtime, S_ISDIR( stats.st_mode ), S_ISREG( stats.st_mode ) );
}

void StreamStdIn( Stream& stream )
{
  uint8_t buffer[4096];

  while ( true )
  {
    ssize_t nread = read( 0, buffer, sizeof( buffer ) );

    if ( nread < 0 )
      throw Error( "Failed to read from stdin: " + StringError( errno ) );

    if ( nread == 0 )
      break;

    stream.Write( DataRef( buffer, buffer + nread ) );
  }

  stream.End();
}

OpenDir::OpenDir( const char* path )
{
  m_dir = opendir( path );

  if ( !m_dir )
    throw Error( FileErrorString( "Failed to open directory", path, errno ) );
}

OpenDir::~OpenDir()
{
  closedir( m_dir );
}

std::vector<std::string> ReadDir( const char* path )
{
  OpenDir dir( path );

  std::vector<std::string> entries;

  while ( true )
  {
    struct dirent entry;
    struct dirent* result;

    int error = readdir_r( dir, &entry, &result );
    if ( error != 0 )
      throw Error( FileErrorString( "Failed to read directory", path, error ) );

    if ( !result )
      break;

    if ( !IsDots( result->d_name ) )
      entries.push_back( result->d_name );
  }

  return entries;
}

// This non-implementation of LocalPathToUTF8Path isn't correct, but there isn't
// a correct thing to do in all cases.

std::string LocalPathToUTF8Path( const char* path )
{
  return path;
}

std::string LocalPathToUTF8Path( const char* path, int codepage )
{
  return path;
}

}
