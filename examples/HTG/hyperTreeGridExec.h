#ifndef HYPERTREEGRID_HYPERTREEGRIDEXEC_H
#define HYPERTREEGRID_HYPERTREEGRIDEXEC_H

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderUniform.h>



template <typename DataType>
struct paramHTGExec
{
  int numberOfHT;
  vtkm::UInt8 Refinement;
  vtkm::UInt8 Dimension;

  std::vector<std::vector<vtkm::Float32>> Bounds;
  std::vector<std::vector<vtkm::UInt8>> ChildBitMask;
  std::vector<std::vector<vtkm::UInt8>> ChildIsLeaf;
  std::vector<std::vector<DataType>> Data;
};

template <typename DataType>
class HyperTreeGridExec
{
private:
  vtkm::UInt8 Refinement = 0;
  vtkm::UInt8 Dimension = 0;
  vtkm::UInt8 NumberOfChild = 0;

  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>> Bounds;
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::UInt8>> ChildBitMask;
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::UInt8>> ChildIsLeaf;
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<DataType>> Data;

  VTKM_CONT
  DataType findData(const vtkm::cont::ArrayHandle<vtkm::UInt8>& childIsLeaf,
                    const vtkm::cont::ArrayHandle<vtkm::UInt8>& childBitMask,
                    const vtkm::cont::ArrayHandle<DataType> arrayData,
                    vtkm::UInt32 ParentOffset,
                    vtkm::UInt32 whichChild)
  {
    auto portalIsLeaf = childIsLeaf.ReadPortal();
    auto portalBitMask = childBitMask.ReadPortal();
    auto portalData = arrayData.ReadPortal();
    vtkm::Int32 posData = 0;
    u_int i = 0, j = 0;
    for (i = 0; i < ParentOffset; i++)
    {
      for (j = 0; j < 8; j++)
      {

        if (((portalIsLeaf.Get(i) & (0x80 >> j)) != 0) &&
            ((portalBitMask.Get(i) & (0x80 >> j)) == 0))
        {
          posData++;
        }
      }
    }
    for (j = 0; j < whichChild; j++)
    {
      if (((portalIsLeaf.Get(i) & (0x80 >> j)) != 0) && ((portalBitMask.Get(i) & (0x80 >> j)) == 0))
      {
        posData++;
      }
    }

    return portalData.Get(posData);
  }

  VTKM_CONT
  vtkm::Int32 findChildPosition(const vtkm::cont::ArrayHandle<vtkm::UInt8>& childBitMask,
                                const vtkm::cont::ArrayHandle<vtkm::UInt8>& childBitLeaf,
                                vtkm::Int32 ParentOffset,
                                vtkm::Int32 whichChild)
  {
    auto portalBitMask = childBitMask.ReadPortal();
    auto portalBitLeaf = childBitLeaf.ReadPortal();

    vtkm::Int32 pos = 0;
    int i = 0, j = 0;
    for (i = 0; i < ParentOffset - 1; i++)
    {
      for (j = 0; j < 8; j++)
      {
        if (((portalBitMask.Get(i) & (0x80 >> j)) == 0) &&
            ((portalBitLeaf.Get(i) & (0x80 >> j)) == 0))
        {
          pos++;
        }
      }
    }
    for (j = 0; j < whichChild + 1; j++)
    {
      if (((portalBitMask.Get(i) & (0x80 >> j)) == 0) &&
          ((portalBitLeaf.Get(i) & (0x80 >> j)) == 0))
      {
        pos++;
      }
    }
    return pos;
  }
  VTKM_CONT
  void addVoxelToDataSet(vtkm::cont::DataSetBuilderExplicitIterative& builder,
                         const vtkm::cont::ArrayHandle<vtkm::Float32>& boundingBox)
  {
    auto portalBoundingBox = boundingBox.ReadPortal();
    builder.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON);
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(0), portalBoundingBox.Get(2), portalBoundingBox.Get(4)));
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(1), portalBoundingBox.Get(2), portalBoundingBox.Get(4)));
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(1), portalBoundingBox.Get(2), portalBoundingBox.Get(5)));
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(0), portalBoundingBox.Get(2), portalBoundingBox.Get(5)));
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(0), portalBoundingBox.Get(3), portalBoundingBox.Get(4)));
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(1), portalBoundingBox.Get(3), portalBoundingBox.Get(4)));
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(1), portalBoundingBox.Get(3), portalBoundingBox.Get(5)));
    builder.AddCellPoint(builder.AddPoint(
      portalBoundingBox.Get(0), portalBoundingBox.Get(3), portalBoundingBox.Get(5)));
  }
  VTKM_CONT
  std::vector<vtkm::Float32> findBoundingBox(const vtkm::cont::ArrayHandle<vtkm::Float32>& bounds,
                                             const vtkm::Int32& whichChild)
  {
    auto portalBounds = bounds.ReadPortal();
    std::vector<vtkm::Float32> newBounds;

    if (whichChild % 2 == 0)
    {
      newBounds.push_back(portalBounds.Get(0));
      newBounds.push_back(portalBounds.Get(1) - (portalBounds.Get(1) - portalBounds.Get(0)) / 2.f);
    }
    else
    {
      newBounds.push_back(portalBounds.Get(1) - (portalBounds.Get(1) - portalBounds.Get(0)) / 2.f);
      newBounds.push_back(portalBounds.Get(1));
    }

    if (whichChild < 2 || (whichChild < 6 && whichChild > 3))
    {
      newBounds.push_back(portalBounds.Get(2));
      newBounds.push_back(portalBounds.Get(3) - (portalBounds.Get(3) - portalBounds.Get(2)) / 2.f);
    }
    else
    {
      newBounds.push_back(portalBounds.Get(3) - (portalBounds.Get(3) - portalBounds.Get(2)) / 2.f);
      newBounds.push_back(portalBounds.Get(3));
    }

    if (whichChild < 4)
    {
      newBounds.push_back(portalBounds.Get(4));
      newBounds.push_back(portalBounds.Get(5) - (portalBounds.Get(5) - portalBounds.Get(4)) / 2.f);
    }
    else
    {
      newBounds.push_back(portalBounds.Get(5) - (portalBounds.Get(5) - portalBounds.Get(4)) / 2.f);
      newBounds.push_back(portalBounds.Get(5));
    }

    return newBounds;
  }

  VTKM_CONT
  void traversalForDataSet(const vtkm::cont::ArrayHandle<vtkm::UInt8>& childBitMask,
                           const vtkm::cont::ArrayHandle<vtkm::UInt8>& childIsLeaf,
                           const vtkm::cont::ArrayHandle<DataType>& dataArray,
                           vtkm::cont::DataSetBuilderExplicitIterative& builder,
                           const std::string& fieldName,
                           std::vector<DataType>& data,
                           const vtkm::cont::ArrayHandle<vtkm::Float32>& boundingBox,
                           vtkm::Int32 currentOffset)
  {
    auto portalBitMask = childBitMask.ReadPortal();
    auto portalIsLeaf = childIsLeaf.ReadPortal();

    if (portalBitMask.Get(currentOffset) != (pow(2, 8) - 1))
    {
      for (int i = 0; i < this->NumberOfChild; i++)
      {
        if ((portalBitMask.Get(currentOffset) & (0x80 >> i)) == 0)
        {
          std::vector<vtkm::Float32> a = findBoundingBox(boundingBox, i);
          vtkm::cont::ArrayHandle<vtkm::Float32> childBoundingBox = vtkm::cont::make_ArrayHandle(a);


          if ((portalIsLeaf.Get(currentOffset) & (0x80 >> i)) != 0)
          {
            data.push_back(findData(childIsLeaf, childBitMask, dataArray, currentOffset, i));
            addVoxelToDataSet(builder, childBoundingBox);
          }
          else
          {
            vtkm::Int32 childOffset =
              findChildPosition(childBitMask, childIsLeaf, currentOffset, i);

            traversalForDataSet(childBitMask,
                                childIsLeaf,
                                dataArray,
                                builder,
                                fieldName,
                                data,
                                childBoundingBox,
                                childOffset);
          }
        }
      }
    }
  }


public:
  HyperTreeGridExec() {}

  HyperTreeGridExec(vtkm::UInt8 refinement,
                    vtkm::UInt8 dimension,
                    vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>> bounds,
                    vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::UInt8>> childBitMask,
                    vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::UInt8>> childIsLeaf,
                    vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<DataType>> data)
    : Refinement(refinement)
    , Dimension(dimension)
    , NumberOfChild(pow(refinement, dimension))
  {

    vtkm::cont::ArrayCopy(bounds, this->Bounds);
    vtkm::cont::ArrayCopy(childBitMask, this->ChildBitMask);
    vtkm::cont::ArrayCopy(childIsLeaf, this->ChildIsLeaf);
    vtkm::cont::ArrayCopy(data, this->Data);
  }
  VTKM_CONT
  HyperTreeGridExec(paramHTGExec<DataType>& param)
    : Refinement(param.Refinement)
    , Dimension(param.Dimension)
    , NumberOfChild(pow(this->Refinement, this->Dimension))
  {
    std::vector<vtkm::cont::ArrayHandle<vtkm::UInt8>> vecBitMask;
    std::vector<vtkm::cont::ArrayHandle<vtkm::UInt8>> vecBitLeaf;
    std::vector<vtkm::cont::ArrayHandle<vtkm::Float32>> vecBounds;
    std::vector<vtkm::cont::ArrayHandle<DataType>> vecData;

    std::cout << "Constructeur param" << std::endl;
    for (int i = 0; i < param.numberOfHT; i++)
    {
      vecBitMask.push_back(vtkm::cont::make_ArrayHandle(param.ChildBitMask[i]));
      vecBitLeaf.push_back(vtkm::cont::make_ArrayHandle(param.ChildIsLeaf[i]));
      vecData.push_back(vtkm::cont::make_ArrayHandle(param.Data[i]));
      vecBounds.push_back(vtkm::cont::make_ArrayHandle(param.Bounds[i]));
    }
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(vecBounds), this->Bounds);
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(vecBitMask), this->ChildBitMask);
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(vecBitLeaf), this->ChildIsLeaf);
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(vecData), this->Data);
    auto portal1 = this->ChildIsLeaf.ReadPortal().Get(0).ReadPortal();
    for (int i = 0; i < 9; i++)
    {
      std::cout << (int)portal1.Get(i) << " ";
    }
    std::cout << std::endl;
  }
  VTKM_CONT
  HyperTreeGridExec(const HyperTreeGridExec<DataType>& HTG)
    : Refinement(HTG.getRefinement())
    , Dimension(HTG.getDimension())
    , NumberOfChild(HTG.getNumberOfChild())
  {

    vtkm::cont::ArrayCopy(HTG.getBounds(), this->Bounds);
    vtkm::cont::ArrayCopy(HTG.getChildBitMask(), this->ChildBitMask);
    vtkm::cont::ArrayCopy(HTG.getChildIsLeaf(), this->ChildIsLeaf);
    vtkm::cont::ArrayCopy(HTG.getData(), this->Data);
  }

  VTKM_CONT
  vtkm::UInt8 getRefinement() const { return this->Refinement; }
  VTKM_CONT
  vtkm::UInt8 getDimension() const { return this->Dimension; }
  VTKM_CONT
  vtkm::UInt8 getNumberOfChild() const { return this->NumberOfChild; }
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::UInt8>> getChildBitMask() const
  {
    return this->ChildBitMask;
  }
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::UInt8>> getChildIsLeaf() const
  {
    return this->ChildIsLeaf;
  }
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<DataType>> getData() const { return this->Data; }
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>> getBounds() const
  {
    return this->Bounds;
  }

  VTKM_CONT
  vtkm::cont::DataSet convertToRenderDataSet(const std::string& fieldName,
                                             std::vector<DataType>& data)
  {

    vtkm::cont::DataSetBuilderExplicitIterative builder;


    vtkm::cont::ArrayHandle<vtkm::UInt8> CopyBitMask;
    vtkm::cont::ArrayHandle<vtkm::UInt8> CopyBitLeaf;
    vtkm::cont::ArrayHandle<DataType> CopyData;
    vtkm::cont::ArrayHandle<vtkm::Float32> CopyBounds;


    for (int i = 0; i < ChildBitMask.GetNumberOfValues(); i++)
    {

      auto portalBitMask = this->ChildBitMask.ReadPortal();
      auto portalIsLeaf = this->ChildIsLeaf.ReadPortal();
      auto portalData = this->Data.ReadPortal();
      auto portalBounds = this->Bounds.ReadPortal();

      auto portalportal = portalData.Get(i).ReadPortal();


      vtkm::cont::ArrayCopy(portalBitMask.Get(i), CopyBitMask);
      vtkm::cont::ArrayCopy(portalIsLeaf.Get(i), CopyBitLeaf);
      vtkm::cont::ArrayCopy(portalData.Get(i), CopyData);
      vtkm::cont::ArrayCopy(portalBounds.Get(i), CopyBounds);


      this->traversalForDataSet(
        CopyBitMask, CopyBitLeaf, CopyData, builder, fieldName, data, CopyBounds, 0);
    }

    vtkm::cont::DataSet dataSet = builder.Create();
    vtkm::cont::ArrayHandle<vtkm::Float32> dataArray = vtkm::cont::make_ArrayHandle(data);
    dataSet.AddField(vtkm::cont::make_FieldCell(fieldName, dataArray));


    return dataSet;
  }
};

#endif //HYPERTREEGRID_HYPERTREEGRIDEXEC_H
