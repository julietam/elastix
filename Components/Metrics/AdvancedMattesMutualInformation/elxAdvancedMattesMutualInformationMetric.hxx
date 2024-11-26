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
#ifndef elxAdvancedMattesMutualInformationMetric_hxx
#define elxAdvancedMattesMutualInformationMetric_hxx

#include "elxAdvancedMattesMutualInformationMetric.h"

#include "itkHardLimiterFunction.h"
#include "itkExponentialLimiterFunction.h"
#include <string>
#include <vnl/vnl_math.h>
#include "itkTimeProbe.h"
#include <itkDeref.h>

namespace elastix
{

/**
 * ****************** Constructor ***********************
 */

template <typename TElastix>
AdvancedMattesMutualInformationMetric<TElastix>::AdvancedMattesMutualInformationMetric()
{
  this->m_CurrentIteration = 0.0;
  this->m_Param_c = 1.0;
  this->m_Param_gamma = 0.101;
  this->SetUseDerivative(true);

} // end Constructor()


/**
 * ******************* Initialize ***********************
 */

template <typename TElastix>
void
AdvancedMattesMutualInformationMetric<TElastix>::Initialize()
{
  itk::TimeProbe timer;
  timer.Start();
  this->Superclass1::Initialize();
  timer.Stop();
  log::info(std::ostringstream{} << "Initialization of AdvancedMattesMutualInformation metric took: "
                                 << static_cast<long>(timer.GetMean() * 1000) << " ms.");

} // end Initialize()


/**
 * ***************** BeforeEachResolution ***********************
 */

template <typename TElastix>
void
AdvancedMattesMutualInformationMetric<TElastix>::BeforeEachResolution()
{
  const Configuration & configuration = itk::Deref(Superclass2::GetConfiguration());
  const std::string     componentLabel = BaseComponent::GetComponentLabel();

  /** Get the current resolution level. */
  unsigned int level = (this->m_Registration->GetAsITKBaseType())->GetCurrentLevel();

  /** Get and set the number of histogram bins. */
  unsigned int numberOfHistogramBins = 32;
  configuration.ReadParameter(numberOfHistogramBins, "NumberOfHistogramBins", componentLabel, level, 0);
  this->SetNumberOfFixedHistogramBins(numberOfHistogramBins);
  this->SetNumberOfMovingHistogramBins(numberOfHistogramBins);

  unsigned int numberOfFixedHistogramBins = numberOfHistogramBins;
  unsigned int numberOfMovingHistogramBins = numberOfHistogramBins;
  configuration.ReadParameter(numberOfFixedHistogramBins, "NumberOfFixedHistogramBins", componentLabel, level, 0);
  configuration.ReadParameter(numberOfMovingHistogramBins, "NumberOfMovingHistogramBins", componentLabel, level, 0);
  this->SetNumberOfFixedHistogramBins(numberOfFixedHistogramBins);
  this->SetNumberOfMovingHistogramBins(numberOfMovingHistogramBins);

  /** Set limiters. */
  using FixedLimiterType = itk::HardLimiterFunction<RealType, FixedImageDimension>;
  using MovingLimiterType = itk::ExponentialLimiterFunction<RealType, MovingImageDimension>;
  this->SetFixedImageLimiter(FixedLimiterType::New());
  this->SetMovingImageLimiter(MovingLimiterType::New());

  /** Get and set the limit range ratios. */
  double fixedLimitRangeRatio = 0.01;
  double movingLimitRangeRatio = 0.01;
  configuration.ReadParameter(fixedLimitRangeRatio, "FixedLimitRangeRatio", componentLabel, level, 0);
  configuration.ReadParameter(movingLimitRangeRatio, "MovingLimitRangeRatio", componentLabel, level, 0);
  this->SetFixedLimitRangeRatio(fixedLimitRangeRatio);
  this->SetMovingLimitRangeRatio(movingLimitRangeRatio);

  /** Set B-spline Parzen kernel orders. */
  unsigned int fixedKernelBSplineOrder = 0;
  unsigned int movingKernelBSplineOrder = 3;
  configuration.ReadParameter(fixedKernelBSplineOrder, "FixedKernelBSplineOrder", componentLabel, level, 0);
  configuration.ReadParameter(movingKernelBSplineOrder, "MovingKernelBSplineOrder", componentLabel, level, 0);
  this->SetFixedKernelBSplineOrder(fixedKernelBSplineOrder);
  this->SetMovingKernelBSplineOrder(movingKernelBSplineOrder);

  /** Set whether a low memory consumption should be used. */
  bool useFastAndLowMemoryVersion = true;
  configuration.ReadParameter(useFastAndLowMemoryVersion, "UseFastAndLowMemoryVersion", componentLabel, level, 0);
  this->SetUseExplicitPDFDerivatives(!useFastAndLowMemoryVersion);

  /** Set whether to use Nick Tustison's preconditioning technique. */
  bool useJacobianPreconditioning = false;
  configuration.ReadParameter(useJacobianPreconditioning, "UseJacobianPreconditioning", componentLabel, level, 0);
  this->SetUseJacobianPreconditioning(useJacobianPreconditioning);

  /** Set whether a finite difference derivative should be used. */
  bool useFiniteDifferenceDerivative = false;
  configuration.ReadParameter(useFiniteDifferenceDerivative, "FiniteDifferenceDerivative", componentLabel, level, 0);
  this->SetUseFiniteDifferenceDerivative(useFiniteDifferenceDerivative);

  /** Prepare for computing the perturbation gain c_k. */
  this->SetCurrentIteration(0);
  if (useFiniteDifferenceDerivative)
  {
    double c = 1.0;
    double gamma = 0.101;
    configuration.ReadParameter(c, "SP_c", componentLabel, level, 0);
    configuration.ReadParameter(gamma, "SP_gamma", componentLabel, level, 0);
    this->SetParam_c(c);
    this->SetParam_gamma(gamma);
    this->SetFiniteDifferencePerturbation(this->Compute_c(0));
  }
// ---------------------------
//     // New code: Load weight matrices
//         // ---------------------------
//
//             // Step 1: Read the weight matrix filenames from the parameter file.
  std::vector<std::string> weightMatrixFilenames;
  configuration.ReadParameter(weightMatrixFilenames, "ImagePairMetricWeights", BaseComponent::GetComponentLabel(), level,0,true);
  std::vector<ImageType::Pointer> weightMatrices;
    for (const auto& filename : weightMatrixFilenames) {
        try {
            typedef itk::ImageFileReader<ImageType> ReaderType;
            ReaderType::Pointer reader = ReaderType::New();
            reader->SetFileName(filename);
            reader->Update();
            ImageType::Pointer weightMatrix = reader->GetOutput();
            weightMatrices.push_back(weightMatrix);
        }
        catch (itk::ExceptionObject& err) {
            std::cerr << "Error loading weight matrix: " << filename << std::endl;
            std::cerr << err << std::endl;
            throw;  // Halt registration if loading fails.
        }
    }
    this->m_WeightMatrices = weightMatrices;
    if (this->m_WeightMatrices.empty())
    {
        itkExceptionMacro("Weight matrices are not initialized. Ensure the parameter file specifies valid matrices.");
    }
    
    

{

template <typename TElastix>
void
AdvancedMattesMutualInformationMetric<TElastix>::ComputePDFs(
  const ParametersType &parameters,
  const JointPDFPointer &weightMatrixFixed,
  const JointPDFPointer &weightMatrixMoving)
  {
    this->SetTransformParameters(parameters);

  // Validate weight matrices
    this->ValidateWeightMatrices();
  //
  //     // Compute PDF contributions with weights
    for (const auto &sample : *this->GetImageSampler()->GetOutput())
   {
      JointPDFIndexType fixedIndex;
      JointPDFIndexType movingIndex;
  //
  //                     // Calculate bin indices for fixed and moving images
      double fixedValue = static_cast<double>(sample.m_FixedValue);
      double movingValue = static_cast<double>(sample.m_MovingValue);
  //
      fixedIndex[0] = static_cast<OffsetValueType>(std::floor(fixedValue / this->m_FixedImageBinSize));
      movingIndex[1] = static_cast<OffsetValueType>(std::floor(movingValue / this->m_MovingImageBinSize));
  //
  //                                         // Apply weights and update joint PDF
      this->UpdateJointPDFAndDerivatives(
        fixedValue * weightMatrixFixed->GetPixel(fixedIndex),
        movingValue * weightMatrixMoving->GetPixel(movingIndex),
        nullptr, nullptr, this->m_JointPDF.GetPointer());
   }
  //
  //                                                                   // Normalize PDF if required
  this->NormalizeJointPDF(this->m_JointPDF, this->m_Alpha);
 }

} // end namespace elastix
 

} // end BeforeEachResolution()


/**
 * ***************** AfterEachIteration ***********************
 */

template <typename TElastix>
void
AdvancedMattesMutualInformationMetric<TElastix>::AfterEachIteration()
{
  if (this->GetUseFiniteDifferenceDerivative())
  {
    this->m_CurrentIteration++;
    this->SetFiniteDifferencePerturbation(this->Compute_c(this->m_CurrentIteration));
  }
} // end AfterEachIteration()


/**
 * ************************** Compute_c *************************
 */

template <typename TElastix>
double
AdvancedMattesMutualInformationMetric<TElastix>::Compute_c(unsigned long k) const
{
  return static_cast<double>(this->m_Param_c / std::pow(k + 1, this->m_Param_gamma));

} // end Compute_c()


} // end namespace elastix

#endif // end #ifndef elxAdvancedMattesMutualInformationMetric_hxx
