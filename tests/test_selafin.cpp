/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2019 ARTELIA - Christophe Coulet
 (christophe dot coulet at arteliagroup dot com)
*/
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <algorithm>

//mdal
#include "mdal.h"
#include "mdal_utils.hpp"
#include "mdal_testutils.hpp"
#include "frmts/mdal_selafin.hpp"

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

// Build a triangulated MemoryMesh (2DM) from interleaved x,y,z coordinates and
// 0-based triangle connectivity, save it as SELAFIN (which takes the compute
// path and exercises computeIPOBO), and return the IPOBO array read back.
static std::vector<int> saveTriMeshAndReadIpobo(
  std::vector<double> coords,        // x,y,z interleaved
  std::vector<int> faceIndices,      // 3 per triangle, 0-based
  const std::string &tmpName )
{
  MDAL_DriverH driver = MDAL_driverFromName( "2DM" );
  MDAL_MeshH mesh = MDAL_CreateMesh( driver );
  const int nVerts = static_cast<int>( coords.size() / 3 );
  const int nFaces = static_cast<int>( faceIndices.size() / 3 );
  MDAL_M_addVertices( mesh, nVerts, coords.data() );
  std::vector<int> faceSizes( static_cast<size_t>( nFaces ), 3 );
  MDAL_M_addFaces( mesh, nFaces, faceSizes.data(), faceIndices.data() );
  std::string savedFile = tmp_file( tmpName );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  MDAL_CloseMesh( mesh );
  return MDAL::SelafinFile::readIPOBO( savedFile );
}

TEST( MeshSLFTest, IPOBOComputation )
{
  // Build a 3x3 triangulated grid in memory and save it as SELAFIN. The mesh
  // is a MemoryMesh, so save() takes the compute path and exercises
  // computeIPOBO. The expected vector is a byte-for-byte cross-check against
  // python-serafin SerafinHeader.build_ipobo() on the same triangulation.
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
  EXPECT_EQ( ipobo, expected ) << "IPOBO does not match python-serafin build_ipobo";
}

TEST( MeshSLFTest, IPOBOIsland )
{
  // 4x4 grid (row-major, x fastest) with the central cell removed, forming an
  // annulus: a 12-node outer boundary (CCW) enclosing a 4-node island (CW).
  // Cross-checked element-wise against python-serafin build_ipobo(): the outer
  // ring is numbered 1..12 first, then the island 13..16.
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
  EXPECT_EQ( ipobo, expected ) << "Island IPOBO does not match python-serafin build_ipobo";
}

TEST( MeshSLFTest, IPOBOMultiDomain )
{
  // Two disjoint unit squares far apart. The domain whose south-west node has
  // the smaller (x+y) is numbered first; both external rings are CCW.
  // Cross-checked element-wise against python-serafin build_ipobo().
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
  EXPECT_EQ( ipobo, expected ) << "Multi-domain IPOBO does not match python-serafin build_ipobo";
}

TEST( MeshSLFTest, IPOBODegenerateBowtie )
{
  // Two triangles meeting at a single shared vertex (node 0) form a non-manifold
  // boundary: node 0 has 4 boundary neighbours (degree != 2). computeIPOBO must
  // fall back to an all-zero IPOBO rather than emit a plausible-but-wrong one.
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

  std::vector<int> ipobo = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_bowtie.slf" );
  ASSERT_EQ( ipobo.size(), 5u );
  for ( int v : ipobo )
    EXPECT_EQ( v, 0 ) << "Non-manifold mesh must yield an all-zero IPOBO";
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

TEST( MeshSLFTest, IPOBOLargeMeshBoundarySet )
{
  // Rebuild a real TELEMAC mesh (Malpasset, 13541 nodes) as a MemoryMesh so
  // save() takes the compute path, then verify computeIPOBO marks EXACTLY the
  // same boundary node set as the file shipped by TELEMAC, numbered
  // consecutively 1..N. (The numbering order may differ from the stored one,
  // which uses a boundary-first node renumbering, but the boundary SET and
  // consecutiveness must match.)
  std::string sourceFile = test_file( "/slf/example.slf" );
  std::vector<int> storedIpobo = MDAL::SelafinFile::readIPOBO( sourceFile );
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

TEST( MeshSLFTest, IPOBOMatchesTelemacReference )
{
  // Real TELEMAC meshes whose stored IPOBO already follows the geometric
  // (south-west walk) convention, so computeIPOBO must reproduce it exactly.
  // geo_siphon has an island (2 contours) and geo_weirs has several contours,
  // exercising the nesting / orientation / DFS-ordering logic on real data.
  // These fixtures were verified to satisfy stored == python-serafin
  // build_ipobo(), so matching the stored array also matches build_ipobo().
  const std::vector<std::string> meshes{ "/slf/geo_siphon.slf", "/slf/geo_weirs.slf" };

  for ( const std::string &name : meshes )
  {
    const std::string sourceFile = test_file( name );
    std::vector<int> stored = MDAL::SelafinFile::readIPOBO( sourceFile );
    ASSERT_FALSE( stored.empty() ) << name;

    // Rebuild as a MemoryMesh so save() takes the compute path (not the cache).
    MDAL_MeshH src = MDAL_LoadMesh( sourceFile.c_str() );
    ASSERT_NE( src, nullptr ) << name;
    const int nVerts = MDAL_M_vertexCount( src );
    const int nFaces = MDAL_M_faceCount( src );
    std::vector<double> coords = getCoordinates( src, nVerts );
    std::vector<int> faceIndices = faceVertexIndices( src, nFaces );
    MDAL_CloseMesh( src );

    std::vector<int> got = saveTriMeshAndReadIpobo( coords, faceIndices, "/ipobo_ref.slf" );
    ASSERT_EQ( got.size(), stored.size() ) << name;
    EXPECT_EQ( got, stored ) << "computeIPOBO does not reproduce the stored IPOBO for " << name;
  }
}

TEST( MeshSLFTest, IPOBORoundTrip )
{
  // Round-tripping a MeshSelafin reuses the cached IPOBO from disk
  // (no recompute). The saved file must therefore have exactly the
  // same IPOBO array as the source — including any non-consecutive
  // numbering produced by other tools.
  std::string sourceFile = test_file( "/slf/example.slf" );
  std::string savedFile = tmp_file( "/ipobo_roundtrip.slf" );

  std::vector<int> sourceIpobo = MDAL::SelafinFile::readIPOBO( sourceFile );
  ASSERT_FALSE( sourceIpobo.empty() );

  MDAL_MeshH mesh = MDAL_LoadMesh( sourceFile.c_str() );
  ASSERT_NE( mesh, nullptr );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );

  std::vector<int> savedIpobo = MDAL::SelafinFile::readIPOBO( savedFile );
  EXPECT_EQ( sourceIpobo, savedIpobo ) << "Round-trip did not preserve IPOBO";
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
