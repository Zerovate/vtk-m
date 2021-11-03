//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ImageDifference_hxx
#define vtk_m_filter_ImageDifference_hxx

#include <vtkm/filter/ImageProcessing/ImageDifference.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/filter/ImageProcessing/worklet/ImageDifference.h>
#include <vtkm/worklet/AveragePointNeighborhood.h>

namespace vtkm
{
namespace filter
{

namespace detail
{
struct GreaterThanThreshold
{
  GreaterThanThreshold(const vtkm::FloatDefault& thresholdError)
    : ThresholdError(thresholdError)
  {
  }
  VTKM_EXEC_CONT bool operator()(const vtkm::FloatDefault& x) const { return x > ThresholdError; }
  vtkm::FloatDefault ThresholdError;
};
} // namespace detail

VTKM_CONT ImageDifference::ImageDifference()
  : vtkm::filter::FilterField() // FIXME: is this even necessary?
  , AverageRadius(0)
  , PixelShiftRadius(0)
  , AllowedPixelErrorRatio(0.00025f)
  , PixelDiffThreshold(0.05f)
  , ImageDiffWithinThreshold(true)
  , SecondaryFieldName("image-2")
  , SecondaryFieldAssociation(vtkm::cont::Field::Association::ANY)
  , ThresholdFieldName("threshold-output")
{
  this->SetPrimaryField("image-1");
  this->SetOutputFieldName("image-diff");
}

VTKM_CONT vtkm::cont::DataSet ImageDifference::DoExecute(const vtkm::cont::DataSet& input)
{
  this->ImageDiffWithinThreshold = true;
  const auto& primaryField = this->GetFieldFromDataSet(input);
  if (!primaryField.IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }
  const auto primary = vtkm::filter::ApplyPolicyFieldActive(
    primaryField, vtkm::filter::PolicyDefault{}, vtkm::filter::FilterTraits<ImageDifference>());
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Performing Image Difference");

  vtkm::cont::Field secondaryField;
  secondaryField = input.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation);

  const auto policy = vtkm::filter::PolicyDefault{};

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> thresholdOutput;
  vtkm::cont::DataSet clone;
  clone.CopyStructure(input);

  auto ResolveFieldType = [&, this](auto concrete) {
    using T = typename decltype(concrete)::ValueType;
    using StorageType = typename decltype(concrete)::StorageTag;

    // FIXME: why do I have to use ApplyPolicyFieldOfType?
    auto secondary = vtkm::filter::ApplyPolicyFieldOfType<T>(secondaryField, policy, *this);
    // TODO: do we need to use StorageType here as well?
    //    vtkm::cont::UnknownArrayHandle secondary = vtkm::cont::ArrayHandle<T>{};
    //    secondary.CopyShallowIfPossible(secondaryField.GetData());

    auto cellSet = vtkm::filter::ApplyPolicyCellSetStructured(input.GetCellSet(), policy, *this);
    vtkm::cont::ArrayHandle<T, StorageType> diffOutput;
    vtkm::cont::ArrayHandle<T, StorageType> primaryOutput;
    vtkm::cont::ArrayHandle<T, StorageType> secondaryOutput;

    if (this->AverageRadius > 0)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Performing Average with radius: " << this->AverageRadius);
      auto averageWorklet = vtkm::worklet::AveragePointNeighborhood(this->AverageRadius);
      this->Invoke(averageWorklet, cellSet, primary, primaryOutput);
      this->Invoke(averageWorklet, cellSet, secondary, secondaryOutput);
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Not performing average");
      primaryOutput = concrete;
      vtkm::cont::ArrayCopyShallowIfPossible(secondaryField.GetData(), secondaryOutput);
    }

    if (this->PixelShiftRadius > 0)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image in Neighborhood");
      auto diffWorklet = vtkm::worklet::ImageDifferenceNeighborhood(this->PixelShiftRadius,
                                                                    this->PixelDiffThreshold);
      this->Invoke(
        diffWorklet, cellSet, primaryOutput, secondaryOutput, diffOutput, thresholdOutput);
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image directly");
      auto diffWorklet = vtkm::worklet::ImageDifference();
      this->Invoke(diffWorklet, primaryOutput, secondaryOutput, diffOutput, thresholdOutput);
    }

    clone.AddPointField(this->GetOutputFieldName(), diffOutput);
  };

  primary.CastAndCall(ResolveFieldType);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> errorPixels;
  vtkm::cont::Algorithm::CopyIf(thresholdOutput,
                                thresholdOutput,
                                errorPixels,
                                detail::GreaterThanThreshold(this->PixelDiffThreshold));
  if (errorPixels.GetNumberOfValues() >
      thresholdOutput.GetNumberOfValues() * this->AllowedPixelErrorRatio)
  {
    this->ImageDiffWithinThreshold = false;
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Difference within threshold: "
               << this->ImageDiffWithinThreshold
               << ", for pixels outside threshold: " << errorPixels.GetNumberOfValues()
               << ", with a total number of pixesl: " << thresholdOutput.GetNumberOfValues()
               << ", and an allowable pixel error ratio: " << this->AllowedPixelErrorRatio
               << ", with a total summed threshold error: "
               << vtkm::cont::Algorithm::Reduce(errorPixels, static_cast<FloatDefault>(0)));

  //  clone.AddField(fieldMetadata.AsField(this->GetOutputFieldName(), diffOutput));
  //  clone.AddField(fieldMetadata.AsField(this->GetThresholdFieldName(), thresholdOutput));
  clone.AddPointField(this->GetThresholdFieldName(), thresholdOutput);

  VTKM_ASSERT(clone.HasField(this->GetOutputFieldName(), vtkm::cont::Field::Association::POINTS));
  VTKM_ASSERT(
    clone.HasField(this->GetThresholdFieldName(), vtkm::cont::Field::Association::POINTS));

  CallMapFieldOntoOutput(input, clone, vtkm::filter::PolicyDefault{});

  return clone;
}

} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_ImageDifference_hxx
