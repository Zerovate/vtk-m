//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/vtkm_filter_common_export.h>

namespace vtkm
{
namespace filter
{
namespace internal
{
VTKM_FILTER_COMMON_EXPORT void RunFilter(Filter* self,
                                         vtkm::filter::DataSetQueue& input,
                                         vtkm::filter::DataSetQueue& output)
{
  auto filterClone = self->Clone();
  VTKM_ASSERT(filterClone != nullptr);

  std::pair<vtkm::Id, vtkm::cont::DataSet> task;
  while (input.GetTask(task))
  {
    auto outDS = CallPrepareForExecution(filterClone, task.second);
    //    filterClone->CallMapFieldOntoOutput(task.second, outDS, policy);
    output.Push(std::make_pair(task.first, std::move(outDS)));
  }

  vtkm::cont::Algorithm::Synchronize();
  delete filterClone;
}
}


}
}
