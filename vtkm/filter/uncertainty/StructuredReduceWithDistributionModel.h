//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_uncertainty_StructuredReduceWithDistributionModel_h
#define vtk_m_filter_uncertainty_StructuredReduceWithDistributionModel_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/uncertainty/vtkm_filter_uncertainty_export.h>

namespace vtkm
{
namespace filter
{
namespace uncertainty
{

/// @brief Reduce the size of a structured data set and model the distribution of the reduced data.
///
/// This filter reduces the resolution of a `vtkm::cont::DataSet` that contains a
/// `vtkm::cont::CellSetStructured` set of cells. It behaves similarly to the
/// `vtkm::filter::entity_extraction::ExtractStructured` filter with a sample rate
/// set. However, instead of selecting samples, this filter groups blocks of data
/// and models the distribution of the data within these blocks rather than just
/// selecting a single sample from it.
///
/// Each field specified to pass in `vtkm::filter::Filter::SetFieldsToPass()` is processed and
/// the selected distribution models will be generated. Depending on the settings of the
/// suffixes for each parameter of the distribution models, all the fields might be
/// somewhat renamed.
///
/// @bug The coordinates of the output are generally bound to the mean value computed.
/// If applying this filter to a partitioned dataset, gaps will be created between the
/// blocks where mean coordinates get contracted to the center.
///
/// @bug The semantics of blocking with the cells is a bit malformed. A block of points
/// is combined to a single point, and the cells are then recreated across these new
/// points. The new cells sort of stradle over the old cells. Currently, the reduction
/// works by grouping the cells that come from the "lower-left corner" block of the new
/// cells (plus the interface cell). This means that the cell data in the blocks on the
/// opposite boundary get dropped.
class VTKM_FILTER_UNCERTAINTY_EXPORT StructuredReduceWithDistributionModel
  : public vtkm::filter::Filter
{
  vtkm::IdComponent3 BlockSize = { 4, 4, 4 };
  bool GenerateMean = true;
  std::string MeanSuffix = "";
  bool GenerateStandardDeviation = true;
  std::string StandardDeviationSuffix = "_stddev";

public:
  /// @brief Specifies the size of blocks the structured data is partitioned into.
  /// The output structure is reduced by this factor in each dimension.
  VTKM_CONT void SetBlockSize(const vtkm::IdComponent3& blocksize) { this->BlockSize = blocksize; }
  /// @copydoc SetBlockSize
  VTKM_CONT vtkm::IdComponent3 GetBlockSize() const { return this->BlockSize; }

  /// @brief Specifies whether the mean of each block is computed.
  /// The mean is simply the average value of all values in the block, and it is the center
  /// value when representing the data as a Gaussian distribution. This is on by default.
  VTKM_CONT void SetGenerateMean(bool flag) { this->GenerateMean = flag; }
  /// @copydoc SetGenerateMean
  VTKM_CONT bool GetGenerateMean() const { return this->GenerateMean; }

  /// @brief Specifies the suffix appended to the name of mean fields.
  /// Each field specified to pass in `vtkm::filter::Filter::SetFieldsToPass()` will have
  /// its mean computed for each block. The corresponding field in the output will have
  /// the input field's name with this suffix attached to it.
  /// The default suffix is blank, meaning that by default the mean fields will have the
  /// same name as the input fields. This is because the mean field is a good analog for
  /// the original field in filters that do not take uncertainty into account.
  VTKM_CONT void SetMeanSuffix(const std::string& suffix) { this->MeanSuffix = suffix; }
  /// @copydoc SetMeanSuffix
  VTKM_CONT std::string GetMeanSuffix() const { return this->MeanSuffix; }

  /// @brief Specifies whether the standard deviation of each block is computed.
  /// The standard deviation is a measure of the amount of variation of values about its mean.
  /// It is the square root of the variance in a Gaussian distribution, and it is often
  /// represented by a σ. This is on by default.
  VTKM_CONT void SetGenerateStandardDeviation(bool flag) { this->GenerateStandardDeviation = flag; }
  /// @copydoc SetGenerateStandardDeviation
  VTKM_CONT bool GetGenerateStandardDeviation() const { return this->GenerateStandardDeviation; }

  /// @brief Specifies the suffix appended to the name of standard deviation fields.
  /// Each field specified to pass in `vtkm::filter::Filter::SetFieldsToPass()` will have
  /// its standard deviation computed for each block. The corresponding field in the output
  /// will have the input field's name with this suffix attached to it.
  /// The default suffix is `_stddev`.
  VTKM_CONT void SetStandardDeviationSuffix(const std::string& suffix)
  {
    this->StandardDeviationSuffix = suffix;
  }
  /// @copydoc SetStandardDeviationSuffix
  VTKM_CONT std::string GetStandardDeviationSuffix() const { return this->StandardDeviationSuffix; }

  /// @brief Specifies whether the independent Gaussian distribution model is generated.
  /// This is a convenience operation that turns on/off the mean and standard deviation
  /// options together (see `SetGenerateMean()` and `SetGenerateStandardDeviation()`).
  VTKM_CONT void SetGenerateGaussian(bool flag)
  {
    this->SetGenerateMean(flag);
    this->SetGenerateStandardDeviation(flag);
  }

protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& indata) override;
};

}
}
} // namespace vtkm::filter::uncertainty

#endif //vtk_m_filter_uncertainty_StructuredReduceWithDistributionModel_h
