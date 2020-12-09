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

#ifndef vtk_m_worklet_particleadvection_DormandPrinceAutonomous_hxx
#define vtk_m_worklet_particleadvection_DormandPrinceAutonomous_hxx

#include <limits>
#include <vector>
#include <algorithm>
#include <vtkm/Types.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/worklet/particleadvection/DormandPrince.hxx>

namespace vtkm { namespace worklet { namespace particleadvection {

// It is not strictly required that we split the integrator into the autonomous and non-autonomous case,
// since (e.g.) a 3+1D non-autonomous ODE is a 4D autonomous ODE.
// However, the user must engage in some data layout tedium, and it's bit faster to have an explicitly autonomous case, so we can just be nice and provide both.
template<typename Real, vtkm::IdComponent dimension>
class DormandPrinceAutonomous : public DormandPrince<Real, dimension> {
public:
    static_assert(dimension >= 1, "The spacial dimension must be >= 1");
    using DormandPrince<Real, dimension>::times_;
    using DormandPrince<Real, dimension>::skeleton_;
    using DormandPrince<Real, dimension>::skeleton_tangent_;
    using DormandPrince<Real, dimension>::rejected_steps_;


    DormandPrinceAutonomous() = delete;

    // Solve the ODE on an unbounded domain.
    template<class RHS>
    DormandPrinceAutonomous(const RHS& f, vtkm::Vec<Real, dimension> const & InitialConditions, ODEParameters<Real> const & params) : DormandPrince<Real, dimension>(params)
    {
        // Solve the ODE in the constructor.
        // There is little hope for parallelism in the body of an adaptive ODE stepper.
        // This design makes it clear that we must parallelize over particles, not within solutions.s
        Real dt = params.MaxTimeOfPropagation/params.assumed_skeleton_points;
        times_.push_back(params.t0);
        skeleton_.push_back(InitialConditions);
        auto y = f(InitialConditions);
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
                k[i] = f(skeleton_.back() + dt*dy);
            }

            auto dy = bt.b1[0]*k[0];
            for (size_t i = 1; i < k.size(); ++i) {
                dy += bt.b1[i]*k[i];
            }
            auto y1 = dt*dy;
            // y2 is for the error estimate:
            dy = bt.b2[0]*k[0];
            for (size_t i = 1; i < k.size(); ++i) {
                dy += bt.b2[i]*k[i];
            }
            auto y2 = dt*dy;

            Real error = 0;
            for (vtkm::IdComponent i = 0; i < dimension; ++i) {
                Real error_i = vtkm::Abs(y1[i] - y2[i]);
                if (error_i > error) {
                    error = error_i;
                }
            }
            // Don't add in y0 until after the error is computed:
            y1 += skeleton_.back();
            //std::cout << "Sup norm error estimate is " << error << " at step " << dt << ", max error is " << params.MaxAcceptableErrorPerStep << "\n";

            // TODO: Use theory of optimal control to choose the best step.
            // Numerical Recipes has a good treatment of this topic.
            if (error > params.MaxAcceptableErrorPerStep) {
                //std::cout << "\tThis step has been rejected.\n";
                ++rejected_steps_;
                dt *= 0.75;
            } else {
                times_.push_back(times_.back() + dt);
                skeleton_.push_back(y1);
                skeleton_tangent_.push_back(f(y1));
                // See: "A family of embedded Runge-Kutta formulae" J. R. Dormand and P. J. Prince,
                // Journal of Computational and Applied Mathematics, volume 6, no 1, 1980.
                // top left of page 21:
                if (error > 0) {
                    Real newdt = 0.9*dt*vtkm::Pow(params.MaxAcceptableErrorPerStep/error, Real(1)/5);
                    // See Numerical Recipes: "Experience shows that it is unwise to let the step grow too much."
                    // This really helps!!!
                    if (newdt > 5*dt) {
                        dt = 5*dt;
                    }
                } else {
                    // This else branch is hit very infrequently.
                    // It's mainly used because the unit tests call right-hand sides that generate constants, lines,
                    // parabolas; things that the Dormand-Prince method integrates exactly.
                    dt *= 2.0;
                }
            }
        }

        times_.shrink_to_fit();
        skeleton_.shrink_to_fit();
        skeleton_tangent_.shrink_to_fit();
    }

    // Solve the ODE on a domain defined by a function.
    template<class RHS>
    DormandPrinceAutonomous(const RHS& f, vtkm::Bounds bounds, vtkm::Vec<Real, dimension> const & InitialConditions, ODEParameters<Real> const & params) : DormandPrince<Real, dimension>(params)
    {
        static_assert(dimension >= 3, "VTK-m Bounds must have dimension > 3");
        // TODO: Implement this with maximum code reuse!
        if (!bounds.Contains(vtkm::Vec<Real, 3>(InitialConditions[0]), InitialConditions[1], InitialConditions[2])) {
            VTKM_LOG_S(vtkm::cont::LogLevel::Error,
                       __FILE__ << ":" << __LINE__ << " Initial condition " << InitialConditions << " is not in spatial boundary.");
            return;
        }

        Real dt = params.MaxTimeOfPropagation/params.assumed_skeleton_points;
        times_.push_back(params.t0);
        skeleton_.push_back(InitialConditions);
        auto y = f(InitialConditions);
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
                auto ep = skeleton_.back() + dt*dy;
                if (!bounds.Contains(vtkm::Vec<Real, 3>(ep[0], ep[1], ep[2]))) {
                    // Snap to boundary:

                }

                k[i] = f(skeleton_.back() + dt*dy);
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
                dt /= 2;
            } else {
                times_.push_back(times_.back() + dt);
                skeleton_.push_back(y1);
                skeleton_tangent_.push_back(f(y1));
                dt *= 1.5;
            }
        }

        times_.shrink_to_fit();
        skeleton_.shrink_to_fit();
        skeleton_tangent_.shrink_to_fit();
    }

};

}}}
#endif
