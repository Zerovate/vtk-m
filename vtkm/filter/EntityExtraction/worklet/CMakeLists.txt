##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# TODO: Do we even need this CMakeList.txt at all?
set(headers
  ExternalFaces.h)

# TODO: do we even need this? Does this mean the "internal" worklet header files will
# be installed as well?
vtkm_declare_headers(${headers})
