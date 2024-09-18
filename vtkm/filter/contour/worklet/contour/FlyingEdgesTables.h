//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_contour_flyingedges_tables_h
#define vtk_m_worklet_contour_flyingedges_tables_h

#include <vtkm/filter/contour/worklet/contour/FlyingEdgesHelpers.h>

#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace flying_edges
{
namespace data
{

VTKM_CONT vtkm::cont::ArrayHandleBasic<vtkm::UInt8> GetNumberOfPrimitivesTable();

VTKM_CONT vtkm::cont::ArrayHandleBasic<vtkm::Vec<vtkm::UInt8, 12>> GetEdgeUsesTable();

VTKM_CONT vtkm::cont::ArrayHandleBasic<vtkm::Vec<vtkm::UInt8, 16>> GetTriEdgeCasesTable();

VTKM_CONT vtkm::cont::ArrayHandleBasic<vtkm::Vec2ui_8> GetVertMapTable();

VTKM_CONT vtkm::cont::ArrayHandleBasic<vtkm::Id3> GetVertOffsetsXAxisTable();
VTKM_CONT vtkm::cont::ArrayHandleBasic<vtkm::Id3> GetVertOffsetsYAxisTable();

class FlyingEdgesTables
{
  template <typename ComponentType>
  using PortalType = typename vtkm::cont::ArrayHandle<ComponentType>::ReadPortalType;
  PortalType<vtkm::UInt8> NumberOfPrimitivesTable;
  PortalType<vtkm::Vec<vtkm::UInt8, 12>> EdgeUsesTable;
  PortalType<vtkm::Vec<vtkm::UInt8, 16>> TriEdgesCaseTable;
  PortalType<vtkm::Vec2ui_8> VertMapTable;
  PortalType<vtkm::Id3> VertOffsetsXAxisTable;
  PortalType<vtkm::Id3> VertOffsetsYAxisTable;

public:
  VTKM_CONT FlyingEdgesTables(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token& token);

  VTKM_EXEC vtkm::UInt8 GetNumberOfPrimitives(vtkm::UInt8 edgecase) const
  {
    return this->NumberOfPrimitivesTable.Get(edgecase);
  }
  VTKM_EXEC const vtkm::Vec<vtkm::UInt8, 12>& GetEdgeUses(vtkm::UInt8 edgecase) const
  {
    // Get the pointer to return a reference to make sure we are not copying all values.
    const vtkm::UInt8 index = (edgecase < 128) ? edgecase : (255 - edgecase);
    return *(this->EdgeUsesTable.GetIteratorBegin() + index);
  }
  VTKM_EXEC const vtkm::Vec<vtkm::UInt8, 16>& GetTriEdgeCases(vtkm::UInt8 edgecase) const
  {
    // Get the pointer to return a reference to make sure we are not copying all values.
    return *(this->TriEdgesCaseTable.GetIteratorBegin() + edgecase);
  }
  VTKM_EXEC vtkm::Vec2ui_8 GetVertMap(vtkm::Id index) const
  {
    return this->VertMapTable.Get(index);
  }
  VTKM_EXEC vtkm::Id3 GetVertOffsets(SumXAxis, vtkm::UInt8 index) const
  {
    return this->VertOffsetsXAxisTable.Get(index);
  }
  VTKM_EXEC vtkm::Id3 GetVertOffsets(SumYAxis, vtkm::UInt8 index) const
  {
    return this->VertOffsetsXAxisTable.Get(index);
  }
};

struct FlyingEdgesTablesExecObject : vtkm::cont::ExecutionObjectBase
{
  VTKM_CONT FlyingEdgesTables PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                  vtkm::cont::Token& token) const
  {
    return FlyingEdgesTables{ device, token };
  }
};

} // namespace data
}
}
}
#endif
