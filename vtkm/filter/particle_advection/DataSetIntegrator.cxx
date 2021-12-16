//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/particle_advection/DataSetIntegrator.h>

namespace vtkm
{
namespace filter
{
namespace particle_advection
{

using GridEvalType = vtkm::worklet::particle_advection::GridEvaluator<
  vtkm::worklet::particle_advection::VelocityField<vtkm::cont::ArrayHandle<vtkm::Vec3f>>>;
using TemporalGridEvalType = vtkm::worklet::particle_advection::TemporalGridEvaluator<
  vtkm::worklet::particle_advection::VelocityField<vtkm::cont::ArrayHandle<vtkm::Vec3f>>>;
using RK4Type = vtkm::worklet::particle_advection::RK4Integrator<GridEvalType>;
using Stepper = vtkm::worklet::particle_advection::Stepper<RK4Type, GridEvalType>;
using TemporalRK4Type = vtkm::worklet::particle_advection::RK4Integrator<TemporalGridEvalType>;
using TemporalStepper =
  vtkm::worklet::particle_advection::Stepper<TemporalRK4Type, TemporalGridEvalType>;

//-----
// Specialization for ParticleAdvection worklet
template <>
template <>
void DataSetIntegratorBase<GridEvalType>::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const Stepper& stepper,
  vtkm::Id maxSteps,
  vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>& result) const
{
  vtkm::worklet::ParticleAdvection Worklet;
  result = Worklet.Run(stepper, seeds, maxSteps);
}

//-----
// Specialization for Streamline worklet
template <>
template <>
void DataSetIntegratorBase<GridEvalType>::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const Stepper& stepper,
  vtkm::Id maxSteps,
  vtkm::worklet::StreamlineResult<vtkm::Particle>& result) const
{
  vtkm::worklet::Streamline Worklet;
  result = Worklet.Run(stepper, seeds, maxSteps);
}

//-----
// Specialization for PathParticle worklet
template <>
template <>
void DataSetIntegratorBase<TemporalGridEvalType>::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const TemporalStepper& stepper,
  vtkm::Id maxSteps,
  vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>& result) const
{
  vtkm::worklet::ParticleAdvection Worklet;
  result = Worklet.Run(stepper, seeds, maxSteps);
}

//-----
// Specialization for Pathline worklet
template <>
template <>
void DataSetIntegratorBase<TemporalGridEvalType>::DoAdvect(
  vtkm::cont::ArrayHandle<vtkm::Particle>& seeds,
  const TemporalStepper& stepper,
  vtkm::Id maxSteps,
  vtkm::worklet::StreamlineResult<vtkm::Particle>& result) const
{
  vtkm::worklet::Streamline Worklet;
  result = Worklet.Run(stepper, seeds, maxSteps);
}

} // namespace particle_advection
} // namespace filter
} // namespace vtkm
