/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2024 Nicogodet
*/
#include "gtest/gtest.h"

#include "mdal.h"
#include "mdal_testutils.hpp"
#include "mdal_utils.hpp"

TEST( MeshT3STest, LoadSimpleMesh )
{
  std::string path = test_file( "/t3s/simple_mesh.t3s" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "T3S:\"" + path + "\"" );

  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );

  EXPECT_EQ( std::string( MDAL_M_driverName( m ) ), "T3S" );
  EXPECT_EQ( MDAL_M_faceVerticesMaximumCount( m ), 3 );

  // Vertices
  ASSERT_EQ( MDAL_M_vertexCount( m ), 4 );
  EXPECT_DOUBLE_EQ( getVertexXCoordinatesAt( m, 0 ), 0.0 );
  EXPECT_DOUBLE_EQ( getVertexYCoordinatesAt( m, 0 ), 0.0 );
  EXPECT_DOUBLE_EQ( getVertexZCoordinatesAt( m, 0 ), 10.0 );
  EXPECT_DOUBLE_EQ( getVertexXCoordinatesAt( m, 2 ), 1.0 );
  EXPECT_DOUBLE_EQ( getVertexYCoordinatesAt( m, 2 ), 1.0 );
  EXPECT_DOUBLE_EQ( getVertexZCoordinatesAt( m, 2 ), 30.0 );

  // Faces
  ASSERT_EQ( MDAL_M_faceCount( m ), 2 );
  EXPECT_EQ( getFaceVerticesCountAt( m, 0 ), 3 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 0, 0 ), 0 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 0, 1 ), 1 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 0, 2 ), 2 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 1, 0 ), 0 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 1, 1 ), 2 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 1, 2 ), 3 );

  // Extent
  double minX, maxX, minY, maxY;
  MDAL_M_extent( m, &minX, &maxX, &minY, &maxY );
  EXPECT_DOUBLE_EQ( minX, 0.0 );
  EXPECT_DOUBLE_EQ( maxX, 1.0 );
  EXPECT_DOUBLE_EQ( minY, 0.0 );
  EXPECT_DOUBLE_EQ( maxY, 1.0 );

  // Bed Elevation dataset
  ASSERT_EQ( MDAL_M_datasetGroupCount( m ), 1 );
  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, 0 );
  ASSERT_NE( g, nullptr );
  EXPECT_EQ( std::string( MDAL_G_name( g ) ), "Bed Elevation" );
  EXPECT_TRUE( MDAL_G_hasScalarData( g ) );
  EXPECT_EQ( MDAL_G_dataLocation( g ), MDAL_DataLocation::DataOnVertices );
  ASSERT_EQ( MDAL_G_datasetCount( g ), 1 );

  MDAL_DatasetH ds = MDAL_G_dataset( g, 0 );
  ASSERT_NE( ds, nullptr );
  EXPECT_TRUE( MDAL_D_isValid( ds ) );
  EXPECT_EQ( MDAL_D_valueCount( ds ), 4 );
  EXPECT_DOUBLE_EQ( getValue( ds, 0 ), 10.0 );
  EXPECT_DOUBLE_EQ( getValue( ds, 1 ), 20.0 );
  EXPECT_DOUBLE_EQ( getValue( ds, 2 ), 30.0 );
  EXPECT_DOUBLE_EQ( getValue( ds, 3 ), 40.0 );

  MDAL_CloseMesh( m );
}

TEST( MeshT3STest, SaveAndReload )
{
  std::string srcPath = test_file( "/t3s/simple_mesh.t3s" );
  std::string outPath = tmp_file( "/simple_mesh_saved.t3s" );

  MDAL_MeshH m = MDAL_LoadMesh( srcPath.c_str() );
  ASSERT_NE( m, nullptr );

  MDAL_SaveMesh( m, outPath.c_str(), "T3S" );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );
  MDAL_CloseMesh( m );

  // Reload saved file
  MDAL_MeshH m2 = MDAL_LoadMesh( outPath.c_str() );
  ASSERT_NE( m2, nullptr );
  ASSERT_EQ( MDAL_Status::None, MDAL_LastStatus() );

  EXPECT_EQ( MDAL_M_vertexCount( m2 ), 4 );
  EXPECT_EQ( MDAL_M_faceCount( m2 ), 2 );

  EXPECT_DOUBLE_EQ( getVertexXCoordinatesAt( m2, 0 ), 0.0 );
  EXPECT_DOUBLE_EQ( getVertexYCoordinatesAt( m2, 0 ), 0.0 );
  EXPECT_DOUBLE_EQ( getVertexZCoordinatesAt( m2, 0 ), 10.0 );

  EXPECT_EQ( getFaceVerticesIndexAt( m2, 0, 0 ), 0 );
  EXPECT_EQ( getFaceVerticesIndexAt( m2, 0, 1 ), 1 );
  EXPECT_EQ( getFaceVerticesIndexAt( m2, 0, 2 ), 2 );

  MDAL_CloseMesh( m2 );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  init_test();
  int ret = RUN_ALL_TESTS();
  finalize_test();
  return ret;
}
