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
#ifndef elxAdvancedMeanSquaresMetric_hxx
#define elxAdvancedMeanSquaresMetric_hxx

#include "elxAdvancedMeanSquaresMetric.h"
#include "itkTimeProbe.h"

namespace elastix
{

/**
 * ******************* Initialize ***********************
 */

template <class TElastix>
void
AdvancedMeanSquaresMetric<TElastix>::Initialize()
{
  itk::TimeProbe timer;
  timer.Start();
  this->Superclass1::Initialize();
  timer.Stop();
  log::info(std::ostringstream{} << "Initialization of AdvancedMeanSquares metric took: "
                                 << static_cast<long>(timer.GetMean() * 1000) << " ms.");

} // end Initialize()

/**
 * ******************* ReadParameterFile ***********************
 */
template <class TElastix>
int
AdvancedMeanSquaresMetric<TElastix>::ReadParameterFile()
{
  // Call the superclass's ReadParameterFile method
  int returnValue = Superclass1::ReadParameterFile();
  if (returnValue != 0)
  {
    return returnValue; // Propagate the error if superclass reading failed
  }

  // Read the FixedWeightMap parameter
  this->GetConfiguration()->ReadParameter(
    this->m_FixedWeightMapFileName,
    "FixedWeightMap",
    this->GetComponentLabel(),
    0,    // Typically the resolution level index; 0 if not multi-resolution
    ""    // Default value if parameter not found
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
    0,    // Typically the resolution level index; 0 if not multi-resolution
    ""    // Default value if parameter not found
  );

  if (!this->m_MovingWeightMapFileName.empty())
  {
    elx::xout << "  MovingWeightMap: " << this->m_MovingWeightMapFileName << std::endl;
  }

  return 0; // Indicate success
}

/**
 * ******************* BeforeAll ***********************
 */
template <class TElastix>
int
AdvancedMeanSquaresMetric<TElastix>::BeforeAll()
{
  // Call the superclass's BeforeAll method
  int returnValue = Superclass1::BeforeAll();
  if (returnValue != 0)
  {
    return returnValue; // Propagate the error if superclass BeforeAll failed
  }

  // If no weight maps are specified, proceed normally
  if (this->m_FixedWeightMapFileName.empty() && this->m_MovingWeightMapFileName.empty())
  {
    return 0;
  }

  // Define typedefs for reading the weight maps
  typedef itk::ImageFileReader<FixedImageType>  FixedWeightReaderType;
  typedef itk::ImageFileReader<MovingImageType> MovingWeightReaderType;

  // Load and set the FixedWeightImage if specified
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
      return 1; // Indicate an error
    }
  }

  // Load and set the MovingWeightImage if specified
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
      return 1; // Indicate an error
    }
  }

  return 0; // Indicate success
}


/**
 * ***************** BeforeEachResolution ***********************
 */

template <class TElastix>
void
AdvancedMeanSquaresMetric<TElastix>::BeforeEachResolution()
{
  /** Get the current resolution level. */
  unsigned int level = (this->m_Registration->GetAsITKBaseType())->GetCurrentLevel();

  /** Get and set the normalization. */
  bool useNormalization = false;
  this->GetConfiguration()->ReadParameter(useNormalization, "UseNormalization", this->GetComponentLabel(), level, 0);
  this->SetUseNormalization(useNormalization);

  /** Experimental options for SelfHessian */

  /** Set the number of samples used to compute the SelfHessian */
  unsigned int numberOfSamplesForSelfHessian = 100000;
  this->GetConfiguration()->ReadParameter(
    numberOfSamplesForSelfHessian, "NumberOfSamplesForSelfHessian", this->GetComponentLabel(), level, 0);
  this->SetNumberOfSamplesForSelfHessian(numberOfSamplesForSelfHessian);

  /** Set the smoothing sigma used to compute the SelfHessian */
  double selfHessianSmoothingSigma = 1.0;
  this->GetConfiguration()->ReadParameter(
    selfHessianSmoothingSigma, "SelfHessianSmoothingSigma", this->GetComponentLabel(), level, 0);
  this->SetSelfHessianSmoothingSigma(selfHessianSmoothingSigma);

  /** Set the smoothing sigma used to compute the SelfHessian */
  double selfHessianNoiseRange = 1.0;
  this->GetConfiguration()->ReadParameter(
    selfHessianNoiseRange, "SelfHessianNoiseRange", this->GetComponentLabel(), level, 0);
  this->SetSelfHessianNoiseRange(selfHessianNoiseRange);

  /** Select the use of an OpenMP implementation for GetValueAndDerivative. */
  std::string useOpenMP = this->m_Configuration->GetCommandLineArgument("-useOpenMP_SSD");
  if (useOpenMP == "true")
  {
    this->SetUseOpenMP(true);
  }

} // end BeforeEachResolution()


} // end namespace elastix

#endif // end #ifndef elxAdvancedMeanSquaresMetric_hxx
