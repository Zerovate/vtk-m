//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_FieldFilter_h
#define vtk_m_filter_FieldFilter_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

template <class Derived>
class FilterField : public virtual vtkm::filter::Filter<Derived>
{
public:
  VTKM_CONT
  FilterField() = default;

  VTKM_CONT
  ~FilterField() = default;

  VTKM_CONT
  void SetOutputFieldName(const std::string& name) { this->OutputFieldName = name; }

  VTKM_CONT
  const std::string& GetOutputFieldName() const { return this->OutputFieldName; }

  //@{
  /// Choose the field to operate on. Note, if
  /// `this->UseCoordinateSystemAsField` is true, then the active field is not used.
  VTKM_CONT
  void SetActiveField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->ActiveFieldName = name;
    this->ActiveFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetActiveFieldName() const { return this->ActiveFieldName; }
  VTKM_CONT vtkm::cont::Field::Association GetActiveFieldAssociation() const
  {
    return this->ActiveFieldAssociation;
  }
  //@}

  //@{
  /// To simply use the active coordinate system as the field to operate on, set
  /// UseCoordinateSystemAsField to true.
  VTKM_CONT
  void SetUseCoordinateSystemAsField(bool val) { this->UseCoordinateSystemAsField = val; }
  VTKM_CONT
  bool GetUseCoordinateSystemAsField() const { return this->UseCoordinateSystemAsField; }
  //@}

  VTKM_CONT
  const vtkm::cont::Field& GetFieldFromDataSet(const vtkm::cont::DataSet& input) const
  {
    if (this->UseCoordinateSystemAsField)
    {
      return input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
    }
    else
    {
      return input.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    }
  }

protected:
  vtkm::filter::FilterField<Derived>& operator=(const vtkm::filter::FilterField<Derived>&) =
    default;
  VTKM_CONT
  void CopyStateFrom(const FilterField<Derived>* filter) { *this = *filter; }

private:
  std::string OutputFieldName;
  std::string ActiveFieldName;
  vtkm::cont::Field::Association ActiveFieldAssociation = vtkm::cont::Field::Association::ANY;
  bool UseCoordinateSystemAsField = false;

  friend class vtkm::filter::Filter<Derived>;
};
}
} // namespace vtkm::filter

#endif // vtk_m_filter_FieldFilter_h
