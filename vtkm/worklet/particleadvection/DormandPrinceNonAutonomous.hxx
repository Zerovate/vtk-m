//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtk_m_worklet_particleadvection_DormandPrinceNonAutonomous_hxx
#define vtk_m_worklet_particleadvection_DormandPrinceNonAutonomous_hxx

#include <limits>
#include <vector>
#include <algorithm>
#include <vtkm/Types.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/worklet/particleadvection/DormandPrince.hxx>

namespace vtkm { namespace worklet { namespace particleadvection {

// Again, it is not strictly required that we split the integrator into the autonomous and non-autonomous case;
// see the associated comments in DormandPrinceAutonomous.hxx
template<typename Real, vtkm::IdComponent dimension>
class DormandPrinceNonAutonomous : public DormandPrince<Real, dimension> {
public:
    static_assert(dimension >= 1, "The spacial dimension must be >= 1");
    using DormandPrince<Real, dimension>::times_;
    using DormandPrince<Real, dimension>::skeleton_;
    using DormandPrince<Real, dimension>::skeleton_tangent_;
    using DormandPrince<Real, dimension>::rejected_steps_;


    DormandPrinceNonAutonomous() = delete;

    // Solve the ODE on an unbounded domain.
    template<class RHS>
    DormandPrinceNonAutonomous(const RHS& f, vtkm::Vec<Real, dimension> const & InitialConditions, ODEParameters<Real> const & params) : DormandPrince<Real, dimension>(params)
    {
        // Solve the ODE in the constructor.
        // There is little hope for parallelism in the body of an adaptive ODE stepper.
        // This design makes it clear that we must parallelize over particles, not within solutions.s
        Real dt = params.MaxTimeOfPropagation/params.assumed_skeleton_points;
        times_.push_back(params.t0);
        skeleton_.push_back(InitialConditions);
        auto y = f(params.t0, InitialConditions);
        skeleton_tangent_.push_back(y);

        std::array<vtkm::Vec<Real, dimension>, 7> k;
        auto bt = DormandPrinceButcherTableau<Real>();
        while (times_.back() < params.t0 + params.MaxTimeOfPropagation) {
            // We differ from Wikipedia's notation only in the sense we zero index.
            k[0] = skeleton_tangent_.back();
            for (size_t i = 1; i < k.size(); ++i) {
                vtkm::Vec<Real, dimension> dy(0);
                for (size_t j = 0; j < i; ++j) {
                    dy += bt.a[6*(i-1) + j]*k[j];
                }
                k[i] = f(times_.back() + bt.c[i-1]*dt, skeleton_.back() + dt*dy);
            }

            auto dy = bt.b1[0]*k[0];
            for (size_t i = 1; i < k.size(); ++i) {
                dy += bt.b1[i]*k[i];
            }
            auto y1 = skeleton_.back() + dt*dy;
            // y2 is for the error estimate:
            dy = bt.b2[0]*k[0];
            for (size_t i = 1; i < k.size(); ++i) {
                dy += bt.b2[i]*k[i];
            }
            auto y2 = skeleton_.back() + dt*dy;

            Real error = 0;
            for (vtkm::IdComponent i = 0; i < dimension; ++i) {
                Real error_i = vtkm::Abs(y1[i] - y2[i]);
                if (error_i > error) {
                    error = error_i;
                }
            }

            // TODO: Use theory of optimal control to choose the best step.
            // Numerical Recipes has a good treatment of this topic.
            if (error > params.MaxAcceptableErrorPerStep) {
                ++rejected_steps_;
                dt /= 2;
            } else {
                Real t = times_.back() + dt;
                times_.push_back(t);
                skeleton_.push_back(y1);
                skeleton_tangent_.push_back(f(t, y1));
                if (error > 0) {
                    dt = 0.9*dt*vtkm::Pow(params.MaxAcceptableErrorPerStep/error, Real(1)/5);
                } else {
                    dt *= 2.0;
                }
            }
        }

        times_.shrink_to_fit();
        skeleton_.shrink_to_fit();
        skeleton_tangent_.shrink_to_fit();
    }

};

}}}
#endif
