//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_AmrDataSet_h
#define vtk_m_cont_AmrDataSet_h
#include <limits>
#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT AmrDataSet : public PartitionedDataSet
{
  using Superclass = vtkm::cont::PartitionedDataSet;

public:
  /// create a new AmrDataSet with the hierarchy structure vector @a partitionIds.
  VTKM_CONT
  explicit AmrDataSet(const std::vector<std::vector<vtkm::Id>> partitionIds);

  VTKM_CONT
  vtkm::Id GetNumberOfLevels() const;

  using Superclass::GetNumberOfPartitions;

  VTKM_CONT
  vtkm::Id GetNumberOfPartitions(const vtkm::Id level) const;

  VTKM_CONT
  vtkm::Id GetStartPartitionId(const vtkm::Id level) const;

  VTKM_CONT
  vtkm::Id GetLevel(const vtkm::Id partitionId) const;

  VTKM_CONT
  vtkm::Id GetBlockId(const vtkm::Id partitionId) const;

  VTKM_CONT
  vtkm::Id GetPartitionId(const vtkm::Id level, const vtkm::Id blockId) const;

  VTKM_CONT
  bool GetParentChildInfoComputed() const;

  VTKM_CONT
  vtkm::Id GetNumberOfParents(const vtkm::Id partitionId) const;

  VTKM_CONT
  vtkm::Id GetNumberOfParents(const vtkm::Id level, const vtkm::Id blockId) const;

  VTKM_CONT
  vtkm::Id GetNumberOfChildren(const vtkm::Id partitionId) const;

  VTKM_CONT
  vtkm::Id GetNumberOfChildren(const vtkm::Id level, const vtkm::Id blockId) const;

  VTKM_CONT
  vtkm::Id GetParentId(const vtkm::Id partitionId, const vtkm::Id parentId) const;

  VTKM_CONT
  vtkm::Id GetParentId(const vtkm::Id level, const vtkm::Id blockId, const vtkm::Id parentId) const;

  VTKM_CONT
  vtkm::Id GetChildId(const vtkm::Id partitionId, const vtkm::Id childId) const;

  VTKM_CONT
  vtkm::Id GetChildId(const vtkm::Id level, const vtkm::Id blockId, const vtkm::Id childId) const;

  using Superclass::GetPartition;

  VTKM_CONT
  const vtkm::cont::DataSet& GetPartition(const vtkm::Id level, const vtkm::Id blockId) const;

  VTKM_CONT
  const vtkm::cont::DataSet& GetParent(const vtkm::Id partitionId, const vtkm::Id parentId) const;

  VTKM_CONT
  const vtkm::cont::DataSet& GetParent(const vtkm::Id level,
                                       const vtkm::Id blockId,
                                       const vtkm::Id parentId) const;

  VTKM_CONT
  const vtkm::cont::DataSet& GetChild(const vtkm::Id partitionId, const vtkm::Id childId) const;

  VTKM_CONT
  const vtkm::cont::DataSet& GetChild(const vtkm::Id level,
                                      const vtkm::Id blockId,
                                      const vtkm::Id childId) const;

  using Superclass::ReplacePartition;

  VTKM_CONT
  void ReplacePartition(const vtkm::Id level,
                        const vtkm::Id blockId,
                        const vtkm::cont::DataSet& ds);

  /// the list of ids contains all amrIds of the level above/below that have an overlap
  VTKM_CONT
  void GenerateParentChildInformation();

  /// the corresponding template function based on the dimension of this dataset
  VTKM_CONT
  template <vtkm::IdComponent Dim>
  void ComputeGenerateParentChildInformation();

  /// generate the vtkGhostType array based on the overlap analogously to vtk
  /// blanked cells: 8 normal cells: 0
  VTKM_CONT
  void GenerateGhostType();

  /// the corresponding template function based on the dimension of this dataset
  VTKM_CONT
  template <vtkm::IdComponent Dim>
  void ComputeGenerateGhostType();

  /// Add helper arrays like in ParaView
  VTKM_CONT
  void GenerateIndexArrays();

  /// do not allow adding of datasets @a ds after construction
  VTKM_CONT
  void AppendPartition(const vtkm::cont::DataSet& ds);

  /// do not allow adding of datasets @a ds after construction
  VTKM_CONT
  void InsertPartition(const vtkm::cont::DataSet& ds);

  /// do not allow adding of datasets @a ds after construction
  VTKM_CONT
  void AppendPartitions(const std::vector<vtkm::cont::DataSet>& partitions);

  VTKM_CONT
  void PrintSummary(std::ostream& stream) const;

private:
  vtkm::Id NumberOfLevels;
  /// per level
  /// helper arrayhandle for PartitionIds
  /// contains the index where the PartitionIds start for each level
  vtkm::cont::ArrayHandle<vtkm::Id> StartPartitionIds;
  /// contains the partitionIds of each level and blockId in a flat storage way at location StartPartitionIndex + blockId
  vtkm::cont::ArrayHandle<vtkm::Id> PartitionIds;

  /// per partitionId
  /// contains the Level of each partitionId
  vtkm::cont::ArrayHandle<vtkm::Id> Level;
  /// contains the BlockId of each partitionId
  vtkm::cont::ArrayHandle<vtkm::Id> BlockId;

  /// helper arrayhandle for ParentsIds
  /// contains the index where the ParentsIds start for each partitionId
  vtkm::cont::ArrayHandle<vtkm::Id> StartParentsIds;
  /// for each partitionId, the list contains all BlockIds of the level above that have an overlap
  /// Warning: this is not the partitionId, but the blockId
  vtkm::cont::ArrayHandle<vtkm::Id> ParentsIds;

  /// helper arrayhandle for ChildrenIds
  /// contains the index where the ChildrenIds start for each partitionId
  vtkm::cont::ArrayHandle<vtkm::Id> StartChildrenIds;
  /// for each partitionId, the list contains all BlockIds of the level below that have an overlap
  /// Warning: this is not the partitionId, but the blockId
  vtkm::cont::ArrayHandle<vtkm::Id> ChildrenIds;

  bool ParentChildInfoComputed = false;
};
}
} // namespace vtkm::cont

#endif
