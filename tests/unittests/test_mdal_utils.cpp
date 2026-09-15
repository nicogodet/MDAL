/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2019 Peter Petrik (zilolv at gmail dot com)
*/
#include "gtest/gtest.h"
#include <limits>
#include <cmath>
#include <string>
#include <vector>

//mdal
#include "mdal.h"
#include "mdal_utils.hpp"
#include "mdal_testutils.hpp"

struct SplitTestData
{
  SplitTestData( const std::string &input,
                 const std::vector<std::string> &results ):
    mInput( input ), mExpectedResult( results ) {}

  std::string mInput;
  std::vector<std::string> mExpectedResult;
};


TEST( MdalUtilsTest, SplitString )
{
  std::vector<SplitTestData> tests =
  {
    SplitTestData( "a;b;c", {"a", "b", "c"} ),
    SplitTestData( "a;;b;c", {"a", "b", "c"} ),
    SplitTestData( "a;b;", {"a", "b"} ),
    SplitTestData( ";b;", {"b"} ),
    SplitTestData( "a", {"a"} ),
    SplitTestData( "", {} )
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.mExpectedResult, MDAL::split( test.mInput, ";" ) );
  }

  // now test for string with multiple chars
  std::vector<SplitTestData> tests2 =
  {
    SplitTestData( "a;;;b;c", {"a", "b;c"} ),
    SplitTestData( "a;;;b;;;c", {"a", "b", "c"} ),
    SplitTestData( "a;;b;c", {"a;;b;c"} ),
    SplitTestData( "b;;;", {"b"} )
  };
  for ( const auto &test : tests2 )
  {
    EXPECT_EQ( test.mExpectedResult, MDAL::split( test.mInput, ";;;" ) );
  }
}

TEST( MdalUtilsTest, SplitChar )
{
  std::vector<SplitTestData> tests =
  {
    SplitTestData( "a;b;c", {"a", "b", "c"} ),
    SplitTestData( "a;;b;c", {"a", "b", "c"} ),
    SplitTestData( "a;b;", {"a", "b"} ),
    SplitTestData( ";b;", {"b"} ),
    SplitTestData( "a", {"a"} ),
    SplitTestData( "", {} )
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.mExpectedResult, MDAL::split( test.mInput, ';' ) );
  }
}

TEST( MdalUtilsTest, TrimString )
{
  std::vector<SplitTestData> tests =
  {
    SplitTestData( "", {"", "", ""} ),
    SplitTestData( " ", {"", "", ""} ),
    SplitTestData( " a", {"a", " a", "a"} ),
    SplitTestData( "a ", {"a", "a", "a "} ),
    SplitTestData( " a ", {"a", " a", "a "} ),
    SplitTestData( " a b ", {"a b", " a b", "a b "} ),
    SplitTestData( "\na b ", {"a b", "\na b", "a b "} )
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.mExpectedResult[0], MDAL::trim( test.mInput ) );
    EXPECT_EQ( test.mExpectedResult[1], MDAL::rtrim( test.mInput ) );
    EXPECT_EQ( test.mExpectedResult[2], MDAL::ltrim( test.mInput ) );
  }
}

TEST( MdalUtilsTest, TimeParsing )
{
  std::vector<std::pair<std::string, double>> tests =
  {
    { "seconds since 2001-05-05 00:00:00", 3600 },
    { "minutes since 2001-05-05 00:00:00", 60 },
    { "hours since 1900-01-01 00:00:0.0", 1 },
    { "hours", 1 },
    { "days since 1961-01-01 00:00:00", 1.0 / 24.0 },
    { "invalid format of time", 1 }
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::parseTimeUnits( test.first ) );
  }
}

TEST( MdalUtilsTest, RemoveFrom )
{
  std::vector<std::pair<std::string, std::string>> tests =
  {
    { "", "" },
    { "aaaa XXX", "aaaa " },
    { "bbb XXX XXX", "bbb XXX " },
    { "ccc", "ccc" },
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::removeFrom( test.first, "XXX" ) );
  }
}

TEST( MdalUtilsTest, CF_TimeUnitParsing )
{
  std::vector<std::pair<std::string, MDAL::RelativeTimestamp::Unit>> tests =
  {
    { "seconds since 2001-05-05 00:00:00", MDAL::RelativeTimestamp::seconds },
    { "minutes since 2001-05-05 00:00:00", MDAL::RelativeTimestamp::minutes },
    { "hours since 1900-01-01 00:00:0.0", MDAL::RelativeTimestamp::hours },
    { "days since 1961-01-01 00:00:00", MDAL::RelativeTimestamp::days },
    { "weeks since 1961-01-01 00:00:00", MDAL::RelativeTimestamp::weeks },
    { "month since 1961-01-01 00:00:00", MDAL::RelativeTimestamp::months_CF },
    { "months since 1961-01-01 00:00:00", MDAL::RelativeTimestamp::months_CF },
    { "year since 1961-01-01 00:00:00", MDAL::RelativeTimestamp::exact_years },
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::parseCFTimeUnit( test.first ) );
  }
}

TEST( MdalUtilsTest, CF_ReferenceTimePArsing )
{
  std::vector<std::pair<std::string, MDAL::DateTime>> tests =
  {
    { "seconds since 2001-05-05 00:00:00", MDAL::DateTime( 2001, 5, 5, 00, 00, 00 ) },
    { "hours since 1900-05-05 10:00:0.0", MDAL::DateTime( 1900, 5, 5, 10, 00, 00 ) },
    { "days since 1200-05-05 00:05:00", MDAL::DateTime( 1200, 5, 5, 00, 5, 00 ) },
    { "weeks since 1961-05-05 00:01:10", MDAL::DateTime( 1961, 5, 5, 00, 1, 10 ) },
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::parseCFReferenceTime( test.first, "gregorian" ) );
  }
}

TEST( MdalUtilsTest, StartsWidth )
{
  // case sensitive
  EXPECT_EQ( false, MDAL::startsWith( "abcs", "" ) );
  EXPECT_EQ( false, MDAL::startsWith( "abcs", "", MDAL::ContainsBehaviour::CaseSensitive ) );

  std::vector<std::pair<std::string, bool>> tests =
  {
    { "abcd", true },
    { " abcd", false },
    { "ab", false },
    { "", false },
    { "abc ", true },
    { "cccc", false },
    { "ABC", false }
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::startsWith( test.first, "abc" ) );
    EXPECT_EQ( test.second, MDAL::startsWith( test.first, "abc", MDAL::ContainsBehaviour::CaseSensitive ) );
  }

  // case insensitive
  EXPECT_EQ( false, MDAL::startsWith( "abcs", "", MDAL::ContainsBehaviour::CaseInsensitive ) );
  tests =
  {
    { "abcd", true },
    { " abcd", false },
    { "ab", false },
    { "", false },
    { "abc ", true },
    { "cccc", false },
    { "ABC", true },
    { "AbC", true }
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::startsWith( test.first, "abc", MDAL::ContainsBehaviour::CaseInsensitive ) );
  }
}

TEST( MdalUtilsTest, EndsWidth )
{
  // case sensitive
  EXPECT_EQ( false, MDAL::endsWith( "abcs", "", MDAL::ContainsBehaviour::CaseSensitive ) );

  std::vector<std::pair<std::string, bool>> tests =
  {
    { "abcd", true },
    { " abcd", true },
    { "ab", false },
    { "", false },
    { "abcd ", false },
    { "cccc", false },
    { "aa ABCD", false }
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::endsWith( test.first, "cd", MDAL::ContainsBehaviour::CaseSensitive ) );
  }

  // case insensitive
  EXPECT_EQ( false, MDAL::endsWith( "abcs", "", MDAL::ContainsBehaviour::CaseInsensitive ) );
  tests =
  {
    { "abCd", true },
    { " abcd", true },
    { "ab", false },
    { "", false },
    { "abcd ", false },
    { "cccc", false },
    { "ABCD", true },
    { "aa AbcD", true }
  };
  for ( const auto &test : tests )
  {
    EXPECT_EQ( test.second, MDAL::endsWith( test.first, "cd", MDAL::ContainsBehaviour::CaseInsensitive ) );
  }
}

struct LoadMeshUri
{
  LoadMeshUri( std::string u, std::string d, std::string mf, std::string mn ) :
    uri( std::move( u ) ),
    driver( std::move( d ) ),
    meshFile( std::move( mf ) ),
    meshName( std::move( mn ) ) {};

  std::string uri;
  std::string driver;
  std::string meshFile;
  std::string meshName;
};


TEST( MdalUtilsTest, ParseLoadMeshUri )
{
  std::string inputUri, driverName, meshFile, specificMeshName;

  std::vector<LoadMeshUri> tests
  {
    LoadMeshUri( "Ugrid:\"mesh.nc\":mesh1d", "Ugrid", "mesh.nc", "mesh1d" ),
    LoadMeshUri( "Ugrid:\"mesh.nc\":1", "Ugrid", "mesh.nc", "1" ),
    LoadMeshUri( "\"mesh.nc\":mesh1d", "", "mesh.nc", "mesh1d" ),
    LoadMeshUri( "\"mesh.nc\":1", "", "mesh.nc", "1" ),
    LoadMeshUri( "Ugrid:\"mesh.nc\"", "Ugrid", "mesh.nc", "" ),
    LoadMeshUri( "\"mesh.nc\"", "", "mesh.nc", "" ),
    LoadMeshUri( "mesh.nc", "", "mesh.nc", "" ),
    LoadMeshUri( "Ugrid:\"C:\\myfile. \\with spaces\\hi.nc\":\"incredible mesh\"", "Ugrid", "C:\\myfile. \\with spaces\\hi.nc", "incredible mesh" )
  };

  for ( const LoadMeshUri &test : tests )
  {
    MDAL::parseDriverAndMeshFromUri( test.uri, driverName, meshFile, specificMeshName );

    EXPECT_EQ( driverName, test.driver );
    EXPECT_EQ( meshFile, test.meshFile );
    EXPECT_EQ( specificMeshName, test.meshName );
  }
}

TEST( MdalUtilsTest, BuildMeshUri )
{
  std::string uri;

  std::vector<LoadMeshUri> tests
  {
    LoadMeshUri( "Ugrid:\"mesh.nc\":mesh1d", "Ugrid", "mesh.nc", "mesh1d" ),
    LoadMeshUri( "Ugrid:\"mesh.nc\":1", "Ugrid", "mesh.nc", "1" ),
    LoadMeshUri( "\"mesh.nc\":mesh1d", "", "mesh.nc", "mesh1d" ),
    LoadMeshUri( "\"mesh.nc\":1", "", "mesh.nc", "1" ),
    LoadMeshUri( "Ugrid:\"mesh.nc\"", "Ugrid", "mesh.nc", "" ),
    LoadMeshUri( "mesh.nc", "", "mesh.nc", "" ),
    LoadMeshUri( "Ugrid:\"C:\\myfile. \\with spaces\\hi.nc\":\"incredible mesh\"", "Ugrid", "C:\\myfile. \\with spaces\\hi.nc", "\"incredible mesh\"" )
  };

  for ( const LoadMeshUri &test : tests )
  {
    uri = MDAL::buildMeshUri( test.meshFile, test.meshName, test.driver );
    EXPECT_EQ( uri, test.uri );
  }
}

struct BuildMeshUri
{
  BuildMeshUri( std::string u, std::string d, std::string mf, std::vector<std::string> mn ) :
    mergedUris( std::move( u ) ),
    driver( std::move( d ) ),
    meshFile( std::move( mf ) ),
    meshNames( std::move( mn ) ) {};

  std::string mergedUris;
  std::string driver;
  std::string meshFile;
  std::vector<std::string> meshNames;
};

TEST( MdalUtilsTest, BuildAndMergeMeshUri )
{
  std::string uris;

  std::vector<BuildMeshUri> tests
  {
    BuildMeshUri( "2DM:\"meshFile\":mesh1;;2DM:\"meshFile\":mesh2", "2DM", "meshFile", {"mesh1", "mesh2"} ),
    BuildMeshUri( "\"meshFile\":mesh1;;\"meshFile\":mesh2", "", "meshFile", {"mesh1", "mesh2"} ),
    BuildMeshUri( "2DM:\"meshFile\"", "2DM", "meshFile", {} ),
    BuildMeshUri( "", "2DM", "", {"v"} ),
  };

  for ( const BuildMeshUri &test : tests )
  {
    uris = MDAL::buildAndMergeMeshUris( test.meshFile, test.meshNames, test.driver );
    EXPECT_EQ( uris, test.mergedUris );
  }
}

TEST( MdalUtilsTest, FileExtensionTest )
{
  std::string extension;

  std::vector<std::pair<std::string, std::string>> tests
  {
    {"C:\\myfile. \\with spaces\\hi.nc", ".nc"},
    {"/home/test/param.txt", ".txt"},
    {"/impossible \f/dfd/ test.2dm", ".2dm"},
    {"/home/no/extension", ""}
  };

  for ( const std::pair<std::string, std::string> &test : tests )
  {
    EXPECT_EQ( MDAL::fileExtension( test.first ), test.second );
  }
}

TEST( MdalUtilsTest, LibraryTest )
{
  // test only invalidity, valid library is tested in MeshDynamicDriverTest
  MDAL::Library library( "nofile" );
  EXPECT_FALSE( library.isValid() );
  std::function<void ( int )> funct = library.getSymbol<int, int>( "function" );
  EXPECT_FALSE( funct );
}

TEST( MdalUtilsTest, SwapBytes )
{
  EXPECT_EQ( 0x78563412u, MDAL::swapBytes32( 0x12345678u ) );
  EXPECT_EQ( 0x12345678u, MDAL::swapBytes32( MDAL::swapBytes32( 0x12345678u ) ) );
  EXPECT_EQ( 0xefcdab8967452301ull, MDAL::swapBytes64( 0x0123456789abcdefull ) );
  EXPECT_EQ( 0x0123456789abcdefull,
             MDAL::swapBytes64( MDAL::swapBytes64( 0x0123456789abcdefull ) ) );

  // swapping twice is the identity, including for floating point values
  std::vector<double> values{ 0.0, -1.5, 3.14159265358979, 1e300, -0.0 };
  const std::vector<double> expected = values;
  MDAL::swapBytesInPlace( values.data(), values.size() );
  EXPECT_NE( values, expected );
  MDAL::swapBytesInPlace( values.data(), values.size() );
  EXPECT_EQ( values, expected );

  std::vector<int> ints{ 0, 1, -1, std::numeric_limits<int>::max(), std::numeric_limits<int>::lowest() };
  const std::vector<int> expectedInts = ints;
  MDAL::swapBytesInPlace( ints.data(), ints.size() );
  MDAL::swapBytesInPlace( ints.data(), ints.size() );
  EXPECT_EQ( ints, expectedInts );
}

//! Reads a whole file as raw bytes (not MDAL::readFileToString, which opens in
//! text mode and would translate line endings on Windows)
static std::string readBinaryFile( const std::string &path )
{
  std::ifstream in( path, std::ifstream::binary );
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

//! Writes \a values with writeArray, reads them back with readArray
template<typename T>
static std::vector<T> arrayRoundTrip( const std::vector<T> &values, bool changeEndianness,
                                      const std::string &name )
{
  const std::string path = tmp_file( name );
  {
    std::ofstream out = MDAL::openOutputFile( path, std::ofstream::binary );
    MDAL::writeArray( values.data(), values.size(), out, changeEndianness );
  }
  std::vector<T> read( values.size() );
  std::ifstream in = MDAL::openInputFile( path, std::ifstream::binary );
  EXPECT_TRUE( MDAL::readArray( read.data(), read.size(), in, changeEndianness ) );
  return read;
}

TEST( MdalUtilsTest, ReadWriteArrayRoundTrip )
{
  std::vector<int> ints{ 0, 1, -1, 42, std::numeric_limits<int>::max(), std::numeric_limits<int>::lowest() };
  EXPECT_EQ( ints, arrayRoundTrip( ints, false, "/array_int_native.bin" ) );
  EXPECT_EQ( ints, arrayRoundTrip( ints, true, "/array_int_swapped.bin" ) );

  std::vector<double> doubles{ 0.0, -1.5, 3.14159265358979, 1e300, -1e-300 };
  EXPECT_EQ( doubles, arrayRoundTrip( doubles, false, "/array_double_native.bin" ) );
  EXPECT_EQ( doubles, arrayRoundTrip( doubles, true, "/array_double_swapped.bin" ) );

  std::vector<float> floats{ 0.0f, -1.5f, 2.5f, 1e30f };
  EXPECT_EQ( floats, arrayRoundTrip( floats, true, "/array_float_swapped.bin" ) );

  // more values than the internal write chunk, to cover chunk boundaries
  std::vector<int> many( 200000 );
  for ( size_t i = 0; i < many.size(); ++i )
    many[i] = static_cast<int>( i ) - 100000;
  EXPECT_EQ( many, arrayRoundTrip( many, true, "/array_int_chunked.bin" ) );
}

TEST( MdalUtilsTest, ReadWriteArrayBytesMatchSingleValues )
{
  // writeArray must produce exactly what a writeValue loop produces
  const std::vector<int> values{ 1, -2, 3, -4, 5 };
  const std::string bulkPath = tmp_file( "/array_bulk.bin" );
  const std::string loopPath = tmp_file( "/array_loop.bin" );
  {
    std::ofstream out = MDAL::openOutputFile( bulkPath, std::ofstream::binary );
    MDAL::writeArray( values.data(), values.size(), out, true );
  }
  {
    std::ofstream out = MDAL::openOutputFile( loopPath, std::ofstream::binary );
    for ( int v : values )
      MDAL::writeValue( v, out, true );
  }
  EXPECT_EQ( readBinaryFile( bulkPath ), readBinaryFile( loopPath ) );

  // and readArray must read back what a readValue loop reads
  std::vector<int> bulk( values.size() );
  std::ifstream in = MDAL::openInputFile( bulkPath, std::ifstream::binary );
  EXPECT_TRUE( MDAL::readArray( bulk.data(), bulk.size(), in, true ) );
  EXPECT_EQ( values, bulk );
}

TEST( MdalUtilsTest, ReadArrayFailsOnTruncatedStream )
{
  const std::string path = tmp_file( "/array_truncated.bin" );
  const std::vector<int> values{ 1, 2, 3 };
  {
    std::ofstream out = MDAL::openOutputFile( path, std::ofstream::binary );
    MDAL::writeArray( values.data(), values.size(), out, true );
  }

  std::vector<int> tooMany( 10 );
  std::ifstream in = MDAL::openInputFile( path, std::ifstream::binary );
  EXPECT_FALSE( MDAL::readArray( tooMany.data(), tooMany.size(), in, true ) );

  // reading nothing always succeeds and leaves the stream untouched
  std::ifstream empty = MDAL::openInputFile( path, std::ifstream::binary );
  EXPECT_TRUE( MDAL::readArray( tooMany.data(), 0, empty, true ) );
  EXPECT_TRUE( empty.good() );
}
