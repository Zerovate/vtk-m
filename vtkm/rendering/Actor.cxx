//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/Actor.h>

#include <vtkm/Assert.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/UnknownCellSet.h>

namespace vtkm
{
namespace rendering
{

struct Actor::InternalsType
{
  vtkm::cont::UnknownCellSet Cells;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::Field ScalarField;
  vtkm::cont::ColorTable ColorTable;
  vtkm::cont::Field Normals;
  vtkm::rendering::MaterialGeneral Material;

  vtkm::Range ScalarRange;
  vtkm::Bounds SpatialBounds;

  VTKM_CONT
  InternalsType(const vtkm::cont::UnknownCellSet& cells,
                const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::Field& scalarField,
                const vtkm::rendering::Color& color)
    : Cells(cells)
    , Coordinates(coordinates)
    , ScalarField(scalarField)
    , ColorTable(vtkm::Range{ 0, 1 }, color.Components, color.Components)
  {
  }

  VTKM_CONT
  InternalsType(const vtkm::cont::UnknownCellSet& cells,
                const vtkm::cont::CoordinateSystem& coordinates,
                const vtkm::cont::Field& scalarField,
                const vtkm::cont::ColorTable& colorTable = vtkm::cont::ColorTable::Preset::Default)
    : Cells(cells)
    , Coordinates(coordinates)
    , ScalarField(scalarField)
    , ColorTable(colorTable)
  {
  }
};

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField)
  : Internals(new InternalsType(cells, coordinates, scalarField))
{
  this->Init(coordinates, scalarField);
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField,
             const vtkm::rendering::Color& color)
  : Internals(new InternalsType(cells, coordinates, scalarField, color))
{
  this->Init(coordinates, scalarField);
}

Actor::Actor(const vtkm::cont::UnknownCellSet& cells,
             const vtkm::cont::CoordinateSystem& coordinates,
             const vtkm::cont::Field& scalarField,
             const vtkm::cont::ColorTable& colorTable)
  : Internals(new InternalsType(cells, coordinates, scalarField, colorTable))
{
  this->Init(coordinates, scalarField);
}

void Actor::Init(const vtkm::cont::CoordinateSystem& coordinates,
                 const vtkm::cont::Field& scalarField)
{
  this->Internals->Material = vtkm::rendering::PhongMaterial();
  scalarField.GetRange(&this->Internals->ScalarRange);
  this->Internals->SpatialBounds = coordinates.GetBounds();
}

void Actor::Render(vtkm::rendering::Mapper& mapper,
                   vtkm::rendering::Canvas& canvas,
                   const vtkm::rendering::Camera& camera,
                   const vtkm::rendering::LightCollection& lights) const
{
  mapper.SetCanvas(&canvas);
  mapper.SetActiveColorTable(this->Internals->ColorTable);
  mapper.SetNormals(this->Internals->Normals);
  mapper.SetMaterial(this->Internals->Material);
  mapper.SetLights(lights);
  mapper.RenderCells(this->Internals->Cells,
                     this->Internals->Coordinates,
                     this->Internals->ScalarField,
                     this->Internals->ColorTable,
                     camera,
                     this->Internals->ScalarRange);
}

const vtkm::cont::UnknownCellSet& Actor::GetCells() const
{
  return this->Internals->Cells;
}

const vtkm::cont::CoordinateSystem& Actor::GetCoordinates() const
{
  return this->Internals->Coordinates;
}

const vtkm::cont::Field& Actor::GetScalarField() const
{
  return this->Internals->ScalarField;
}

const vtkm::cont::ColorTable& Actor::GetColorTable() const
{
  return this->Internals->ColorTable;
}

const vtkm::Range& Actor::GetScalarRange() const
{
  return this->Internals->ScalarRange;
}

const vtkm::Bounds& Actor::GetSpatialBounds() const
{
  return this->Internals->SpatialBounds;
}

void Actor::SetScalarRange(const vtkm::Range& scalarRange)
{
  this->Internals->ScalarRange = scalarRange;
}

const vtkm::cont::Field& Actor::GetNormals() const
{
  return this->Internals->Normals;
}

void Actor::SetNormals(const vtkm::cont::Field& normals)
{
  this->Internals->Normals = normals;
}

const vtkm::rendering::MaterialGeneral& Actor::GetMaterial() const
{
  return this->Internals->Material;
}

void Actor::SetMaterial(const vtkm::rendering::MaterialGeneral& material)
{
  this->Internals->Material = material;
}

}
} // namespace vtkm::rendering
