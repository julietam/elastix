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
#include "elxConfiguration.h"           // For GetConfiguration()
#include "itkImageFileReader.h"         // For reading weight images
#include "elxMacro.h"                   // For elx::xout

// This macro installs the AdvancedMeanSquaresMetric into elastix.
elxInstallMacro(AdvancedMeanSquaresMetric);

namespace elastix
{

int
AdvancedMeanSquaresMetric::ReadParameterFile()
{
  // First call the superclass method
  int returnValue = Superclass::ReadParameterFile();
  if (returnValue != 0)
  {
    return returnValue; // If superclass reading failed, propagate error.
  }

  // Read the FixedWeightMap parameter
  this->GetConfiguration()->ReadParameter(
    this->m_FixedWeightMapFileName,
    "FixedWeightMap",
    this->GetComponentLabel(),
    0,  // Usually the resolution level index is given here; 0 if single-level
    ""  // Default value if parameter not found
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


int
AdvancedMeanSquaresMetric::BeforeAll()
{
  // Call superclass first
  Superclass::BeforeAll();

  // If no filenames were read, just proceed normally.
  if (this->m_FixedWeightMapFileName.empty() && this->m_MovingWeightMapFileName.empty())
  {
    return 0;
  }

  // Define typedefs for reading the weight images
  typedef itk::ImageFileReader<FixedImageType>  FixedWeightReaderType;
  typedef itk::ImageFileReader<MovingImageType> MovingWeightReaderType;

  // Load and set the fixed weight image if specified
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

  // Load and set the moving weight image if specified
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

} // end namespace elastix

