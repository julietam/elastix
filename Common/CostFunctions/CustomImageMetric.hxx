#ifndef CustomImageMetric_hxx
#define CustomImageMetric_hxx

#include "CustomImageMetric.h"

namespace itk
{

template <class TFixedImage, class TMovingImage, class TCoordRep>
void
CustomImageMetric<TFixedImage, TMovingImage, TCoordRep>::GetValueAndDerivative(
  const ParametersType & parameters,
  MeasureType &          value,
  DerivativeType &       derivative) const
{
    MeasureType totalValue = 0.0;
    DerivativeType totalDerivative;
    totalDerivative.SetSize(parameters.Size());
    totalDerivative.Fill(0.0);

    // Check if image weights have been provided
    if (this->m_ImageWeights.size() != this->GetNumberOfImages())
    {
    itkExceptionMacro("The number of image weights does not match the number of images.");
    }
    //
    // Loop over each image and apply the corresponding weight
    for (size_t i = 0; i < this->m_ImageWeights.size(); ++i)
    {
       MeasureType value_i;
       DerivativeType derivative_i;

      // Compute value and derivative for each image
       this->ComputeValueAndDerivativeForImage(i, parameters, value_i, derivative_i);
   
      // Apply the image weight to the value and derivative
       double imageWeight = this->m_ImageWeights[i];
       totalValue += imageWeight * value_i;
       totalDerivative += imageWeight * derivative_i;
     }
    
      // Set the final value and derivative
value = totalValue;
derivative = totalDerivative;
}
    //
    //
template <class TFixedImage, class TMovingImage, class TCoordRep>
void
CustomImageMetric<TFixedImage, TMovingImage, TCoordRep>::Initialize()
{
    Superclass::Initialize();  // Call superclass Initialize()
    //
    std::vector<std::string> imageWeightStrings;
    if (this->GetOptimizerParameter("ImageWeights", imageWeightStrings))
    {
      this->m_ImageWeights.clear();
      for (const auto & weightStr : imageWeightStrings)
      {
        this->m_ImageWeights.push_back(std::stod(weightStr));  // Convert weight string to double
      }
    }
    else
    {
    // If no weights are provided, default all weights to 1.0
    this->m_ImageWeights.resize(this->GetNumberOfImages(), 1.0);
    }
    //
    if (this->m_ImageWeights.size() != this->GetNumberOfImages())
    {
      itkExceptionMacro("The number of weights provided does not match the number of images.");
    }
template <class TFixedImage, class TMovingImage, class TCoordRep>
void
CustomImageMetric<TFixedImage, TMovingImage, TCoordRep>::SetImageWeights(const std::vector<double> &weights)
{
  if (weights.size() != this->GetNumberOfImages())
  {
      itkExceptionMacro("The number of weights does not match the number of images.");
  }
  this->m_ImageWeights = weights;
}
} // end namespace itk
#endif
