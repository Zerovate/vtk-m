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

#ifndef vtk_m_worklet_particleadvection_DormandPrince_hxx
#define vtk_m_worklet_particleadvection_DormandPrince_hxx

#include <limits>
#include <vector>
#include <array>
#include <utility>
#include <algorithm>
#include <vtkm/Types.h>
#include <vtkm/Bounds.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm { namespace worklet { namespace particleadvection {
// See:
// https://www.ams.org/journals/mcom/1986-46-173/S0025-5718-1986-0815836-3/S0025-5718-1986-0815836-3.pdf

template<typename Real>
struct ODEParameters {
    // Maximum acceptable value of ‖xᵢ - x̂ᵢ‖_∞.
    // Step is rejected and stepsize reduced if estimated error exceeds this threshold.
    Real MaxAcceptableErrorPerStep;
    // Default number of points we assume will be generated via the solve:
    vtkm::Id AssumedSkeletonPoints = 256;
    // A dense skeleton adds "superfluous points" to the skeleton which are not strictly required for accuracy.
    // These points double the size of the skeleton, at the cost of only one extra function evaluation.
    // Why do we sometimes need this? Well, in computer graphics, we often want to visualize the solution,
    // and the Dormand-Prince method is so effective that the points are *extremely* far apart:
    // " . . .the popular Runge-Kutta codes implement formulas of orders 3-5.
    // At these orders, the formulas do considerably more work per step than, say, the Adams methods.
    // To be competitive, and they are, they must take comparatively large steps, with the
    // consequence that the solution can change a relatively large amount in the course of a
    // single step. For this reason, it is seen in experiment that at the cruder tolerances
    // interpolation at these orders is often not as accurate as one might wish. Asymptotically
    // the interpolant is fully justified; the difficulty is that at common accuracy requests, the
    // asymptotic results may be somewhat optimistic."
    // -Lawrence Shampine, Interpolation of Runge-Kutta formalus, https://epubs.siam.org/doi/pdf/10.1137/0722060
    bool DenseSkeleton = true;
};


// The Butcher tableau is given in Wikipedia:
// https://en.wikipedia.org/wiki/Dormand–Prince_method
// See the bottom row of the Dormand-Prince Butcher tableau.
template<typename Real>
struct DormandPrinceButcherTableau {
    const std::array<Real, 7> b1{Real(35)/384, 0, Real(500)/1113, Real(125)/192, -Real(2187)/6784, Real(11)/84, 0};
    // The second row is used for the error estimate:
    const std::array<Real, 7> b2{Real(5179)/57600, 0, Real(7571)/16695, Real(393)/640, -Real(92097)/339200, Real(187)/2100, Real(1)/40};
    // See "Some Practical Runge-Kutta Formulas", Lawrence Shampine, page 149.
    // I have converted Shampine's rational numbers to floats at high precision instead of just leaving them in the source
    // because they are so huge they aren't representable in 32 bit floats. (For double precision we'd be fine leaving the compiler to create floats from the rationals.)
    // His notation calls this c*; we'll call it b_star to match our notation above.
    // This array is only used to produce dense output.
    // {6025192743/30085553152, 0, 51252292925/65400821598, -2691868925/45128329728, 187940372067/1594534317056, -1776094331/19743644256, 11237099/235043384}
    const std::array<Real, 7> b_star{0.200268637660047899923020,
                                     0,
                                     0.783664358836851809789400,
                                    -0.059649203531896335642728,
                                     0.117865366744815903176507,
                                    -0.08995777618208722246945250,
                                     0.0478086164722679452232529};

    // Since this matrix is lower triangular, we could get away with using less storage.
    // However, IIRC then the indexing requires divisions. So let's just use a square matrix:
    const std::array<Real, 6*6> a{Real(1)/5,0,0,0,0,0,
                                  Real(3)/40, Real(9)/40,0,0,0,0,
                                  Real(44)/45, -Real(56)/15, Real(32)/9,0,0,0,
                                  Real(19372)/6561, -Real(25360)/2187, Real(64448)/6561, -Real(212)/729,0,0,
                                  Real(9017)/3168, -Real(355)/33, Real(46732)/5247, Real(49)/176, -Real(5103)/18656,0,
                                  Real(35)/384, 0, Real(500)/1113, Real(125)/192, -Real(2187)/6784, Real(11)/84};
    const std::array<Real, 6> c{Real(1)/5, Real(3)/10, Real(4)/5, Real(8)/9, 1, 1};
};

template<typename Real, vtkm::IdComponent dimension>
class DormandPrince {
public:

    DormandPrince(ODEParameters<Real> const & params) {
        static_assert(dimension >= 1, "The spacial dimension must be >= 1.");
        times_.reserve(params.AssumedSkeletonPoints);
        skeleton_.reserve(params.AssumedSkeletonPoints);
        skeleton_tangent_.reserve(params.AssumedSkeletonPoints);
        rejected_steps_ = 0;
    }

    std::vector<Real> const & times() const { return times_; }

    std::vector<vtkm::Vec<Real, dimension>> const & skeleton() const { return skeleton_; }

    std::vector<vtkm::Vec<Real, dimension>> const & skeleton_tangent() const { return skeleton_tangent_; }

    vtkm::Id rejected_steps() const { return rejected_steps_; }

    // [t₀, t_f]
    std::pair<Real, Real> support() const {
        return std::make_pair(times_.front(), times_.back());
    }

    // Return value of solution at time t.
    // This uses quintic Hermite interpolation in order to respect the order of accuracy
    // provided by the Dormand-Prince method.
    // See: http://www.personal.psu.edu/jjb23/web/htmls/sl455SP12/ch3/CH03_4B.pdf
    // This reference explicitly gives the divided-difference table that we use here.
    vtkm::Vec<Real, dimension> operator()(Real t) const {
        if  (t < times_[0] || t > times_.back()) {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa t = " << t << ", which is outside of allowed range ["
                << times_[0] << ", " << times_.back() << "]";
            throw std::domain_error(oss.str());
        }
        // We need x := (t-tᵢ)/(t_{i+1}-tᵢ) \in [0,1) for this to work.
        // Sadly this neccessitates this loathesome check, otherwise we get x = 1 at t = tf.
        if (t == times_.back())
        {
            return skeleton_.back();
        }

        auto it = std::upper_bound(times_.begin(), times_.end(), t);
        auto i = std::distance(times_.begin(), it) - 1;
        Real t0 = *(it-1);
        Real t1 = *it;
        auto& y0 = skeleton_[i];
        auto& y1 = skeleton_[i+1];
        auto& s0 = skeleton_tangent_[i];
        auto& s1 = skeleton_tangent_[i+1];
        Real dt = (t1-t0);
        Real x = (t-t0)/dt;

        // See the section 'Representations' in the page
        // https://en.wikipedia.org/wiki/Cubic_Hermite_spline
        return (1-x)*(1-x)*(y0*(1+2*x) + s0*(t-t0))
              + x*x*(y1*(3-2*x) + dt*s1*(x-1));
    }

    // Return value of derivative of the solution at time t:
    vtkm::Vec<Real, dimension> prime(Real t) const {
        if  (t < times_[0] || t > times_.back()) {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa t = " << t << ", which is outside of allowed range ["
                << times_[0] << ", " << times_.back() << "]";
            throw std::domain_error(oss.str());
        }
        if (t == times_.back())
        {
            return skeleton_tangent_.back();
        }

        auto it = std::upper_bound(times_.begin(), times_.end(), t);
        auto i = std::distance(times_.begin(), it) - 1;
        Real t0 = *(it-1);
        Real t1 = *it;
        auto y0 = skeleton_[i];
        auto y1 = skeleton_[i+1];
        auto s0 = skeleton_tangent_[i];
        auto s1 = skeleton_tangent_[i+1];
        Real dt = (t1-t0);

        auto d1 = (y1 - y0 - s0*dt)/(dt*dt);
        auto d2 = (s1 - s0)/(2*dt);
        auto c2 = 3*d1 - 2*d2;
        auto c3 = 2*(d2 - d1)/dt;
        return s0 + 2*c2*(t-t0) + 3*c3*(t-t0)*(t-t0);
    }

    // Return value of second derivative of the solution at time t:
    vtkm::Vec<Real, dimension> double_prime(Real t) const {
        if  (t < times_[0] || t > times_.back()) {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa t = " << t << ", which is outside of allowed range ["
                << times_[0] << ", " << times_.back() << "]";
            throw std::domain_error(oss.str());
        }

        auto it = std::upper_bound(times_.begin(), times_.end(), t);
        auto i = std::distance(times_.begin(), it) - 1;
        Real t0 = *(it-1);
        Real t1 = *it;
        auto const & p0 = skeleton_[i];
        auto const & p1 = skeleton_[i+1];
        auto s0 = skeleton_tangent_[i];
        auto s1 = skeleton_tangent_[i+1];
        Real dt = (t1-t0);
        Real x = (t-t0)/dt;
        // See: https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Representations
        Real h00_dbl_prime = 6*(2*x-1);
        Real h10_dbl_prime = 2*(3*x-2);
        Real h01_dbl_prime = -h00_dbl_prime;
        Real h11_dbl_prime = 2*(3*x-1);

        return h00_dbl_prime*p0/(dt*dt) + h10_dbl_prime*s0/dt + h01_dbl_prime*p1/(dt*dt) + h11_dbl_prime*s1/dt;
    }

    // Return value of third derivative of the solution at time t:
    vtkm::Vec<Real, dimension> triple_prime(Real t) const {
        if  (t < times_[0] || t > times_.back()) {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<Real>::digits10+3);
            oss << "Requested abscissa t = " << t << ", which is outside of allowed range ["
                << times_[0] << ", " << times_.back() << "]";
            throw std::domain_error(oss.str());
        }

        auto it = std::upper_bound(times_.begin(), times_.end(), t);
        auto i = std::distance(times_.begin(), it) - 1;
        Real t0 = *(it-1);
        Real t1 = *it;
        auto const & p0 = skeleton_[i];
        auto const & p1 = skeleton_[i+1];
        auto s0 = skeleton_tangent_[i];
        auto s1 = skeleton_tangent_[i+1];
        Real dt = (t1-t0);
        // See: https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Representations
        Real h00_triple_prime = 12;
        Real h10_triple_prime = 6;
        Real h01_triple_prime = -h00_triple_prime;
        Real h11_triple_prime = 6;

        return h00_triple_prime*p0/(dt*dt*dt) + h10_triple_prime*s0/(dt*dt) + h01_triple_prime*p1/(dt*dt*dt) + h11_triple_prime*s1/(dt*dt);
    }

    // Returns κ = 1/R, where R is the radius of the oscullating circle to the ODE solution at time t.
    Real curvature(Real t) const {
        auto dvdt = this->prime(t);
        auto d2vdt2 = this->double_prime(t);
        Real dvdt_norm = vtkm::Magnitude(dvdt);

        // For notation, see: https://en.wikipedia.org/wiki/Differentiable_curve#Curvature
        auto e1_prime = d2vdt2/dvdt_norm - vtkm::Dot(dvdt, d2vdt2)*dvdt/(2*dvdt_norm*dvdt_norm*dvdt_norm);
        return vtkm::Magnitude(e1_prime)/dvdt_norm;
    }

    // Returns a number quantifying how fast the curve is twisting out of the plane of curvature.
    // https://en.wikipedia.org/wiki/Torsion_of_a_curve
    Real torsion(Real t) const {
        static_assert(dimension == 3, "Torsion is undefined in dimension < 3; and we have only implemented in dimension 3.");

        auto dvdt = this->prime(t);
        auto d2vdt2 = this->double_prime(t);
        auto d3vdt3 = this->triple_prime(t);

        auto cross = vtkm::Cross(dvdt, d2vdt2);
        Real numerator = vtkm::Dot(cross, d3vdt3);
        Real denominator = vtkm::Dot(cross, cross);
        // Is this the correct limit???
        if (denominator == 0) {
            return Real(0);
        }
        return numerator/denominator;
    }

    vtkm::Vec<vtkm::Vec<Real, dimension>, dimension> frenet_frame(Real t) const {
        static_assert(dimension <= 3, "We cannot take more than 3 derivatives of a Hermite spline, so we cannot get the Frenet frame in more than 3 dimensions");
        vtkm::Vec<vtkm::Vec<Real, dimension>, dimension> derivatives;
        derivatives[0] = this->prime(t);
        derivatives[1] = this->double_prime(t);
        derivatives[2] = this->triple_prime(t);
        vtkm::Vec<vtkm::Vec<Real, dimension>, dimension> frame;
        int num_vecs = vtkm::Orthonormalize(derivatives, frame, std::numeric_limits<Real>::epsilon());
        if (num_vecs != dimension) {
            VTKM_LOG_S(vtkm::cont::LogLevel::Error, "Orthogonalization failed due to numerically collinear vectors");
        }
        return frame;
    }

    // List of tᵢs:
    std::vector<Real> times_;
    // Solution skeleton. This mirrors the language of Corless's book, "A Graduate Introduction to Numerical Methods".
    // A list of vᵢs at time tᵢ:
    std::vector<vtkm::Vec<Real, dimension>> skeleton_;
    // Derivatives (evaluations of right-hand side) at solution skeleton.
    // A list of f(tᵢ, vᵢ) where v'(t) = f(t,v) is the ODE.
    std::vector<vtkm::Vec<Real, dimension>> skeleton_tangent_;
    // The number of rejected steps is a common measure of ODE stepper efficiency:
    vtkm::Id rejected_steps_;
};

}}}
#endif
