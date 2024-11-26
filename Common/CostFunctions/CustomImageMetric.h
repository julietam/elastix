#ifndef CustomImageMetric_h
#define CustomImageMetric_h

#include <itkAdvancedImageToImageMetric.h>
#include <vector>

template <class TFixedImage, class TMovingImage, class TCoordRep = double>
class CustomImageMetric : public itk::AdvancedImageToImageMetric<TFixedImage, TMovingImage, TCoordRep>
{
public:
  /** Standard class typedefs. */
  typedef CustomImageMetric Self;
  typedef itk::AdvancedImageToImageMetric<TFixedImage, TMovingImage, TCoordRep> Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Method for object creation through the object factory. */
  itkNewMacro(Self);

  // Constructor
  CustomImageMetric() 
  {
   this->m_ImageWeights.clear();  // Initialize the weights vector
  }
  //
  // Method to set the image weights
  void SetImageWeights(const std::vector<double> &weights)
  {
   this->m_ImageWeights = weights;
  }

protected:
  /** Destructor */
  ~CustomImageMetric() override = default;

  /** Method to compute metric value and derivative */
  void GetValueAndDerivative(const typename Superclass::ParametersType &parameters,
                             typename Superclass::MeasureType &value,
                             typename Superclass::DerivativeType &derivative) const override;

  /** Member variable to store the image weights */
  std::vector<double> m_ImageWeights;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "CustomImageMetric.hxx"
#endif

#endif // CustomImageMetric_h

