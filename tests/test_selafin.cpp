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

TEST( MeshSLFTest, IPOBOComputation )
{
  // Build a 3x3 triangulated grid in memory and save it as SELAFIN.
  // The mesh is a MemoryMesh, so save() takes the compute path and
  // exercises computeIPOBO. Expected: 8 perimeter vertices numbered
  // consecutively 1..8, centre vertex 4 marked as interior (0).
  //
  //   6 -- 7 -- 8
  //   |  / |  / |
  //   3 -- 4 -- 5
  //   |  / |  / |
  //   0 -- 1 -- 2
  MDAL_DriverH driver = MDAL_driverFromName( "2DM" );
  ASSERT_NE( driver, nullptr );
  MDAL_MeshH mesh = MDAL_CreateMesh( driver );
  ASSERT_NE( mesh, nullptr );

  std::vector<double> coords
  {
    0, 0, 0,   1, 0, 0,   2, 0, 0,
    0, 1, 0,   1, 1, 0,   2, 1, 0,
    0, 2, 0,   1, 2, 0,   2, 2, 0,
  };
  MDAL_M_addVertices( mesh, 9, coords.data() );

  std::vector<int> faceSizes( 8, 3 );
  std::vector<int> faceIndices
  {
    0, 1, 4,   0, 4, 3,
    1, 2, 5,   1, 5, 4,
    3, 4, 7,   3, 7, 6,
    4, 5, 8,   4, 8, 7,
  };
  MDAL_M_addFaces( mesh, 8, faceSizes.data(), faceIndices.data() );

  std::string savedFile = tmp_file( "/ipobo_grid.slf" );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );

  std::vector<int> ipobo = MDAL::SelafinFile::readIPOBO( savedFile );
  ASSERT_EQ( ipobo.size(), 9u );

  EXPECT_EQ( ipobo[4], 0 ) << "Centre vertex should be interior";

  // TELEMAC convention: numbering starts at the bottom-left node (min x+y)
  // and follows the external boundary counter-clockwise.
  // Walk: 0 -> 1 -> 2 -> 5 -> 8 -> 7 -> 6 -> 3
  std::vector<int> expected {1, 2, 3, 8, 0, 4, 7, 6, 5};
  EXPECT_EQ( ipobo, expected );
}

TEST( MeshSLFTest, IPOBORoundTrip )
{
  // Saving always recomputes IPOBO from the mesh topology. The recomputed
  // array must flag exactly the same set of boundary nodes as the source
  // file, with values renumbered consecutively from 1.
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
  ASSERT_EQ( sourceIpobo.size(), savedIpobo.size() );

  // Same boundary node set as the source
  std::vector<int> savedBoundaryVals;
  for ( size_t i = 0; i < sourceIpobo.size(); ++i )
  {
    EXPECT_EQ( sourceIpobo[i] > 0, savedIpobo[i] > 0 )
        << "Boundary flag differs from source at vertex " << i;
    if ( savedIpobo[i] > 0 )
      savedBoundaryVals.push_back( savedIpobo[i] );
  }

  // Consecutive numbering 1..N
  std::sort( savedBoundaryVals.begin(), savedBoundaryVals.end() );
  for ( size_t i = 0; i < savedBoundaryVals.size(); ++i )
    ASSERT_EQ( savedBoundaryVals[i], static_cast<int>( i + 1 ) )
        << "IPOBO boundary indices are not consecutive";
}

TEST( MeshSLFTest, IPOBOIsland )
{
  // 4x4 vertex grid with the central cell removed: an external boundary of
  // 12 nodes and a 4-node island (hole). Expected: external contour numbered
  // first, CCW from the bottom-left node; island numbered after, CW from its
  // own min(x+y) node.
  //
  //   12 - 13 - 14 - 15
  //   |    |    |    |
  //   8 -- 9 -- 10 - 11
  //   |    | ** |    |     ** = hole
  //   4 -- 5 -- 6 -- 7
  //   |    |    |    |
  //   0 -- 1 -- 2 -- 3
  MDAL_DriverH driver = MDAL_driverFromName( "2DM" );
  ASSERT_NE( driver, nullptr );
  MDAL_MeshH mesh = MDAL_CreateMesh( driver );
  ASSERT_NE( mesh, nullptr );

  std::vector<double> coords;
  for ( int j = 0; j < 4; ++j )
    for ( int i = 0; i < 4; ++i )
    {
      coords.push_back( i );
      coords.push_back( j );
      coords.push_back( 0 );
    }
  MDAL_M_addVertices( mesh, 16, coords.data() );

  std::vector<int> faceIndices;
  for ( int cy = 0; cy < 3; ++cy )
    for ( int cx = 0; cx < 3; ++cx )
    {
      if ( cx == 1 && cy == 1 )
        continue;  // hole
      int bl = cy * 4 + cx;
      faceIndices.insert( faceIndices.end(), { bl, bl + 1, bl + 5 } );
      faceIndices.insert( faceIndices.end(), { bl, bl + 5, bl + 4 } );
    }
  std::vector<int> faceSizes( 16, 3 );
  MDAL_M_addFaces( mesh, 16, faceSizes.data(), faceIndices.data() );

  std::string savedFile = tmp_file( "/ipobo_island.slf" );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );

  std::vector<int> ipobo = MDAL::SelafinFile::readIPOBO( savedFile );
  ASSERT_EQ( ipobo.size(), 16u );

  // External CCW from vertex 0: 0,1,2,3,7,11,15,14,13,12,8,4 -> 1..12
  // Island CW from vertex 5:    5,9,10,6                     -> 13..16
  std::vector<int> expected
  {
    1,  2,  3,  4,
    12, 13, 16, 5,
    11, 14, 15, 6,
    10, 9,  8,  7,
  };
  EXPECT_EQ( ipobo, expected );
}

TEST( MeshSLFTest, IPOBOPinchNode )
{
  // Two triangles sharing a single vertex (figure-eight). Edge-consuming
  // traversal must trace both contours; the shared node keeps the number of
  // the last contour, so one value is missing from the sequence.
  //
  //   1     4
  //   | \ / |
  //   |  0  |
  //   | / \ |
  //   2     3
  MDAL_DriverH driver = MDAL_driverFromName( "2DM" );
  ASSERT_NE( driver, nullptr );
  MDAL_MeshH mesh = MDAL_CreateMesh( driver );
  ASSERT_NE( mesh, nullptr );

  std::vector<double> coords
  {
    0, 0, 0,
    -1, 1, 0,
    -1, -1, 0,
    1, -1, 0,
    1, 1, 0,
  };
  MDAL_M_addVertices( mesh, 5, coords.data() );

  std::vector<int> faceSizes( 2, 3 );
  std::vector<int> faceIndices
  {
    0, 1, 2,
    0, 3, 4,
  };
  MDAL_M_addFaces( mesh, 2, faceSizes.data(), faceIndices.data() );

  std::string savedFile = tmp_file( "/ipobo_pinch.slf" );
  MDAL_SaveMesh( mesh, savedFile.c_str(), "SELAFIN" );
  // The pinch node must be reported as a non-manifold boundary warning
  ASSERT_EQ( MDAL_Status::Warn_InvalidElements, MDAL_LastStatus() );
  MDAL_CloseMesh( mesh );

  std::vector<int> ipobo = MDAL::SelafinFile::readIPOBO( savedFile );
  ASSERT_EQ( ipobo.size(), 5u );

  // Both contours must be numbered: every vertex is a boundary node.
  for ( int v : ipobo )
    EXPECT_GT( v, 0 ) << "All vertices of a figure-eight are boundary nodes";

  // Left triangle first (contains the min x+y node): 2,0,1 -> 1,2,3.
  // Right triangle second, reversed to CW: 0,4,3 -> 4,5,6.
  // The pinch node 0 keeps the number of the last contour (4); the value 2
  // is lost — IPOBO cannot fully represent a non-manifold boundary.
  std::vector<int> expected {4, 3, 1, 6, 5};
  EXPECT_EQ( ipobo, expected );
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
