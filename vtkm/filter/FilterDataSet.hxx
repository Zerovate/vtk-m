//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/FieldMetadata.h>
#include <vtkm/filter/FilterTraits.h>
#include <vtkm/filter/PolicyDefault.h>

#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/filter/internal/ResolveFieldTypeAndMap.h>

namespace vtkm
{
namespace filter
{

//----------------------------------------------------------------------------
inline VTKM_CONT FilterDataSet::FilterDataSet() {}

//----------------------------------------------------------------------------
inline VTKM_CONT FilterDataSet::~FilterDataSet() {}

}
}
