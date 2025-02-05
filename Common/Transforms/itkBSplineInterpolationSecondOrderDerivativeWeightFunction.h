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
#ifndef itkBSplineInterpolationSecondOrderDerivativeWeightFunction_h
#define itkBSplineInterpolationSecondOrderDerivativeWeightFunction_h

#include "itkBSplineInterpolationWeightFunctionBase.h"
#include <vnl/vnl_vector_fixed.h>

namespace itk
{

/** \class BSplineInterpolationSecondOrderDerivativeWeightFunction
 * \brief Returns the weights over the support region used for B-spline
 * interpolation/reconstruction.
 *
 * Computes/evaluate the B-spline interpolation weights over the
 * support region of the B-spline.
 *
 * This class is templated over the coordinate representation type,
 * the space dimension and the spline order.
 *
 * \sa Point
 * \sa Index
 * \sa ContinuousIndex
 *
 * \ingroup Functions ImageInterpolators
 */

template <typename TCoordinate = float, unsigned int VSpaceDimension = 2, unsigned int VSplineOrder = 3>
class ITK_TEMPLATE_EXPORT BSplineInterpolationSecondOrderDerivativeWeightFunction
  : public BSplineInterpolationWeightFunctionBase<TCoordinate, VSpaceDimension, VSplineOrder>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(BSplineInterpolationSecondOrderDerivativeWeightFunction);

  /** Standard class typedefs. */
  using Self = BSplineInterpolationSecondOrderDerivativeWeightFunction;
  using Superclass = BSplineInterpolationWeightFunctionBase<TCoordinate, VSpaceDimension, VSplineOrder>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** New macro for creation of through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkOverrideGetNameOfClassMacro(BSplineInterpolationSecondOrderDerivativeWeightFunction);

  /** Space dimension. */
  itkStaticConstMacro(SpaceDimension, unsigned int, VSpaceDimension);

  /** Spline order. */
  itkStaticConstMacro(SplineOrder, unsigned int, VSplineOrder);

  /** Typedefs from Superclass. */
  using typename Superclass::WeightsType;
  using typename Superclass::IndexType;
  using typename Superclass::SizeType;
  using typename Superclass::ContinuousIndexType;

  /** Set the second order derivative directions. */
  virtual void
  SetDerivativeDirections(unsigned int dir0, unsigned int dir1);

protected:
  BSplineInterpolationSecondOrderDerivativeWeightFunction();
  ~BSplineInterpolationSecondOrderDerivativeWeightFunction() override = default;

  /** Interpolation kernel types. */
  using typename Superclass::KernelType;
  using typename Superclass::DerivativeKernelType;
  using typename Superclass::SecondOrderDerivativeKernelType;
  using typename Superclass::OneDWeightsType;

  /** Compute the 1D weights, which are:
   * \f[ \beta( x[i] - startIndex[i] ), \beta( x[i] - startIndex[i] - 1 ),
   * \beta( x[i] - startIndex[i] - 2 ), \beta( x[i] - startIndex[i] - 3 ), \f]
   * with \f$\beta( x ) = \beta^2( x + 1/2 ) - \beta^2( x - 1/2 )\f$,
   * in case of non-equal derivative directions,
   * with \f$\beta( x ) = \beta^1( x + 1 ) - 2 * \beta^1( x ) + \beta^1( x - 1 ),\f$
   * in case of equal derivative directions,
   * with \f$\beta(x) = \beta^3(x)\f$ for the non-derivative directions.
   */
  void
  Compute1DWeights(const ContinuousIndexType & index,
                   const IndexType &           startIndex,
                   OneDWeightsType &           weights1D) const override;

  /** Print the member variables. */
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

private:
  vnl_vector_fixed<double, 2> m_DerivativeDirections{};
  bool                        m_EqualDerivativeDirections{};
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkBSplineInterpolationSecondOrderDerivativeWeightFunction.hxx"
#endif

#endif
