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

#include "BigFix/DataRef.h"
#include "BigFix/Error.h"
#include "BigFix/InflateStream.h"
#include "BigFix/TestSeams.h"
#include "TestUtility.h"
#include "ScopedMock.h"
#include <gtest/gtest.h>

using namespace BigFix;

TEST( InflateStreamTest, ShortRaw )
{
  std::string output;
  StringStream stringStream( output );
  InflateStream inflateStream( stringStream );

  DataRef data( "hello" );

  WriteOneByOneAndEnd( inflateStream, data );

  EXPECT_EQ( "hello", output );
  EXPECT_TRUE( stringStream.ended );
}

TEST( InflateStreamTest, LongRaw )
{
  std::string output;
  StringStream stringStream( output );
  InflateStream inflateStream( stringStream );

  DataRef data( "hello, world! blah blah blah" );

  WriteOneByOneAndEnd( inflateStream, data );

  EXPECT_EQ( "hello, world! blah blah blah", output );
  EXPECT_TRUE( stringStream.ended );
}

TEST( InflateStreamTest, DecompressHelloWorld )
{
  std::string output;
  StringStream stringStream( output );
  InflateStream inflateStream( stringStream );

  uint8_t data[] =
  {
    0x23, 0x23, 0x53, 0x43, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x9c, 0xf3, 0x48, 0xcd, 0xc9, 0xc9, 0xd7, 0x51, 0x28, 0xcf, 0x2f,
    0xca, 0x49, 0x51, 0x04, 0x00, 0x20, 0x5e, 0x04, 0x8a
  };

  WriteOneByOneAndEnd( inflateStream, DataRef( data, data + sizeof( data ) ) );

  EXPECT_EQ( "Hello, world!", output );
  EXPECT_TRUE( stringStream.ended );
}

TEST( InflateStreamTest, DecompressEmpty )
{
  std::string output;
  StringStream stringStream( output );
  InflateStream inflateStream( stringStream );

  uint8_t data[] =
  {
    0x23, 0x23, 0x53, 0x43, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01
  };

  WriteOneByOneAndEnd( inflateStream, DataRef( data, data + sizeof( data ) ) );

  EXPECT_EQ( "", output );
  EXPECT_TRUE( stringStream.ended );
}

TEST( InflateStreamTest, ThrowsIfEndedInChecksum )
{
  NullStream nullStream;
  InflateStream inflateStream( nullStream );

  uint8_t data[] =
  {
    0x23, 0x23, 0x53, 0x43, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00
  };

  inflateStream.Write( DataRef( data, data + sizeof( data ) ) );
  EXPECT_THROW( inflateStream.End(), Error );
}

TEST( InflateStreamTest, ThrowsIfCompressedDataIsIncomplete )
{
  NullStream nullStream;
  InflateStream inflateStream( nullStream );

  uint8_t data[] =
  {
    0x23, 0x23, 0x53, 0x43, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x78, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00
  };

  inflateStream.Write( DataRef( data, data + sizeof( data ) ) );
  EXPECT_THROW( inflateStream.End(), Error );
}

TEST( InflateStreamTest, ThrowsIfCompressedDataIsInvalid )
{
  NullStream nullStream;
  InflateStream inflateStream( nullStream );

  uint8_t data[] =
  {
    0x23, 0x23, 0x53, 0x43, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00,
    'h', 'o', 'd', 'o', 'r'
  };

  EXPECT_THROW( inflateStream.Write( DataRef( data, data + sizeof( data ) ) ),
                Error );
}

static int inflateInitError( z_stream* )
{
  return Z_STREAM_ERROR;
}

TEST( InflateStreamTest, InitFails )
{
  ScopedMock<Type_inflateInit> guard(
    inflateInitError, Real_inflateInit, Set_inflateInit );

  try
  {
    NullStream nullStream;
    InflateStream inflateStream( nullStream );
    FAIL();
  }
  catch ( const std::exception& err )
  {
    std::string message = err.what();
    std::string expected = "Failed to initialize zlib";
    EXPECT_EQ( expected, message );
  }
  catch ( ... ) { FAIL(); }
}
