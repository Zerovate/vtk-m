//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/worklet/particleadvection/DormandPrinceAutonomous.hxx>

#include <vtkm/io/VTKDataSetWriter.h>

using vtkm::worklet::particleadvection::DormandPrinceAutonomous;
using vtkm::worklet::particleadvection::ODEParameters;

template<typename Real>
void WriteSolution(DormandPrinceAutonomous<Real, 2> const & dp) {
  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  // We need *way* more points to render in Paraview than we actually need for an accurate solution.
  // If this wasn't the case, we could just write the skeleton directly to the vtkm dataset.
  vtkm::Id lineSegments = 10*dp.skeleton().size();
  auto supp = dp.support();
  Real t0 = supp.first;
  Real tf = supp.second;
  std::vector<Real> times(lineSegments);
  std::vector<Real> curvatures(lineSegments);
  for (vtkm::Id i = 0; i < lineSegments; ++i)
  {
    vtkm::FloatDefault t = t0 + i*(tf - t0) / lineSegments;
    auto p = dp(t);
    vtkm::Id pid = dsb.AddPoint(p[0], p[1]);
    times[i] = t;
    curvatures[i] = dp.curvature(t);
    ids.push_back(pid);
  }
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  vtkm::cont::DataSet ds = dsb.Create();
  ds.AddPointField("time", times);
  ds.AddPointField("curvature", curvatures);
  auto writer = vtkm::io::VTKDataSetWriter("vanderpol.vtk");
  writer.WriteDataSet(ds);
  std::cout << "Now open 'vanderpol.vtk' in Paraview to examine the solution.\n";
}

// This example is taken from The Princeton Companion to Applied Mathematics, IV.2, section 10.
// "The van der Pol oscillator was introduced as a model of a nonlinear circuit with a triode tube."
template<typename Real>
void SolveVanderPolSystem(Real mu, vtkm::Vec<Real, 2> const & initialConditions) {
    // The equation to be solved is:
    // dx/dt = y
    // dy/dt = -x + 2mu y - x^2 y
    // Since t does not appear in the RHS, this is an autonomous equation.
    // Therefore we define the RHS via:
    auto f = [&mu](vtkm::Vec<Real, 2> const & v) -> vtkm::Vec<Real, 2> {
        vtkm::Vec<Real, 2> dvdt;
        dvdt[0] = v[1];
        dvdt[1] = -v[0] + 2*mu*v[1] - v[0]*v[0]*v[1];
        return dvdt;
    };


    ODEParameters<Real> parameters;
    // How long do we want to integrate the solution?
    std::pair<Real, Real> tspan{0.0, 500.0};
    // On each step, what error is acceptable to us?
    parameters.MaxAcceptableErrorPerStep = 0.05;
    // How many points do we expect to compute?
    // In general, you don't need to worry about this parameter,
    // because the stepper resizes the vectors if you guess wrong.
    // But if you guess right, you don't do any resizing!
    parameters.AssumedSkeletonPoints = 256;
    // Pass the rhs of v' = f(v), the initial conditions, and the parameter to the constructor:
    auto dp = DormandPrinceAutonomous<Real, 2>(f, initialConditions, tspan, parameters);
    // Note that the constructor solves the equation!
    // the dp object is an interpolator, so we can query it at any time:

    Real t = 0.1;
    std::cout << "At time " << t << ", the solution is " << dp(t) << "\n";
    std::cout << "At time " << t << ", the derivative of the solution is " << dp.prime(t) << "\n";

    // The interpolator is the best way to think about the solution, since the Dormand-Prince method is *very* accurate.
    // This generates very sparse solution skeletons:
    std::cout << "The van der Pol oscillator was solved in "  << dp.skeleton().size() << " steps.\n";

    // One measure of efficiency of an adaptive ODE solver is the number of rejected steps:
    std::cout << dp.rejected_steps() << " steps were rejected.\n";

    // We cannot query the solution past the final timestep, nor before the first timestep.
    // But what are the values we can query?
    auto support = dp.support();
    std::cout << "The solution is defined on the interval [" << support.first << ", " << support.second << "].\n";

    // Now we wish to render this object.
    // Unfortunately, we do not have the capability to render callables,
    // so we cannot pass the Dormand-Prince object as a callback to a rendering engine.
    // We do, however, have the ability to render polylines.
    // It's a bit sad to regenerate a ton of data, after using a sophisticated ODE stepper whose entire purpose is to
    // *not* generate a ton of data, but this is where we're at!
    WriteSolution(dp);
}

int main() {
    // These are the same parameters and initial conditions chosen in The Princeton Companion to Applied Mathematics, page 189.
    vtkm::Vec<double, 2> initialConditions{-2, 0};
    SolveVanderPolSystem<double>(0.2, initialConditions);
}
