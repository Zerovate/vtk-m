//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/StaticAssert.h>

#include <vtkm/testing/Testing.h>

namespace
{

struct TestExecObject
{
  VTKM_EXEC_CONT
  TestExecObject()
    : Value(nullptr)
  {
  }

  VTKM_EXEC_CONT
  TestExecObject(vtkm::Id* value)
    : Value(value)
  {
  }

  vtkm::Id* Value;
};

struct MyOutputToInputMapPortal
{
  using ValueType = vtkm::Id;
  VTKM_EXEC_CONT
  vtkm::Id Get(vtkm::Id index) const { return index; }
};

struct MyVisitArrayPortal
{
  using ValueType = vtkm::IdComponent;
  vtkm::IdComponent Get(vtkm::Id) const { return 1; }
};

struct MyThreadToOutputMapPortal
{
  using ValueType = vtkm::Id;
  VTKM_EXEC_CONT
  vtkm::Id Get(vtkm::Id index) const { return index; }
};

struct TestFetchTagInput
{
};
struct TestFetchTagOutput
{
};

// Missing TransportTag, but we are not testing that so we can leave it out.
struct TestControlSignatureTagInput
{
  using FetchTag = TestFetchTagInput;
};
struct TestControlSignatureTagOutput
{
  using FetchTag = TestFetchTagOutput;
};

} // anonymous namespace

namespace vtkm
{
namespace exec
{
namespace arg
{

template <>
struct Fetch<TestFetchTagInput, vtkm::exec::arg::AspectTagDefault, TestExecObject>
{
  using ValueType = vtkm::Id;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic& indices,
                 const TestExecObject& execObject) const
  {
    return *execObject.Value + 10 * indices.GetInputIndex();
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic&, const TestExecObject&, ValueType) const
  {
    // No-op
  }
};

template <>
struct Fetch<TestFetchTagOutput, vtkm::exec::arg::AspectTagDefault, TestExecObject>
{
  using ValueType = vtkm::Id;

  VTKM_EXEC
  ValueType Load(const vtkm::exec::arg::ThreadIndicesBasic&, const TestExecObject&) const
  {
    // No-op
    return ValueType();
  }

  VTKM_EXEC
  void Store(const vtkm::exec::arg::ThreadIndicesBasic& indices,
             const TestExecObject& execObject,
             ValueType value) const
  {
    *execObject.Value = value + 20 * indices.GetOutputIndex();
  }
};
}
}
} // vtkm::exec::arg

namespace
{

using TestControlSignature = void(TestControlSignatureTagInput, TestControlSignatureTagOutput);

using TestExecutionSignature1 = void(vtkm::exec::arg::BasicArg<1>, vtkm::exec::arg::BasicArg<2>);

using TestExecutionSignature2 = vtkm::exec::arg::BasicArg<2>(vtkm::exec::arg::BasicArg<1>);

using TestControlSignatureReverse = void(TestControlSignatureTagOutput,
                                         TestControlSignatureTagInput);

using TestExecutionSignatureReverse = void(vtkm::exec::arg::BasicArg<2>,
                                           vtkm::exec::arg::BasicArg<1>);

// Not a full worklet, but provides operators that we expect in a worklet.
template <typename ControlSig, typename ExecSig, vtkm::IdComponent InDomain>
struct TestWorkletProxy : vtkm::exec::FunctorBase
{
  using ControlSignature = ControlSig;
  using ExecutionSignature = ExecSig;

  using InputDomain = vtkm::exec::arg::BasicArg<InDomain>;

  VTKM_EXEC
  void operator()(vtkm::Id input, vtkm::Id& output) const { output = input + 100; }

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id input) const { return input + 200; }

  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            typename G>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const G& globalThreadIndexOffset) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};

template <typename Worklet, typename... ValueReferences>
void CallWorkletInvokeFunctor(const Worklet& worklet,
                              vtkm::Id index,
                              ValueReferences&... valueReferences)
{
  vtkm::exec::internal::WorkletInvokeFunctor(
    worklet,
    vtkm::exec::arg::ThreadIndicesBasic(index, index, 1, index),
    vtkm::internal::NullType{}, // Device tag stand in
    vtkm::MakeTuple(TestExecObject(&valueReferences)...));
}

void TestDoWorkletInvoke()
{
  std::cout << "Testing internal worklet invoke." << std::endl;

  vtkm::Id inputTestValue;
  vtkm::Id outputTestValue;

  std::cout << "  Try void return." << std::endl;
  inputTestValue = 5;
  outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  CallWorkletInvokeFunctor(TestWorkletProxy<TestControlSignature, TestExecutionSignature1, 1>{},
                           1,
                           inputTestValue,
                           outputTestValue);
  VTKM_TEST_ASSERT(inputTestValue == 5, "Input value changed.");
  VTKM_TEST_ASSERT(outputTestValue == inputTestValue + 100 + 30, "Output value not set right.");

  std::cout << "  Try return value." << std::endl;
  inputTestValue = 6;
  outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  CallWorkletInvokeFunctor(TestWorkletProxy<TestControlSignature, TestExecutionSignature2, 1>{},
                           2,
                           inputTestValue,
                           outputTestValue);
  VTKM_TEST_ASSERT(inputTestValue == 6, "Input value changed.");
  VTKM_TEST_ASSERT(outputTestValue == inputTestValue + 200 + 30 * 2, "Output value not set right.");

  std::cout << "  Try reversed arguments." << std::endl;
  inputTestValue = 7;
  outputTestValue = static_cast<vtkm::Id>(0xDEADDEAD);
  CallWorkletInvokeFunctor(
    TestWorkletProxy<TestControlSignatureReverse, TestExecutionSignatureReverse, 2>{},
    3,
    outputTestValue,
    inputTestValue);
  VTKM_TEST_ASSERT(inputTestValue == 7, "Input value changed.");
  VTKM_TEST_ASSERT(outputTestValue == inputTestValue + 100 + 30 * 3, "Output value not set right.");
}

void TestWorkletInvokeFunctor()
{
  TestDoWorkletInvoke();
}

} // anonymous namespace

int UnitTestWorkletInvokeFunctor(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestWorkletInvokeFunctor, argc, argv);
}
