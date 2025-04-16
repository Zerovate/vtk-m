//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================



#include <vtkm/filter/Filter.h>

namespace vtkm
{
namespace filter
{
namespace uncertainty
{
class Fiber : public vtkm::filter::Filter
{
  vtkm::Pair<vtkm::FloatDefault, vtkm::FloatDefault> minAxis;
  vtkm::Pair<vtkm::FloatDefault, vtkm::FloatDefault> maxAxis;
  std::string Approach = "ClosedForm"; // MonteCarlo, ClosedForm, Mean, Truth
  vtkm::Id NumSamples = 500;

public:
  VTKM_CONT void SetMinAxis(const vtkm::Pair<vtkm::FloatDefault, vtkm::FloatDefault>& minCoordinate)
  {
    this->minAxis = minCoordinate;
  }

  VTKM_CONT void SetMaxAxis(const vtkm::Pair<vtkm::FloatDefault, vtkm::FloatDefault>& maxCoordinate)
  {
    this->maxAxis = maxCoordinate;
  }
  VTKM_CONT void SetMinX(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, vtkm::cont::Field::Association::Points);
  }
  VTKM_CONT void SetMaxX(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, vtkm::cont::Field::Association::Points);
  }
  VTKM_CONT void SetMinY(const std::string& fieldName)
  {
    this->SetActiveField(2, fieldName, vtkm::cont::Field::Association::Points);
  }
  VTKM_CONT void SetMaxY(const std::string& fieldName)
  {
    this->SetActiveField(3, fieldName, vtkm::cont::Field::Association::Points);
  }
  VTKM_CONT void SetMinZ(const std::string& fieldName)
  {
    this->SetActiveField(4, fieldName, vtkm::cont::Field::Association::Points);
  }
  VTKM_CONT void SetMaxZ(const std::string& fieldName)
  {
    this->SetActiveField(5, fieldName, vtkm::cont::Field::Association::Points);
  }

  VTKM_CONT void SetNumSamples(const vtkm::Id& numSamples) { this->NumSamples = numSamples; }

  VTKM_CONT void SetApproach(const std::string& approach) { this->Approach = approach; }


private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
}
}
}




//====================================================================================================================
//#include "./2VariateUncertianty.h"
#include "./worklet/2VariateUncertianty.h"
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Timer.h>

namespace vtkm
{
namespace filter
{
namespace uncertainty
{
VTKM_CONT vtkm::cont::DataSet Fiber::DoExecute(const vtkm::cont::DataSet& input)
{
  std::string FieldName;


  vtkm::cont::Field EnsembleMinX = this->GetFieldFromDataSet(0, input);
  vtkm::cont::Field EnsembleMaxX = this->GetFieldFromDataSet(1, input);
  vtkm::cont::Field EnsembleMinY = this->GetFieldFromDataSet(2, input);
  vtkm::cont::Field EnsembleMaxY = this->GetFieldFromDataSet(3, input);

  // Output Field
  vtkm::cont::UnknownArrayHandle OutputProbability;


  //  For Invoker
  auto resolveType = [&](auto ConcreteEnsembleMinX) {
    //  Obtaining Type
    using ArrayType = std::decay_t<decltype(ConcreteEnsembleMinX)>;
    using ValueType = typename ArrayType::ValueType;

    // Temporary Input Variable to add input values
    ArrayType ConcreteEnsembleMaxX;
    ArrayType ConcreteEnsembleMinY;
    ArrayType ConcreteEnsembleMaxY;

    vtkm::cont::ArrayCopyShallowIfPossible(EnsembleMaxX.GetData(), ConcreteEnsembleMaxX);
    vtkm::cont::ArrayCopyShallowIfPossible(EnsembleMinY.GetData(), ConcreteEnsembleMinY);
    vtkm::cont::ArrayCopyShallowIfPossible(EnsembleMaxY.GetData(), ConcreteEnsembleMaxY);

    // Temporary Output Variable
    vtkm::cont::ArrayHandle<ValueType> Probability;
    // Invoker

    if (this->Approach == "MonteCarlo")
    {
      FieldName = "MonteCarlo";
      std::cout << "Adopt monte carlo with numsamples " << this->NumSamples << std::endl;
      this->Invoke(vtkm::worklet::detail::MultiVariateMonteCarlo{ this->minAxis,
                                                                  this->maxAxis,
                                                                  this->NumSamples },
                   ConcreteEnsembleMinX,
                   ConcreteEnsembleMaxX,
                   ConcreteEnsembleMinY,
                   ConcreteEnsembleMaxY,
                   Probability);
    }
    else if (this->Approach == "ClosedForm")
    {
      FieldName = "ClosedForm";
      std::cout << "Adopt ClosedForm" << std::endl;
      this->Invoke(vtkm::worklet::detail::MultiVariateClosedForm{ this->minAxis, this->maxAxis },
                   ConcreteEnsembleMinX,
                   ConcreteEnsembleMaxX,
                   ConcreteEnsembleMinY,
                   ConcreteEnsembleMaxY,
                   Probability);
    }
    else if (this->Approach == "Mean")
    {
      FieldName = "Mean";
      std::cout << "Adopt Mean" << std::endl;
      this->Invoke(vtkm::worklet::detail::MultiVariateMean{ this->minAxis, this->maxAxis },
                   ConcreteEnsembleMinX,
                   ConcreteEnsembleMaxX,
                   ConcreteEnsembleMinY,
                   ConcreteEnsembleMaxY,
                   Probability);
    }
    else if (this->Approach == "Truth")
    {
      FieldName = "Truth";
      std::cout << "Adopt Truth" << std::endl;
      this->Invoke(vtkm::worklet::detail::MultiVariateTruth{ this->minAxis, this->maxAxis },
                   ConcreteEnsembleMinX,
                   ConcreteEnsembleMaxX,
                   ConcreteEnsembleMinY,
                   ConcreteEnsembleMaxY,
                   Probability);
    }
    else
    {
      throw std::runtime_error("unsupported approach:" + this->Approach);
    }

    // From Temporary Output Variable to Output Variable
    OutputProbability = Probability;
  };
  this->CastAndCallScalarField(EnsembleMinX, resolveType);

  // Creating Result
  vtkm::cont::DataSet result = this->CreateResult(input);
  result.AddPointField(FieldName, OutputProbability);


  return result;
}
}
}
}
