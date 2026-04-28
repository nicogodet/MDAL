/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2024 Nicogodet
*/

#include <stddef.h>
#include <iosfwd>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <limits>
#include <ctime>

#include "mdal_t3s.hpp"
#include "mdal.h"
#include "mdal_utils.hpp"
#include "mdal_logger.hpp"

#define DRIVER_NAME "T3S"

MDAL::DriverT3S::DriverT3S():
  Driver( DRIVER_NAME,
          "BlueKenue 2D T3 Scalar Mesh",
          "*.t3s",
          Capability::ReadMesh | Capability::SaveMesh
        )
{
}

MDAL::DriverT3S *MDAL::DriverT3S::create()
{
  return new DriverT3S();
}

int MDAL::DriverT3S::faceVerticesMaximumCount() const
{
  return 3;
}

bool MDAL::DriverT3S::canReadMesh( const std::string &uri )
{
  std::ifstream in = MDAL::openInputFile( uri, std::ifstream::in );
  std::string line;
  while ( std::getline( in, line ) )
  {
    if ( line.empty() || line[0] == '#' )
      continue;
    return MDAL::startsWith( line, ":FileType t3s", MDAL::ContainsBehaviour::CaseInsensitive );
  }
  return false;
}

std::unique_ptr<MDAL::Mesh> MDAL::DriverT3S::load( const std::string &meshFile, const std::string & )
{
  MDAL::Log::resetLastStatus();

  std::ifstream in = MDAL::openInputFile( meshFile, std::ifstream::in );
  if ( !in.is_open() )
  {
    MDAL::Log::error( MDAL_Status::Err_FileNotFound, name(), "Could not open file " + meshFile );
    return nullptr;
  }

  size_t nodeCount = 0;
  size_t elemCount = 0;
  bool headerDone = false;
  std::string line;

  // Parse header
  while ( std::getline( in, line ) )
  {
    MDAL::trim( line );
    if ( line.empty() || line[0] == '#' )
      continue;

    if ( MDAL::startsWith( line, ":EndHeader", MDAL::ContainsBehaviour::CaseInsensitive ) )
    {
      headerDone = true;
      break;
    }

    if ( MDAL::startsWith( line, ":NodeCount", MDAL::ContainsBehaviour::CaseInsensitive ) )
    {
      std::vector<std::string> chunks = MDAL::split( line, ' ' );
      if ( chunks.size() < 2 )
      {
        MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Invalid :NodeCount line" );
        return nullptr;
      }
      nodeCount = MDAL::toSizeT( chunks[1] );
    }
    else if ( MDAL::startsWith( line, ":ElementCount", MDAL::ContainsBehaviour::CaseInsensitive ) )
    {
      std::vector<std::string> chunks = MDAL::split( line, ' ' );
      if ( chunks.size() < 2 )
      {
        MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Invalid :ElementCount line" );
        return nullptr;
      }
      elemCount = MDAL::toSizeT( chunks[1] );
    }
  }

  if ( !headerDone )
  {
    MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Missing :EndHeader in " + meshFile );
    return nullptr;
  }

  if ( nodeCount == 0 || elemCount == 0 )
  {
    MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "NodeCount or ElementCount is zero in " + meshFile );
    return nullptr;
  }

  // Read vertices
  Vertices vertices( nodeCount );
  for ( size_t i = 0; i < nodeCount; ++i )
  {
    if ( !std::getline( in, line ) )
    {
      MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Not enough vertex lines in " + meshFile );
      return nullptr;
    }
    MDAL::trim( line );
    std::vector<std::string> chunks = MDAL::split( line, ' ' );
    if ( chunks.size() < 3 )
    {
      MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Invalid vertex line in " + meshFile );
      return nullptr;
    }
    vertices[i].x = MDAL::toDouble( chunks[0] );
    vertices[i].y = MDAL::toDouble( chunks[1] );
    vertices[i].z = MDAL::toDouble( chunks[2] );
  }

  // Read triangular faces (1-based indices → 0-based)
  Faces faces( elemCount );
  for ( size_t i = 0; i < elemCount; ++i )
  {
    if ( !std::getline( in, line ) )
    {
      MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Not enough element lines in " + meshFile );
      return nullptr;
    }
    MDAL::trim( line );
    std::vector<std::string> chunks = MDAL::split( line, ' ' );
    if ( chunks.size() < 3 )
    {
      MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Invalid element line in " + meshFile );
      return nullptr;
    }
    faces[i].resize( 3 );
    for ( int j = 0; j < 3; ++j )
    {
      size_t idx = MDAL::toSizeT( chunks[j] );
      if ( idx < 1 )
      {
        MDAL::Log::error( MDAL_Status::Err_IncompatibleMesh, name(), "Element index < 1 in " + meshFile );
        return nullptr;
      }
      faces[i][j] = idx - 1;
    }
  }

  std::unique_ptr<MemoryMesh> mesh(
    new MemoryMesh(
      DRIVER_NAME,
      3,
      meshFile
    )
  );
  mesh->setVertices( std::move( vertices ) );
  mesh->setFaces( std::move( faces ) );

  MDAL::addBedElevationDatasetGroup( mesh.get(), mesh->vertices() );

  return std::unique_ptr<Mesh>( mesh.release() );
}

void MDAL::DriverT3S::save( const std::string &fileName, const std::string &, MDAL::Mesh *mesh )
{
  MDAL::Log::resetLastStatus();

  std::ofstream file = MDAL::openOutputFile( fileName, std::ofstream::out );
  if ( !file.is_open() )
  {
    MDAL::Log::error( MDAL_Status::Err_FailToWriteToDisk, name(), "Could not open file " + fileName );
    return;
  }

  // Current UTC time
  std::time_t now = std::time( nullptr );
  char timeBuf[20];
  std::strftime( timeBuf, sizeof( timeBuf ), "%Y-%m-%d %H:%M:%S", std::gmtime( &now ) );

  size_t nNodes = mesh->verticesCount();
  size_t nElems = mesh->facesCount();

  file << ":FileType t3s  ASCII  EnSim 1.0\n";
  file << "# Canadian Hydraulics Centre/National Research Council (c) 1998-2012\n";
  file << "# DataType                 2D T3 Scalar Mesh\n";
  file << "#\n";
  file << ":Application              BlueKenue\n";
  file << ":Version                  3.3.4\n";
  file << ":WrittenBy                MDAL\n";
  file << ":CreationDate             " << timeBuf << "\n";
  file << "#\n";
  file << "#------------------------------------------------------------------------\n";
  file << "#\n";
  file << ":NodeCount " << nNodes << "\n";
  file << ":ElementCount " << nElems << "\n";
  file << ":ElementType  T3\n";
  file << "#\n";
  file << ":EndHeader\n";

  // Write vertices
  std::unique_ptr<MDAL::MeshVertexIterator> vertexIterator = mesh->readVertices();
  double vertex[3];
  for ( size_t i = 0; i < nNodes; ++i )
  {
    vertexIterator->next( 1, vertex );
    file << MDAL::doubleToString( vertex[0] )
         << " " << MDAL::doubleToString( vertex[1] )
         << " " << MDAL::doubleToString( vertex[2] )
         << "\n";
  }

  // Write faces (1-based indices)
  std::vector<int> vertexIndices( 3 );
  std::unique_ptr<MDAL::MeshFaceIterator> faceIterator = mesh->readFaces();
  for ( size_t i = 0; i < nElems; ++i )
  {
    int faceOffsets[1];
    faceIterator->next( 1, faceOffsets, 3, vertexIndices.data() );
    for ( int j = 0; j < faceOffsets[0]; ++j )
    {
      if ( j > 0 )
        file << " ";
      file << ( vertexIndices[j] + 1 );
    }
    file << "\n";
  }

  file.close();
}

std::string MDAL::DriverT3S::saveMeshOnFileSuffix() const
{
  return "t3s";
}
