//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/cont/AmrDataSet.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{
template <vtkm::IdComponent Dim>
struct GenerateGhostTypeWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInPoint pointArray,
                                FieldInOutCell ghostArray);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  GenerateGhostTypeWorklet(vtkm::Bounds boundsChild)
    : BoundsChild(boundsChild)
  {
  }

  template <typename pointArrayType, typename cellArrayType>
  VTKM_EXEC void operator()(vtkm::IdComponent numPoints,
                            const pointArrayType pointArray,
                            cellArrayType& ghostArray) const
  {
    vtkm::Bounds boundsCell = vtkm::Bounds();
    for (vtkm::IdComponent pointId = 0; pointId < numPoints; pointId++)
    {
      boundsCell.Include(pointArray[pointId]);
    }
    vtkm::Bounds boundsIntersection = boundsCell.Intersection(BoundsChild);
    if ((Dim == 2 && boundsIntersection.Area() > 0) ||
        (Dim == 3 && boundsIntersection.Volume() > 0))
    {
      ////            std::cout<<cellId<<" is (partly) contained in level "<<l + 1<<" block  "<<this->GetChildrenIds(l, b).at(childId)<<" "<<boundsCell<<" "<<boundsChild<<" "<<boundsIntersection<<" "<<boundsIntersection.Area()<<std::endl;
      ghostArray = 8;
    }
  }

  vtkm::Bounds BoundsChild;
};
} // worklet
} // vtkm


namespace vtkm
{
namespace cont
{

VTKM_CONT
AmrDataSet::AmrDataSet(const std::vector<std::vector<vtkm::Id>> partitionIds)
{
  this->NumberOfLevels = partitionIds.size();
  this->StartPartitionIds.Allocate(this->NumberOfLevels);
  vtkm::Id numberOfPartitions = 0;

  for (vtkm::Id l = 0; l < this->NumberOfLevels; l++)
  {
    this->StartPartitionIds.WritePortal().Set(l, numberOfPartitions);
    numberOfPartitions += partitionIds.at(l).size();
  }

  this->PartitionIds.Allocate(numberOfPartitions);
  this->Level.Allocate(numberOfPartitions);
  this->BlockId.Allocate(numberOfPartitions);
  for (vtkm::Id l = 0; l < this->NumberOfLevels; l++)
  {
    for (vtkm::Id b = 0; b < this->GetNumberOfPartitions(l); b++)
    {
      this->Level.WritePortal().Set(partitionIds.at(l).at(b), l);
      this->BlockId.WritePortal().Set(partitionIds.at(l).at(b), b);
      this->PartitionIds.WritePortal().Set(this->GetStartPartitionId(l) + b,
                                           partitionIds.at(l).at(b));
    }
  }

  Superclass::AppendPartitions(std::vector<vtkm::cont::DataSet>(numberOfPartitions));
}

VTKM_CONT
vtkm::Id AmrDataSet::GetNumberOfLevels() const
{
  return this->NumberOfLevels;
}

VTKM_CONT
vtkm::Id AmrDataSet::GetNumberOfPartitions(vtkm::Id level) const
{
  if (level == this->GetNumberOfLevels() - 1)
  {
    return this->PartitionIds.GetNumberOfValues() - this->StartPartitionIds.ReadPortal().Get(level);
  }
  return this->StartPartitionIds.ReadPortal().Get(level + 1) -
    this->StartPartitionIds.ReadPortal().Get(level);
}

VTKM_CONT
vtkm::Id AmrDataSet::GetStartPartitionId(vtkm::Id level) const
{
  return this->StartPartitionIds.ReadPortal().Get(level);
}

VTKM_CONT
vtkm::Id AmrDataSet::GetLevel(vtkm::Id partitionId) const
{
  return this->Level.ReadPortal().Get(static_cast<std::size_t>(partitionId));
}

VTKM_CONT
vtkm::Id AmrDataSet::GetBlockId(vtkm::Id partitionId) const
{
  return this->BlockId.ReadPortal().Get(static_cast<std::size_t>(partitionId));
}

VTKM_CONT
vtkm::Id AmrDataSet::GetPartitionId(vtkm::Id level, vtkm::Id blockId) const
{
  return this->PartitionIds.ReadPortal().Get(GetStartPartitionId(level) + blockId);
}

VTKM_CONT
bool AmrDataSet::GetParentChildInfoComputed() const
{
  return ParentChildInfoComputed;
}

VTKM_CONT
vtkm::Id AmrDataSet::GetNumberOfParents(vtkm::Id partitionId) const
{
  if (partitionId == this->GetNumberOfPartitions() - 1)
  {
    return this->ParentsIds.GetNumberOfValues() -
      this->StartParentsIds.ReadPortal().Get(partitionId);
  }
  return this->StartParentsIds.ReadPortal().Get(partitionId + 1) -
    this->StartParentsIds.ReadPortal().Get(partitionId);
}

VTKM_CONT
vtkm::Id AmrDataSet::GetNumberOfParents(vtkm::Id level, vtkm::Id blockId) const
{
  return this->GetNumberOfParents(this->GetPartitionId(level, blockId));
}

VTKM_CONT
vtkm::Id AmrDataSet::GetNumberOfChildren(vtkm::Id partitionId) const
{
  if (partitionId == this->GetNumberOfPartitions() - 1)
  {
    return this->ChildrenIds.GetNumberOfValues() -
      this->StartChildrenIds.ReadPortal().Get(partitionId);
  }
  return this->StartChildrenIds.ReadPortal().Get(partitionId + 1) -
    this->StartChildrenIds.ReadPortal().Get(partitionId);
}

VTKM_CONT
vtkm::Id AmrDataSet::GetNumberOfChildren(vtkm::Id level, vtkm::Id blockId) const
{
  return this->GetNumberOfChildren(this->GetPartitionId(level, blockId));
}

VTKM_CONT
vtkm::Id AmrDataSet::GetParentId(vtkm::Id partitionId, vtkm::Id parentId) const
{
  assert(ParentChildInfoComputed);
  assert(parentId < this->GetNumberOfParents(partitionId));
  return (this->ParentsIds.ReadPortal().Get(this->StartParentsIds.ReadPortal().Get(partitionId) +
                                            parentId));
}

VTKM_CONT
vtkm::Id AmrDataSet::GetParentId(vtkm::Id level, vtkm::Id blockId, vtkm::Id parentId) const
{
  return this->GetParentId(this->GetPartitionId(level, blockId), parentId);
}

VTKM_CONT
vtkm::Id AmrDataSet::GetChildId(vtkm::Id partitionId, vtkm::Id childId) const
{
  assert(ParentChildInfoComputed);
  assert(childId < this->GetNumberOfChildren(partitionId));
  return (this->ChildrenIds.ReadPortal().Get(this->StartChildrenIds.ReadPortal().Get(partitionId) +
                                             childId));
}

VTKM_CONT
vtkm::Id AmrDataSet::GetChildId(vtkm::Id level, vtkm::Id blockId, vtkm::Id childId) const
{
  return this->GetChildId(this->GetPartitionId(level, blockId), childId);
}

VTKM_CONT
const vtkm::cont::DataSet& AmrDataSet::GetPartition(vtkm::Id level, vtkm::Id blockId) const
{
  return Superclass::GetPartition(GetPartitionId(level, blockId));
}

VTKM_CONT
const vtkm::cont::DataSet& AmrDataSet::GetParent(vtkm::Id partitionId, vtkm::Id parentId) const
{
  return this->GetPartition(this->GetLevel(partitionId) - 1,
                            this->GetParentId(partitionId, parentId));
}

VTKM_CONT
const vtkm::cont::DataSet& AmrDataSet::GetParent(vtkm::Id level,
                                                 vtkm::Id blockId,
                                                 vtkm::Id parentId) const
{
  return this->GetPartition(level - 1, this->GetParentId(level, blockId, parentId));
}

VTKM_CONT
const vtkm::cont::DataSet& AmrDataSet::GetChild(vtkm::Id partitionId, vtkm::Id childId) const
{
  return this->GetPartition(this->GetLevel(partitionId) + 1,
                            this->GetChildId(partitionId, childId));
}

VTKM_CONT
const vtkm::cont::DataSet& AmrDataSet::GetChild(vtkm::Id level,
                                                vtkm::Id blockId,
                                                vtkm::Id childId) const
{
  return this->GetPartition(level + 1, this->GetChildId(level, blockId, childId));
}

VTKM_CONT
void AmrDataSet::ReplacePartition(vtkm::Id level, vtkm::Id blockId, const vtkm::cont::DataSet& ds)
{
  Superclass::ReplacePartition(this->GetPartitionId(level, blockId), ds);
}

VTKM_CONT
void AmrDataSet::GenerateParentChildInformation()
{
  vtkm::Bounds bounds = vtkm::cont::BoundsCompute(*this);
  if (bounds.Z.Max - bounds.Z.Min < vtkm::Epsilon<vtkm::FloatDefault>())
  {
    ComputeGenerateParentChildInformation<2>();
  }
  else
  {
    ComputeGenerateParentChildInformation<3>();
  }
}

template void AmrDataSet::ComputeGenerateParentChildInformation<2>();

template void AmrDataSet::ComputeGenerateParentChildInformation<3>();

VTKM_CONT
template <vtkm::IdComponent Dim>
void AmrDataSet::ComputeGenerateParentChildInformation()
{
  // do not execute if it has been run previously
  if (ParentChildInfoComputed)
  {
    throw ErrorExecution("The parent child Relationships has already been computed.\n");
    return;
  }
  ParentChildInfoComputed = true;

  // compute in vector
  std::vector<std::vector<vtkm::Id>> parentsIdsVector(this->GetNumberOfPartitions());
  std::vector<std::vector<vtkm::Id>> childrenIdsVector(this->GetNumberOfPartitions());
  for (vtkm::Id l = 0; l < this->GetNumberOfLevels() - 1; l++)
  {
    for (vtkm::Id bParent = 0; bParent < this->GetNumberOfPartitions(l); bParent++)
    {
      std::cout << std::endl << "level  " << l << " block  " << bParent << std::endl;
      vtkm::Bounds boundsParent = this->GetPartition(l, bParent).GetCoordinateSystem().GetBounds();

      // compute size of a cell to compare overlap against
      auto coords = this->GetPartition(l, bParent).GetCoordinateSystem().GetDataAsMultiplexer();
      vtkm::cont::CellSetStructured<Dim> cellset;
      vtkm::Id ptids[cellset.GetNumberOfPointsInCell(0)];
      this->GetPartition(l, bParent).GetCellSet().CopyTo(cellset);
      cellset.GetCellPointIds(0, ptids);
      vtkm::Bounds boundsCell = vtkm::Bounds();
      for (vtkm::IdComponent pointId = 0; pointId < cellset.GetNumberOfPointsInCell(0); pointId++)
      {
        boundsCell.Include(coords.ReadPortal().Get(ptids[pointId]));
      }

      // see if there is overlap of at least one cell
      for (vtkm::Id bChild = 0; bChild < this->GetNumberOfPartitions(l + 1); bChild++)
      {
        vtkm::Bounds boundsChild =
          this->GetPartition(l + 1, bChild).GetCoordinateSystem().GetBounds();
        vtkm::Bounds boundsIntersection = boundsParent.Intersection(boundsChild);
        if ((Dim == 2 && boundsIntersection.Area() >= boundsCell.Area()) ||
            (Dim == 3 && boundsIntersection.Volume() >= boundsCell.Volume()))
        {
          parentsIdsVector.at(this->GetPartitionId(l + 1, bChild)).push_back(bParent);
          childrenIdsVector.at(this->GetPartitionId(l, bParent)).push_back(bChild);
          std::cout << " overlaps with level " << l + 1 << " block  " << bChild << " "
                    << boundsParent << " " << boundsChild << " " << boundsIntersection << " "
                    << boundsIntersection.Area() << " " << boundsIntersection.Volume() << std::endl;
        }
        else
        {
          std::cout << " does not overlap with level " << l + 1 << " block  " << bChild << " "
                    << boundsParent << " " << boundsChild << " " << boundsIntersection << " "
                    << boundsIntersection.Area() << " " << boundsIntersection.Volume() << std::endl;
        }
      }
    }
  }

  // translate to ArrayHandles
  this->StartParentsIds.Allocate(this->GetNumberOfPartitions());
  this->StartChildrenIds.Allocate(this->GetNumberOfPartitions());
  vtkm::Id numberOfParents = 0;
  vtkm::Id numberOfChildren = 0;
  for (vtkm::Id p = 0; p < this->GetNumberOfPartitions(); p++)
  {
    this->StartParentsIds.WritePortal().Set(p, numberOfParents);
    this->StartChildrenIds.WritePortal().Set(p, numberOfChildren);
    numberOfParents += parentsIdsVector.at(p).size();
    numberOfChildren += childrenIdsVector.at(p).size();
  }
  assert(numberOfParents == numberOfChildren);

  this->ParentsIds.Allocate(numberOfParents);
  this->ChildrenIds.Allocate(numberOfChildren);
  for (vtkm::Id p = 0; p < this->GetNumberOfPartitions() - 1; p++)
  {
    for (vtkm::Id parentId = this->StartParentsIds.ReadPortal().Get(p);
         parentId < this->StartParentsIds.ReadPortal().Get(p + 1);
         parentId++)
    {
      this->ParentsIds.WritePortal().Set(
        parentId, parentsIdsVector.at(p).at(parentId - this->StartParentsIds.ReadPortal().Get(p)));
    }
    for (vtkm::Id childId = this->StartChildrenIds.ReadPortal().Get(p);
         childId < this->StartChildrenIds.ReadPortal().Get(p + 1);
         childId++)
    {
      this->ChildrenIds.WritePortal().Set(
        childId, childrenIdsVector.at(p).at(childId - this->StartChildrenIds.ReadPortal().Get(p)));
    }
  }
  for (vtkm::Id parentId =
         this->StartParentsIds.ReadPortal().Get(this->GetNumberOfPartitions() - 1);
       parentId < numberOfParents;
       parentId++)
  {
    this->ParentsIds.WritePortal().Set(
      parentId,
      parentsIdsVector.at(this->GetNumberOfPartitions() - 1)
        .at(parentId - this->StartParentsIds.ReadPortal().Get(this->GetNumberOfPartitions() - 1)));
  }
  for (vtkm::Id childId =
         this->StartChildrenIds.ReadPortal().Get(this->GetNumberOfPartitions() - 1);
       childId < numberOfChildren;
       childId++)
  {
    this->ChildrenIds.WritePortal().Set(
      childId,
      childrenIdsVector.at(this->GetNumberOfPartitions() - 1)
        .at(childId - this->StartChildrenIds.ReadPortal().Get(this->GetNumberOfPartitions() - 1)));
  }
}

VTKM_CONT
void AmrDataSet::GenerateGhostType()
{
  vtkm::Bounds bounds = vtkm::cont::BoundsCompute(*this);
  if (bounds.Z.Max - bounds.Z.Min < vtkm::Epsilon<vtkm::FloatDefault>())
  {
    ComputeGenerateGhostType<2>();
  }
  else
  {
    ComputeGenerateGhostType<3>();
  }
}

template void AmrDataSet::ComputeGenerateGhostType<2>();

template void AmrDataSet::ComputeGenerateGhostType<3>();

VTKM_CONT
template <vtkm::IdComponent Dim>
void AmrDataSet::ComputeGenerateGhostType()
{
  assert(ParentChildInfoComputed);

  for (vtkm::Id l = 0; l < this->GetNumberOfLevels(); l++)
  {
    for (vtkm::Id b = 0; b < this->GetNumberOfPartitions(l); b++)
    {
      //      std::cout<<std::endl<<"level  "<<l<<" block  "<<b<<" has  "<<this->GetNumberOfChildren(l, b)<<" children"<<std::endl;
      vtkm::cont::DataSet partition = this->GetPartition(l, b);
      vtkm::cont::CellSetStructured<Dim> cellset;
      partition.GetCellSet().CopyTo(cellset);
      vtkm::cont::ArrayHandle<vtkm::UInt8> ghostField;
      vtkm::cont::ArrayCopy(
        vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(0, partition.GetNumberOfCells()), ghostField);
      auto pointField = partition.GetCoordinateSystem().GetDataAsMultiplexer();

      for (vtkm::Id childId = 0; childId < this->GetNumberOfChildren(l, b); childId++)
      {
        vtkm::Bounds boundsChild = this->GetChild(l, b, childId).GetCoordinateSystem().GetBounds();
        vtkm::cont::Invoker invoke;
        invoke(vtkm::worklet::GenerateGhostTypeWorklet<Dim>{ boundsChild },
               cellset,
               pointField,
               ghostField);
      }
      partition.AddCellField("vtkGhostType", ghostField);
      this->ReplacePartition(l, b, partition);
    }
  }
}

// Add helper arrays like in ParaView
VTKM_CONT
void AmrDataSet::GenerateIndexArrays()
{
  for (vtkm::Id l = 0; l < this->GetNumberOfLevels(); l++)
  {
    for (vtkm::Id b = 0; b < this->GetNumberOfPartitions(l); b++)
    {
      vtkm::cont::DataSet partition = this->GetPartition(l, b);

      vtkm::cont::ArrayHandle<vtkm::UInt8> fieldAmrLevel;
      vtkm::cont::ArrayCopy(
        vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(l, partition.GetNumberOfCells()),
        fieldAmrLevel);
      partition.AddCellField("vtkAmrLevel", fieldAmrLevel);

      vtkm::cont::ArrayHandle<vtkm::UInt8> fieldBlockId;
      vtkm::cont::ArrayCopy(
        vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(b, partition.GetNumberOfCells()),
        fieldBlockId);
      partition.AddCellField("vtkAmrIndex", fieldBlockId);

      vtkm::cont::ArrayHandle<vtkm::UInt8> fieldPartitionIndex;
      vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::UInt8>(
                              this->GetPartitionId(l, b), partition.GetNumberOfCells()),
                            fieldPartitionIndex);
      partition.AddCellField("vtkCompositeIndex", fieldPartitionIndex);

      this->ReplacePartition(l, b, partition);
    }
  }
}

VTKM_CONT
void AmrDataSet::AppendPartition([[maybe_unused]] const vtkm::cont::DataSet& ds)
{
  throw ErrorExecution(
    "AmrDataSet does not support appending of partitions. Use ReplacePartition instead \n");
}

VTKM_CONT
void AmrDataSet::InsertPartition([[maybe_unused]] const vtkm::cont::DataSet& ds)
{
  throw ErrorExecution(
    "AmrDataSet does not support insertion of partitions. Use ReplacePartition instead \n");
}

VTKM_CONT
void AmrDataSet::AppendPartitions([
  [maybe_unused]] const std::vector<vtkm::cont::DataSet>& partitions)
{
  throw ErrorExecution(
    "AmrDataSet does not support insertion of partitions. Use ReplacePartition instead \n");
}

VTKM_CONT
void AmrDataSet::PrintSummary(std::ostream& stream) const
{
  stream << "AmrDataSet [" << this->Partitions.size() << " partitions]:\n";
  for (size_t part = 0; part < this->Partitions.size(); ++part)
  {
    stream << "Partition " << part << ":\n";
    this->Partitions[part].PrintSummary(stream);
  }
  stream << "Number of levels " << this->GetNumberOfLevels() << ":\n";
  for (vtkm::Id l = 0; l < this->GetNumberOfLevels(); l++)
  {
    stream << "Level " << l << " has " << this->GetNumberOfPartitions(l)
           << " blocks/partitions starting at index " << this->GetStartPartitionId(l) << ".\n";
  }
  if (!ParentChildInfoComputed)
  {
    stream << "The parent child Relationships have not been computed.\n";
  }
  else
  {
    stream << "The parent child Relationships are as follows:\n";
    for (vtkm::Id l = 0; l < this->GetNumberOfLevels(); l++)
    {
      stream << "Level " << l << ":\n";
      for (vtkm::Id b = 0; b < this->GetNumberOfPartitions(l); b++)
      {
        stream << "BlockId " << b << ":\n has parents ids: ";
        for (vtkm::Id p = 0; p < this->GetNumberOfParents(l, b); p++)
        {
          stream << this->GetParentId(l, b, p) << " ";
        }
        stream << "\n has children ids: ";
        for (vtkm::Id c = 0; c < this->GetNumberOfChildren(l, b); c++)
        {
          stream << this->GetChildId(l, b, c) << " ";
        }
        stream << "\n";
      }
    }
  }
}
}
} // namespace vtkm::cont
