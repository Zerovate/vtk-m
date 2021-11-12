//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/testing/Testing.h>

#include <vtkm/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

struct TestIndexPortal
{
  using ValueType = vtkm::Id;

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return index; }
};

struct TestZeroPortal
{
  using ValueType = vtkm::IdComponent;

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id) const { return 0; }
};


template <typename T>
struct FetchArrayTopologyMapInTests
{

  template <typename InputDomain,
            typename Parameter,
            typename OutToInMap,
            typename VisitPortal,
            typename ThreadToOut>
  void TryInvoke(const InputDomain& inputDomain,
                 const Parameter& parameter,
                 const OutToInMap& outToInMap,
                 const VisitPortal& visitPortal,
                 const ThreadToOut& threadToOut) const
  {
    using ThreadIndicesType =
      vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomain,
                                                vtkm::exec::arg::CustomScatterOrMaskTag>;
    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
                                             vtkm::exec::arg::AspectTagDefault,
                                             TestPortal<T>>;
    FetchType fetch;


    const vtkm::Id threadIndex = 0;
    const vtkm::Id outputIndex = threadToOut.Get(threadIndex);
    const vtkm::Id inputIndex = outToInMap.Get(outputIndex);
    const vtkm::IdComponent visitIndex = visitPortal.Get(outputIndex);
    ThreadIndicesType indices(threadIndex, inputIndex, visitIndex, outputIndex, inputDomain);

    auto value = fetch.Load(indices, parameter);
    VTKM_TEST_ASSERT(value.GetNumberOfComponents() == 8,
                     "Topology fetch got wrong number of components.");

    VTKM_TEST_ASSERT(test_equal(value[0], TestValue(0, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[1], TestValue(1, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[2], TestValue(3, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[3], TestValue(2, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[4], TestValue(4, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[5], TestValue(5, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[6], TestValue(7, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[7], TestValue(6, T())), "Got invalid value from Load.");
  }

  void operator()() const
  {
    std::cout << "Trying ArrayTopologyMapIn fetch on parameter with type "
              << vtkm::testing::TypeName<T>::Name() << std::endl;

    vtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals;
    connectivityInternals.SetPointDimensions(vtkm::Id3(2, 2, 2));
    vtkm::exec::
      ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
        connectivity(connectivityInternals);

    this->TryInvoke(
      connectivity, TestPortal<T>{}, TestIndexPortal{}, TestZeroPortal{}, TestIndexPortal{});
  }
};


struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayTopologyMapInTests<T>()();
  }
};

template <vtkm::IdComponent NumDimensions,
          typename InputDomain,
          typename Parameter,
          typename OutToInMap,
          typename VisitPortal,
          typename ThreadToOut>
void TryStructuredPointCoordinatesInvoke(const InputDomain& inputDomain,
                                         const Parameter& parameter,
                                         const OutToInMap& outToInMap,
                                         const VisitPortal& visitPortal,
                                         const ThreadToOut& threadToOut)
{
  using ThreadIndicesType =
    vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomain, vtkm::exec::arg::CustomScatterOrMaskTag>;

  vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
                         vtkm::exec::arg::AspectTagDefault,
                         vtkm::internal::ArrayPortalUniformPointCoordinates>
    fetch;

  vtkm::Vec3f origin = TestValue(0, vtkm::Vec3f());
  vtkm::Vec3f spacing = TestValue(1, vtkm::Vec3f());

  {
    const vtkm::Id threadIndex = 0;
    const vtkm::Id outputIndex = threadToOut.Get(threadIndex);
    const vtkm::Id inputIndex = outToInMap.Get(outputIndex);
    const vtkm::IdComponent visitIndex = visitPortal.Get(outputIndex);
    vtkm::VecAxisAlignedPointCoordinates<NumDimensions> value = fetch.Load(
      ThreadIndicesType(threadIndex, inputIndex, visitIndex, outputIndex, inputDomain), parameter);
    VTKM_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    VTKM_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }

  origin[0] += spacing[0];
  {
    const vtkm::Id threadIndex = 1;
    const vtkm::Id outputIndex = threadToOut.Get(threadIndex);
    const vtkm::Id inputIndex = outToInMap.Get(outputIndex);
    const vtkm::IdComponent visitIndex = visitPortal.Get(outputIndex);
    vtkm::VecAxisAlignedPointCoordinates<NumDimensions> value = fetch.Load(
      ThreadIndicesType(threadIndex, inputIndex, visitIndex, outputIndex, inputDomain), parameter);
    VTKM_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    VTKM_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }
}

template <vtkm::IdComponent NumDimensions>
void TryStructuredPointCoordinates(
  const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                           vtkm::TopologyElementTagPoint,
                                           NumDimensions>& connectivity,
  const vtkm::internal::ArrayPortalUniformPointCoordinates& coordinates)
{
  TryStructuredPointCoordinatesInvoke<NumDimensions>(
    connectivity, coordinates, TestIndexPortal{}, TestZeroPortal{}, TestIndexPortal{});
}

void TryStructuredPointCoordinates()
{
  std::cout << "*** Fetching special case of uniform point coordinates. *****" << std::endl;

  vtkm::internal::ArrayPortalUniformPointCoordinates coordinates(
    vtkm::Id3(3, 2, 2), TestValue(0, vtkm::Vec3f()), TestValue(1, vtkm::Vec3f()));

  std::cout << "3D" << std::endl;
  vtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals3d;
  connectivityInternals3d.SetPointDimensions(vtkm::Id3(3, 2, 2));
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
    connectivity3d(connectivityInternals3d);
  TryStructuredPointCoordinates(connectivity3d, coordinates);

  std::cout << "2D" << std::endl;
  vtkm::internal::ConnectivityStructuredInternals<2> connectivityInternals2d;
  connectivityInternals2d.SetPointDimensions(vtkm::Id2(3, 2));
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 2>
    connectivity2d(connectivityInternals2d);
  TryStructuredPointCoordinates(connectivity2d, coordinates);

  std::cout << "1D" << std::endl;
  vtkm::internal::ConnectivityStructuredInternals<1> connectivityInternals1d;
  connectivityInternals1d.SetPointDimensions(3);
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 1>
    connectivity1d(connectivityInternals1d);
  TryStructuredPointCoordinates(connectivity1d, coordinates);
}

void TestArrayTopologyMapIn()
{
  vtkm::testing::Testing::TryTypes(TryType(), vtkm::TypeListCommon());

  TryStructuredPointCoordinates();
}

} // anonymous namespace

int UnitTestFetchArrayTopologyMapIn(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestArrayTopologyMapIn, argc, argv);
}
