/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2024 Nicogodet
*/

#ifndef MDAL_T3S_HPP
#define MDAL_T3S_HPP

#include <string>
#include <memory>

#include "mdal_data_model.hpp"
#include "mdal_memory_data_model.hpp"
#include "mdal.h"
#include "mdal_driver.hpp"

namespace MDAL
{
  /**
   * BlueKenue 2D T3 Scalar Mesh format (*.t3s)
   * ASCII EnSim 1.0 format
   * https://nrc.canada.ca/en/research-development/products-services/software-applications/blue-kenuetm-software-tool-hydraulic-modellers
   */
  class DriverT3S: public Driver
  {
    public:
      DriverT3S();
      ~DriverT3S() override = default;
      DriverT3S *create() override;

      int faceVerticesMaximumCount() const override;

      bool canReadMesh( const std::string &uri ) override;
      std::unique_ptr<Mesh> load( const std::string &meshFile, const std::string &meshName = "" ) override;
      void save( const std::string &fileName, const std::string &meshName, Mesh *mesh ) override;
      std::string saveMeshOnFileSuffix() const override;
  };

} // namespace MDAL
#endif // MDAL_T3S_HPP
