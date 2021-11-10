//
// Created by ollie on 10/31/21.
//
#if 0
VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Int8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Float32>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Float64>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END
#endif