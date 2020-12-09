//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>
#include <vtkm/Math.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/particleadvection/DormandPrince.hxx>
#include <vtkm/worklet/particleadvection/DormandPrinceAutonomous.hxx>
#include <vtkm/worklet/particleadvection/DormandPrinceNonAutonomous.hxx>

namespace {
using vtkm::worklet::particleadvection::DormandPrinceAutonomous;
using vtkm::worklet::particleadvection::DormandPrinceNonAutonomous;
using vtkm::worklet::particleadvection::ODEParameters;

template<typename Real, vtkm::IdComponent dimension>
void TestConstant() {
    // Solve y' = 0 => y(t) = const.
    auto f = [](vtkm::Vec<Real, dimension> const &) {
      vtkm::Vec<Real, dimension> z(0);
      return z;
    };
    std::random_device rd;
    std::uniform_real_distribution<Real> dis(-1, 1);
    ODEParameters<Real> parameters;
    parameters.MaxAcceptableErrorPerStep = 0.01;
    parameters.MaxTimeOfPropagation = 10;

    vtkm::Vec<Real, dimension> initialConditions;
    for (vtkm::IdComponent i = 0; i < dimension; ++i) {
      initialConditions[i] = dis(rd);
    }
    auto dpa = DormandPrinceAutonomous<Real, dimension>(f, initialConditions, parameters);
    auto const & skeleton = dpa.skeleton();
    for (auto const & bone : skeleton) {
      for (vtkm::IdComponent i = 0; i < dimension; ++i) {
        auto dist = vtkm::FloatDistance(bone[i], initialConditions[i]);
        if (dist >= 5) {
          std::cerr << "Float distance between exact and numerical solution is " << dist << "\n";
          VTKM_TEST_FAIL("Zero RHS's integrating to constants are computed incorrectly by the Dormand-Prince integrator.");
        }
      }
    }

    auto const & skeleton_tangent = dpa.skeleton_tangent();
    VTKM_TEST_ASSERT(skeleton.size() == skeleton_tangent.size(), "Number of points in tangent field should equal the number of points in the solution skeleton");

    for (auto const & bone : skeleton_tangent) {
      for (vtkm::IdComponent i = 0; i < dimension; ++i) {

        VTKM_TEST_ASSERT(bone[i] == 0, // There should be no reason for this to be contaminated by numerical error, so compare the floats for equality.
                         "Zero RHS's integrating to constants are computed incorrectly by the Dormand-Prince integrator.");
      }
    }

    auto const & times = dpa.times();
    for (size_t i = 0; i < times.size() - 1; ++i) {
      // The interpolator should interpolate constants exactly:
      Real t = times[i] + times.back()/(4*times.size());
      auto interpolated = dpa(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated[j], initialConditions[j]);
        if (dist > 5) {
          VTKM_TEST_FAIL("Interpolated value of Dormand-Prince solution is incorrect");
        }
      }

      auto interpolated_prime = dpa.prime(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated_prime[j], 0);
        if (dist > 5) {
          VTKM_TEST_FAIL("Interpolated derivative is incorrect.");
        }
      }

      auto interpolated_dbl_prime = dpa.double_prime(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated_dbl_prime[j], 0);
        if (dist > 5) {
          VTKM_TEST_FAIL("Interpolated second derivative is incorrect.");
        }
      }

      Real kappa = dpa.curvature(t);
      // There is no notion of the curvature of a constant:
      VTKM_ASSERT(std::isnan(kappa));

      t = times[i];
      interpolated = dpa(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated[j], initialConditions[j]);
        if (dist > 5) {
          VTKM_TEST_FAIL("Interpolated value of Dormand-Prince solution is incorrect");
        }
      }

      interpolated_prime = dpa.prime(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated_prime[j], 0);
        if (dist > 5) {
          VTKM_TEST_FAIL("Interpolated derivative is incorrect.");
        }
      }

      interpolated_dbl_prime = dpa.double_prime(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated_dbl_prime[j], 0);
        if (dist > 5) {
          VTKM_TEST_FAIL("Interpolated second derivative is incorrect.");
        }
      }

      kappa = dpa.curvature(t);
      VTKM_ASSERT(std::isnan(kappa));
    }

}

template<typename Real, vtkm::IdComponent dimension>
void TestConstantNonAutonomous() {
    // Solve y' = 0 => y(t) = const.
    auto f = [](Real, vtkm::Vec<Real, dimension> const &) {
      vtkm::Vec<Real, dimension> z(0);
      return z;
    };
    std::random_device rd;
    std::uniform_real_distribution<Real> dis(-1, 1);
    ODEParameters<Real> parameters;
    parameters.MaxAcceptableErrorPerStep = 0.01;
    parameters.MaxTimeOfPropagation = 10;

    vtkm::Vec<Real, dimension> initialConditions;
    for (vtkm::IdComponent i = 0; i < dimension; ++i) {
      initialConditions[i] = dis(rd);
    }
    auto dpna = DormandPrinceNonAutonomous<Real, dimension>(f, initialConditions, parameters);
    auto const & skeleton = dpna.skeleton();
    for (auto const & bone : skeleton) {
      for (vtkm::IdComponent i = 0; i < dimension; ++i) {
        auto dist = vtkm::FloatDistance(bone[i], initialConditions[i]);
        if (dist >= 5) {
          std::cerr << "Float distance between exact and numerical solution is " << dist << "\n";
          VTKM_TEST_FAIL("Zero RHS's integrating to constants are computed incorrectly by the Dormand-Prince integrator.");
        }
      }
    }

    auto const & skeleton_tangent = dpna.skeleton_tangent();
    VTKM_TEST_ASSERT(skeleton.size() == skeleton_tangent.size(), "Number of points in tangent field should equal the number of points in the solution skeleton");

    for (auto const & bone : skeleton_tangent) {
      for (vtkm::IdComponent i = 0; i < dimension; ++i) {

        VTKM_TEST_ASSERT(bone[i] == 0, // There should be no reason for this to be contaminated by numerical error, so compare the floats for equality.
                         "Zero RHS's integrating to constants are computed incorrectly by the Dormand-Prince integrator.");
      }
    }

    auto const & times = dpna.times();
    for (size_t i = 0; i < times.size() - 1; ++i) {
      // Interpolator should interpolate constants exactly:
      Real t = times[i] + times.back()/(4*times.size());
      auto interpolated = dpna(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated[j], initialConditions[j]);
        if (dist > 5) {
          std::cerr << "Float distance is " << dist << "\n";
          VTKM_TEST_FAIL("Non-autonomous Dormand-Prince integrator does not interpolate constants correctly");
        }
      }
    }
}

// Solve y' = constant => y(t) = constant*t + y(0).
template<typename Real, vtkm::IdComponent dimension>
void TestLine() {
    std::random_device rd;
    std::uniform_real_distribution<Real> dis(1, 2);
    ODEParameters<Real> parameters;
    vtkm::Vec<Real, dimension> constant;
    vtkm::Vec<Real, dimension> initialConditions;
    for (vtkm::IdComponent i = 0; i < dimension; ++i) {
      initialConditions[i] = dis(rd);
      constant[i] = dis(rd);
    }

    auto f = [&constant](vtkm::Vec<Real, dimension> const &) {
        return constant;
    };

    parameters.MaxAcceptableErrorPerStep = 0.01;
    parameters.MaxTimeOfPropagation = 10;
    auto dpa = DormandPrinceAutonomous<Real, dimension>(f, initialConditions, parameters);
    auto const & skeleton = dpa.skeleton();
    std::vector<Real> const & times = dpa.times();
    VTKM_TEST_ASSERT(times.size() == skeleton.size(),
                     "Number of points in time table should equal the number of points in the solution skeleton");
    for (size_t i = 0; i < skeleton.size(); ++i) {
      auto const & bone = skeleton[i];
      Real t = times[i];
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(bone[j], initialConditions[j] + constant[j]*t);
        if (dist > 5) {
          VTKM_TEST_FAIL("Float distance is " + std::to_string(dist) + " so constant RHS's integrating to lines are computed incorrectly by the Dormand-Prince integrator.");
        }
      }
    }

    auto const & skeleton_tangent = dpa.skeleton_tangent();
    VTKM_TEST_ASSERT(skeleton.size() == skeleton_tangent.size(),
                     "Number of points in tangent field should equal the number of points in the solution skeleton");

    for (auto const & bone : skeleton_tangent) {
      for (vtkm::IdComponent i = 0; i < dimension; ++i) {
        VTKM_TEST_ASSERT(bone[i] == constant[i], // There should be no reason for this to be contaminated by numerical error, so compare the floats for equality.
                         "Constant RHS's integrating to lines are computed incorrectly by the Dormand-Prince integrator.");
      }
    }

    for (size_t i = 0; i < times.size() - 1; ++i) {
      // Should be exact on a line:
      Real t = times[i] + times.back()/(4*times.size());
      auto interpolated = dpa(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated[j], initialConditions[j] + constant[j]*t);
        if (dist > 5) {
          VTKM_TEST_FAIL("Autonomous Dormand-Prince integrator does not interpolate lines exactly");
        }
      }

      auto interpolated_prime = dpa.prime(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated_prime[j], constant[j]);
        if (dist > 8) {
          std::cerr << "Float distance from interpolated derivative from expected constant is "<< dist << "\n";
          VTKM_TEST_FAIL("Interpolated derivative is incorrect.");
        }
      }

      auto interpolated_double_prime = dpa.double_prime(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto mag = vtkm::Abs(interpolated_double_prime[j]);
        if (mag > 200*std::numeric_limits<Real>::epsilon()) {
          std::cerr << "Interpolated second derivative is " << interpolated_double_prime << ", but expected zero.\n";
          std::cerr << "This is " << mag/std::numeric_limits<Real>::epsilon() << " times the "
                    << vtkm::cont::TypeToString<Real>() << " epsilon\n";
          VTKM_TEST_FAIL("Interpolated second derivative on a line is incorrect.");
        }
      }

      interpolated_double_prime = dpa.double_prime(times[i]);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto mag = vtkm::Abs(interpolated_double_prime[j]);
        // The accuracy of the second derivative at the interpolation nodes is disappointing.
        if (mag > 11000*std::numeric_limits<Real>::epsilon()) {
          std::cerr << "Interpolated second derivative is " << interpolated_double_prime << ", but expected zero.\n";
          std::cerr << "This is " << mag/std::numeric_limits<Real>::epsilon() << " times the "
                    << vtkm::cont::TypeToString<Real>() << " epsilon\n";
          VTKM_TEST_FAIL("Interpolated second derivative on a line is incorrect.");
        }
      }

      Real kappa = dpa.curvature(t);
      if (kappa > 50*std::numeric_limits<Real>::epsilon()) {
        std::cerr << "Curvature of a line should be zero, but is computed to be " << kappa << "\n";
        std::cerr << "This is " << kappa/std::numeric_limits<Real>::epsilon() << " times the "
                  << vtkm::cont::TypeToString<Real>() << " epsilon\n";
        VTKM_TEST_FAIL("Curvature of a line is incorrect.");
      }
    }
}

// Solve y' = constant => y(t) = constant*t + y(0).
template<typename Real, vtkm::IdComponent dimension>
void TestLineNonAutonomous() {
    std::random_device rd;
    std::uniform_real_distribution<Real> dis(1, 2);
    ODEParameters<Real> parameters;
    vtkm::Vec<Real, dimension> constant;
    vtkm::Vec<Real, dimension> initialConditions;
    for (vtkm::IdComponent i = 0; i < dimension; ++i) {
      initialConditions[i] = dis(rd);
      constant[i] = dis(rd);
    }

    auto f = [&constant](Real, vtkm::Vec<Real, dimension> const &) {
        return constant;
    };

    parameters.MaxAcceptableErrorPerStep = 0.01;
    parameters.MaxTimeOfPropagation = 10;
    auto dpna = DormandPrinceNonAutonomous<Real, dimension>(f, initialConditions, parameters);
    auto const & skeleton = dpna.skeleton();
    std::vector<Real> const & times = dpna.times();
    VTKM_TEST_ASSERT(times.size() == skeleton.size(),
                     "Number of points in time table should equal the number of points in the solution skeleton");
    for (size_t i = 0; i < skeleton.size(); ++i) {
      auto const & bone = skeleton[i];
      Real t = times[i];
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(bone[j], initialConditions[j] + constant[j]*t);
        if (dist > 5) {
          VTKM_TEST_FAIL("Float distance is " + std::to_string(dist) + " so constant RHS's integrating to lines are computed incorrectly by the Dormand-Prince integrator.");
        }
      }
    }

    auto const & skeleton_tangent = dpna.skeleton_tangent();
    VTKM_TEST_ASSERT(skeleton.size() == skeleton_tangent.size(),
                     "Number of points in tangent field should equal the number of points in the solution skeleton");

    for (auto const & bone : skeleton_tangent) {
      for (vtkm::IdComponent i = 0; i < dimension; ++i) {
        VTKM_TEST_ASSERT(bone[i] == constant[i], // There should be no reason for this to be contaminated by numerical error, so compare the floats for equality.
                         "Constant RHS's integrating to lines are computed incorrectly by the Dormand-Prince integrator.");
      }
    }

    for (size_t i = 0; i < times.size() - 1; ++i) {
      // Should be exact on a line:
      Real t = times[i] + times.back()/(4*times.size());
      auto interpolated = dpna(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated[j], initialConditions[j] + constant[j]*t);
        if (dist > 5) {
          VTKM_TEST_FAIL("Non-autonomous Dormand-Prince integrator does not interpolate lines exactly");
        }
      }
    }
}

// Solve y' = constant*t => y(t) = constant*t^2/2 + y(0).
template<typename Real, vtkm::IdComponent dimension>
void TestParabolaNonAutonomous() {
    std::random_device rd;
    std::uniform_real_distribution<Real> dis(1, 2);
    ODEParameters<Real> parameters;
    vtkm::Vec<Real, dimension> constant;
    vtkm::Vec<Real, dimension> initialConditions;
    for (vtkm::IdComponent i = 0; i < dimension; ++i) {
      initialConditions[i] = dis(rd);
      constant[i] = dis(rd);
    }

    auto f = [&constant](Real t, vtkm::Vec<Real, dimension> const &) {
        return constant*t;
    };

    parameters.MaxAcceptableErrorPerStep = 0.01;
    parameters.MaxTimeOfPropagation = 10;
    auto dpna = DormandPrinceNonAutonomous<Real, dimension>(f, initialConditions, parameters);
    auto const & skeleton = dpna.skeleton();
    std::vector<Real> const & times = dpna.times();
    VTKM_TEST_ASSERT(times.size() == skeleton.size(),
                     "Number of points in time table should equal the number of points in the solution skeleton");
    for (size_t i = 0; i < skeleton.size(); ++i) {
      auto const & bone = skeleton[i];
      Real t = times[i];
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(bone[j], initialConditions[j] + constant[j]*t*t/2);
        if (dist > 5) {
          VTKM_TEST_FAIL("Float distance is " + std::to_string(dist) + " so constant RHS's integrating to lines are computed incorrectly by the Dormand-Prince integrator.");
        }
      }
    }

    auto const & skeleton_tangent = dpna.skeleton_tangent();
    VTKM_TEST_ASSERT(skeleton.size() == skeleton_tangent.size(),
                     "Number of points in tangent field should equal the number of points in the solution skeleton");

    for (size_t i = 0; i < skeleton_tangent.size(); ++i) {
      auto bone = skeleton_tangent[i];
      Real t = times[i];
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        VTKM_TEST_ASSERT(bone[j] == constant[j]*t, // There should be no reason for this to be contaminated by numerical error, so compare the floats for equality.
                         "Constant RHS's integrating to lines are computed incorrectly by the Dormand-Prince integrator.");
      }
    }

    for (size_t i = 0; i < times.size() - 1; ++i) {
      // Should be exact on a parabola:
      Real t = times[i] + times.back()/(4*times.size());
      auto interpolated = dpna(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated[j], initialConditions[j] + constant[j]*t*t/2);
        if (dist > 15) {
          std::cerr << "The float distance between the parabola and the interpolator is " << dist << "\n";
          VTKM_TEST_FAIL("Non-autonomous Dormand-Prince integrator does not interpolate lines exactly");
        }
      }

      auto interpolated_prime = dpna.prime(t);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated_prime[j], constant[j]*t);
        if (dist > 15) {
          std::cerr << "The float distance between the derivative of the parabola and the interpolator derivative is " << dist << "\n";
          VTKM_TEST_FAIL("Interpolated derivative is incorrect.");
        }
      }
    }
}


template<typename Real, vtkm::IdComponent dimension>
void TestExp() {
    // Solve y' = ky:
    Real k = -2.0;
    auto f = [&k](vtkm::Vec<Real, dimension> const & y) {
        return k*y;
    };
    ODEParameters<Real> parameters;
    parameters.MaxAcceptableErrorPerStep = 0.001;
    parameters.MaxTimeOfPropagation = 3;

    auto initialConditions = vtkm::Vec<Real, dimension>(1);
    auto dpa = DormandPrinceAutonomous<Real, dimension>(f, initialConditions, parameters);

    auto const & skeleton = dpa.skeleton();
    auto const & times = dpa.times();
    for (size_t i = 0; i < times.size(); ++i) {
      Real expected = vtkm::Exp(k*times[i]);
      auto computed = skeleton[i];
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        Real diff = vtkm::Abs(expected - computed[j]);
        if (diff > parameters.MaxAcceptableErrorPerStep) {
          std::cerr << "Expected = " << expected << ", computed = " << computed << ", diff = " << diff << "\n";
          std::cerr << "Failure occurs at time " << times[i] << " of max time " << times.back() << "\n";
          VTKM_TEST_FAIL(std::string("Difference between exact and computed solution is ") + std::to_string(diff) + std::string(", but maximum acceptable error is ") + std::to_string(parameters.MaxAcceptableErrorPerStep));
        }
      }
      // Should be exact at the interpolation node:
      auto interpolated = dpa(times[i]);
      for (vtkm::IdComponent j = 0; j < dimension; ++j) {
        auto dist = vtkm::FloatDistance(interpolated[j], computed[j]);
        if (dist > 5) {
          VTKM_TEST_FAIL("Interpolation of Dormand-Prince solution failed.");
        }
      }
    }
}

// y'(t) = a(t)y(t) => y(t) = exp(\int_{0}^t a(τ) dτ)y(0)
template<typename Real>
void TestOscillatoryNonAutonomous(){
  Real omega = 2;
  auto f = [&omega](Real t, vtkm::Vec<Real, 1> y) {
    return vtkm::Cos(omega*t)*y;
  };

  ODEParameters<Real> parameters;
  parameters.MaxAcceptableErrorPerStep = 0.001;
  parameters.MaxTimeOfPropagation = 2;
  auto initialConditions = vtkm::Vec<Real, 1>(1);
  auto dpna = DormandPrinceNonAutonomous<Real, 1>(f, initialConditions, parameters);
  auto const & skeleton = dpna.skeleton();
  auto const & times = dpna.times();
  for (size_t i = 0; i < times.size(); ++i) {
    Real expected = vtkm::Exp(vtkm::Sin(omega*times[i])/omega);
    auto computed = skeleton[i];

    Real diff = vtkm::Abs(expected - computed[0]);
    // It is not the case that the maximum error is bounded by steps*(maximum error per step)
    // Error can accumulate exponentially. However, without doing insanely difficult error propagation,
    // this is a sensible way to think about it:
    if (diff > (i+1)*parameters.MaxAcceptableErrorPerStep) {
      std::cerr << "Expected = " << expected << ", computed = " << computed << ", diff = " << diff << "\n";
      std::cerr << "Failure occurs at step " << i << " of " << times.size() << " and time " << times[i] << " of max time " << times.back() << "\n";
      VTKM_TEST_FAIL(std::string("Difference between exact and computed solution is ") + std::to_string(diff) + std::string(", but maximum acceptable error is ") + std::to_string((i+1)*parameters.MaxAcceptableErrorPerStep));
    }
  }
}

// A helix obeys the equation:
// dx/dt = -y,
// dy/dt = x,
// dz/dt = c,
// The solution is of course
// (rcos(t), rsin(t), ct)
// and has curvature κ = r/(r^2 + c^2).
template<typename Real>
void TestHelix()
{
  Real r = 2;
  Real c = 2;
  auto f = [&c](vtkm::Vec<Real, 3> v) {
    vtkm::Vec<Real, 3> w{-v[1], v[0], c};
    return w;
  };

  ODEParameters<Real> parameters;
  parameters.MaxAcceptableErrorPerStep = 0.001;
  parameters.MaxTimeOfPropagation = 1;
  auto initialConditions = vtkm::Vec<Real, 3>(r, 0, 0);

  auto dpa = DormandPrinceAutonomous<Real, 3>(f, initialConditions, parameters);
  auto const & times = dpa.times();
  Real kappa_expected = r/(r*r+c*c);
  for (size_t i = 0; i < times.size() - 1; ++i) {
    Real t = times[i] + times.back()/(4*times.size());
    auto v = dpa(t);
    // We cannot use FloatDistance, because we have no theorems that helices are integrated exactly by Dormand-Prince.
    VTKM_TEST_ASSERT(test_equal(v[0], r*vtkm::Cos(t), (i+1)*1e-2));
    VTKM_TEST_ASSERT(test_equal(v[1], r*vtkm::Sin(t), (i+1)*1e-2));
    VTKM_TEST_ASSERT(test_equal(v[2], c*t));

    Real kappa_computed = dpa.curvature(t);
    if (vtkm::Abs(kappa_computed - kappa_expected) > 0.01) {
      std::cerr << "Computed curvature of a helix is " << kappa_computed << ", but expected is " << kappa_expected << "\n";
      std::cerr << "Difference is " << vtkm::Abs(kappa_computed - kappa_expected) << "\n";
      VTKM_TEST_ASSERT(false, "Computed curvature of a helix is wrong.");
    }
  }

}

template<typename Real>
void TestButcherTableau() {
  auto bt = vtkm::worklet::particleadvection::DormandPrinceButcherTableau<Real>();

  Real sum = std::accumulate(bt.b1.begin(), bt.b1.end(), Real(0));
  auto dist = vtkm::FloatDistance(sum, Real(1));
  VTKM_TEST_ASSERT(dist <= 2, "A Runge-Kutta method is consistent iff sum(b_i) = 1.");

  sum = std::accumulate(bt.b2.begin(), bt.b2.end(), Real(0));
  dist = vtkm::FloatDistance(sum, Real(1));
  VTKM_TEST_ASSERT(dist <= 2, "A Runge-Kutta method is consistent iff sum(b_i) = 1.");

  for (size_t i = 0; i < bt.c.size(); ++i) {
    sum = 0;
    for (size_t j = 0; j < 6; ++j) {
      sum += bt.a[6*i + j];
    }
    dist = vtkm::FloatDistance(sum, bt.c[i]);
    VTKM_TEST_ASSERT(dist <= 5, "sum(a_ij) != c_j\n");
  }
}

void TestDormandPrince()
{
  TestConstant<double, 1>();
  TestConstant<float, 2>();
  TestConstant<double, 3>();
  TestConstant<float, 4>();

  TestConstantNonAutonomous<double, 1>();
  TestConstantNonAutonomous<float, 2>();
  TestConstantNonAutonomous<double, 3>();
  TestConstantNonAutonomous<float, 4>();

  TestLine<float, 1>();
  TestLine<double, 1>();
  TestLine<double, 2>();
  TestLine<double, 3>();
  TestLine<double, 4>();

  TestLineNonAutonomous<float, 1>();
  TestLineNonAutonomous<double, 1>();
  TestLineNonAutonomous<double, 2>();
  TestLineNonAutonomous<double, 3>();
  TestLineNonAutonomous<double, 4>();

  TestParabolaNonAutonomous<float, 1>();
  TestParabolaNonAutonomous<double, 1>();
  TestParabolaNonAutonomous<double, 2>();
  TestParabolaNonAutonomous<double, 3>();
  TestParabolaNonAutonomous<double, 4>();

  TestExp<double, 1>();
  TestExp<double, 2>();
  TestExp<double, 3>();

  TestExp<float, 1>();
  TestExp<float, 2>();
  TestExp<float, 3>();

  TestHelix<float>();
  TestHelix<double>();

  TestButcherTableau<float>();
  TestButcherTableau<double>();

  TestOscillatoryNonAutonomous<float>();
  TestOscillatoryNonAutonomous<double>();
}
}

int UnitTestDormandPrince(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDormandPrince, argc, argv);
}
