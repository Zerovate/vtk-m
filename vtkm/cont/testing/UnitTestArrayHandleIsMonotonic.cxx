//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIsMonotonic.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{
template <typename T>
void CheckArray(const std::vector<T>& input)
{
  auto array = vtkm::cont::make_ArrayHandle(input, vtkm::CopyFlag::Off);
  bool isInc = vtkm::cont::IsMonotonicIncreasing(array);
  bool isDec = vtkm::cont::IsMonotonicDecreasing(array);

  if (input.empty() || input.size() == 1)
  {
    VTKM_TEST_ASSERT(isInc && isDec,
                     "Array with zero or 1 should be both monotonic increasing and decreasing");
    return;
  }

  VTKM_TEST_ASSERT(isInc, "Array is not monotonic increasing");
  VTKM_TEST_ASSERT(!isDec, "Array should not be monotonic decreasing");

  //Check the reverse of the array
  auto copy = input;
  std::reverse(copy.begin(), copy.end());
  array = vtkm::cont::make_ArrayHandle(copy, vtkm::CopyFlag::Off);
  isInc = vtkm::cont::IsMonotonicIncreasing(array);
  isDec = vtkm::cont::IsMonotonicDecreasing(array);

  VTKM_TEST_ASSERT(!isInc, "Reversed array is not monotonic decreasing");
  VTKM_TEST_ASSERT(isDec, "Reversed array is not monotonic increasing");
}

template <typename OutputType, typename InputType>
std::vector<OutputType> ConvertVec(const std::vector<InputType>& input)
{
  std::vector<OutputType> output;
  output.reserve(input.size());

  for (const auto& value : input)
    output.push_back(static_cast<OutputType>(value));
  return output;
}

void CheckTypes(const std::vector<vtkm::Id>& input)
{
  CheckArray(input);
  CheckArray(ConvertVec<vtkm::Float32>(input));
  CheckArray(ConvertVec<vtkm::Float64>(input));
  CheckArray(ConvertVec<vtkm::IdComponent>(input));
}

} //namespace anonymous

void TestArrayHandleIsMonotonic()
{
  CheckTypes({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
  CheckTypes({ -5, -4, -3, -2, -1, 0, 1, 2, 3, 4 });

  //check duplicate values
  CheckTypes({ 0, 1, 1, 2, 3, 4, 4, 5, 6 });
  CheckTypes({ -3, -2, -2, -1, 0, 0, 1, 2, 3 });

  //check empty and single element arrays
  CheckTypes({});
  CheckTypes({ 0 });
}

int UnitTestArrayHandleIsMonotonic(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleIsMonotonic, argc, argv);
}
