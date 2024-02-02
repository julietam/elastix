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
#ifndef itkImageFullSampler_hxx
#define itkImageFullSampler_hxx

#include "itkImageFullSampler.h"

#include "itkImageRegionConstIteratorWithIndex.h"
#include "elxDeref.h"

#include <algorithm> // For copy_n and min.
#include <cassert>

namespace itk
{

/**
 * ******************* GenerateWorkUnits *******************
 */

template <class TInputImage>
auto
ImageFullSampler<TInputImage>::GenerateWorkUnits(const ThreadIdType             numberOfWorkUnits,
                                                 const InputImageRegionType &   croppedInputImageRegion,
                                                 std::vector<ImageSampleType> & samples) -> std::vector<WorkUnit>
{
  auto * sampleData = samples.data();

  const auto subregions = Superclass::SplitRegion(
    croppedInputImageRegion, std::min(numberOfWorkUnits, MultiThreaderBase::GetGlobalMaximumNumberOfThreads()));

  const auto            numberOfSubregions = subregions.size();
  std::vector<WorkUnit> workUnits{};
  workUnits.reserve(numberOfSubregions);

  // Add a work unit for each subregion.
  for (const auto & subregion : subregions)
  {
    workUnits.push_back({ subregion, sampleData, size_t{} });
    sampleData += subregion.GetNumberOfPixels();
  }
  assert(workUnits.size() <= numberOfSubregions);
  return workUnits;
}


/**
 * ******************* SingleThreadedGenerateData *******************
 */

template <class TInputImage>
void
ImageFullSampler<TInputImage>::SingleThreadedGenerateData(const TInputImage &            inputImage,
                                                          const MaskType * const         mask,
                                                          const InputImageRegionType &   croppedInputImageRegion,
                                                          std::vector<ImageSampleType> & samples)
{
  samples.resize(croppedInputImageRegion.GetNumberOfPixels());
  WorkUnit workUnit{ croppedInputImageRegion, samples.data(), size_t{} };

  if (mask)
  {
    GenerateDataForWorkUnit<true>(workUnit, inputImage, mask, mask->GetObjectToWorldTransformInverse());

    assert(workUnit.NumberOfSamples <= samples.size());
    samples.resize(workUnit.NumberOfSamples);
  }
  else
  {
    GenerateDataForWorkUnit<false>(workUnit, inputImage, nullptr, nullptr);
  }
}

/**
 * ******************* MultiThreadedGenerateData *******************
 */

template <class TInputImage>
void
ImageFullSampler<TInputImage>::MultiThreadedGenerateData(MultiThreaderBase &            multiThreader,
                                                         const ThreadIdType             numberOfWorkUnits,
                                                         const TInputImage &            inputImage,
                                                         const MaskType * const         mask,
                                                         const InputImageRegionType &   croppedInputImageRegion,
                                                         std::vector<ImageSampleType> & samples)
{
  samples.resize(croppedInputImageRegion.GetNumberOfPixels());

  UserData userData{ inputImage,
                     mask,
                     mask ? mask->GetObjectToWorldTransformInverse() : nullptr,
                     GenerateWorkUnits(numberOfWorkUnits, croppedInputImageRegion, samples) };

  multiThreader.SetSingleMethod(mask ? &Self::ThreaderCallback<true> : &Self::ThreaderCallback<false>, &userData);
  multiThreader.SingleMethodExecute();

  if (mask)
  {
    if (auto & workUnits = userData.WorkUnits; !workUnits.empty())
    {
      auto * sampleData = samples.data() + workUnits.front().NumberOfSamples;

      for (size_t i{ 1 }; i < workUnits.size(); ++i)
      {
        const WorkUnit & workUnit = workUnits[i];

        sampleData = std::copy_n(workUnit.Samples, workUnit.NumberOfSamples, sampleData);
      }

      samples.resize(sampleData - samples.data());
    }
  }
}
/**
 * ******************* GenerateData *******************
 */

template <class TInputImage>
void
ImageFullSampler<TInputImage>::GenerateData()
{
  /** Get handles to the input image, output sample container, and the mask. */
  const InputImageType &     inputImage = elastix::Deref(this->GetInput());
  ImageSampleContainerType & sampleContainer = elastix::Deref(this->GetOutput());
  const MaskType * const     mask = this->Superclass::GetMask();

  if (mask)
  {
    mask->UpdateSource();
  }

  // Take capacity from the output container, and clear it.
  std::vector<ImageSampleType> sampleVector;
  sampleContainer.swap(sampleVector);
  sampleVector.clear();

  const auto croppedInputImageRegion = this->GetCroppedInputImageRegion();

  if (Superclass::m_UseMultiThread)
  {
    MultiThreadedGenerateData(elastix::Deref(this->ProcessObject::GetMultiThreader()),
                              ProcessObject::GetNumberOfWorkUnits(),
                              inputImage,
                              mask,
                              croppedInputImageRegion,
                              sampleVector);
  }
  else
  {
    SingleThreadedGenerateData(inputImage, mask, croppedInputImageRegion, sampleVector);
  }
  // Move the samples from the vector into the output container.
  sampleContainer.swap(sampleVector);


} // end GenerateData()


template <class TInputImage>
template <bool VUseMask>
ITK_THREAD_RETURN_FUNCTION_CALL_CONVENTION
ImageFullSampler<TInputImage>::ThreaderCallback(void * const arg)
{
  assert(arg);
  const auto & info = *static_cast<const MultiThreaderBase::WorkUnitInfo *>(arg);

  assert(info.UserData);
  auto & userData = *static_cast<UserData *>(info.UserData);

  const auto workUnitID = info.WorkUnitID;

  if (workUnitID >= userData.WorkUnits.size())
  {
    return ITK_THREAD_RETURN_DEFAULT_VALUE;
  }

  GenerateDataForWorkUnit<VUseMask>(
    userData.WorkUnits[workUnitID], userData.InputImage, userData.Mask, userData.WorldToObjectTransform);

  return ITK_THREAD_RETURN_DEFAULT_VALUE;
}


template <class TInputImage>
template <bool VUseMask>
void
ImageFullSampler<TInputImage>::GenerateDataForWorkUnit(WorkUnit &                               workUnit,
                                                       const InputImageType &                   inputImage,
                                                       const MaskType * const                   mask,
                                                       const WorldToObjectTransformType * const worldToObjectTransform)
{
  assert((mask == nullptr) == (!VUseMask));
  assert((worldToObjectTransform == nullptr) == (!VUseMask));

  auto * samples = workUnit.Samples;

  /** Simply loop over the image and store all samples in the container. */
  for (ImageRegionConstIteratorWithIndex<InputImageType> iter(&inputImage, workUnit.imageRegion); !iter.IsAtEnd();
       ++iter)
  {
    /** Get sampled index */
    InputImageIndexType index = iter.GetIndex();

    // Translate index to point.
    const auto point = inputImage.template TransformIndexToPhysicalPoint<SpacePrecisionType>(index);

    using RealType = typename ImageSampleType::RealType;

    if constexpr (VUseMask)
    {
      // Equivalent to `mask->IsInsideInWorldSpace(point)`, but much faster.
      if (mask->MaskType::IsInsideInObjectSpace(
            worldToObjectTransform->WorldToObjectTransformType::TransformPoint(point)))
      {
        // Store sample in container.
        *samples = { point, static_cast<RealType>(inputImage.GetPixel(index)) };
        ++samples;
      }
    }
    else
    {
      // Store sample in container.
      *samples = { point, static_cast<RealType>(inputImage.GetPixel(index)) };
      ++samples;
    }
  }

  if constexpr (VUseMask)
  {
    workUnit.NumberOfSamples = samples - workUnit.Samples;
  }
}

} // end namespace itk

#endif // end #ifndef itkImageFullSampler_hxx
