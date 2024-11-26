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
#include <itkDeref.h>
#include <itkImage.h>  // For ImageType
#include <itkImageFileReader.h>  // For reading images

// Declare ImageType (adjust based on your image format).
using ImageType = itk::Image<float, 3>;  // Example: 3D float image
using WeightMatrixType = itk::Image<float, 3>; // Adjust as needed

//
namespace elastix
{

/**
 * ******************* Initialize ***********************
 */

template <typename TElastix>
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
 * ***************** BeforeEachResolution ***********************
 */

template <typename TElastix>
void
AdvancedMeanSquaresMetric<TElastix>::BeforeEachResolution()
{
  const Configuration & configuration = itk::Deref(Superclass2::GetConfiguration());

  
  /** Get the current resolution level. */
  unsigned int level = (this->m_Registration->GetAsITKBaseType())->GetCurrentLevel();
  // Clear weight matrices before loading
  m_WeightMatrices.clear();

  /** Get and set the normalization. */
  bool useNormalization = false;
  configuration.ReadParameter(useNormalization, "UseNormalization", BaseComponent::GetComponentLabel(), level, 0);
  this->SetUseNormalization(useNormalization);
  //NewCodeAdaptation
  // Clear weight matrices before loading
  this->m_WeightMatrices.clear();
  
  // Load the weight matrices
  std::vector<std::string> weightMatrixFilenames;
  std::vector<typename WeightMatrixType::Pointer> m_WeightMatrices;

  // Read weight matrix filenames from the configuration
  if (!configuration.ReadParameter(weightMatrixFilenames, "ImagePairMetricWeights", BaseComponent::GetComponentLabel(), level, 0, true))
  {
    itkExceptionMacro("Weight matrix filenames not found in the configuration for ImagePairMetricWeights.");
  }

  // Ensure at least two weight matrix files are provided
  if (weightMatrixFilenames.size() < 2)
  {
    itkExceptionMacro("Insufficient weight matrix files provided. Expecting at least two files: one for the fixed image and one for the moving image.");
  }  

  //Load the weight matrices
  for (const auto &filename : weightMatrixFilenames)
  {
    try
    {
      using ReaderType = itk::ImageFileReader<ImageType>;
      typename ReaderType::Pointer reader = ReaderType::New();
      reader->SetFileName(filename);
      reader->Update();

      typename ImageType::Pointer weightMatrix = reader->GetOutput();
      this->m_WeightMatrices.push_back(weightMatrix);
    }
    catch (itk::ExceptionObject &err)
    {
      std::cerr << "Error loading weight matrix: " << filename << std::endl;
      std::cerr << err << std::endl;
      throw; // Stop execution if loading fails
    }
  }
  this-> ValidateWeightMatrices();
} // end BeforeEachResolution()

* ***************** ValidateWeightMatrices ***********************

template <typename TElastix>
void
AdvancedMeanSquaresMetric<TElastix>::ValidateWeightMatrices() const
{
  if (this->m_WeightMatrices.empty())
  {
    itkExceptionMacro("No weight matrices loaded.");
  }

  // Check that the size of each weight matrix matches the corresponding image size
  const auto fixedImageRegion = this->GetFixedImage()->GetLargestPossibleRegion();
  const auto movingImageRegion = this->GetMovingImage()->GetLargestPossibleRegion();

  if (this->m_WeightMatrices.size() != 2)
  {
    itkExceptionMacro("Expected exactly two weight matrices: one for the fixed image and one for the moving image.");
  }

  if (this->m_WeightMatrices[0]->GetLargestPossibleRegion() != fixedImageRegion)
  {
    itkExceptionMacro("The fixed weight matrix size does not match the fixed image size.");
  }

  if (this->m_WeightMatrices[1]->GetLargestPossibleRegion() != movingImageRegion)
  {
    itkExceptionMacro("The moving weight matrix size does not match the moving image size.");
  }
} // end ValidateWeightMatrices()


} // end namespace elastix

#endif // end #ifndef elxAdvancedMeanSquaresMetric_hxx
