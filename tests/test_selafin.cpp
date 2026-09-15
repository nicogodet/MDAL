/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2019 ARTELIA - Christophe Coulet
 (christophe dot coulet at arteliagroup dot com)
*/
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <iterator>

//mdal
#include "mdal.h"
#include "mdal_utils.hpp"
#include "mdal_testutils.hpp"

#ifdef _MSC_VER
#include <locale>
#include <codecvt>
#include <stringapiset.h>
#endif

TEST( MeshSLFTest, Driver )
{
  MDAL_DriverH driver = MDAL_driverFromName( "SELAFIN" );
  EXPECT_EQ( strcmp( MDAL_DR_filters( driver ), "*.slf;;*.ser;;*.geo;;*.res" ), 0 );
  EXPECT_TRUE( MDAL_DR_meshLoadCapability( driver ) );
  EXPECT_TRUE( MDAL_DR_saveMeshCapability( driver ) );
  EXPECT_EQ( strcmp( MDAL_DR_saveMeshSuffix( driver ), "slf" ), 0 );
  EXPECT_EQ( MDAL_DR_faceVerticesMaximumCount( driver ), 3 );
}

TEST( MeshSLFTest, MalpassetGeometry )
{
  std::string path = test_file( "/slf/example.slf" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );
  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  MDAL_Status s = MDAL_LastStatus();
  EXPECT_EQ( MDAL_Status::None, s );

  const char *projection = MDAL_M_projection( m );
  EXPECT_EQ( std::string( "" ), std::string( projection ) );

  std::string driverName = MDAL_M_driverName( m );
  EXPECT_EQ( driverName, "SELAFIN" );

  // ///////////
  // Vertices
  // ///////////
  int v_count = MDAL_M_vertexCount( m );
  EXPECT_EQ( v_count, 13541 );
  double x = getVertexXCoordinatesAt( m, 0 );
  double y = getVertexYCoordinatesAt( m, 0 );
  double z = getVertexZCoordinatesAt( m, 0 );
  EXPECT_DOUBLE_EQ( 5905.615234375, x );
  EXPECT_DOUBLE_EQ( 4695.9560546875, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  x = getVertexXCoordinatesAt( m, 1000 );
  y = getVertexYCoordinatesAt( m, 1000 );
  z = getVertexZCoordinatesAt( m, 1000 );
  EXPECT_DOUBLE_EQ( 16275.708984375, x );
  EXPECT_DOUBLE_EQ( -936.93072509765625, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  // ///////////
  // Faces
  // ///////////
  int f_count = MDAL_M_faceCount( m );
  EXPECT_EQ( 26000, f_count );

  // ///////////
  // Edges
  // ///////////
  EXPECT_EQ( 0, MDAL_M_edgeCount( m ) );

  // ///////////
  // Extent
  // ///////////
  double xmin, xmax, ymin, ymax;
  MDAL_M_extent( m, &xmin, &xmax, &ymin, &ymax );
  EXPECT_EQ( xmin, 536.4716186523438 );
  EXPECT_EQ( xmax, 17763.0703125 );
  EXPECT_EQ( ymin, -2343.5400390625 );
  EXPECT_EQ( ymax, 6837.7900390625 );

  // test face 1
  int f_v_count = getFaceVerticesCountAt( m, 1 );
  EXPECT_EQ( 3, f_v_count ); //only triangles!
  int f_v = getFaceVerticesIndexAt( m, 100, 0 );
  EXPECT_EQ( 6807, f_v );
  f_v = getFaceVerticesIndexAt( m, 100, 1 );
  EXPECT_EQ( 6277, f_v ); \
  f_v = getFaceVerticesIndexAt( m, 100, 2 );
  EXPECT_EQ( 6811, f_v );

  // Datasets
  ASSERT_EQ( 1, MDAL_M_datasetGroupCount( m ) );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, 0 );
  ASSERT_NE( g, nullptr );

  int meta_count = MDAL_G_metadataCount( g );
  ASSERT_EQ( 1, meta_count );

  const char *name = MDAL_G_name( g );
  EXPECT_EQ( std::string( "bottom" ), std::string( name ) );

  bool scalar = MDAL_G_hasScalarData( g );
  EXPECT_EQ( true, scalar );

  MDAL_DataLocation dataLocation = MDAL_G_dataLocation( g );
  EXPECT_EQ( dataLocation, MDAL_DataLocation::DataOnVertices );

  ASSERT_EQ( 1, MDAL_G_datasetCount( g ) );
  MDAL_DatasetH ds = MDAL_G_dataset( g, 0 );
  ASSERT_NE( ds, nullptr );

  bool valid = MDAL_D_isValid( ds );
  EXPECT_EQ( true, valid );

  int count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 13541, count );

  double value = getValue( ds, 0 );
  EXPECT_DOUBLE_EQ( 70.0, value );
  value = getValue( ds, 2 );
  EXPECT_DOUBLE_EQ( 94.5398330688477, value );
  value = getValue( ds, 1000 );
  EXPECT_DOUBLE_EQ( 1.73051724061679e-008, value );
  value = getValue( ds, 9571 );
  EXPECT_DOUBLE_EQ( 7.5623664855957, value );

  std::vector<double> newVertex{10, 10, 10};
  MDAL_M_addVertices( m, 1, newVertex.data() );
  EXPECT_EQ( MDAL_LastStatus(), Err_IncompatibleMesh );

  MDAL_CloseMesh( m );
}

static void testPreExistingScalarDatasetGroup( MDAL_DatasetGroupH r )
{
  ASSERT_NE( r, nullptr );

  int meta_count = MDAL_G_metadataCount( r );
  ASSERT_EQ( 1, meta_count );

  const char *name = MDAL_G_name( r );
  EXPECT_EQ( std::string( "surface libre   m" ), std::string( name ) );

  bool scalar = MDAL_G_hasScalarData( r );
  EXPECT_EQ( true, scalar );

  MDAL_DataLocation dataLocation = MDAL_G_dataLocation( r );
  EXPECT_EQ( dataLocation, MDAL_DataLocation::DataOnVertices );

  ASSERT_EQ( 2, MDAL_G_datasetCount( r ) );
  MDAL_DatasetH ds = MDAL_G_dataset( r, 1 );
  ASSERT_NE( ds, nullptr );

  double time = MDAL_D_time( ds );
  EXPECT_TRUE( compareDurationInHours( 1.111111111, time ) );

  bool valid = MDAL_D_isValid( ds );
  EXPECT_EQ( true, valid );

  int count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 13541, count );

  double value = getValue( ds, 8667 );
  EXPECT_DOUBLE_EQ( 31.965662002563477, value );

  double min, max;
  MDAL_D_minimumMaximum( ds, &min, &max );
  EXPECT_DOUBLE_EQ( -0.00673320097848773, min );
  EXPECT_DOUBLE_EQ( 100.00228118896484, max );

  MDAL_G_minimumMaximum( r, &min, &max );
  EXPECT_DOUBLE_EQ( -0.00673320097848773, min );
  EXPECT_DOUBLE_EQ( 100.00228118896484, max );
}

static void testPreExisitingVectorDatasetGroup( MDAL_DatasetGroupH r )
{
  ASSERT_NE( r, nullptr );

  size_t meta_count = MDAL_G_metadataCount( r );
  ASSERT_EQ( 1, meta_count );

  std::string name = MDAL_G_name( r );
  EXPECT_EQ( std::string( "vitesse       ms" ), name );

  double scalar = MDAL_G_hasScalarData( r );
  EXPECT_EQ( false, scalar );

  MDAL_DataLocation dataLocation = MDAL_G_dataLocation( r );
  EXPECT_EQ( dataLocation, MDAL_DataLocation::DataOnVertices );

  ASSERT_EQ( 2, MDAL_G_datasetCount( r ) );
  MDAL_DatasetH ds = MDAL_G_dataset( r, 1 );
  ASSERT_NE( ds, nullptr );

  bool valid = MDAL_D_isValid( ds );
  EXPECT_EQ( true, valid );

  size_t count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 13541, count );

  double value = getValueX( ds, 8667 );
  EXPECT_DOUBLE_EQ( 6.2320127487182617, value );
  value = getValueY( ds, 8667 );
  EXPECT_DOUBLE_EQ( -0.97271907329559326, value );

  double min, max;
  MDAL_D_minimumMaximum( ds, &min, &max );
  EXPECT_TRUE( MDAL::equals( 0, min ) );
  EXPECT_TRUE( MDAL::equals( 7.5673562379016834, max ) );

  EXPECT_TRUE( compareReferenceTime( r, "1900-01-01T00:00:00" ) );
}

TEST( MeshSLFTest, MalpassetResultFrench )
{
  std::string path = test_file( "/slf/example_res_fr.slf" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );
  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  MDAL_Status s = MDAL_LastStatus();
  EXPECT_EQ( MDAL_Status::None, s );

  const char *projection = MDAL_M_projection( m );
  EXPECT_EQ( std::string( "" ), std::string( projection ) );

  std::string driverName = MDAL_M_driverName( m );
  EXPECT_EQ( driverName, "SELAFIN" );

  // ///////////
  // Vertices
  // ///////////
  int v_count = MDAL_M_vertexCount( m );
  EXPECT_EQ( v_count, 13541 );
  double z = getVertexZCoordinatesAt( m, 0 );
  EXPECT_DOUBLE_EQ( 0.0, z );
  // ///////////
  // Faces
  // ///////////
  int f_count = MDAL_M_faceCount( m );
  EXPECT_EQ( 26000, f_count );

  // test face 1
  int f_v_count = getFaceVerticesCountAt( m, 1 );
  EXPECT_EQ( 3, f_v_count ); //only triangles!

  int var_count = MDAL_M_datasetGroupCount( m );
  ASSERT_EQ( 4, var_count ); // 4 variables (Velocity, Water Depth, Free Surface and Bottom)

  // ///////////
  // Scalar Dataset
  // ///////////
  testPreExistingScalarDatasetGroup( MDAL_M_datasetGroup( m, 2 ) );

  // ///////////
  // Vector Dataset
  // ///////////
  testPreExisitingVectorDatasetGroup( MDAL_M_datasetGroup( m, 0 ) );

  MDAL_CloseMesh( m );
}


TEST( MeshSLFTest, SaveMeshFrame )
{
  saveAndCompareMesh(
    test_file( "/slf/example_res_fr.slf" ),
    tmp_file( "/emptymesh.slf" ),
    "SELAFIN" );
}

// Minimal standalone reader for the IPOBO record of a SELAFIN file: records
// are framed by two 4-byte lengths, so every record before the IPOBO one can
// be skipped generically. Local to the tests on purpose — the test binaries
// only consume the exported C API plus this reader.
static int readInt( std::ifstream &f, bool bigEndian )
{
  unsigned char b[4] = { 0, 0, 0, 0 };
  f.read( reinterpret_cast<char *>( b ), 4 );
  if ( bigEndian )
    return ( b[0] << 24 ) | ( b[1] << 16 ) | ( b[2] << 8 ) | b[3];
  return ( b[3] << 24 ) | ( b[2] << 16 ) | ( b[1] << 8 ) | b[0];
}

static int readBigEndianInt( std::ifstream &f )
{
  return readInt( f, true );
}

static std::vector<int> readIpoboFromFile( const std::string &fileName )
{
  std::ifstream f( fileName, std::ios::binary );
  if ( !f.is_open() )
    return std::vector<int>();

  // Selafin files are big-endian, but some tools (Janet) write little-endian
  // ones. The first record holds the 80-character title, so its length prefix
  // tells them apart, the way SelafinFile::initialize does.
  const bool bigEndian = readBigEndianInt( f ) == 80;
  f.seekg( 0 );

  int len = readInt( f, bigEndian );  // title record (80 chars)
  if ( len != 80 )
    return std::vector<int>();
  f.seekg( len + 4, std::ios::cur );

  len = readInt( f, bigEndian );  // NBV(1), NBV(2)
  const std::streamoff nbvPos = f.tellg();
  const int nbv1 = readInt( f, bigEndian );
  const int nbv2 = readInt( f, bigEndian );
  f.seekg( nbvPos + len + 4 );

  for ( int i = 0; i < nbv1 + nbv2; ++i )  // variable names
  {
    len = readInt( f, bigEndian );
    f.seekg( len + 4, std::ios::cur );
  }

  len = readInt( f, bigEndian );  // IPARAM
  const std::streamoff iparamPos = f.tellg();
  std::vector<int> iparam( 10, 0 );
  for ( int i = 0; i < 10 && i * 4 < len; ++i )
    iparam[i] = readInt( f, bigEndian );
  f.seekg( iparamPos + len + 4 );

  if ( iparam[9] == 1 )  // date record
  {
    len = readInt( f, bigEndian );
    f.seekg( len + 4, std::ios::cur );
  }

  len = readInt( f, bigEndian );  // NELEM, NPOIN, NDP, 1
  const std::streamoff elemPos = f.tellg();
  readInt( f, bigEndian );  // NELEM
  const int npoin = readInt( f, bigEndian );
  f.seekg( elemPos + len + 4 );

  len = readInt( f, bigEndian );  // connectivity
  f.seekg( len + 4, std::ios::cur );

  len = readInt( f, bigEndian );  // IPOBO
  if ( !f || npoin <= 0 || len != npoin * 4 )
    return std::vector<int>();
  std::vector<int> ipobo( npoin );
  for ( int i = 0; i < npoin; ++i )
    ipobo[i] = readInt( f, bigEndian );
  if ( !f )
    return std::vector<int>();
  return ipobo;
}

static bool fileIsPresent( const std::string &fileName )
{
  std::ifstream f( fileName, std::ios::binary );
  return f.is_open();
}

static std::string fileContent( const std::string &fileName )
{
  std::ifstream f( fileName, std::ios::binary );
  return std::string( ( std::istreambuf_iterator<char>( f ) ), std::istreambuf_iterator<char>() );
}

// Builds a triangulated MemoryMesh (2DM) from interleaved x,y,z coordinates
// and 0-based triangle connectivity.
static MDAL_MeshH createTriMesh( std::vector<double> &coords, std::vector<int> &faceIndices )
{
  MDAL_DriverH driver = MDAL_driverFromName( "2DM" );
  MDAL_MeshH mesh = MDAL_CreateMesh( driver );
  const int nVerts = static_cast<int>( coords.size() / 3 );
  const int nFaces = static_cast<int>( faceIndices.size() / 3 );
  MDAL_M_addVertices( mesh, nVerts, coords.data() );
  std::vector<int> faceSizes( static_cast<size_t>( nFaces ), 3 );
  MDAL_M_addFaces( mesh, nFaces, faceSizes.data(), faceIndices.data() );
  return mesh;
}

// Saves such a mesh as SELAFIN (a MemoryMesh, so save() takes the compute path
// and exercises computeIPOBO) and returns the IPOBO array read back.
static std::vector<int> saveTriMeshAndReadIpobo(
  std::vector<double> coords,        // x,y,z interleaved (C API needs mutable data)
  std::vector<int> faceIndices,      // 3 per triangle, 0-based
  const std::string &tmpName )
{
  MDAL_MeshH mesh = createTriMesh( coords, faceIndices );
  std::string savedFile = tmp_file( tmpName );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  EXPECT_EQ( MDAL_Status::None, MDAL_LastStatus() ) << "SELAFIN save failed";
  MDAL_CloseMesh( mesh );
  return readIpoboFromFile( savedFile );
}

// Saves a mesh whose boundary cannot be numbered: the save must fail with
// Err_IncompatibleMesh, leave the file it was about to replace untouched and
// leave no temporary file behind.
static void expectSaveRejected( std::vector<double> coords,
                                std::vector<int> faceIndices,
                                const std::string &tmpName )
{
  MDAL_MeshH mesh = createTriMesh( coords, faceIndices );
  std::string savedFile = tmp_file( tmpName );
  const std::string existing = "this file must survive the failed save";
  {
    std::ofstream target( savedFile, std::ios::binary | std::ios::trunc );
    ASSERT_TRUE( target.is_open() );
    target << existing;
  }
  std::remove( ( savedFile + ".tmp" ).c_str() );  // a previous run may have left one

  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  EXPECT_EQ( MDAL_Status::Err_IncompatibleMesh, MDAL_LastStatus() )
      << "A mesh whose boundary cannot be numbered must be rejected";
  MDAL_CloseMesh( mesh );

  EXPECT_EQ( existing, fileContent( savedFile ) ) << "the target file must be left untouched";
  EXPECT_FALSE( fileIsPresent( savedFile + ".tmp" ) ) << "no temporary file must be left behind";
}

TEST( MeshSLFTest, IPOBOComputation )
{
  // Build a 3x3 triangulated grid in memory and save it as SELAFIN. The mesh
  // is a MemoryMesh, so save() takes the compute path and exercises
  // computeIPOBO. The expected vector was traced by hand from the documented
  // algorithm and cross-checked with an independent reference implementation.
  //
  //   6 -- 7 -- 8
  //   |  / |  / |
  //   3 -- 4 -- 5
  //   |  / |  / |
  //   0 -- 1 -- 2
  std::vector<double> coords
  {
    0, 0, 0,   1, 0, 0,   2, 0, 0,
    0, 1, 0,   1, 1, 0,   2, 1, 0,
    0, 2, 0,   1, 2, 0,   2, 2, 0,
  };
  std::vector<int> faceIndices
  {
    0, 1, 4,   0, 4, 3,
    1, 2, 5,   1, 5, 4,
    3, 4, 7,   3, 7, 6,
    4, 5, 8,   4, 8, 7,
  };

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_grid.slf" );
  // build_ipobo() reference: SW corner (vertex 0) starts at 1, perimeter CCW,
  // centre vertex 4 interior.
  const std::vector<int> expected{ 1, 2, 3, 8, 0, 4, 7, 6, 5 };
  EXPECT_EQ( ipobo, expected ) << "IPOBO does not match the expected boundary numbering";
}

TEST( MeshSLFTest, IPOBOIsland )
{
  // 4x4 grid (row-major, x fastest) with the central cell removed, forming an
  // annulus: a 12-node outer boundary (CCW) enclosing a 4-node island (CW).
  // The outer ring is numbered 1..12 first, then the island 13..16
  // (hand-traced and cross-checked with an independent reference
  // implementation).
  std::vector<double> coords;
  for ( int yy = 0; yy < 4; ++yy )
    for ( int xx = 0; xx < 4; ++xx )
    {
      coords.push_back( xx );
      coords.push_back( yy );
      coords.push_back( 0 );
    }
  std::vector<int> faceIndices
  {
    0, 1, 5,    0, 5, 4,
    1, 2, 6,    1, 6, 5,
    2, 3, 7,    2, 7, 6,
    4, 5, 9,    4, 9, 8,
    6, 7, 11,   6, 11, 10,
    8, 9, 13,   8, 13, 12,
    9, 10, 14,  9, 14, 13,
    10, 11, 15, 10, 15, 14,
  };

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_island.slf" );
  const std::vector<int> expected{ 1, 2, 3, 4, 12, 13, 16, 5, 11, 14, 15, 6, 10, 9, 8, 7 };
  EXPECT_EQ( ipobo, expected ) << "Island IPOBO does not match the expected boundary numbering";
}

TEST( MeshSLFTest, IPOBOMultiDomain )
{
  // Two disjoint unit squares far apart. The domain whose south-west node has
  // the smaller (x+y) is numbered first; both external rings are CCW.
  std::vector<double> coords
  {
    0,  0,  0,    1,  0,  0,    1,  1,  0,    0,  1,  0,
    10, 10, 0,    11, 10, 0,    11, 11, 0,    10, 11, 0,
  };
  std::vector<int> faceIndices
  {
    0, 1, 2,   0, 2, 3,
    4, 5, 6,   4, 6, 7,
  };

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_multidomain.slf" );
  const std::vector<int> expected{ 1, 2, 3, 4, 5, 6, 7, 8 };
  EXPECT_EQ( ipobo, expected ) << "Multi-domain IPOBO does not match the expected boundary numbering";
}

TEST( MeshSLFTest, IPOBOSuperimposedNodes )
{
  // Two sub-domains separated by a zero-width weir: nodes 4,5 (domain A) and
  // 6,7 (domain B) share the same coordinates, so B's south-west node lies
  // exactly ON A's ring. The representative-point fallback must keep BOTH
  // contours classified as external (depth 0) and hence CCW.
  std::vector<double> coords
  {
    0, 0, 0,    2, 0, 0,    2, 4, 0,    0, 4, 0,
    0, 1, 0,    0, 3, 0,
    0, 1, 0,    0, 3, 0,    -1, 3.5, 0,
  };
  std::vector<int> faceIndices
  {
    1, 2, 3,   1, 3, 5,   1, 5, 4,   1, 4, 0,
    6, 7, 8,
  };

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_superimposed.slf" );
  const std::vector<int> expected{ 1, 2, 3, 4, 6, 5, 7, 8, 9 };
  EXPECT_EQ( ipobo, expected ) << "Superimposed weir nodes must not demote a domain to an island";
}

TEST( MeshSLFTest, IPOBOIslandInIsland )
{
  // The 4x4 annulus of IPOBOIsland plus a small triangle floating inside the
  // hole: depth 2 (even) so the triangle is an external CCW contour, child of
  // the hole in the containment forest, numbered right after it.
  std::vector<double> coords;
  for ( int yy = 0; yy < 4; ++yy )
    for ( int xx = 0; xx < 4; ++xx )
    {
      coords.push_back( xx );
      coords.push_back( yy );
      coords.push_back( 0 );
    }
  const std::vector<double> triangle{ 1.2, 1.2, 0,   1.8, 1.2, 0,   1.5, 1.8, 0 };
  coords.insert( coords.end(), triangle.begin(), triangle.end() );
  std::vector<int> faceIndices
  {
    0, 1, 5,    0, 5, 4,
    1, 2, 6,    1, 6, 5,
    2, 3, 7,    2, 7, 6,
    4, 5, 9,    4, 9, 8,
    6, 7, 11,   6, 11, 10,
    8, 9, 13,   8, 13, 12,
    9, 10, 14,  9, 14, 13,
    10, 11, 15, 10, 15, 14,
    16, 17, 18,
  };

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_island2.slf" );
  const std::vector<int> expected{ 1, 2, 3, 4, 12, 13, 16, 5, 11, 14, 15, 6, 10, 9, 8, 7, 17, 18, 19 };
  EXPECT_EQ( ipobo, expected ) << "Doubly-nested contour must be CCW and numbered after its parent";
}

TEST( MeshSLFTest, IPOBOTieBreakSouthWestKey )
{
  // Two disjoint squares whose south-west corners share the same x+y key (0):
  // the tie must break on the smallest node id, so the square owning node 0
  // is numbered first.
  std::vector<double> coords
  {
    0, 0, 0,    1, 0, 0,    1, 1, 0,    0, 1, 0,
    2, -2, 0,   3, -2, 0,   3, -1, 0,   2, -1, 0,
  };
  std::vector<int> faceIndices
  {
    0, 1, 2,   0, 2, 3,
    4, 5, 6,   4, 6, 7,
  };

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_tiebreak.slf" );
  const std::vector<int> expected{ 1, 2, 3, 4, 5, 6, 7, 8 };
  EXPECT_EQ( ipobo, expected ) << "Equal x+y keys must break ties on the smallest node id";
}

TEST( MeshSLFTest, IPOBODegenerateBowtie )
{
  // Two triangles meeting at a single shared vertex (node 0) form a pinch
  // point: node 0 is the end of 4 boundary edges, so the boundary does not
  // decompose into simple contours and cannot be numbered. An all-zero IPOBO
  // would read as NPTFR = 0 in Telemac, so the save must fail instead.
  std::vector<double> coords
  {
    0,  0, 0,    1, 0, 0,    0,  1, 0,
    -1, 0, 0,    0, -1, 0,
  };
  std::vector<int> faceIndices
  {
    0, 1, 2,
    0, 3, 4,
  };

  expectSaveRejected( coords, faceIndices, "/ipobo_bowtie.slf" );
}

TEST( MeshSLFTest, IPOBONonManifoldEdge )
{
  // Three triangles sharing the edge 0-1. That edge is used by more than two
  // faces, so it is neither a boundary edge nor a regular interior one: the
  // boundary it belongs to cannot be traced and the save must fail rather than
  // silently number a wrong boundary.
  std::vector<double> coords
  {
    0, 0, 0,      1, 0, 0,
    0.5, 1, 0,    0.5, -1, 0,    1.5, 0.5, 0,
  };
  std::vector<int> faceIndices
  {
    0, 1, 2,
    0, 1, 3,
    0, 1, 4,
  };

  expectSaveRejected( coords, faceIndices, "/ipobo_nonmanifold.slf" );
}

TEST( MeshSLFTest, IPOBONonTriangular )
{
  // SELAFIN is triangles-only: a quad mesh must be rejected up front by
  // MDAL_SaveMesh (Err_IncompatibleMesh) rather than silently producing a file.
  // (computeIPOBO keeps an internal verticesPerFace != 3 guard as defence in
  // depth, but the public save path never reaches it.)
  MDAL_DriverH driver = MDAL_driverFromName( "2DM" );
  MDAL_MeshH mesh = MDAL_CreateMesh( driver );
  std::vector<double> coords{ 0, 0, 0,   1, 0, 0,   1, 1, 0,   0, 1, 0 };
  MDAL_M_addVertices( mesh, 4, coords.data() );
  std::vector<int> faceSizes{ 4 };
  std::vector<int> faceIndices{ 0, 1, 2, 3 };
  MDAL_M_addFaces( mesh, 1, faceSizes.data(), faceIndices.data() );
  std::string savedFile = tmp_file( "/ipobo_quad.slf" );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  EXPECT_EQ( MDAL_Status::Err_IncompatibleMesh, MDAL_LastStatus() )
      << "A non-triangular mesh must be rejected by the SELAFIN driver";
  MDAL_CloseMesh( mesh );
}

TEST( MeshSLFTest, IPOBOManyIslands )
{
  // A 70x70 lattice of nodes with a 2x2-cell hole punched every 5 cells: one
  // outer contour and 196 islands, each an 8-node ring around a centre node
  // left out of every triangle. Contour tracing and contour nesting both have
  // to visit every contour, so this is where a quadratic implementation shows
  // up; what is checked here is the numbering itself, not the time it takes.
  const int side = 70;             // nodes per row
  const int holeSize = 2;          // cells
  const int period = 5;            // cells
  const int islandRingNodes = 8;
  const int outerRingNodes = 4 * ( side - 1 );

  std::vector<double> coords;
  coords.reserve( static_cast<size_t>( side ) * side * 3 );
  for ( int i = 0; i < side; ++i )
    for ( int j = 0; j < side; ++j )
    {
      coords.push_back( i );
      coords.push_back( j );
      coords.push_back( 0 );
    }
  auto nodeAt = [side]( int i, int j ) { return i * side + j; };

  std::vector<std::pair<int, int>> holes;  // lower-left cell of each hole
  for ( int i = 1; i + holeSize <= side - 2; i += period )
    for ( int j = 1; j + holeSize <= side - 2; j += period )
      holes.push_back( std::make_pair( i, j ) );

  std::vector<bool> removed( static_cast<size_t>( side - 1 ) * ( side - 1 ), false );
  for ( const auto &hole : holes )
    for ( int a = hole.first; a < hole.first + holeSize; ++a )
      for ( int b = hole.second; b < hole.second + holeSize; ++b )
        removed[static_cast<size_t>( a ) * ( side - 1 ) + b] = true;

  std::vector<int> faceIndices;
  for ( int i = 0; i + 1 < side; ++i )
    for ( int j = 0; j + 1 < side; ++j )
    {
      if ( removed[static_cast<size_t>( i ) * ( side - 1 ) + j] )
        continue;
      const int n00 = nodeAt( i, j );
      const int n10 = nodeAt( i + 1, j );
      const int n01 = nodeAt( i, j + 1 );
      const int n11 = nodeAt( i + 1, j + 1 );
      faceIndices.insert( faceIndices.end(), { n00, n10, n11 } );
      faceIndices.insert( faceIndices.end(), { n00, n11, n01 } );
    }

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_many_islands.slf" );
  ASSERT_EQ( ipobo.size(), static_cast<size_t>( side ) * side );

  // consecutive 1..NPTFR, no gap and no duplicate
  std::vector<int> numbering;
  for ( int value : ipobo )
    if ( value > 0 )
      numbering.push_back( value );
  std::sort( numbering.begin(), numbering.end() );
  ASSERT_EQ( numbering.size(),
             static_cast<size_t>( outerRingNodes + islandRingNodes * static_cast<int>( holes.size() ) ) );
  for ( size_t i = 0; i < numbering.size(); ++i )
    ASSERT_EQ( numbering[i], static_cast<int>( i + 1 ) ) << "IPOBO numbering is not consecutive";

  // the outer contour comes first, starting from the south-west corner
  EXPECT_EQ( 1, ipobo[nodeAt( 0, 0 )] );
  for ( int i = 0; i < side; ++i )
    for ( int j = 0; j < side; ++j )
    {
      const bool onBorder = i == 0 || j == 0 || i == side - 1 || j == side - 1;
      if ( onBorder )
      {
        EXPECT_GT( ipobo[nodeAt( i, j )], 0 ) << "border node " << i << "," << j << " is not numbered";
        EXPECT_LE( ipobo[nodeAt( i, j )], outerRingNodes ) << "border node " << i << "," << j
            << " is not part of the outer contour";
      }
    }

  // every island owns a consecutive block of 8 numbers, after the outer contour
  for ( const auto &hole : holes )
  {
    EXPECT_EQ( 0, ipobo[nodeAt( hole.first + 1, hole.second + 1 )] )
        << "the node left inside a hole must not be numbered";

    std::vector<int> ring;
    for ( int a = hole.first; a <= hole.first + holeSize; ++a )
      for ( int b = hole.second; b <= hole.second + holeSize; ++b )
        if ( a != hole.first + 1 || b != hole.second + 1 )
          ring.push_back( ipobo[nodeAt( a, b )] );
    ASSERT_EQ( ring.size(), static_cast<size_t>( islandRingNodes ) );
    std::sort( ring.begin(), ring.end() );
    EXPECT_GT( ring[0], outerRingNodes ) << "islands must be numbered after the outer contour";
    for ( size_t k = 0; k < ring.size(); ++k )
      EXPECT_EQ( ring[k], ring[0] + static_cast<int>( k ) )
          << "island at cell " << hole.first << "," << hole.second << " is not numbered consecutively";
  }
}

TEST( MeshSLFTest, IPOBOMatchesTelemacFiles )
{
  // The numbering convention is locked against real files, written by four
  // different tools. Each mesh is rebuilt as a MemoryMesh through the public
  // API, which is what QGIS hands to the driver, so the save takes the compute
  // path; the recomputed IPOBO must then be the array stored in the source
  // file, node for node.
  const std::vector<std::string> sources
  {
    "/slf/test_sd_6.slf",                  // Janet, little-endian
    "/slf/test_sd_7.slf",                  // Telemac-2D v7p2r0, IPARAM(8) = NPTFR
    "/slf/example_res_fr.slf",             // Malpasset reference
    "/slf/geo_Fudaa_doublePrecision.geo",  // Fudaa-Prepro
  };

  for ( const std::string &source : sources )
  {
    const std::string sourceFile = test_file( source );
    const std::vector<int> storedIpobo = readIpoboFromFile( sourceFile );
    ASSERT_FALSE( storedIpobo.empty() ) << source;

    MDAL_MeshH src = MDAL_LoadMesh( sourceFile.c_str() );
    ASSERT_NE( src, nullptr ) << source;
    const int nVerts = MDAL_M_vertexCount( src );
    const int nFaces = MDAL_M_faceCount( src );
    std::vector<double> coords = getCoordinates( src, nVerts );
    std::vector<int> faceIndices = faceVertexIndices( src, nFaces );  // 3 per face
    MDAL_CloseMesh( src );

    const std::vector<int> ipobo =
      saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_convention.slf" );
    EXPECT_EQ( storedIpobo, ipobo ) << "the recomputed IPOBO differs from the one stored in " << source;
  }
}

TEST( MeshSLFTest, IPOBOLargeMeshBoundarySet )
{
  // Rebuild a real TELEMAC mesh (Malpasset, 13541 nodes) as a MemoryMesh so
  // save() takes the compute path, then verify computeIPOBO marks EXACTLY the
  // same boundary node set as the file shipped by TELEMAC, numbered
  // consecutively 1..N. example.slf does not store a real numbering, so only
  // the boundary SET and its consecutiveness can be checked here;
  // IPOBOMatchesTelemacFiles above compares the numbering itself against the
  // files that do store one.
  std::string sourceFile = test_file( "/slf/example.slf" );
  std::vector<int> storedIpobo = readIpoboFromFile( sourceFile );
  ASSERT_FALSE( storedIpobo.empty() );

  MDAL_MeshH src = MDAL_LoadMesh( sourceFile.c_str() );
  ASSERT_NE( src, nullptr );
  const int nVerts = MDAL_M_vertexCount( src );
  const int nFaces = MDAL_M_faceCount( src );
  std::vector<double> coords = getCoordinates( src, nVerts );
  std::vector<int> faceIndices = faceVertexIndices( src, nFaces );  // 3 per face
  MDAL_CloseMesh( src );

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_large.slf" );
  ASSERT_EQ( ipobo.size(), storedIpobo.size() );

  // Same boundary node set.
  int nBoundary = 0;
  for ( size_t i = 0; i < ipobo.size(); ++i )
  {
    EXPECT_EQ( ipobo[i] > 0, storedIpobo[i] > 0 )
        << "Boundary classification differs at node " << i;
    if ( ipobo[i] > 0 )
      ++nBoundary;
  }
  EXPECT_GT( nBoundary, 0 );

  // Consecutive 1..nBoundary, no gaps or duplicates.
  std::vector<int> vals;
  for ( int v : ipobo )
    if ( v > 0 ) vals.push_back( v );
  std::sort( vals.begin(), vals.end() );
  for ( size_t i = 0; i < vals.size(); ++i )
    EXPECT_EQ( vals[i], static_cast<int>( i + 1 ) ) << "IPOBO numbering is not consecutive";
}

// Byte offsets inside a frame-only SELAFIN file written by MDAL itself
// (layout is deterministic): 80-char title record, NBV record (2 ints),
// IPARAM record (10 ints), NELEM record (4 ints), connectivity record,
// IPOBO record. Each record is framed by two 4-byte lengths.
static std::streamoff mdalWrittenIpoboPayloadOffset( int nFaces )
{
  return ( 4 + 80 + 4 ) + ( 4 + 8 + 4 ) + ( 4 + 40 + 4 ) + ( 4 + 16 + 4 )
         + ( 4 + nFaces * 3 * 4 + 4 ) + 4;
}

static const std::streamoff sMdalWrittenIparamPayloadOffset = ( 4 + 80 + 4 ) + ( 4 + 8 + 4 ) + 4;

// SELAFIN files are big-endian on disk
static void patchBigEndianInts( const std::string &fileName, std::streamoff pos, const std::vector<int> &values )
{
  std::fstream f( fileName, std::ios::in | std::ios::out | std::ios::binary );
  ASSERT_TRUE( f.is_open() );
  f.seekp( pos );
  for ( int value : values )
  {
    unsigned char b[4];
    b[0] = static_cast<unsigned char>( ( value >> 24 ) & 0xff );
    b[1] = static_cast<unsigned char>( ( value >> 16 ) & 0xff );
    b[2] = static_cast<unsigned char>( ( value >> 8 ) & 0xff );
    b[3] = static_cast<unsigned char>( value & 0xff );
    f.write( reinterpret_cast<char *>( b ), 4 );
  }
}

// The 3x3 grid of IPOBOComputation, saved by MDAL to tmpName.
static void saveReferenceGrid( const std::string &savedFile )
{
  std::vector<double> coords
  {
    0, 0, 0,   1, 0, 0,   2, 0, 0,
    0, 1, 0,   1, 1, 0,   2, 1, 0,
    0, 2, 0,   1, 2, 0,   2, 2, 0,
  };
  std::vector<int> faceIndices
  {
    0, 1, 4,   0, 4, 3,
    1, 2, 5,   1, 5, 4,
    3, 4, 7,   3, 7, 6,
    4, 5, 8,   4, 8, 7,
  };
  MDAL_DriverH driver = MDAL_driverFromName( "2DM" );
  MDAL_MeshH mesh = MDAL_CreateMesh( driver );
  MDAL_M_addVertices( mesh, 9, coords.data() );
  std::vector<int> faceSizes( 8, 3 );
  MDAL_M_addFaces( mesh, 8, faceSizes.data(), faceIndices.data() );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );
}

TEST( MeshSLFTest, IPOBOAllZeroStoredIsRecomputed )
{
  // Files written by older MDAL versions store an all-zero IPOBO. Re-saving
  // such a MeshSelafin must NOT faithfully copy the zeros: the cached path is
  // rejected and the array is recomputed.
  std::string file = tmp_file( "/ipobo_zeros_src.slf" );
  saveReferenceGrid( file );
  patchBigEndianInts( file, mdalWrittenIpoboPayloadOffset( 8 ), std::vector<int>( 9, 0 ) );

  MDAL_MeshH mesh = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( mesh, nullptr );
  std::string savedFile = tmp_file( "/ipobo_zeros_dst.slf" );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );

  const std::vector<int> expected{ 1, 2, 3, 8, 0, 4, 7, 6, 5 };
  EXPECT_EQ( readIpoboFromFile( savedFile ), expected )
      << "An all-zero stored IPOBO must be recomputed, not copied";
}

TEST( MeshSLFTest, IPOBOPartitionedFileNotReused )
{
  // On a partitioned SELAFIN file (IPARAM(9) = NPTIR != 0) the record in the
  // IPOBO slot holds KNOLG. Re-saving must ignore that record and recompute a
  // real IPOBO for the (serial) output file.
  std::string file = tmp_file( "/ipobo_knolg_src.slf" );
  saveReferenceGrid( file );
  // IPARAM(9) = 1 and a bogus KNOLG-like payload that must not be copied
  patchBigEndianInts( file, sMdalWrittenIparamPayloadOffset + 8 * 4, { 1 } );
  patchBigEndianInts( file, mdalWrittenIpoboPayloadOffset( 8 ), std::vector<int>( 9, 7 ) );

  MDAL_MeshH mesh = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( mesh, nullptr );
  std::string savedFile = tmp_file( "/ipobo_knolg_dst.slf" );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );

  const std::vector<int> expected{ 1, 2, 3, 8, 0, 4, 7, 6, 5 };
  EXPECT_EQ( readIpoboFromFile( savedFile ), expected )
      << "A partitioned file's KNOLG record must not be reused as IPOBO";
}

// Loads \a sourceFile as a MeshSelafin and saves it again as SELAFIN, which
// takes the cached path when the stored IPOBO is a genuine boundary numbering.
static std::vector<int> resaveSelafinAndReadIpobo( const std::string &sourceFile,
    const std::string &tmpName )
{
  MDAL_MeshH mesh = MDAL_LoadMesh( sourceFile.c_str() );
  EXPECT_NE( mesh, nullptr );
  if ( !mesh )
    return std::vector<int>();
  std::string savedFile = tmp_file( tmpName );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  EXPECT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );
  return readIpoboFromFile( savedFile );
}

TEST( MeshSLFTest, IPOBORoundTrip )
{
  // Round-tripping a MeshSelafin reuses the IPOBO stored in the source file
  // (no recompute), so the saved file must have exactly the same array.
  std::string sourceFile = test_file( "/slf/example_res_fr.slf" );

  std::vector<int> sourceIpobo = readIpoboFromFile( sourceFile );
  ASSERT_FALSE( sourceIpobo.empty() );

  EXPECT_EQ( sourceIpobo, resaveSelafinAndReadIpobo( sourceFile, "/ipobo_roundtrip.slf" ) )
      << "Round-trip did not preserve IPOBO";
}

TEST( MeshSLFTest, IPOBOSerialTelemacFileReused )
{
  // A serial Telemac v7 result sets IPARAM(8) = NPTFR next to a real IPOBO,
  // which the manual describes as the partitioned case. Its numbering must be
  // reused as it is, not thrown away.
  std::string sourceFile = test_file( "/slf/test_sd_7.slf" );

  std::vector<int> sourceIpobo = readIpoboFromFile( sourceFile );
  ASSERT_FALSE( sourceIpobo.empty() );

  EXPECT_EQ( sourceIpobo, resaveSelafinAndReadIpobo( sourceFile, "/ipobo_serial_telemac.slf" ) )
      << "The IPOBO of a serial Telemac file must be preserved";
}

TEST( MeshSLFTest, IPOBOInvalidStoredIsRecomputed )
{
  // example.slf numbers its boundary nodes with their own node index (up to
  // 12452 for 1080 boundary nodes), which is not a valid boundary numbering:
  // it must be recomputed instead of being copied. The result is checked
  // against the array stored by example_res_fr.slf, the same mesh with a
  // proper IPOBO.
  std::vector<int> brokenIpobo = readIpoboFromFile( test_file( "/slf/example.slf" ) );
  ASSERT_FALSE( brokenIpobo.empty() );
  std::vector<int> referenceIpobo = readIpoboFromFile( test_file( "/slf/example_res_fr.slf" ) );
  ASSERT_FALSE( referenceIpobo.empty() );
  ASSERT_NE( brokenIpobo, referenceIpobo );

  EXPECT_EQ( referenceIpobo, resaveSelafinAndReadIpobo( test_file( "/slf/example.slf" ),
             "/ipobo_invalid_stored.slf" ) )
      << "An invalid stored IPOBO must be recomputed, not copied";
}

TEST( MeshSLFTest, TruncatedFileUnderOpenHandle )
{
  // A Selafin mesh reads its frame and its values lazily, so a file rewritten
  // or truncated by another program while MDAL holds it open fails in the
  // middle of a read. Those reads happen under the C API, which has no
  // exception handling of its own: they must report the failure and return
  // nothing instead of terminating the calling process.
  std::string file = tmp_file( "/selafin_truncated_under_handle.slf" );
  copy( test_file( "/slf/example_res_fr.slf" ), file );

  MDAL_MeshH mesh = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( mesh, nullptr );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_DatasetGroupH group = MDAL_M_datasetGroup( mesh, 0 );
  ASSERT_NE( group, nullptr );
  const int datasetCount = MDAL_G_datasetCount( group );
  ASSERT_GT( datasetCount, 0 );
  MDAL_DatasetH dataset = MDAL_G_dataset( group, datasetCount - 1 );
  ASSERT_NE( dataset, nullptr );

  // truncate the file while the mesh handle is open
  {
    std::ofstream truncated( file, std::ios::binary | std::ios::trunc );
    ASSERT_TRUE( truncated.is_open() );
  }

  // the frame has been parsed already, so the counts stay available
  EXPECT_GT( MDAL_M_vertexCount( mesh ), 0 );

  MDAL_ResetStatus();
  std::vector<double> coordinates( 3 * 10 );
  MDAL_MeshVertexIteratorH vertexIterator = MDAL_M_vertexIterator( mesh );
  EXPECT_EQ( 0, MDAL_VI_next( vertexIterator, 10, coordinates.data() ) );
  EXPECT_NE( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_VI_close( vertexIterator );

  MDAL_ResetStatus();
  std::vector<int> faceOffsets( 10 );
  std::vector<int> vertexIndices( 30 );
  MDAL_MeshFaceIteratorH faceIterator = MDAL_M_faceIterator( mesh );
  EXPECT_EQ( 0, MDAL_FI_next( faceIterator, 10, faceOffsets.data(), 30, vertexIndices.data() ) );
  EXPECT_NE( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_FI_close( faceIterator );

  MDAL_ResetStatus();
  std::vector<double> values( 2 * 10 );
  const MDAL_DataType dataType = MDAL_G_hasScalarData( group ) ?
                                 MDAL_DataType::SCALAR_DOUBLE : MDAL_DataType::VECTOR_2D_DOUBLE;
  EXPECT_EQ( 0, MDAL_D_data( dataset, 0, 10, dataType, values.data() ) );
  EXPECT_NE( MDAL_Status::None, MDAL_LastStatus() );

  MDAL_CloseMesh( mesh );
}

TEST( MeshSLFTest, SaveMeshOntoItsOwnFile )
{
  // What QGIS does when an edited mesh layer is saved: the mesh is written to
  // the very file the layer is still reading. The save must succeed, the file
  // must remain a valid mesh, and the handle that is still open must keep
  // working - it now reads the file that has just been written, which holds
  // the frame but no dataset any more.
  std::string file = tmp_file( "/selafin_save_onto_itself.slf" );
  copy( test_file( "/slf/example_res_fr.slf" ), file );

  MDAL_MeshH mesh = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( mesh, nullptr );
  const int vertexCount = MDAL_M_vertexCount( mesh );
  const int faceCount = MDAL_M_faceCount( mesh );
  ASSERT_GT( vertexCount, 0 );
  ASSERT_GT( faceCount, 0 );
  MDAL_DatasetGroupH group = MDAL_M_datasetGroup( mesh, 0 );
  ASSERT_NE( group, nullptr );
  MDAL_DatasetH dataset = MDAL_G_dataset( group, 0 );
  ASSERT_NE( dataset, nullptr );

  MDAL_SaveMesh( mesh, file.c_str(), "SELAFIN" );
  EXPECT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  EXPECT_FALSE( fileIsPresent( file + ".tmp" ) ) << "no temporary file must be left behind";

  // the frame is still served on the open handle
  EXPECT_EQ( vertexCount, MDAL_M_vertexCount( mesh ) );
  EXPECT_EQ( faceCount, MDAL_M_faceCount( mesh ) );

  // the datasets are not in the file any more: reading them reports an error
  // instead of terminating the process
  MDAL_ResetStatus();
  std::vector<double> values( 2 * 10 );
  const MDAL_DataType dataType = MDAL_G_hasScalarData( group ) ?
                                 MDAL_DataType::SCALAR_DOUBLE : MDAL_DataType::VECTOR_2D_DOUBLE;
  EXPECT_EQ( 0, MDAL_D_data( dataset, 0, 10, dataType, values.data() ) );
  EXPECT_NE( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );

  // and the file on disk is a valid frame-only mesh
  MDAL_MeshH reloaded = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( reloaded, nullptr );
  EXPECT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  EXPECT_EQ( vertexCount, MDAL_M_vertexCount( reloaded ) );
  EXPECT_EQ( faceCount, MDAL_M_faceCount( reloaded ) );
  EXPECT_EQ( 0, MDAL_M_datasetGroupCount( reloaded ) );
  MDAL_CloseMesh( reloaded );
}

static MDAL_DatasetGroupH addNewScalarDatasetGroup( MDAL_MeshH mesh, MDAL_DriverH driver, std::string file )
{
  MDAL_DatasetGroupH newScalarGroup = MDAL_M_addDatasetGroup( mesh, "New Scalar Dataset Group", DataOnVertices, true, driver, file.c_str() );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
  size_t v_count = MDAL_M_vertexCount( mesh );
  std::vector<double> scalarValue1( v_count );
  std::vector<double> scalarValue2( v_count );
  for ( size_t i = 0; i < v_count; i++ )
  {
    scalarValue1[i] = ( i % 15 ) / 3.0;
    scalarValue2[i] = ( i % 30 ) / 3.0;
  }
  MDAL_G_addDataset( newScalarGroup, 0, scalarValue1.data(), nullptr );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
  MDAL_G_addDataset( newScalarGroup, 1.111111111111111111, scalarValue2.data(), nullptr );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
  MDAL_G_closeEditMode( newScalarGroup );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );

  return newScalarGroup;
}

static MDAL_DatasetGroupH addNewVectorDatasetGroup( MDAL_MeshH mesh, MDAL_DriverH driver, std::string file )
{
  MDAL_DatasetGroupH newVectorGroup = MDAL_M_addDatasetGroup( mesh, "New Vector Dataset Group", DataOnVertices, false, driver, file.c_str() );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
  size_t v_count = MDAL_M_vertexCount( mesh );
  std::vector<double> vectorValue1( v_count * 2 );
  std::vector<double> vectorValue2( v_count * 2 );
  for ( size_t i = 0; i < v_count * 2; i++ )
  {
    vectorValue1[i] = ( i % 10 ) / 3.0;
    vectorValue2[i] = ( i % 20 ) / 3.0;
  }
  MDAL_G_addDataset( newVectorGroup, 0, vectorValue1.data(), nullptr );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
  MDAL_G_addDataset( newVectorGroup, 1.111111111, vectorValue2.data(), nullptr );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
  MDAL_G_closeEditMode( newVectorGroup );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );

  return newVectorGroup;
}

static void testScalarDatasetGroupAdded( MDAL_DatasetGroupH r )
{
  ASSERT_NE( r, nullptr );

  double scalar = MDAL_G_hasScalarData( r );
  EXPECT_EQ( true, scalar );

  MDAL_DatasetH ds = MDAL_G_dataset( r, 1 );
  ASSERT_NE( ds, nullptr );

  double time = MDAL_D_time( ds );
  EXPECT_TRUE( compareDurationInHours( 1.111111111, time ) );

  size_t count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 13541, count );

  double value = getValue( ds, 8667 );
  EXPECT_DOUBLE_EQ( 9, value );
}

static void testVectorDatasetGroupAdded( MDAL_DatasetGroupH r )
{
  ASSERT_NE( r, nullptr );

  double scalar = MDAL_G_hasScalarData( r );
  EXPECT_EQ( false, scalar );

  MDAL_DatasetH ds = MDAL_G_dataset( r, 1 );
  ASSERT_NE( ds, nullptr );

  double time = MDAL_D_time( ds );
  EXPECT_TRUE( compareDurationInHours( 1.111111111, time ) );

  size_t count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 13541, count );

  double value = getValueX( ds, 8667 );
  EXPECT_TRUE( MDAL::equals( 4.66666, value, 0.0001 ) );

}

TEST( MeshSLFTest, WriteDatasetInExistingFile )
{
  std::string path = test_file( "/slf/example_res_fr.slf" );

  MDAL_DriverH driver = MDAL_driverFromName( "SELAFIN" );
  ASSERT_NE( driver, nullptr );

  //Add dataset
  std::string file = tmp_file( "/selafin_adding_dataset_existing.slf" );
  copy( path, file );

  MDAL_MeshH meshAdded = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( meshAdded, nullptr );

  addNewScalarDatasetGroup( meshAdded, driver, file );
  addNewVectorDatasetGroup( meshAdded, driver, file );
  MDAL_CloseMesh( meshAdded );

  meshAdded = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( meshAdded, nullptr );

  EXPECT_EQ( 6, MDAL_M_datasetGroupCount( meshAdded ) );

  testPreExistingScalarDatasetGroup( MDAL_M_datasetGroup( meshAdded, 2 ) );
  testPreExisitingVectorDatasetGroup( MDAL_M_datasetGroup( meshAdded, 0 ) );

  // Scalar dataset group added
  testScalarDatasetGroupAdded( MDAL_M_datasetGroup( meshAdded, 4 ) );

  // Vector dataset group added
  testVectorDatasetGroupAdded( MDAL_M_datasetGroup( meshAdded, 5 ) );

  MDAL_CloseMesh( meshAdded );
}

TEST( MeshSLFTest, WriteDatasetInNewFile )
{
  std::string path = test_file( "/slf/example_res_fr.slf" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );
  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );

  MDAL_DriverH driver = MDAL_driverFromName( "SELAFIN" );
  ASSERT_NE( driver, nullptr );

  //Add dataset
  std::string file = tmp_file( "/selafin_adding_dataset_newFile.slf" ) ;
  deleteFile( file );

  addNewScalarDatasetGroup( m, driver, file );
  addNewVectorDatasetGroup( m, driver, file );
  MDAL_CloseMesh( m );

  MDAL_MeshH newMesh = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( newMesh, nullptr );

  EXPECT_EQ( 2, MDAL_M_datasetGroupCount( newMesh ) );

  // Scalar dataset group added
  testScalarDatasetGroupAdded( MDAL_M_datasetGroup( newMesh, 0 ) );

  // Vector dataset group added
  testVectorDatasetGroupAdded( MDAL_M_datasetGroup( newMesh, 1 ) );

  MDAL_CloseMesh( newMesh );
}

TEST( MeshSLFTest, WriteDatasetSpecialCharacters )
{
  std::string path = test_file( "/slf/example_res_fr.slf" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );
  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );

  MDAL_DriverH driver = MDAL_driverFromName( "SELAFIN" );
  ASSERT_NE( driver, nullptr );

  //Add dataset
#ifdef _MSC_VER
  std::wstring wFileName = std::wstring( L"/selafin_\u00E4\u00F6\u00FC\u00DF.slf" );
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::string fileName = converter.to_bytes( wFileName );
#else
  std::string fileName = "/selafin_äöüß.slf";
#endif
  std::string file = tmp_file( fileName );
  deleteFile( file );

  addNewScalarDatasetGroup( m, driver, file );
  addNewVectorDatasetGroup( m, driver, file );
  MDAL_CloseMesh( m );

  MDAL_MeshH newMesh = MDAL_LoadMesh( file.c_str() );
  ASSERT_NE( newMesh, nullptr );

  EXPECT_EQ( 2, MDAL_M_datasetGroupCount( newMesh ) );

  // Scalar dataset group added
  testScalarDatasetGroupAdded( MDAL_M_datasetGroup( newMesh, 0 ) );

  // Vector dataset group added
  testVectorDatasetGroupAdded( MDAL_M_datasetGroup( newMesh, 1 ) );

  MDAL_CloseMesh( newMesh );
}

TEST( MeshSLFTest, loadDatasetFromFile )
{
  std::string path = test_file( "/slf/example.slf" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );
  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );

  EXPECT_EQ( 1, MDAL_M_datasetGroupCount( m ) );

  std::string datasetFile = test_file( "/slf/example_res_fr.slf" );
  MDAL_M_LoadDatasets( m, datasetFile.c_str() );


  EXPECT_EQ( 5, MDAL_M_datasetGroupCount( m ) );

  testPreExistingScalarDatasetGroup( MDAL_M_datasetGroup( m, 3 ) );
  testPreExisitingVectorDatasetGroup( MDAL_M_datasetGroup( m, 1 ) );

  MDAL_CloseMesh( m );
}

TEST( MeshSLFTest, DoublePrecision )
{
  std::string path = test_file( "/slf/test_sd_7.slf" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );

  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  MDAL_Status s = MDAL_LastStatus();
  EXPECT_EQ( MDAL_Status::None, s );

  const char *projection = MDAL_M_projection( m );
  EXPECT_EQ( std::string( "" ), std::string( projection ) );

  std::string driverName = MDAL_M_driverName( m );
  EXPECT_EQ( driverName, "SELAFIN" );

  // ///////////
  // Vertices
  // ///////////
  int v_count = MDAL_M_vertexCount( m );
  EXPECT_EQ( v_count, 17830 );
  double x = getVertexXCoordinatesAt( m, 0 );
  double y = getVertexYCoordinatesAt( m, 0 );
  double z = getVertexZCoordinatesAt( m, 0 );
  EXPECT_DOUBLE_EQ( 440745.06147386681, x );
  EXPECT_DOUBLE_EQ( 5420249.8978509316, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  x = getVertexXCoordinatesAt( m, 1000 );
  y = getVertexYCoordinatesAt( m, 1000 );
  z = getVertexZCoordinatesAt( m, 1000 );
  EXPECT_DOUBLE_EQ( 440750.06147266628, x );
  EXPECT_DOUBLE_EQ( 5420258.4996587345, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  // ///////////
  // Faces
  // ///////////
  int f_count = MDAL_M_faceCount( m );
  EXPECT_EQ( 35093, f_count );

  // ///////////
  // Edges
  // ///////////
  EXPECT_EQ( 0, MDAL_M_edgeCount( m ) );

  // ///////////
  // Extent
  // ///////////
  double xmin, xmax, ymin, ymax;
  MDAL_M_extent( m, &xmin, &xmax, &ymin, &ymax );
  EXPECT_EQ( xmin, 440745.0614738668 );
  EXPECT_EQ( xmax, 440755.0614738668 );
  EXPECT_EQ( ymin, 5420249.897850932 );
  EXPECT_EQ( ymax, 5420349.908870826 );

  // test face 1
  int f_v_count = getFaceVerticesCountAt( m, 1 );
  EXPECT_EQ( 3, f_v_count ); //only triangles!
  int f_v = getFaceVerticesIndexAt( m, 100, 0 );
  EXPECT_EQ( 2133, f_v );
  f_v = getFaceVerticesIndexAt( m, 100, 1 );
  EXPECT_EQ( 2011, f_v ); \
  f_v = getFaceVerticesIndexAt( m, 100, 2 );
  EXPECT_EQ( 2012, f_v );

  // Datasets
  ASSERT_EQ( 9, MDAL_M_datasetGroupCount( m ) );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, 0 );
  ASSERT_NE( g, nullptr );

  EXPECT_TRUE( compareReferenceTime( g, "1900-01-01T00:00:00" ) );

  int meta_count = MDAL_G_metadataCount( g );
  ASSERT_EQ( 1, meta_count );

  const char *name = MDAL_G_name( g );
  EXPECT_EQ( std::string( "velocity      ms" ), std::string( name ) );

  bool scalar = MDAL_G_hasScalarData( g );
  EXPECT_EQ( false, scalar );

  MDAL_DataLocation dataLocation = MDAL_G_dataLocation( g );
  EXPECT_EQ( dataLocation, MDAL_DataLocation::DataOnVertices );

  ASSERT_EQ( 11, MDAL_G_datasetCount( g ) );
  MDAL_DatasetH ds = MDAL_G_dataset( g, 5 );
  ASSERT_NE( ds, nullptr );

  bool valid = MDAL_D_isValid( ds );
  EXPECT_EQ( true, valid );

  int count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 17830, count );

  double valueX = getValueX( ds, 0 );
  double valueY = getValueY( ds, 0 );
  EXPECT_DOUBLE_EQ( 0.0, valueX );
  EXPECT_DOUBLE_EQ( 0.027486738969071053, valueY );
  valueY = getValueY( ds, 20 );
  EXPECT_DOUBLE_EQ( 0.33878578833223305, valueY );
  valueY = getValueY( ds, 1000 );
  EXPECT_DOUBLE_EQ( 0.37488353797245938, valueY );
  valueY = getValueY( ds, 10000 );
  EXPECT_DOUBLE_EQ( -4.4024387562236051e-35, valueY );

  MDAL_CloseMesh( m );
}

TEST( MeshSLFTest, JanetFile )
{
  std::string path = test_file( "/slf/test_sd_6.slf" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );

  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  MDAL_Status s = MDAL_LastStatus();
  EXPECT_EQ( MDAL_Status::None, s );

  const char *projection = MDAL_M_projection( m );
  EXPECT_EQ( std::string( "" ), std::string( projection ) );

  std::string driverName = MDAL_M_driverName( m );
  EXPECT_EQ( driverName, "SELAFIN" );

  // ///////////
  // Vertices
  // ///////////
  int v_count = MDAL_M_vertexCount( m );
  EXPECT_EQ( v_count, 17830 );
  double x = getVertexXCoordinatesAt( m, 0 );
  double y = getVertexYCoordinatesAt( m, 0 );
  double z = getVertexZCoordinatesAt( m, 0 );
  EXPECT_DOUBLE_EQ( 440745.06147386681, x );
  EXPECT_DOUBLE_EQ( 5420249.8978509316, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  x = getVertexXCoordinatesAt( m, 1000 );
  y = getVertexYCoordinatesAt( m, 1000 );
  z = getVertexZCoordinatesAt( m, 1000 );
  EXPECT_DOUBLE_EQ( 440750.06147266628, x );
  EXPECT_DOUBLE_EQ( 5420258.4996587345, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  // ///////////
  // Faces
  // ///////////
  int f_count = MDAL_M_faceCount( m );
  EXPECT_EQ( 35093, f_count );

  // ///////////
  // Edges
  // ///////////
  EXPECT_EQ( 0, MDAL_M_edgeCount( m ) );

  // ///////////
  // Extent
  // ///////////
  double xmin, xmax, ymin, ymax;
  MDAL_M_extent( m, &xmin, &xmax, &ymin, &ymax );
  EXPECT_EQ( xmin, 440745.0614738668 );
  EXPECT_EQ( xmax, 440755.0614738668 );
  EXPECT_EQ( ymin, 5420249.897850932 );
  EXPECT_EQ( ymax, 5420349.908870826 );

  // test face 1
  int f_v_count = getFaceVerticesCountAt( m, 1 );
  EXPECT_EQ( 3, f_v_count ); //only triangles!
  int f_v = getFaceVerticesIndexAt( m, 100, 0 );
  EXPECT_EQ( 2133, f_v );
  f_v = getFaceVerticesIndexAt( m, 100, 1 );
  EXPECT_EQ( 2011, f_v ); \
  f_v = getFaceVerticesIndexAt( m, 100, 2 );
  EXPECT_EQ( 2012, f_v );

  // Datasets
  ASSERT_EQ( 2, MDAL_M_datasetGroupCount( m ) );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, 0 );
  ASSERT_NE( g, nullptr );

  EXPECT_TRUE( compareReferenceTime( g, "" ) );

  int meta_count = MDAL_G_metadataCount( g );
  ASSERT_EQ( 1, meta_count );

  const char *name = MDAL_G_name( g );
  EXPECT_EQ( std::string( "bottom          m" ), std::string( name ) );

  bool scalar = MDAL_G_hasScalarData( g );
  EXPECT_EQ( true, scalar );

  MDAL_DataLocation dataLocation = MDAL_G_dataLocation( g );
  EXPECT_EQ( dataLocation, MDAL_DataLocation::DataOnVertices );

  ASSERT_EQ( 1, MDAL_G_datasetCount( g ) );
  MDAL_DatasetH ds = MDAL_G_dataset( g, 0 );
  ASSERT_NE( ds, nullptr );

  bool valid = MDAL_D_isValid( ds );
  EXPECT_EQ( true, valid );

  int count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 17830, count );

  double value = getValue( ds, 0 );
  EXPECT_TRUE( MDAL::equals( 101.1, value ) );
  value = getValue( ds, 20 );
  EXPECT_TRUE( MDAL::equals( 99.1, value ) );
  value = getValue( ds, 1000 );
  EXPECT_TRUE( MDAL::equals( 99.09139914, value ) );
  value = getValue( ds, 10000 );
  EXPECT_TRUE( MDAL::equals( 100.50871584346136, value ) );

  MDAL_CloseMesh( m );
}

TEST( MeshSLFTest, FudaaFileDoublePrecision )
{
  std::string path = test_file( "/slf/geo_Fudaa_doublePrecision.geo" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );

  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  MDAL_Status s = MDAL_LastStatus();
  EXPECT_EQ( MDAL_Status::None, s );

  const char *projection = MDAL_M_projection( m );
  EXPECT_EQ( std::string( "" ), std::string( projection ) );

  std::string driverName = MDAL_M_driverName( m );
  EXPECT_EQ( driverName, "SELAFIN" );

  int v_count = MDAL_M_vertexCount( m );
  EXPECT_EQ( v_count, 8215 );
  double x = getVertexXCoordinatesAt( m, 0 );
  double y = getVertexYCoordinatesAt( m, 0 );
  double z = getVertexZCoordinatesAt( m, 0 );
  EXPECT_DOUBLE_EQ( 515638.68018023379, x );
  EXPECT_DOUBLE_EQ( 6476431.3079803586, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  x = getVertexXCoordinatesAt( m, 1000 );
  y = getVertexYCoordinatesAt( m, 1000 );
  z = getVertexZCoordinatesAt( m, 1000 );
  EXPECT_DOUBLE_EQ( 515843.41624046268, x );
  EXPECT_DOUBLE_EQ( 6474959.9174060756, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  int f_count = MDAL_M_faceCount( m );
  EXPECT_EQ( 16099, f_count );


  double xmin, xmax, ymin, ymax;
  MDAL_M_extent( m, &xmin, &xmax, &ymin, &ymax );
  EXPECT_EQ( xmin, 515638.6801802338 );
  EXPECT_EQ( xmax, 517986.85726595984 );
  EXPECT_EQ( ymin, 6474893.417353791 );
  EXPECT_EQ( ymax, 6476852.987668288 );

  // test face 1
  int f_v_count = getFaceVerticesCountAt( m, 1 );
  EXPECT_EQ( 3, f_v_count ); //only triangles!
  int f_v = getFaceVerticesIndexAt( m, 100, 0 );
  EXPECT_EQ( 44, f_v );
  f_v = getFaceVerticesIndexAt( m, 100, 1 );
  EXPECT_EQ( 54, f_v ); \
  f_v = getFaceVerticesIndexAt( m, 100, 2 );
  EXPECT_EQ( 82, f_v );

  // Datasets
  ASSERT_EQ( 2, MDAL_M_datasetGroupCount( m ) );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, 0 );
  ASSERT_NE( g, nullptr );

  std::string tim = MDAL_G_referenceTime( g );

  EXPECT_TRUE( compareReferenceTime( g, "1969-12-01T01:00:00" ) );

  int meta_count = MDAL_G_metadataCount( g );
  ASSERT_EQ( 1, meta_count );

  const char *name = MDAL_G_name( g );
  EXPECT_EQ( std::string( "fond            m" ), std::string( name ) );

  bool scalar = MDAL_G_hasScalarData( g );
  EXPECT_EQ( true, scalar );

  MDAL_DataLocation dataLocation = MDAL_G_dataLocation( g );
  EXPECT_EQ( dataLocation, MDAL_DataLocation::DataOnVertices );

  ASSERT_EQ( 1, MDAL_G_datasetCount( g ) );
  MDAL_DatasetH ds = MDAL_G_dataset( g, 0 );
  ASSERT_NE( ds, nullptr );

  bool valid = MDAL_D_isValid( ds );
  EXPECT_EQ( true, valid );

  int count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 8215, count );

  double value = getValue( ds, 0 );
  EXPECT_TRUE( MDAL::equals( 0, value ) );
  value = getValue( ds, 20 );
  EXPECT_TRUE( MDAL::equals( 0, value ) );
  value = getValue( ds, 0 );
  EXPECT_TRUE( MDAL::equals( 0, value ) );

  MDAL_CloseMesh( m );
}

TEST( MeshSLFTest, FudaaFileSimplePrecision )
{
  std::string path = test_file( "/slf/init_Fudaa_simplePrecision.ser" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "SELAFIN:\"" + path + "\"" );

  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  MDAL_Status s = MDAL_LastStatus();
  EXPECT_EQ( MDAL_Status::None, s );

  const char *projection = MDAL_M_projection( m );
  EXPECT_EQ( std::string( "" ), std::string( projection ) );

  std::string driverName = MDAL_M_driverName( m );
  EXPECT_EQ( driverName, "SELAFIN" );

  int v_count = MDAL_M_vertexCount( m );
  EXPECT_EQ( v_count, 8215 );
  double x = getVertexXCoordinatesAt( m, 0 );
  double y = getVertexYCoordinatesAt( m, 0 );
  double z = getVertexZCoordinatesAt( m, 0 );
  EXPECT_DOUBLE_EQ( 515638.6875, x );
  EXPECT_DOUBLE_EQ( 6476431.5, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  x = getVertexXCoordinatesAt( m, 1000 );
  y = getVertexYCoordinatesAt( m, 1000 );
  z = getVertexZCoordinatesAt( m, 1000 );
  EXPECT_DOUBLE_EQ( 515843.40625, x );
  EXPECT_DOUBLE_EQ( 6474960.0, y );
  EXPECT_DOUBLE_EQ( 0.0, z );

  int f_count = MDAL_M_faceCount( m );
  EXPECT_EQ( 16099, f_count );


  double xmin, xmax, ymin, ymax;
  MDAL_M_extent( m, &xmin, &xmax, &ymin, &ymax );
  EXPECT_EQ( xmin, 515638.6875 );
  EXPECT_EQ( xmax, 517986.84375 );
  EXPECT_EQ( ymin, 6474893.5 );
  EXPECT_EQ( ymax, 6476853 );

  // test face 1
  int f_v_count = getFaceVerticesCountAt( m, 1 );
  EXPECT_EQ( 3, f_v_count ); //only triangles!
  int f_v = getFaceVerticesIndexAt( m, 100, 0 );
  EXPECT_EQ( 44, f_v );
  f_v = getFaceVerticesIndexAt( m, 100, 1 );
  EXPECT_EQ( 54, f_v ); \
  f_v = getFaceVerticesIndexAt( m, 100, 2 );
  EXPECT_EQ( 82, f_v );

  // Datasets
  ASSERT_EQ( 3, MDAL_M_datasetGroupCount( m ) );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, 0 );
  ASSERT_NE( g, nullptr );

  std::string tim = MDAL_G_referenceTime( g );

  EXPECT_TRUE( compareReferenceTime( g, "1969-12-01T01:00:00" ) );

  int meta_count = MDAL_G_metadataCount( g );
  ASSERT_EQ( 1, meta_count );

  const char *name = MDAL_G_name( g );
  EXPECT_EQ( std::string( "surface libre   m" ), std::string( name ) );

  bool scalar = MDAL_G_hasScalarData( g );
  EXPECT_EQ( true, scalar );

  MDAL_DataLocation dataLocation = MDAL_G_dataLocation( g );
  EXPECT_EQ( dataLocation, MDAL_DataLocation::DataOnVertices );

  ASSERT_EQ( 1, MDAL_G_datasetCount( g ) );
  MDAL_DatasetH ds = MDAL_G_dataset( g, 0 );
  ASSERT_NE( ds, nullptr );

  bool valid = MDAL_D_isValid( ds );
  EXPECT_EQ( true, valid );

  int count = MDAL_D_valueCount( ds );
  ASSERT_EQ( 8215, count );

  double value = getValue( ds, 0 );
  EXPECT_TRUE( MDAL::equals( 159.97970581054688, value ) );
  value = getValue( ds, 20 );
  EXPECT_TRUE( MDAL::equals( 149.5483856201172, value ) );
  value = getValue( ds, 0 );
  EXPECT_TRUE( MDAL::equals( 159.97970581054688, value ) );

  MDAL_CloseMesh( m );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  init_test();
  int ret =  RUN_ALL_TESTS();
  finalize_test();
  return ret;
}
