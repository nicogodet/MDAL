/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2019 Peter Petrik (zilolv at gmail dot com)
*/

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <iosfwd>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <limits>
#include <cassert>
#include <memory>
#include <algorithm>

#include "mdal_selafin.hpp"
#include "mdal.h"
#include "mdal_utils.hpp"
#include <math.h>
#include "mdal_logger.hpp"

#define BUFFER_SIZE 2000

// //////////////////////////////
// SelafinFile
// //////////////////////////////

MDAL::SelafinFile::SelafinFile( const std::string &fileName ):
  mFileName( fileName )
{}

void MDAL::SelafinFile::initialize()
{
  if ( !MDAL::fileExists( mFileName ) )
  {
    throw MDAL::Error( MDAL_Status::Err_FileNotFound, "Did not find file " + mFileName );
  }
  mIn = MDAL::openInputFile( mFileName, std::ios_base::in | std::ios_base::binary );
  if ( !mIn )
    throw MDAL::Error( MDAL_Status::Err_FileNotFound, "File " + mFileName + " could not be open" ); // Couldn't open the file

  // get length of file:
  mIn.seekg( 0, mIn.end );
  mFileSize = mIn.tellg();
  mIn.seekg( 0, mIn.beg );

  mChangeEndianness = MDAL::isNativeLittleEndian();

  //Check if need to change the endianness
  // read first size_t that has to be 80
  size_t firstInt = readSizeT();
  mIn.seekg( 0, mIn.beg );
  if ( firstInt != 80 )
  {
    mChangeEndianness = !mChangeEndianness;
    //Retry
    firstInt = readSizeT();
    if ( firstInt != 80 )
      throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File " + mFileName + " could not be open" );
    mIn.seekg( 0, mIn.beg );
  }

  mParsed = false;
}

void MDAL::SelafinFile::parseMeshFrame()
{
  /* 1 record containing the title of the study (72 characters) and a 8 characters
  string indicating the type of format (SERAFIN or SERAFIND)
  */
  readHeader();

  /* 1 record containing the two integers NBV(1) and NBV(2) (number of linear
     and quadratic variables, NBV(2) with the value of 0 for Telemac, as
     quadratic values are not saved so far), numbered from 1 in docs
  */
  std::vector<int> nbv = readIntArr( 2 );

  /* NBV(1) records containing the names and units of each variable (over 32 characters)
  */
  mVariableNames.clear();
  for ( int i = 0; i < nbv[0]; ++i )
  {
    mVariableNames.push_back( readString( 32 ) );
  }

  /* 1 record containing the integers table IPARAM (10 integers, of which only
      the 6 are currently being used), numbered from 1 in docs

      - if IPARAM (3) != 0: the value corresponds to the x-coordinate of the
      origin of the mesh,

      - if IPARAM (4) != 0: the value corresponds to the y-coordinate of the
      origin of the mesh,

      - if IPARAM (7) != 0: the value corresponds to the number of  planes
      on the vertical (3D computation),

      - if IPARAM (8) != 0: the value corresponds to the number of
      boundary points (in parallel),

      - if IPARAM (9) != 0: the value corresponds to the number of interface
      points (in parallel),

      - if IPARAM(8) or IPARAM(9) != 0: the array IPOBO below is replaced
      by the array KNOLG (total initial number of points). All the other
      numbers are local to the sub-domain, including IKLE
  */
  mParameters = readIntArr( 10 );
  mXOrigin = static_cast<double>( mParameters[2] );
  mYOrigin = static_cast<double>( mParameters[3] );

  if ( mParameters[6] != 0 && mParameters[6] != 1 ) //some tools set this value to one for 2D mesh
  {
    // would need additional parsing
    throw MDAL::Error( MDAL_Status::Err_MissingDriver, "File " + mFileName + " would need additional parsing" );
  }

  /*
    if IPARAM (10) = 1: a record containing the computation starting date
  */

  if ( mParameters[9] == 1 )
  {
    std::vector<int> datetime = readIntArr( 6 );
    mReferenceTime = DateTime( datetime[0], datetime[1], datetime[2], datetime[3], datetime[4], double( datetime[5] ) );
  }

  /* 1 record containing the integers NELEM,NPOIN,NDP,1 (number of
     elements, number of points, number of points per element and the value 1)
   */
  std::vector<int> numbers = readIntArr( 4 );
  mFacesCount = numbers[0];
  mVerticesCount = numbers[1];
  mVerticesPerFace = numbers[2];

  /* 1 record containing table IKLE (integer array
     of dimension (NDP,NELEM)
     which is the connectivity table.
  */
  size_t size = mFacesCount * mVerticesPerFace;
  if ( ! checkIntArraySize( size ) )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading connectivity table" );
  mConnectivityStreamPosition = passThroughIntArray( size );

  /* 1 record containing table IPOBO (integer array of dimension NPOIN); the
     value of one element is 0 for an internal point, and
     gives the numbering of boundary points for the others
  */
  size = mVerticesCount;
  if ( ! checkIntArraySize( size ) )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading IPOBO table" );
  mIPOBOStreamPosition = passThroughIntArray( size );

  /* 1 record containing table X (real array of dimension NPOIN containing the
     abscisse of the points)
     AND here, we can know if float of this file is simple or double precision:
     result of size of record divided by number of vertices gives the byte size of the float:
     -> 4 : simple precision -> 8 : double precision
  */
  size = mVerticesCount;
  size_t recordSize = readSizeT();
  mStreamInFloatPrecision = recordSize / size == 4;
  if ( !mStreamInFloatPrecision && recordSize / size != 8 )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem: could not determine if simple or double precision" );
  mXStreamPosition = passThroughDoubleArray( size );

  /* 1 record containing table Y (real array of dimension NPOIN containing the
     ordinate of the points)
  */
  size = mVerticesCount;
  if ( ! checkDoubleArraySize( size ) )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading abscisse values" );
  mYStreamPosition = passThroughDoubleArray( size );
}

void MDAL::SelafinFile::parseFile()
{
  parseMeshFrame();

  /* Next, for each time step, the following are found:
     - 1 record containing time T (real),
     - NBV(1)+NBV(2) records containing the results tables for each variable at time
  */

  size_t realSize = mStreamInFloatPrecision ? 4 : 8;
  size_t nTimesteps = remainingBytes() / ( 8 + realSize + ( 4 + ( mVerticesCount ) * realSize + 4 ) * mVariableNames.size() );
  mVariableStreamPosition.resize( mVariableNames.size(), std::vector<std::streampos>( nTimesteps ) );
  mTimeSteps.resize( nTimesteps );
  for ( size_t nT = 0; nT < nTimesteps; ++nT )
  {
    std::vector<double> times = readDoubleArr( 1 );
    mTimeSteps[nT] = RelativeTimestamp( times[0], RelativeTimestamp::seconds );
    for ( size_t i = 0; i < mVariableNames.size(); ++i )
    {
      if ( ! checkDoubleArraySize( mVerticesCount ) )
        throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading dataset values" );
      mVariableStreamPosition[i][nT] = passThroughDoubleArray( mVerticesCount );
    }
  }

  mParsed = true;
}

std::string MDAL::SelafinFile::readHeader()
{
  initialize();
  std::string header = readString( 80 );

  std::string title = header.substr( 0, 72 );
  title = trim( title );

  if ( header.size() < 80 ) // IF "SERAFIN", the readString method remove the last character that is a space
    header.append( " " );
  return header;
}

std::vector<int> MDAL::SelafinFile::connectivityIndex( size_t offset, size_t count )
{
  return readIntArr( mConnectivityStreamPosition, offset, count );
}

std::vector<double> MDAL::SelafinFile::vertices( size_t offset, size_t count )
{
  std::vector<double> xValues = readDoubleArr( mXStreamPosition, offset, count );
  std::vector<double> yValues = readDoubleArr( mYStreamPosition, offset, count );

  if ( xValues.size() != count || yValues.size() != count )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading vertices" );

  std::vector<double> coordinates( count * 3 );
  for ( size_t i = 0; i < count; ++i )
  {
    coordinates[i * 3] = xValues.at( i ) + mXOrigin;
    coordinates[i * 3 + 1] = yValues.at( i ) + mYOrigin;
    coordinates[i * 3 + 2] = 0;
  }
  return coordinates;
}

std::unique_ptr<MDAL::Mesh> MDAL::SelafinFile::createMesh( const std::string &fileName )
{
  std::shared_ptr<SelafinFile> reader = std::make_shared<SelafinFile>( fileName );
  reader->initialize();
  reader->parseFile();

  std::unique_ptr<Mesh> mesh( new MeshSelafin( fileName, reader ) );
  populateDataset( mesh.get(), std::move( reader ) );

  return mesh;
}

std::vector<int> MDAL::SelafinFile::ipoboArray()
{
  if ( !mParsed )
    parseFile();

  // on a partitioned file (IPARAM(8) or IPARAM(9) != 0) the record at this
  // position holds KNOLG, not IPOBO — nothing reusable
  if ( mParameters.size() < 9 || mParameters[7] != 0 || mParameters[8] != 0 )
    return std::vector<int>();

  return readIntArr( mIPOBOStreamPosition, 0, mVerticesCount );
}

void MDAL::SelafinFile::populateDataset( MDAL::Mesh *mesh, const std::string &fileName )
{
  std::shared_ptr<SelafinFile> reader = std::make_shared<SelafinFile>( fileName );
  reader->initialize();
  reader->parseFile();

  if ( mesh->verticesCount() != reader->verticesCount() || mesh->facesCount() != reader->facesCount() )
    throw MDAL::Error( MDAL_Status::Err_IncompatibleDataset, "Faces or vertices counts in the file are not the same" );

  populateDataset( mesh, std::move( reader ) );
}

size_t MDAL::SelafinFile::facesCount()
{
  if ( !mParsed )
    parseFile();
  return mFacesCount;
}

size_t MDAL::SelafinFile::verticesCount()
{
  if ( !mParsed )
    parseFile();
  return mVerticesCount;
}

size_t MDAL::SelafinFile::verticesPerFace()
{
  if ( !mParsed )
    parseFile();
  return mVerticesPerFace;
}

std::vector<double> MDAL::SelafinFile::datasetValues( size_t timeStepIndex, size_t variableIndex, size_t offset, size_t count )
{
  if ( !mParsed )
    parseFile();
  if ( variableIndex < mVariableStreamPosition.size() &&  timeStepIndex < mVariableStreamPosition[variableIndex].size() )
    return readDoubleArr( mVariableStreamPosition[variableIndex][timeStepIndex], offset, count );
  else
    return std::vector<double>();
}

void MDAL::SelafinFile::populateDataset( MDAL::Mesh *mesh, std::shared_ptr<MDAL::SelafinFile> reader )
{
  std::map<std::string, std::shared_ptr<DatasetGroup>> groupsByName;
  std::vector< std::shared_ptr<DatasetGroup>> groupsInOrder;

  for ( size_t nName = 0; nName < reader->mVariableNames.size(); ++nName )
  {
    std::string var_name( reader->mVariableNames[nName] );
    var_name = MDAL::toLower( MDAL::trim( var_name ) );
    var_name = MDAL::replace( var_name, "/", "" ); // slash is represented as sub-dataset group but we do not have such subdatasets groups in Selafin

    bool is_vector = false;
    bool is_x = true;

    if ( MDAL::contains( var_name, "velocity u" ) || MDAL::contains( var_name, "along x" ) )
    {
      is_vector = true;
      var_name = MDAL::replace( var_name, "velocity u", "velocity" );
      var_name = MDAL::replace( var_name, "along x", "" );
    }
    else if ( MDAL::contains( var_name, "velocity v" ) || MDAL::contains( var_name, "along y" ) )
    {
      is_vector = true;
      is_x =  false;
      var_name =  MDAL::replace( var_name, "velocity v", "velocity" );
      var_name =  MDAL::replace( var_name, "along y", "" );
    }

    if ( MDAL::contains( var_name, "vitesse u" ) || MDAL::contains( var_name, "suivant x" ) )
    {
      is_vector = true;
      var_name = MDAL::replace( var_name, "vitesse u", "vitesse" );
      var_name = MDAL::replace( var_name, "suivant x", "" );
    }
    else if ( MDAL::contains( var_name, "vitesse v" ) || MDAL::contains( var_name, "suivant y" ) )
    {
      is_vector = true;
      is_x =  false;
      var_name =  MDAL::replace( var_name, "vitesse v", "vitesse" );
      var_name =  MDAL::replace( var_name, "suivant y", "" );
    }

    std::shared_ptr<DatasetGroup> group;
    auto it = groupsByName.find( var_name );
    if ( it == groupsByName.end() )
    {
      group = std::make_shared< DatasetGroup >(
                mesh->driverName(),
                mesh,
                mesh->uri(),
                var_name
              );
      group->setDataLocation( MDAL_DataLocation::DataOnVertices );
      group->setIsScalar( !is_vector );

      groupsInOrder.push_back( group );
      groupsByName[var_name] = group;
    }
    else
      group = it->second;

    group->setReferenceTime( reader->mReferenceTime );

    for ( size_t nT = 0; nT < reader->mTimeSteps.size(); nT++ )
    {
      std::shared_ptr<MDAL::DatasetSelafin> dataset;
      if ( group->datasets.size() > nT )
      {
        dataset = std::dynamic_pointer_cast<DatasetSelafin>( group->datasets[nT] );
      }
      else
      {
        dataset = std::make_shared< DatasetSelafin >( group.get(), reader, nT );
        dataset->setTime( reader->mTimeSteps.at( nT ) );
        group->datasets.push_back( dataset );
      }
      if ( is_vector )
      {
        if ( is_x )
        {
          dataset->setXVariableIndex( nName );
        }
        else
        {
          dataset->setYVariableIndex( nName );
        }
      }
      else
      {
        dataset->setXVariableIndex( nName );
      }

    }
  }

  // now calculate statistics
  for ( const std::shared_ptr<DatasetGroup> &group : groupsInOrder )
  {
    for ( const std::shared_ptr<Dataset> &dataset : group->datasets )
    {
      MDAL::Statistics stats = MDAL::calculateStatistics( dataset );
      dataset->setStatistics( stats );
    }

    MDAL::Statistics stats = MDAL::calculateStatistics( group );
    group->setStatistics( stats );
  }

  // As everything seems to be ok (no exception thrown), push the groups in the mesh
  for ( const std::shared_ptr<DatasetGroup> &group : groupsInOrder )
    mesh->datasetGroups.push_back( group );
}

std::string MDAL::SelafinFile::readString( size_t len )
{
  size_t length = readSizeT();
  if ( length != len ) throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "Unable to read string" );
  std::string ret = readStringWithoutLength( len );
  ignoreArrayLength();
  return ret;
}

std::vector<double> MDAL::SelafinFile::readDoubleArr( size_t len )
{
  size_t length = readSizeT();
  if ( mStreamInFloatPrecision )
  {
    if ( length != len * 4 )
      throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading double array" );
  }
  else
  {
    if ( length != len * 8 )
      throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading double array" );
  }
  std::vector<double> ret( len );
  for ( size_t i = 0; i < len; ++i )
  {
    ret[i] = readDouble();
  }
  ignoreArrayLength();
  return ret;
}

std::vector<double> MDAL::SelafinFile::readDoubleArr( const std::streampos &position, size_t offset, size_t len )
{
  std::vector<double> ret( len );
  std::streamoff off;
  if ( mStreamInFloatPrecision )
    off = offset * 4;
  else
    off = offset * 8;

  mIn.seekg( position + off );
  for ( size_t i = 0; i < len; ++i )
    ret[i] = readDouble();

  return ret;
}

std::vector<int> MDAL::SelafinFile::readIntArr( size_t len )
{
  size_t length = readSizeT();
  if ( length != len * 4 ) throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading int array" );
  std::vector<int> ret( len );
  for ( size_t i = 0; i < len; ++i )
  {
    ret[i] = readInt();
  }
  ignoreArrayLength();
  return ret;
}

std::vector<int> MDAL::SelafinFile::readIntArr( const std::streampos &position, size_t offset, size_t len )
{
  std::vector<int> ret( len );
  std::streamoff off = offset * 4;

  mIn.seekg( position + off );
  for ( size_t i = 0; i < len; ++i )
    ret[i] = readInt();

  return ret;
}

std::string MDAL::SelafinFile::readStringWithoutLength( size_t len )
{
  std::vector<char> ptr( len );
  mIn.read( ptr.data(), static_cast<int>( len ) );
  if ( !mIn )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "Unable to open stream for reading string without length" );

  size_t str_length = 0;
  for ( size_t i = len; i > 0; --i )
  {
    if ( ptr[i - 1] != 0x20 )
    {
      str_length = i;
      break;
    }
  }
  std::string ret( ptr.data(), str_length );
  return ret;
}

double MDAL::SelafinFile::readDouble( )
{
  double ret;

  if ( mStreamInFloatPrecision )
  {
    float ret_f;
    if ( !readValue( ret_f, mIn, mChangeEndianness ) )
      throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "Reading double failed" );
    ret = static_cast<double>( ret_f );
  }
  else
  {
    if ( !readValue( ret, mIn, mChangeEndianness ) )
      throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "Reading double failed" );
  }
  return ret;
}


int MDAL::SelafinFile::readInt( )
{
  unsigned char data[4];

  if ( !mIn.read( reinterpret_cast< char * >( &data ), 4 ) )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "Unable to read int, stream failed" );
  if ( mChangeEndianness )
  {
    std::reverse( reinterpret_cast< char * >( &data ), reinterpret_cast< char * >( &data ) + 4 );
  }
  int var;
  memcpy( reinterpret_cast< char * >( &var ),
          reinterpret_cast< char * >( &data ),
          4 );
  return var;
}

size_t MDAL::SelafinFile::readSizeT()
{
  int var = readInt( );
  return static_cast<size_t>( var );
}

bool MDAL::SelafinFile::checkIntArraySize( size_t len )
{
  return ( len * 4 == readSizeT() );
}

bool MDAL::SelafinFile::checkDoubleArraySize( size_t len )
{
  if ( mStreamInFloatPrecision )
  {
    return ( len * 4 ) == readSizeT();
  }
  else
  {
    return ( len * 8 ) == readSizeT();
  }
}

size_t MDAL::SelafinFile::remainingBytes()
{
  if ( mIn.eof() )
    return 0;
  return static_cast<size_t>( mFileSize - mIn.tellg() );
}

std::streampos MDAL::SelafinFile::passThroughIntArray( size_t size )
{
  std::streampos pos = mIn.tellg();
  mIn.seekg( size * 4, std::ios_base::cur );
  ignoreArrayLength();
  return pos;
}

std::streampos MDAL::SelafinFile::passThroughDoubleArray( size_t size )
{
  std::streampos pos = mIn.tellg();
  if ( mStreamInFloatPrecision )
    size *= 4;
  else
    size *= 8;

  mIn.seekg( size, std::ios_base::cur );
  ignoreArrayLength();
  return pos;
}

void MDAL::SelafinFile::ignore( int len )
{
  mIn.ignore( len );
  if ( !mIn )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "Unable to ignore characters (invalid stream)" );
}

void MDAL::SelafinFile::ignoreArrayLength( )
{
  ignore( 4 );
}

// //////////////////////////////
// DRIVER
// //////////////////////////////

MDAL::DriverSelafin::DriverSelafin():
  Driver( "SELAFIN",
          "Selafin File",
          "*.slf;;*.ser;;*.geo;;*.res",
          Capability::ReadMesh | Capability::SaveMesh | Capability::WriteDatasetsOnVertices | Capability::ReadDatasets
        )
{
}

MDAL::DriverSelafin::~DriverSelafin() = default;

MDAL::DriverSelafin *MDAL::DriverSelafin::create()
{
  return new DriverSelafin();
}

bool MDAL::DriverSelafin::canReadMesh( const std::string &uri )
{
  if ( !MDAL::fileExists( uri ) ) return false;

  try
  {
    SelafinFile file( uri );
    file.parseMeshFrame();
    return true;
  }
  catch ( ... )
  {
    return false;
  }
}

bool MDAL::DriverSelafin::canReadDatasets( const std::string &uri )
{
  if ( !MDAL::fileExists( uri ) ) return false;

  try
  {
    SelafinFile file( uri );
    file.parseMeshFrame();
    return true;
  }
  catch ( ... )
  {
    return false;
  }
}

std::unique_ptr<MDAL::Mesh> MDAL::DriverSelafin::load( const std::string &meshFile, const std::string & )
{
  MDAL::Log::resetLastStatus();
  std::unique_ptr<Mesh> mesh;

  try
  {
    mesh = SelafinFile::createMesh( meshFile );
  }
  catch ( MDAL_Status error )
  {
    MDAL::Log::error( error, name(), "Error while loading file " + meshFile );
    mesh.reset();
  }
  catch ( MDAL::Error &err )
  {
    MDAL::Log::error( err, name() );
    mesh.reset();
  }
  return mesh;
}

void MDAL::DriverSelafin::load( const std::string &datFile, MDAL::Mesh *mesh )
{
  MDAL::Log::resetLastStatus();

  try
  {
    SelafinFile::populateDataset( mesh, datFile );
  }
  catch ( MDAL_Status error )
  {
    MDAL::Log::error( error, name(), "Error while loading dataset in file " + datFile );
  }
  catch ( MDAL::Error &err )
  {
    MDAL::Log::error( err, name() );
  }
}

bool MDAL::DriverSelafin::persist( MDAL::DatasetGroup *group )
{
  if ( !group || ( group->dataLocation() != MDAL_DataLocation::DataOnVertices ) )
  {
    MDAL::Log::error( MDAL_Status::Err_IncompatibleDataset, name(), "Selafin can store only 2D vertices datasets" );
    return true;
  }

  try
  {
    saveDatasetGroupOnFile( group );
    return false;
  }
  catch ( MDAL::Error &err )
  {
    MDAL::Log::error( err, name() );
    return true;
  }
}

bool MDAL::DriverSelafin::saveDatasetGroupOnFile( MDAL::DatasetGroup *datasetGroup )
{
  const std::string &fileName = datasetGroup->uri();

  if ( ! MDAL::fileExists( fileName ) )
  {
    //create a new mesh file
    save( fileName, "", datasetGroup->mesh() );

    if ( ! MDAL::fileExists( fileName ) )
      throw MDAL::Error( MDAL_Status::Err_FailToWriteToDisk, "Unable to create new file" );
  }

  SelafinFile file( fileName );
  return file.addDatasetGroup( datasetGroup );
}

// //////////////////////////////
// MeshSelafin
// //////////////////////////////

MDAL::MeshSelafin::MeshSelafin( const std::string &uri,
                                std::shared_ptr<MDAL::SelafinFile> reader ):
  Mesh( "SELAFIN", reader->verticesPerFace(), uri )
  , mReader( std::move( reader ) )
{}

std::unique_ptr<MDAL::MeshVertexIterator> MDAL::MeshSelafin::readVertices()
{
  return std::unique_ptr<MDAL::MeshVertexIterator>( new MeshSelafinVertexIterator(
           mReader ) );
}

std::unique_ptr<MDAL::MeshEdgeIterator> MDAL::MeshSelafin::readEdges()
{
  return std::unique_ptr<MDAL::MeshEdgeIterator>();
}

std::unique_ptr<MDAL::MeshFaceIterator> MDAL::MeshSelafin::readFaces()
{
  return std::unique_ptr<MDAL::MeshFaceIterator>(
           new MeshSelafinFaceIterator( mReader ) );
}

MDAL::BBox MDAL::MeshSelafin::extent() const
{
  if ( mIsExtentUpToDate )
    return mExtent;
  calculateExtent();
  return mExtent;
}

void MDAL::MeshSelafin::closeSource()
{
  if ( mReader )
  {
    mReader->mIn.close();
    mReader->mParsed = false;
  }
}

void MDAL::MeshSelafin::calculateExtent() const
{
  size_t bufferSize = 1000;
  std::unique_ptr<MeshSelafinVertexIterator> vertexIt(
    new MeshSelafinVertexIterator( mReader ) );
  size_t count = 0;
  BBox bbox;
  std::vector<Vertex> vertices( mReader->verticesCount() );
  size_t index = 0;
  do
  {
    std::vector<double> verticesCoord( bufferSize * 3 );
    count = vertexIt->next( bufferSize, verticesCoord.data() );

    for ( size_t i = 0; i < count; ++i )
    {
      vertices[index + i].x = verticesCoord.at( i * 3 );
      vertices[index + i].y = verticesCoord.at( i * 3 + 1 );
      vertices[index + i].z = verticesCoord.at( i * 3 + 2 );
    }
    index += count;
  }
  while ( count != 0 );

  mExtent = MDAL::computeExtent( vertices );
  mIsExtentUpToDate = true;
}

MDAL::MeshSelafinVertexIterator::MeshSelafinVertexIterator( std::shared_ptr<MDAL::SelafinFile> reader ):
  mReader( std::move( reader ) )
{}

size_t MDAL::MeshSelafinVertexIterator::next( size_t vertexCount, double *coordinates )
{
  size_t count = std::min( vertexCount, mReader->verticesCount() - mPosition );

  if ( count == 0 )
    return 0;

  std::vector<double> coord = mReader->vertices( mPosition, count );

  memcpy( coordinates, coord.data(), count * 24 );

  mPosition += count;

  return count;
}

MDAL::MeshSelafinFaceIterator::MeshSelafinFaceIterator( std::shared_ptr<MDAL::SelafinFile> reader ):
  mReader( std::move( reader ) )
{}

size_t MDAL::MeshSelafinFaceIterator::next( size_t faceOffsetsBufferLen, int *faceOffsetsBuffer, size_t vertexIndicesBufferLen, int *vertexIndicesBuffer )
{
  assert( faceOffsetsBuffer );
  assert( vertexIndicesBuffer );
  assert( mReader->verticesPerFace() != 0 );

  const size_t verticesPerFace = mReader->verticesPerFace();
  size_t count = std::min( faceOffsetsBufferLen, mReader->facesCount() - mPosition );

  count = std::min( count, vertexIndicesBufferLen / verticesPerFace );

  if ( count == 0 )
    return 0;

  std::vector<int> indexes = mReader->connectivityIndex( mPosition * verticesPerFace, count * verticesPerFace );

  if ( indexes.size() != count * verticesPerFace )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading faces" );

  int vertexLocalIndex = 0;

  for ( size_t i = 0; i < count; i++ )
  {
    for ( size_t j = 0; j < verticesPerFace; ++j )
    {
      if ( size_t( indexes[j + i * verticesPerFace] ) > mReader->verticesCount() )
        throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading faces" );
      vertexIndicesBuffer[vertexLocalIndex + j] = indexes[j + i * verticesPerFace] - 1;
    }
    vertexLocalIndex += MDAL::toInt( verticesPerFace );
    faceOffsetsBuffer[i] = vertexLocalIndex;
  }

  mPosition += count;

  return count;

}

MDAL::DatasetSelafin::DatasetSelafin( MDAL::DatasetGroup *parent,
                                      std::shared_ptr<MDAL::SelafinFile> reader, size_t timeStepIndex ):
  Dataset2D( parent )
  , mReader( std::move( reader ) )
  , mTimeStepIndex( timeStepIndex )
{
}

size_t MDAL::DatasetSelafin::scalarData( size_t indexStart, size_t count, double *buffer )
{
  count = std::min( mReader->verticesCount() - indexStart, count );
  std::vector<double> values = mReader->datasetValues( mTimeStepIndex, mXVariableIndex, indexStart, count );
  if ( values.size() != count )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading dataset value" );

  memcpy( buffer, values.data(), count * 8 );

  return count;
}

size_t MDAL::DatasetSelafin::vectorData( size_t indexStart, size_t count, double *buffer )
{
  count = std::min( mReader->verticesCount() - indexStart, count );
  std::vector<double> xValues = mReader->datasetValues( mTimeStepIndex, mXVariableIndex, indexStart, count );
  std::vector<double> yValues = mReader->datasetValues( mTimeStepIndex, mYVariableIndex, indexStart, count );

  if ( xValues.size() != count  || yValues.size() != count )
    throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "File format problem while reading dataset value" );

  for ( size_t i = 0; i < count; ++i )
  {
    buffer[2 * i] = xValues[i];
    buffer[2 * i + 1] = yValues[i];
  }

  return count;
}

void MDAL::DatasetSelafin::setXVariableIndex( size_t index )
{
  mXVariableIndex = index;
}

void MDAL::DatasetSelafin::setYVariableIndex( size_t index )
{
  mYVariableIndex = index;
}

template<typename T>
static void writeValue( std::ofstream &file, T value )
{
  bool isNativeLittleEndian = MDAL::isNativeLittleEndian();
  MDAL::writeValue( value, file, isNativeLittleEndian );
}

static void writeInt( std::ofstream &file, int i )
{
  bool isNativeLittleEndian = MDAL::isNativeLittleEndian();
  MDAL::writeValue( i, file, isNativeLittleEndian );
}

static void writeStringRecord( std::ofstream &file, const std::string &str )
{
  writeInt( file, MDAL::toInt( str.size() ) );
  file.write( str.data(), str.size() );
  writeInt( file, MDAL::toInt( str.size() ) );
}

template<typename T>
static void writeValueArrayRecord( std::ofstream &file, const std::vector<T> &array )
{
  writeValue( file, int( array.size()*sizeof( T ) ) );
  for ( const T value : array )
    writeValue( file, value );
  writeValue( file, int( array.size()*sizeof( T ) ) );
}

template<typename T>
static void writeValueArray( std::ofstream &file, const std::vector<T> &array )
{
  for ( const T value : array )
    writeValue( file, value );
}

// Writes the connectivity record (1-based indices). When `collect` is
// non-null the same indices are appended to it (needed to build IPOBO).
static void writeConnectivityRecord( std::ofstream &file, MDAL::Mesh *mesh,
                                     size_t facesCount, size_t verticesPerFace,
                                     std::vector<int> *collect )
{
  const int recordSize = MDAL::toInt( facesCount * verticesPerFace * 4 );
  writeInt( file, recordSize );
  if ( collect )
    collect->reserve( facesCount * verticesPerFace );
  if ( facesCount > 0 )
  {
    const int bufSize = BUFFER_SIZE;
    std::vector<int> faceOffsetBuffer( bufSize );
    std::vector<int> inkle( bufSize * verticesPerFace );
    std::unique_ptr<MDAL::MeshFaceIterator> faceIter = mesh->readFaces();
    size_t count = 0;
    do
    {
      count = faceIter->next( bufSize, faceOffsetBuffer.data(), bufSize * verticesPerFace, inkle.data() );
      const size_t n = count * verticesPerFace;
      for ( size_t i = 0; i < n; ++i )
      {
        const int node = inkle[i] + 1;  // SELAFIN is 1-based
        writeInt( file, node );
        if ( collect )
          collect->push_back( node );
      }
    }
    while ( count != 0 );
  }
  writeInt( file, recordSize );
}

static void readVerticesXY( MDAL::Mesh *mesh, std::vector<double> &xValues, std::vector<double> &yValues )
{
  const size_t verticesCount = mesh->verticesCount();
  xValues.resize( verticesCount );
  yValues.resize( verticesCount );
  const size_t bufferSize = BUFFER_SIZE;
  std::vector<double> coordinates( bufferSize * 3 );
  std::unique_ptr<MDAL::MeshVertexIterator> vertexIter = mesh->readVertices();
  size_t count = 0;
  size_t vertexIndex = 0;
  do
  {
    count = vertexIter->next( bufferSize, coordinates.data() );
    for ( size_t i = 0; i < count; ++i )
    {
      xValues[vertexIndex + i] = coordinates[i * 3];
      yValues[vertexIndex + i] = coordinates[i * 3 + 1];
    }
    vertexIndex += count;
  }
  while ( count != 0 );
}

// Helpers for computeIPOBO. All operate on 1-based node ids: x[node-1] / y[node-1].

// Shoelace signed area of an OPEN ring; > 0 means counter-clockwise (CCW).
static double ipoboSignedArea( const std::vector<int> &ring,
                               const std::vector<double> &x,
                               const std::vector<double> &y )
{
  double area = 0.0;
  const size_t n = ring.size();
  for ( size_t i = 0; i < n; ++i )
  {
    const int a = ring[i];
    const int b = ring[( i + 1 ) % n];
    area += x[a - 1] * y[b - 1] - x[b - 1] * y[a - 1];
  }
  return 0.5 * area;
}

// Ray-casting point-in-polygon over an OPEN ring.
static bool ipoboPointInPolygon( double px, double py,
                                 const std::vector<int> &ring,
                                 const std::vector<double> &x,
                                 const std::vector<double> &y )
{
  bool inside = false;
  const size_t n = ring.size();
  size_t j = n - 1;
  for ( size_t i = 0; i < n; ++i )
  {
    const double pxi = x[ring[i] - 1];
    const double pyi = y[ring[i] - 1];
    const double pxj = x[ring[j] - 1];
    const double pyj = y[ring[j] - 1];
    // the straddle test guarantees pyi != pyj, so the division is safe
    if ( ( ( pyi > py ) != ( pyj > py ) ) &&
         ( px < ( pxj - pxi ) * ( py - pyi ) / ( pyj - pyi ) + pxi ) )
      inside = !inside;
    j = i;
  }
  return inside;
}

// True when (px,py) lies exactly on a vertex or an edge of the OPEN ring.
static bool ipoboPointOnRing( double px, double py,
                              const std::vector<int> &ring,
                              const std::vector<double> &x,
                              const std::vector<double> &y )
{
  const size_t n = ring.size();
  for ( size_t i = 0; i < n; ++i )
  {
    const double ax = x[ring[i] - 1];
    const double ay = y[ring[i] - 1];
    const double bx = x[ring[( i + 1 ) % n] - 1];
    const double by = y[ring[( i + 1 ) % n] - 1];
    const double cross = ( bx - ax ) * ( py - ay ) - ( by - ay ) * ( px - ax );
    if ( cross != 0.0 )
      continue;
    const double dot = ( px - ax ) * ( bx - ax ) + ( py - ay ) * ( by - ay );
    if ( dot < 0.0 )
      continue;
    if ( dot <= ( bx - ax ) * ( bx - ax ) + ( by - ay ) * ( by - ay ) )
      return true;
  }
  return false;
}

// First node of ringI that does not lie on ringJ, usable as a
// point-in-polygon probe. Superimposed boundary nodes (e.g. zero-width
// weirs) can place ringI's start exactly ON ringJ, where ray casting is
// ill-defined. Returns -1 when every node of ringI lies on ringJ.
static int ipoboRepresentative( const std::vector<int> &ringI,
                                const std::vector<int> &ringJ,
                                const std::vector<double> &x,
                                const std::vector<double> &y )
{
  for ( const int node : ringI )
    if ( !ipoboPointOnRing( x[node - 1], y[node - 1], ringJ, x, y ) )
      return node;
  return -1;
}

// Boundary node with minimum (x+y); ties broken by smallest node id (the map
// is ordered by id, so the first strict minimum wins).
static int ipoboPickSouthwest( const std::map<int, std::set<int>> &neighbours,
                               const std::vector<double> &x,
                               const std::vector<double> &y )
{
  int best = -1;
  double bestXY = std::numeric_limits<double>::max();
  for ( const auto &kv : neighbours )
  {
    const int node = kv.first;
    const double xy = x[node - 1] + y[node - 1];
    if ( xy < bestXY )
    {
      bestXY = xy;
      best = node;
    }
  }
  return best;
}

// Walk one closed loop from `start`, consuming edges from `neighbours`.
// Returns the closed loop [start, ..., start] in `loopOut` and true on
// success; false on a dead end or if `maxSteps` is exceeded.
static bool ipoboWalkOneLoop( std::map<int, std::set<int>> &neighbours,
                              int start,
                              size_t maxSteps,
                              std::vector<int> &loopOut )
{
  loopOut.clear();
  auto eraseEdge = [&neighbours]( int a, int b )
  {
    auto it = neighbours.find( a );
    if ( it != neighbours.end() )
    {
      it->second.erase( b );
      if ( it->second.empty() )
        neighbours.erase( it );
    }
  };

  auto itStart = neighbours.find( start );
  if ( itStart == neighbours.end() || itStart->second.empty() )
    return false;

  loopOut.push_back( start );
  int nxt = *itStart->second.begin();
  eraseEdge( start, nxt );
  eraseEdge( nxt, start );

  size_t steps = 0;
  while ( nxt != start )
  {
    loopOut.push_back( nxt );
    if ( ++steps > maxSteps )
      return false;
    auto it = neighbours.find( nxt );
    if ( it == neighbours.end() || it->second.empty() )
      return false;  // dead end
    const int newNxt = *it->second.begin();
    eraseEdge( nxt, newNxt );
    eraseEdge( newNxt, nxt );
    nxt = newNxt;
  }
  loopOut.push_back( start );  // close the ring
  return true;
}

// Iterative DFS over the containment forest; children[] must already be
// sorted in the desired visit order.
static void ipoboDfs( int root,
                      const std::vector<std::vector<int>> &children,
                      std::vector<int> &orderOut )
{
  std::vector<int> stack( 1, root );
  while ( !stack.empty() )
  {
    const int node = stack.back();
    stack.pop_back();
    orderOut.push_back( node );
    // push in reverse so children pop in ascending key order
    for ( auto it = children[node].rbegin(); it != children[node].rend(); ++it )
      stack.push_back( *it );
  }
}

// Computes the IPOBO array from the mesh connectivity and vertex coordinates:
//   IPOBO[i] = 0 for interior nodes,
//   IPOBO[i] = N (consecutive, starting at 1) for boundary nodes.
//
// Boundary extraction and contour nesting follow the approach of opentelemac's
// pretel/extract_contour.py: boundary edges are the edges used by exactly one
// face, each contour is walked from its south-west node, external contours are
// oriented CCW and holes CW. The consecutive IPOBO numbering itself — depth
// parity for arbitrarily nested contours, containment forest and DFS emission
// order — is original to MDAL. Implemented independently and validated against
// IPOBO arrays produced by TELEMAC tools.
//
// Connectivity indices are 1-based (SELAFIN convention); MDAL writes 2D only.
// Returns a 0-indexed vector sized like x. On degenerate, non-manifold or
// non-triangular input, returns an all-zero vector and warns.
static std::vector<int> computeIPOBO(
  const std::vector<int> &connectivity,  // 1-based vertex indices
  const std::vector<double> &x,
  const std::vector<double> &y,
  size_t verticesPerFace )
{
  const size_t verticesCount = x.size();

  auto failZeros = [verticesCount]() -> std::vector<int>
  {
    MDAL::Log::warning( MDAL_Status::Warn_InvalidElements,
                        "SELAFIN: IPOBO could not be built "
                        "(degenerate/non-manifold/non-triangular mesh); written as zeros" );
    return std::vector<int>( verticesCount, 0 );
  };

  std::vector<int> ipobo( verticesCount, 0 );

  // --- Pre-validation: triangles only, well-formed 1-based connectivity ---
  // computeIPOBO is a free-standing helper; guard against malformed input so
  // x[node-1] / ipobo[node-1] can never read or write out of bounds.
  if ( connectivity.empty() )
    return ipobo;  // no faces — no boundary to build (zeros, no warning)
  if ( verticesPerFace != 3 )
    return failZeros();  // SELAFIN 2D is triangles only
  if ( connectivity.size() % verticesPerFace != 0 || x.size() != y.size() )
    return failZeros();
  for ( const int node : connectivity )
    if ( node < 1 || static_cast<size_t>( node ) > verticesCount )
      return failZeros();

  const size_t facesCount = connectivity.size() / verticesPerFace;

  // --- Canonical edges: sorted packed (min,max) keys ---
  std::vector<long long> edges;
  edges.reserve( connectivity.size() );
  for ( size_t f = 0; f < facesCount; ++f )
  {
    for ( size_t v = 0; v < verticesPerFace; ++v )
    {
      const long long a = connectivity[f * verticesPerFace + v];
      const long long b = connectivity[f * verticesPerFace + ( v + 1 ) % verticesPerFace];
      edges.push_back( ( std::min( a, b ) << 32 ) | std::max( a, b ) );
    }
  }
  std::sort( edges.begin(), edges.end() );

  // --- Boundary adjacency (edges used by exactly one face) ---
  // Ordered map + ordered set: deterministic SW start and *begin() walk choice.
  std::map<int, std::set<int>> neighbours;
  size_t boundaryEdgeCount = 0;
  for ( size_t i = 0; i < edges.size(); )
  {
    size_t j = i + 1;
    while ( j < edges.size() && edges[j] == edges[i] )
      ++j;
    if ( j - i == 1 )
    {
      const int a = static_cast<int>( edges[i] >> 32 );
      const int b = static_cast<int>( edges[i] & 0xffffffffLL );
      neighbours[a].insert( b );
      neighbours[b].insert( a );
      ++boundaryEdgeCount;
    }
    i = j;
  }

  if ( neighbours.empty() )
    return ipobo;  // closed surface / no boundary — zeros, NO warning

  // Manifold invariant: every boundary node must have degree exactly 2, so the
  // boundary decomposes into disjoint simple cycles and the edge-consuming walk
  // can never close a wrong sub-loop. Pinch points break this invariant and
  // are deliberately rejected (zeros + warning) rather than risk a mis-traced
  // boundary.
  for ( const auto &kv : neighbours )
    if ( kv.second.size() != 2 )
      return failZeros();

  // --- Trace every closed boundary loop (consumes `neighbours`) ---
  const size_t maxSteps = boundaryEdgeCount + 1;
  std::vector<std::vector<int>> loops;  // each CLOSED [start..start]
  while ( !neighbours.empty() )
  {
    const int start = ipoboPickSouthwest( neighbours, x, y );
    std::vector<int> loop;
    if ( !ipoboWalkOneLoop( neighbours, start, maxSteps, loop ) )
      return failZeros();
    loops.push_back( std::move( loop ) );
  }

  const size_t nLoops = loops.size();

  // --- Open rings (drop the closing duplicate) ---
  // `rings` keep the WALK orientation and are never reversed, so rings[i][0] is
  // always the south-west start node — the point-in-polygon probe base for the
  // depth and parent tests, and the south-west key of the DFS ordering.
  // (Orientation reverses the CLOSED loops[] used only for numbering, which
  // keeps loops[i][0] == rings[i][0].)
  std::vector<std::vector<int>> rings( nLoops );
  for ( size_t i = 0; i < nLoops; ++i )
    rings[i].assign( loops[i].begin(), loops[i].end() - 1 );

  // --- Nesting depth via point-in-polygon (before any orientation) ---
  std::vector<int> depth( nLoops, 0 );
  for ( size_t i = 0; i < nLoops; ++i )
  {
    for ( size_t j = 0; j < nLoops; ++j )
    {
      if ( i == j )
        continue;
      const int rep = ipoboRepresentative( rings[i], rings[j], x, y );
      if ( rep < 0 )
        return failZeros();  // rings fully superimposed
      if ( ipoboPointInPolygon( x[rep - 1], y[rep - 1], rings[j], x, y ) )
        ++depth[i];
    }
  }

  // --- Orient by depth parity (even = external = CCW; odd = island = CW) ---
  // Reverse the CLOSED loops[i] (numbering direction only); rings[] stay frozen.
  for ( size_t i = 0; i < nLoops; ++i )
  {
    if ( loops[i].size() <= 3 )
      continue;  // unreachable: the degree-2 check makes the minimum cycle 3 nodes (closed size 4); kept as guard
    const bool ccw = ipoboSignedArea( rings[i], x, y ) > 0.0;
    const bool even = ( depth[i] % 2 == 0 );
    if ( ( even && !ccw ) || ( !even && ccw ) )
      std::reverse( loops[i].begin(), loops[i].end() );
  }

  // --- Containment forest (parent = deepest enclosing loop) ---
  // Tie-break: first j in ascending index order at the max qualifying depth.
  std::vector<std::vector<int>> children( nLoops );
  for ( size_t i = 0; i < nLoops; ++i )
  {
    if ( depth[i] == 0 )
      continue;
    int bestParent = -1;
    int bestDepth = -1;
    for ( size_t j = 0; j < nLoops; ++j )
    {
      if ( i == j || depth[j] >= depth[i] )
        continue;
      const int rep = ipoboRepresentative( rings[i], rings[j], x, y );
      if ( rep < 0 )
        return failZeros();
      if ( ipoboPointInPolygon( x[rep - 1], y[rep - 1], rings[j], x, y ) && depth[j] > bestDepth )
      {
        bestDepth = depth[j];
        bestParent = static_cast<int>( j );
      }
    }
    if ( bestParent >= 0 )
      children[bestParent].push_back( static_cast<int>( i ) );
  }

  // --- DFS order; roots (depth 0) and children sorted by SW key ---
  auto lessByKey = [&rings, &x, &y]( int a, int b )
  {
    const int na = rings[a][0];
    const int nb = rings[b][0];
    const double ka = x[na - 1] + y[na - 1];
    const double kb = x[nb - 1] + y[nb - 1];
    if ( ka != kb )
      return ka < kb;
    return na < nb;
  };

  std::vector<int> roots;
  for ( size_t i = 0; i < nLoops; ++i )
    if ( depth[i] == 0 )
      roots.push_back( static_cast<int>( i ) );
  std::sort( roots.begin(), roots.end(), lessByKey );
  for ( auto &kids : children )
    std::sort( kids.begin(), kids.end(), lessByKey );

  std::vector<int> order;
  order.reserve( nLoops );
  for ( const int r : roots )
    ipoboDfs( r, children, order );

  // every loop must be reachable through the forest; a loop with depth > 0 but
  // no qualifying parent would otherwise be silently left unnumbered
  if ( order.size() != nLoops )
    return failZeros();

  // --- Consecutive 1-based numbering in traversal order ---
  // Yield each oriented loop without its closing duplicate (loops[idx][:-1]).
  int id = 0;
  for ( const int idx : order )
    for ( size_t k = 0; k + 1 < loops[idx].size(); ++k )
      ipobo[loops[idx][k] - 1] = ++id;

  return ipobo;
}

static void writeMeshFrame( std::ofstream &file, MDAL::Mesh *mesh )
{
  std::string header( "Selafin file created by MDAL library" );
  std::string remainingStr( 72 - header.size(), ' ' );
  header.append( remainingStr );
  header.append( "SERAFIND" );
  assert( header.size() == 80 );
  writeStringRecord( file, header );

// NBV(1) NBV(2) size
  std::vector<int> nbvSize( 2 );
  nbvSize[0] = 0;
  nbvSize[1] = 0;
  writeValueArrayRecord( file, nbvSize );

  //don't write variable name

  //parameter table, all values are 0
  std::vector<int> param( 10, 0 );
  writeValueArrayRecord( file, param );

  //NELEM,NPOIN,NDP,1
  size_t verticesPerFace = mesh->faceVerticesMaximumCount();
  size_t verticesCount = mesh->verticesCount();
  size_t facesCount = mesh->facesCount();
  std::vector<int> elem( 4 );
  elem[0] = MDAL::toInt( facesCount );
  elem[1] = MDAL::toInt( verticesCount );
  elem[2] = MDAL::toInt( verticesPerFace );
  elem[3] = 1;
  writeValueArrayRecord( file, elem );

  // Reuse the IPOBO from disk when saving a MeshSelafin: its frame cannot be
  // modified through MDAL (Mesh::addVertices/addFaces are no-ops on
  // non-editable meshes), so the source topology always matches. The stored
  // array is reused only when it carries real data: files written by older
  // MDAL versions stored zeros, which are recomputed instead, and an
  // unreadable record simply falls back to the compute path.
  std::vector<int> cachedIpobo;
  if ( auto *meshSlf = dynamic_cast<MDAL::MeshSelafin *>( mesh ) )
  {
    try
    {
      cachedIpobo = meshSlf->ipoboArray();
    }
    catch ( MDAL::Error & )
    {
      cachedIpobo.clear();
    }
  }
  const bool cachedUsable = cachedIpobo.size() == verticesCount &&
  std::any_of( cachedIpobo.begin(), cachedIpobo.end(), []( int v ) { return v != 0; } );

  // Stream the connectivity while writing it; gather a copy only when the
  // IPOBO must be computed from it.
  std::vector<int> connectivity;
  writeConnectivityRecord( file, mesh, facesCount, verticesPerFace,
                           cachedUsable ? nullptr : &connectivity );

  std::vector<double> xValues, yValues;
  readVerticesXY( mesh, xValues, yValues );

  std::vector<int> ipobo;
  if ( cachedUsable )
    ipobo = std::move( cachedIpobo );
  else
    ipobo = computeIPOBO( connectivity, xValues, yValues, verticesPerFace );

  writeValueArrayRecord( file, ipobo );
  writeValueArrayRecord( file, xValues );
  writeValueArrayRecord( file, yValues );
}

void MDAL::DriverSelafin::save( const std::string &fileName, const std::string &, MDAL::Mesh *mesh )
{
  // Write to a temporary file first: a MeshSelafin's frame and IPOBO are read
  // lazily from the source file during the save, which may be fileName itself.
  const std::string tempFileName = fileName + ".tmp";

  try
  {
    std::ofstream file = MDAL::openOutputFile( tempFileName, std::ofstream::binary );
    if ( !file.is_open() )
      throw MDAL::Error( MDAL_Status::Err_FailToWriteToDisk, "Could not open file " + tempFileName );

    writeMeshFrame( file, mesh );
    file.close();

    // a MeshSelafin keeps its source file open; release it before replacing
    // the target in case the mesh is saved onto its own source (the reader
    // reopens lazily on next access)
    mesh->closeSource();

    if ( MDAL::fileExists( fileName ) && !MDAL::deleteFile( fileName ) )
      throw MDAL::Error( MDAL_Status::Err_FailToWriteToDisk, "Unable to replace file " + fileName );
    if ( !MDAL::renameFile( tempFileName, fileName ) )
      throw MDAL::Error( MDAL_Status::Err_FailToWriteToDisk, "Unable to write file " + fileName );
  }
  catch ( MDAL::Error &err )
  {
    MDAL::deleteFile( tempFileName );
    MDAL::Log::error( err, name() );
  }
  catch ( MDAL_Status status )
  {
    MDAL::deleteFile( tempFileName );
    MDAL::Log::error( status, name(), "error occurred while saving mesh frame" );
  }
}

std::string MDAL::DriverSelafin::writeDatasetOnFileSuffix() const
{
  return "slf";
}

std::string MDAL::DriverSelafin::saveMeshOnFileSuffix() const
{
  return "slf";
}

// return false if fails
static void streamToStream( std::ostream &destination,
                            std::ifstream &source,
                            std::streampos sourceStartPosition,
                            std::streamoff len,
                            std::streamoff maxBufferSize )
{
  assert( maxBufferSize != 0 );
  std::streampos position = sourceStartPosition;
  source.seekg( 0, source.end );
  std::streampos end = std::min( source.tellg(), sourceStartPosition + len );
  source.seekg( sourceStartPosition );

  while ( position < end )
  {
    size_t bufferSize = std::min( maxBufferSize, end - position );
    std::vector<char> buffer( bufferSize );
    source.read( &buffer[0], bufferSize );
    destination.write( &buffer[0], bufferSize );
    position += bufferSize;
  }
}

static void writeScalarDataset( std::ofstream &file, MDAL::Dataset *dataset, bool isFloat )
{
  size_t valuesCount = dataset->valuesCount();
  size_t bufferSize = BUFFER_SIZE;
  size_t count = 0;
  size_t indexStart = 0;
  writeInt( file, MDAL::toInt( valuesCount * ( isFloat ? 4 : 8 ) ) );
  do
  {
    std::vector<double> values( bufferSize );
    count = dataset->scalarData( indexStart, bufferSize, values.data() );
    values.resize( count );
    if ( isFloat )
    {
      std::vector<float> floatValues( count );
      for ( int i = 0; i < MDAL::toInt( count ); ++i )
        floatValues[i] = static_cast<float>( values[i] );
      writeValueArray( file, floatValues );
    }
    else
      writeValueArray( file, values );

    indexStart += count;
  }
  while ( count != 0 );
  writeInt( file, MDAL::toInt( valuesCount * ( isFloat ? 4 : 8 ) ) );
}

template<typename T>
static void writeVectorDataset( std::ofstream &file, MDAL::Dataset *dataset )
{
  size_t valuesCount = dataset->valuesCount();

  std::vector<T> valuesX( valuesCount );
  std::vector<T> valuesY( valuesCount );
  size_t bufferSize = BUFFER_SIZE;
  size_t count = 0;
  size_t indexStart = 0;
  do
  {
    std::vector<double> values( bufferSize * 2 );
    count = dataset->vectorData( indexStart, bufferSize, values.data() );
    values.resize( count * 2 );
    for ( int i = 0; i < MDAL::toInt( count ); ++i )
    {
      valuesX[indexStart + i] = static_cast< T >( values[i * 2] );
      valuesY[indexStart + i] = static_cast< T >( values[i * 2 + 1] );
    }
    indexStart += count;
  }
  while ( count != 0 );
  writeValueArrayRecord( file, valuesX );
  writeValueArrayRecord( file, valuesY );
}

bool MDAL::SelafinFile::addDatasetGroup( MDAL::DatasetGroup *datasetGroup )
{
  // Create a new file with same data but with another datasetGroup
  initialize();

  if ( !mIn.is_open() )
    return false;

  parseFile();

  size_t realSize;
  if ( mStreamInFloatPrecision )
    realSize = 4;
  else
    realSize = 8;

  std::string tempFileName = mFileName;
  tempFileName.append( ".tmp" );

  mIn.seekg( 0, mIn.beg );
  std::ofstream out = MDAL::openOutputFile( tempFileName, std::ios_base::binary );
  if ( ! out.is_open() )
    throw MDAL::Error( MDAL_Status::Err_FailToWriteToDisk, "Unable to add dataset in file" );

  //write the same header
  writeStringRecord( out, readHeader() );

  //Read the NBV1//NBV2 size, and add 1 to NBV1
  std::vector<int> nbv = readIntArr( 2 );

  int addedVariable = 1;
  if ( !datasetGroup->isScalar() )
    addedVariable = 2;

  nbv[0] = nbv[0] + addedVariable;
  writeValueArrayRecord( out, nbv );

  // write pre-existing dataset name
  for ( size_t i = 0; i < mVariableNames.size(); ++i )
  {
    std::string variableName = mVariableNames.at( i );
    variableName.resize( 32, ' ' );
    writeStringRecord( out, variableName );
  }

  // write new(s) variable name
  std::string datasetGroupName = datasetGroup->name();
  if ( datasetGroup->isScalar() )
  {
    datasetGroupName.resize( 32, ' ' );
    writeStringRecord( out, datasetGroupName );
  }
  else
  {
    if ( datasetGroupName.size() > 27 )
      datasetGroupName.resize( 27 );
    std::string xName = datasetGroupName + " along x";
    std::string yName = datasetGroupName + " along y";
    xName.resize( 32 );
    yName.resize( 32 );
    writeStringRecord( out, xName );
    writeStringRecord( out, yName );
  }

  //check if valid reference time
  if ( !mReferenceTime.isValid() && datasetGroup->referenceTime().isValid() )
    mReferenceTime = datasetGroup->referenceTime();

  //handlle time step
  if ( mVariableNames.empty() ) //if no variable name, no valid time step before
  {
    mTimeSteps = std::vector<RelativeTimestamp>( datasetGroup->datasets.size() );
    for ( size_t i = 0; i < datasetGroup->datasets.size(); ++i )
    {
      mTimeSteps[i] = RelativeTimestamp( datasetGroup->datasets.at( i )->timestamp() );
    }
  }
  else
  {
    if ( datasetGroup->datasets.size() != mTimeSteps.size() )
      throw MDAL::Error( MDAL_Status::Err_UnknownFormat, "Incompatible dataset group : time steps count are not the same" );
  }

  //parameters table
  mParameters[9] = mReferenceTime.isValid() ?  1 : 0;
  writeValueArrayRecord( out, mParameters );

  if ( mReferenceTime.isValid() )
  {
    writeValueArrayRecord( out, mReferenceTime.expandToCalendarArray() );
  }

  //elems count
  writeValueArrayRecord( out, std::vector<int> {int( mFacesCount ), int( mVerticesCount ), int( mVerticesPerFace ), 1} );

  //IKLE
  writeInt( out, MDAL::toInt( mFacesCount * mVerticesPerFace * 4 ) );
  streamToStream( out, mIn, mConnectivityStreamPosition, mFacesCount * mVerticesPerFace * 4, BUFFER_SIZE );
  writeInt( out, MDAL::toInt( mFacesCount * mVerticesPerFace * 4 ) );
  //vertices

  //IPOBO
  writeInt( out, MDAL::toInt( mVerticesCount * 4 ) );
  streamToStream( out, mIn, mIPOBOStreamPosition, mVerticesCount * 4, BUFFER_SIZE );
  writeInt( out, MDAL::toInt( mVerticesCount * 4 ) );

  //X Vertices
  writeInt( out, MDAL::toInt( mVerticesCount * realSize ) );
  streamToStream( out, mIn, mXStreamPosition, mVerticesCount * realSize, BUFFER_SIZE );
  writeInt( out, MDAL::toInt( mVerticesCount * realSize ) );
  //Y Vertices
  writeInt( out, MDAL::toInt( mVerticesCount * realSize ) );
  streamToStream( out, mIn, mYStreamPosition, mVerticesCount * realSize, BUFFER_SIZE );
  writeInt( out, MDAL::toInt( mVerticesCount * realSize ) );

  // Write datasets
  for ( size_t nT = 0; nT < mTimeSteps.size(); nT++ )
  {
    // Time step
    if ( mStreamInFloatPrecision )
    {
      std::vector<float> time( 1, static_cast<float>( mTimeSteps.at( nT ).value( RelativeTimestamp::seconds ) ) );
      writeValueArrayRecord( out, time );
    }
    else
    {
      std::vector<double> time( 1, mTimeSteps.at( nT ).value( RelativeTimestamp::seconds ) );
      writeValueArrayRecord( out, time );
    }

    // First, prexisting datasets from the original file
    for ( int i = 0; i < nbv[0] - addedVariable; ++i )
    {
      writeInt( out, MDAL::toInt( mVerticesCount * realSize ) );
      streamToStream( out, mIn, mVariableStreamPosition[i][nT], realSize * mVerticesCount, BUFFER_SIZE );
      writeInt( out, MDAL::toInt( mVerticesCount * realSize ) );
    }

    // Then, new datasets from the new dataset group
    Dataset *dataset = datasetGroup->datasets[nT].get();
    if ( datasetGroup->isScalar() )
    {
      writeScalarDataset( out, dataset, mStreamInFloatPrecision );
    }
    else
    {
      if ( mStreamInFloatPrecision )
        writeVectorDataset<float>( out, dataset );
      else
        writeVectorDataset<double>( out, dataset );
    }
  }

  out.close();
  mIn.close();

  // if the uri of the dataset group is the same than the file name, be sure to close it before replace it
  if ( datasetGroup->uri() == mFileName )
    datasetGroup->mesh()->closeSource();

  if ( !MDAL::deleteFile( mFileName ) || !MDAL::renameFile( tempFileName, mFileName ) )
  {
    MDAL::deleteFile( tempFileName );
    throw MDAL::Error( MDAL_Status::Err_FailToWriteToDisk, "Unable to write dataset in file" );
  }

  parseFile();

  return true;
}
