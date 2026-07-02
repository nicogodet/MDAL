/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2026 Lutra Consulting Limited
*/
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <cmath>
#include <limits>

//mdal
#include "mdal.h"
#include "mdal_testutils.hpp"

namespace
{
  // Multi-timestep scalar group in a SELAFIN file; the dataset at `peakIndex`
  // holds an outlier so that sampling which misses it misses the global range.
  MDAL_MeshH buildMultiTimestepMesh( const std::string &savedFile,
                                     int nDatasets,
                                     int peakIndex,
                                     double peakValue )
  {
    std::string sourceFile = test_file( "/slf/example.slf" );
    MDAL_MeshH sourceMesh = MDAL_LoadMesh( sourceFile.c_str() );
    EXPECT_NE( sourceMesh, nullptr );
    MDAL_SaveMesh( sourceMesh, savedFile.c_str(), "SELAFIN" );
    EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
    MDAL_CloseMesh( sourceMesh );

    MDAL_MeshH mesh = MDAL_LoadMesh( savedFile.c_str() );
    EXPECT_NE( mesh, nullptr );

    MDAL_DriverH driver = MDAL_driverFromName( "SELAFIN" );
    MDAL_DatasetGroupH g = MDAL_M_addDatasetGroup( mesh,
                           "TestGroup",
                           DataOnVertices,
                           true /*scalar*/,
                           driver,
                           savedFile.c_str() );
    EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );

    const size_t v_count = MDAL_M_vertexCount( mesh );
    for ( int i = 0; i < nDatasets; ++i )
    {
      std::vector<double> values( v_count, static_cast<double>( i ) );
      if ( i == peakIndex )
        values[0] = peakValue;
      MDAL_G_addDataset( g, static_cast<double>( i ), values.data(), nullptr );
      EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
    }
    MDAL_G_closeEditMode( g );
    EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::None );
    return mesh;
  }
}

TEST( MeshApproxStatisticsTest, SampleCountZeroEqualsExact )
{
  std::string file = tmp_file( "/approx_stats_sc0.slf" );
  MDAL_MeshH m = buildMultiTimestepMesh( file, 10, 5, 1000.0 );
  ASSERT_NE( m, nullptr );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, MDAL_M_datasetGroupCount( m ) - 1 );
  ASSERT_NE( g, nullptr );

  double minE = NAN, maxE = NAN, minA = NAN, maxA = NAN;
  MDAL_G_minimumMaximum( g, &minE, &maxE );
  MDAL_G_minimumMaximumApprox( g, 0, &minA, &maxA );

  EXPECT_DOUBLE_EQ( minE, minA );
  EXPECT_DOUBLE_EQ( maxE, maxA );

  MDAL_CloseMesh( m );
}

TEST( MeshApproxStatisticsTest, SampleCountAboveOrEqualToDatasetCountEqualsExact )
{
  std::string file = tmp_file( "/approx_stats_full.slf" );
  MDAL_MeshH m = buildMultiTimestepMesh( file, 10, 5, 1000.0 );
  ASSERT_NE( m, nullptr );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, MDAL_M_datasetGroupCount( m ) - 1 );
  ASSERT_NE( g, nullptr );

  double minE = NAN, maxE = NAN, minA = NAN, maxA = NAN;
  MDAL_G_minimumMaximum( g, &minE, &maxE );
  MDAL_G_minimumMaximumApprox( g, 10, &minA, &maxA );

  EXPECT_DOUBLE_EQ( minE, minA );
  EXPECT_DOUBLE_EQ( maxE, maxA );

  EXPECT_DOUBLE_EQ( 1000.0, maxE );

  MDAL_CloseMesh( m );
}

TEST( MeshApproxStatisticsTest, SampleCountThreeMissesMiddleOutlier )
{
  // for n=10 and sampleCount=3 the sampled indices are {0, 4, 9}
  std::string file = tmp_file( "/approx_stats_sc3.slf" );
  MDAL_MeshH m = buildMultiTimestepMesh( file, 10, 5, 1000.0 );
  ASSERT_NE( m, nullptr );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, MDAL_M_datasetGroupCount( m ) - 1 );
  ASSERT_NE( g, nullptr );

  double minE = NAN, maxE = NAN, minA = NAN, maxA = NAN;
  MDAL_G_minimumMaximum( g, &minE, &maxE );
  MDAL_G_minimumMaximumApprox( g, 3, &minA, &maxA );

  EXPECT_DOUBLE_EQ( 1000.0, maxE );
  EXPECT_LT( maxA, 1000.0 );
  EXPECT_LE( minE, minA );
  EXPECT_GE( maxE, maxA );

  MDAL_CloseMesh( m );
}

TEST( MeshApproxStatisticsTest, ExactCacheUntouchedAfterApproximate )
{
  std::string file = tmp_file( "/approx_stats_cache.slf" );
  MDAL_MeshH m = buildMultiTimestepMesh( file, 10, 5, 1000.0 );
  ASSERT_NE( m, nullptr );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, MDAL_M_datasetGroupCount( m ) - 1 );
  ASSERT_NE( g, nullptr );

  double minA = NAN, maxA = NAN;
  MDAL_G_minimumMaximumApprox( g, 3, &minA, &maxA );
  EXPECT_LT( maxA, 1000.0 );

  double minE = NAN, maxE = NAN;
  MDAL_G_minimumMaximum( g, &minE, &maxE );
  EXPECT_DOUBLE_EQ( 1000.0, maxE );

  MDAL_CloseMesh( m );
}

TEST( MeshLoadFlagsTest, SkipStatisticsThenLazyExact )
{
  std::string file = tmp_file( "/skipstats_eager.slf" );
  MDAL_MeshH eager = buildMultiTimestepMesh( file, 10, 5, 1000.0 );
  ASSERT_NE( eager, nullptr );
  MDAL_CloseMesh( eager );

  MDAL_MeshH m = MDAL_LoadMeshWithFlags( file.c_str(), MDAL_LF_SkipStatistics );
  ASSERT_NE( m, nullptr );

  ASSERT_GE( MDAL_M_datasetGroupCount( m ), 1 );
  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, MDAL_M_datasetGroupCount( m ) - 1 );
  ASSERT_NE( g, nullptr );

  double minA = NAN, maxA = NAN;
  MDAL_G_minimumMaximumApprox( g, 3, &minA, &maxA );
  EXPECT_FALSE( std::isnan( maxA ) );
  EXPECT_LT( maxA, 1000.0 );

  // lazy exact computes on first access and caches
  double minE = NAN, maxE = NAN;
  MDAL_G_minimumMaximum( g, &minE, &maxE );
  EXPECT_DOUBLE_EQ( 1000.0, maxE );

  double minE2 = NAN, maxE2 = NAN;
  MDAL_G_minimumMaximum( g, &minE2, &maxE2 );
  EXPECT_DOUBLE_EQ( minE, minE2 );
  EXPECT_DOUBLE_EQ( maxE, maxE2 );

  MDAL_CloseMesh( m );
}

TEST( MeshLoadFlagsTest, NoSkipFlagPreservesEagerBehavior )
{
  std::string file = tmp_file( "/skipstats_default.slf" );
  MDAL_MeshH eager = buildMultiTimestepMesh( file, 10, 5, 1000.0 );
  ASSERT_NE( eager, nullptr );
  MDAL_CloseMesh( eager );

  MDAL_MeshH m = MDAL_LoadMeshWithFlags( file.c_str(), 0 );
  ASSERT_NE( m, nullptr );

  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, MDAL_M_datasetGroupCount( m ) - 1 );
  ASSERT_NE( g, nullptr );

  double min = NAN, max = NAN;
  MDAL_G_minimumMaximum( g, &min, &max );
  EXPECT_DOUBLE_EQ( 1000.0, max );

  MDAL_CloseMesh( m );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  init_test();
  int ret = RUN_ALL_TESTS();
  finalize_test();
  return ret;
}
