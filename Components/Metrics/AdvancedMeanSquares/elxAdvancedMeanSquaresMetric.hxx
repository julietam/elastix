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
  // Call superclass implementation
  Superclass1::BeforeEachResolution();

  // Read weight image if specified
  this->ReadWeightImageFromFile();
}

template <typename TElastix>
void
AdvancedMeanSquaresMetric<TElastix>::ReadWeightImageFromFile()
{
  /** Get weight image filename from parameter file. */
  std::string weightImageFileName;
  this->GetConfiguration()->ReadParameter(weightImageFileName,
                                        "MetricWeightImage",
                                        this->GetComponentLabel(),
                                        0);

  if (!weightImageFileName.empty())
  {
    // Verify file extension is .nii
    const std::string extension = weightImageFileName.substr(weightImageFileName.length() - 4);
    if (extension != ".nii")
    {
      xl::xout["error"] << "Error: MetricWeightImage must be a NIfTI (.nii) file, got: " 
                        << weightImageFileName << std::endl;
      return;
    }

    // Read the weight image
    using WeightImageReaderType = itk::ImageFileReader<WeightImageType>;
    auto weightImageReader = WeightImageReaderType::New();
    weightImageReader->SetFileName(weightImageFileName);

    try
    {
      weightImageReader->Update();
      
      // Verify the weight image has same dimensions as fixed image
      WeightImagePointer weightImage = weightImageReader->GetOutput();
      if (weightImage->GetLargestPossibleRegion().GetSize() != 
          this->GetFixedImage()->GetLargestPossibleRegion().GetSize())
      {
        xl::xout["error"] << "Error: Weight image dimensions do not match fixed image dimensions" << std::endl;
        return;
      }

      this->SetImagePairWeights(weightImage);
      xl::xout["standard"] << "Successfully loaded weight image: " << weightImageFileName << std::endl;
    }
    catch (itk::ExceptionObject & err)
    {
      xl::xout["error"] << "Error reading weight image:" << std::endl;
      xl::xout["error"] << err << std::endl;
    }
  }
}

} // end namespace elastix

#endif // end #ifndef elxAdvancedMeanSquaresMetric_hxx
