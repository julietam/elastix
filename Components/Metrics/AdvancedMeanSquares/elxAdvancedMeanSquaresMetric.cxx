/*=========================================================================
 *
 *  Copyright UMC Utrecht and contributors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "elxAdvancedMeanSquaresMetric.h"
#include "elxConfiguration.h"    // for GetConfiguration()
#include "elxMacro.h"            // for elx::xout
#include "itkImageFileReader.h"  // for reading weight images

// This macro installs the AdvancedMeanSquaresMetric into elastix.
elxInstallMacro(AdvancedMeanSquaresMetric);

namespace elastix
{

// Define the methods with the template parameter
template <class TElastix>
int
AdvancedMeanSquaresMetric<TElastix>::ReadParameterFile()
{
  // Call the superclass version if it exists
  int returnValue = Superclass::ReadParameterFile();
  if (returnValue != 0)
  {
    return returnValue;
  }

  // Read the FixedWeightMap parameter
  this->GetConfiguration()->ReadParameter(
    this->m_FixedWeightMapFileName,
    "FixedWeightMap",
    this->GetComponentLabel(),
    0,   // level index if needed
    ""   // default
  );
  if (!this->m_FixedWeightMapFileName.empty())
  {
    elx::xout << "  FixedWeightMap: " << this->m_FixedWeightMapFileName << std::endl;
  }

  // Read the MovingWeightMap parameter
  this->GetConfiguration()->ReadParameter(
    this->m_MovingWeightMapFileName,
    "MovingWeightMap",
    this->GetComponentLabel(),
    0,
    ""
  );
  if (!this->m_MovingWeightMapFileName.empty())
  {
    elx::xout << "  MovingWeightMap: " << this->m_MovingWeightMapFileName << std::endl;
  }

  return 0;
}

template <class TElastix>
int
AdvancedMeanSquaresMetric<TElastix>::BeforeAll()
{
  // Call superclass first if it exists
  Superclass::BeforeAll();

  // If no weight maps are specified, just return
  if (this->m_FixedWeightMapFileName.empty() && this->m_MovingWeightMapFileName.empty())
  {
    return 0;
  }

  // Define typedefs for reading weight maps
  using FixedWeightReaderType = itk::ImageFileReader<FixedImageType>;
  using MovingWeightReaderType = itk::ImageFileReader<MovingImageType>;

  // Load and set the fixed weight map if given
  if (!this->m_FixedWeightMapFileName.empty())
  {
    typename FixedWeightReaderType::Pointer fixedWeightReader = FixedWeightReaderType::New();
    fixedWeightReader->SetFileName(this->m_FixedWeightMapFileName);

    try
    {
      fixedWeightReader->Update();
      this->m_Metric->SetFixedWeightImage(fixedWeightReader->GetOutput());
      elx::xout << "Fixed weight map set in the metric." << std::endl;
    }
    catch (itk::ExceptionObject & err)
    {
      elx::xout << "Error loading fixed weight map: " << err << std::endl;
      return 1;
    }
  }

  // Load and set the moving weight map if given
  if (!this->m_MovingWeightMapFileName.empty())
  {
    typename MovingWeightReaderType::Pointer movingWeightReader = MovingWeightReaderType::New();
    movingWeightReader->SetFileName(this->m_MovingWeightMapFileName);

    try
    {
      movingWeightReader->Update();
      this->m_Metric->SetMovingWeightImage(movingWeightReader->GetOutput());
      elx::xout << "Moving weight map set in the metric." << std::endl;
    }
    catch (itk::ExceptionObject & err)
    {
      elx::xout << "Error loading moving weight map: " << err << std::endl;
      return 1;
    }
  }

  return 0;
}

// Explicit template instantiation if required
// This often depends on how elastix handles templates.
// Check other metric .cxx files for similar patterns.
template class AdvancedMeanSquaresMetric<elastix::ElastixTemplate>;

} // end namespace elastix
