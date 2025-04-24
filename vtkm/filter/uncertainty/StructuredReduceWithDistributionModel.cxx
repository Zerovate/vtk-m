//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/uncertainty/StructuredReduceWithDistributionModel.h>

#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleRecombineVec.h>
#include <vtkm/cont/ArrayHandleRuntimeVec.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/Token.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/Math.h>

namespace
{

template <typename T>
struct DistributionExecObjects
{
  using ReadPortalType = typename vtkm::cont::ArrayHandleRecombineVec<T>::ReadPortalType;
  using WritePortalType = typename vtkm::cont::ArrayHandleRuntimeVec<T>::WritePortalType;

  ReadPortalType Input;

  bool GenerateMean;
  bool GenerateStandardDeviation;

  WritePortalType Mean;
  WritePortalType StandardDeviation;
};

template <typename T>
struct DistributionContObjects : vtkm::cont::ExecutionObjectBase
{
  vtkm::cont::ArrayHandleRecombineVec<T> Input;

  vtkm::Id OutputSize;

  bool GenerateMean;
  bool GenerateStandardDeviation;

  vtkm::cont::ArrayHandleRuntimeVec<T> Mean;
  vtkm::cont::ArrayHandleRuntimeVec<T> StandardDeviation;

  DistributionContObjects(const vtkm::cont::UnknownArrayHandle& input, vtkm::Id outputSize)
    : Input(input.ExtractArrayFromComponents<T>())
    , OutputSize(outputSize)
    , Mean(input.GetNumberOfComponentsFlat())
    , StandardDeviation(input.GetNumberOfComponentsFlat())
  {
  }

  VTKM_CONT DistributionExecObjects<T> PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                           vtkm::cont::Token& token) const
  {
    DistributionExecObjects<T> execObject;
    execObject.Input = this->Input.PrepareForInput(device, token);

    execObject.GenerateMean = this->GenerateMean;
    execObject.Mean =
      this->Mean.PrepareForOutput(this->GenerateMean ? this->OutputSize : 0, device, token);

    execObject.GenerateStandardDeviation = this->GenerateStandardDeviation;
    execObject.StandardDeviation = this->StandardDeviation.PrepareForOutput(
      this->GenerateStandardDeviation ? this->OutputSize : 0, device, token);

    return execObject;
  }
};

struct ReduceFieldWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn outIndices, ExecObject arrays);

  vtkm::Id3 InputDimensions;
  vtkm::Id3 OutputDimensions;
  vtkm::IdComponent3 BlockSize;

  template <typename T>
  VTKM_EXEC void operator()(vtkm::Id outIndex, const DistributionExecObjects<T>& arrays) const
  {
    vtkm::Id3 outIJK = { outIndex % this->OutputDimensions[0],
                         (outIndex / this->OutputDimensions[0]) % this->OutputDimensions[1],
                         (outIndex / (this->OutputDimensions[0] * this->OutputDimensions[1])) };

    vtkm::Id3 inStart = outIJK * this->BlockSize;
    vtkm::Id3 inEnd = vtkm::Min(inStart + this->BlockSize, this->InputDimensions);

    vtkm::Id actualBlockSize = [](vtkm::Id3 x) { return x[0] * x[1] * x[2]; }(inEnd - inStart);

    // Accumulate distributions
    const vtkm::Id blockStart =
      (inStart[2] * this->InputDimensions[1] + inStart[1]) * this->InputDimensions[0] + inStart[0];
    vtkm::IdComponent numComponents = arrays.Input.Get(blockStart).GetNumberOfComponents();
    for (vtkm::IdComponent component = 0; component < numComponents; ++component)
    {
      T mean = 0;
      T stddev = 0;
      vtkm::Id slabStart = blockStart;
      for (vtkm::Id k = inStart[2]; k < inEnd[2]; ++k)
      {
        vtkm::Id shaftStart = slabStart;
        for (vtkm::Id j = inStart[1]; j < inEnd[1]; ++j)
        {
          vtkm::Id index = shaftStart;
          for (vtkm::Id i = inStart[0]; i < inEnd[0]; ++i)
          {
            T inValue = arrays.Input.Get(index)[component];
            if (arrays.GenerateMean || arrays.GenerateStandardDeviation)
            {
              mean += inValue;
            }
            if (arrays.GenerateStandardDeviation)
            {
              stddev += inValue * inValue;
            }
            ++index;
          }
          shaftStart += this->InputDimensions[0];
        }
        slabStart += this->InputDimensions[0] * this->InputDimensions[1];
      }

      if (arrays.GenerateMean || arrays.GenerateStandardDeviation)
      {
        mean /= actualBlockSize;
        if (arrays.GenerateMean)
        {
          // Works because arrays.Mean is a VecFromPortal
          arrays.Mean.Get(outIndex)[component] = mean;
        }
        if (arrays.GenerateStandardDeviation)
        {
          stddev /= actualBlockSize;
          stddev -= mean * mean;
          stddev = vtkm::Sqrt(stddev);
          // Works because arrays.StandardDeviation is a VecFromPortal
          arrays.StandardDeviation.Get(outIndex)[component] = stddev;
        }
      }
    }
  }
};

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace uncertainty
{

vtkm::cont::DataSet StructuredReduceWithDistributionModel::DoExecute(
  const vtkm::cont::DataSet& input)
{
  vtkm::cont::UnknownCellSet cells = input.GetCellSet();

  if (!cells.CanConvert<vtkm::cont::CellSetStructured<3>>())
  {
    // TODO: Support 1D and 2D CellSetStructured.
    throw vtkm::cont::ErrorBadType(
      "Input with invalid cell set passed to StructuredReduceWithDistributionModel");
  }

  vtkm::cont::CellSetStructured<3> cellSet;
  cells.AsCellSet(cellSet);
  vtkm::Id3 inputPoints = cellSet.GetPointDimensions();
  vtkm::Id3 inputCells = cellSet.GetCellDimensions();

  // Adjust block size to preserve dimensionality.
  vtkm::IdComponent3 blockSize = this->BlockSize;
  for (vtkm::IdComponent i = 0; i < 3; ++i)
  {
    if (blockSize[i] >= inputPoints[i])
    {
      blockSize[i] = ((inputPoints[i] - 1) / 2) + 1;
    }
  }

  vtkm::Id3 outputCells = (inputPoints - vtkm::Id3(1)) / blockSize;
  vtkm::Id3 outputPoints = outputCells + vtkm::Id3(1);

  auto mapField = [&](vtkm::cont::DataSet& output, const vtkm::cont::Field& inField) {
    vtkm::Id3 inputDims;
    vtkm::Id3 outputDims;
    switch (inField.GetAssociation())
    {
      case vtkm::cont::Field::Association::Points:
        inputDims = inputPoints;
        outputDims = outputPoints;
        break;
      case vtkm::cont::Field::Association::Cells:
        inputDims = inputCells;
        outputDims = outputCells;
        break;
      default:
        output.AddField(inField);
        return;
    }
    vtkm::Id outputSize = outputDims[0] * outputDims[1] * outputDims[2];

    // Special cases
    if (inField.GetData().CanConvert<vtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      vtkm::cont::ArrayHandleUniformPointCoordinates inCoords;
      inField.GetData().AsArrayHandle(inCoords);

      vtkm::Vec3f fBlockSize = blockSize;
      if (this->GetGenerateMean())
      {
        vtkm::Vec3f outOrigin =
          inCoords.GetOrigin() + 0.5 * inCoords.GetSpacing() * (fBlockSize - 1);
        vtkm::Vec3f outSpacing = inCoords.GetSpacing() * fBlockSize;
        vtkm::cont::ArrayHandleUniformPointCoordinates outCoords{ outputDims,
                                                                  outOrigin,
                                                                  outSpacing };
        output.AddField(
          { inField.GetName() + this->GetMeanSuffix(), inField.GetAssociation(), outCoords });
      }
      if (this->GetGenerateStandardDeviation())
      {
        auto square = [](auto x) { return x * x; };
        vtkm::Vec3f stddev =
          vtkm::Sqrt(square(inCoords.GetSpacing()) *
                     (((fBlockSize - 1) * (2 * fBlockSize - 1)) / 6 - square(fBlockSize - 1) / 4));
        // vtkm::Vec3f stddev =
        //   vtkm::Sqrt(square(inCoords.GetSpacing()));
        vtkm::cont::ArrayHandleConstant<vtkm::Vec3f> outArray{ stddev, outputSize };
        output.AddField({ inField.GetName() + this->GetStandardDeviationSuffix(),
                          inField.GetAssociation(),
                          outArray });
      }

      return;
    }

    // General case
    auto resolveArray = [&](auto fieldArray) {
      using T = typename decltype(fieldArray)::ValueType::ComponentType;
      DistributionContObjects<T> arrays{ fieldArray, outputSize };
      arrays.GenerateMean = this->GetGenerateMean();
      arrays.GenerateStandardDeviation = this->GetGenerateStandardDeviation();

      ReduceFieldWorklet worklet;
      worklet.InputDimensions = inputDims;
      worklet.OutputDimensions = outputDims;
      worklet.BlockSize = blockSize;

      this->Invoke(worklet, vtkm::cont::make_ArrayHandleIndex(outputSize), arrays);

      if (this->GetGenerateMean())
      {
        output.AddField(
          { inField.GetName() + this->GetMeanSuffix(), inField.GetAssociation(), arrays.Mean });
      }
      if (this->GetGenerateStandardDeviation())
      {
        output.AddField({ inField.GetName() + this->GetStandardDeviationSuffix(),
                          inField.GetAssociation(),
                          arrays.StandardDeviation });
      }
    };

    this->CastAndCallVariableVecField(inField, resolveArray);
  };

  vtkm::cont::CellSetStructured<3> outputCellSet;
  outputCellSet.SetPointDimensions(outputPoints);
  vtkm::cont::DataSet output = this->CreateResult(input, outputCellSet, mapField);

  for (vtkm::IdComponent csIndex = 0; csIndex < input.GetNumberOfCoordinateSystems(); ++csIndex)
  {
    std::string csName = input.GetCoordinateSystemName(csIndex);
    if (this->GetGenerateMean())
    {
      output.AddCoordinateSystem(csName + this->GetMeanSuffix());
    }
    else
    {
      // No generated field makes sense for coordinates. Output will have no coordinates.
    }
  }

  return output;
}

}
}
} // vtkm::filter::uncertainty
