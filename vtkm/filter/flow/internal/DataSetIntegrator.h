//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_internal_DataSetIntegrator_h
#define vtk_m_filter_flow_internal_DataSetIntegrator_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/worklet/EulerIntegrator.h>
#include <vtkm/filter/flow/worklet/IntegratorStatus.h>
#include <vtkm/filter/flow/worklet/ParticleAdvection.h>
#include <vtkm/filter/flow/worklet/RK4Integrator.h>
#include <vtkm/filter/flow/worklet/Stepper.h>

#include <vtkm/cont/Variant.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class DSIHelperInfo
{
public:
  DSIHelperInfo(const std::vector<ParticleType>& v,
                const vtkm::filter::flow::internal::BoundsMap& boundsMap,
                const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
    , V(v)
  {
  }

  const vtkm::filter::flow::internal::BoundsMap BoundsMap;
  const std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> ParticleBlockIDsMap;

  std::vector<ParticleType> A, I, V;
  std::unordered_map<vtkm::Id, std::vector<vtkm::Id>> IdMapA, IdMapI;
  std::vector<vtkm::Id> TermIdx, TermID;
};

template <typename Derived, typename ParticleType>
class DataSetIntegrator
{
public:
  DataSetIntegrator(vtkm::Id id, vtkm::filter::flow::IntegrationSolverType solverType)
    : Id(id)
    , SolverType(solverType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  VTKM_CONT vtkm::Id GetID() const { return this->Id; }
  VTKM_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  VTKM_CONT
  void Advect(DSIHelperInfo<ParticleType>& b,
              vtkm::FloatDefault stepSize) //move these to member data(?)
  {
    Derived* inst = static_cast<Derived*>(this);
    inst->DoAdvect(b, stepSize);
  }

  VTKM_CONT bool GetOutput(vtkm::cont::DataSet& dataset) const
  {
    Derived* inst = static_cast<Derived*>(this);
    return inst->GetOutput(dataset);
  }

protected:
  VTKM_CONT inline void ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                          DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  vtkm::Id Id;
  vtkm::filter::flow::IntegrationSolverType SolverType;
  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id Rank;
  bool CopySeedArray = false;
};

//template <typename Derived, typename ParticleType, typename AnalysisType>
template <typename Derived, typename ParticleType>
VTKM_CONT inline void DataSetIntegrator<Derived, ParticleType>::ClassifyParticles(
  const vtkm::cont::ArrayHandle<ParticleType>& particles,
  DSIHelperInfo<ParticleType>& dsiInfo) const
{
  dsiInfo.A.clear();
  dsiInfo.I.clear();
  dsiInfo.TermID.clear();
  dsiInfo.TermIdx.clear();
  dsiInfo.IdMapI.clear();
  dsiInfo.IdMapA.clear();

  auto portal = particles.WritePortal();
  vtkm::Id n = portal.GetNumberOfValues();

  for (vtkm::Id i = 0; i < n; i++)
  {
    auto p = portal.Get(i);

    if (p.GetStatus().CheckTerminate())
    {
      dsiInfo.TermIdx.emplace_back(i);
      dsiInfo.TermID.emplace_back(p.GetID());
    }
    else
    {
      const auto& it = dsiInfo.ParticleBlockIDsMap.find(p.GetID());
      VTKM_ASSERT(it != dsiInfo.ParticleBlockIDsMap.end());
      auto currBIDs = it->second;
      VTKM_ASSERT(!currBIDs.empty());

      std::vector<vtkm::Id> newIDs;
      if (p.GetStatus().CheckSpatialBounds() && !p.GetStatus().CheckTookAnySteps())
        newIDs.assign(std::next(currBIDs.begin(), 1), currBIDs.end());
      else
        newIDs = dsiInfo.BoundsMap.FindBlocks(p.GetPosition(), currBIDs);

      //reset the particle status.
      p.GetStatus() = vtkm::ParticleStatus();

      if (newIDs.empty()) //No blocks, we're done.
      {
        p.GetStatus().SetTerminate();
        dsiInfo.TermIdx.emplace_back(i);
        dsiInfo.TermID.emplace_back(p.GetID());
      }
      else
      {
        //If we have more than blockId, we want to minimize communication
        //and put any blocks owned by this rank first.
        if (newIDs.size() > 1)
        {
          for (auto idit = newIDs.begin(); idit != newIDs.end(); idit++)
          {
            vtkm::Id bid = *idit;
            if (dsiInfo.BoundsMap.FindRank(bid) == this->Rank)
            {
              newIDs.erase(idit);
              newIDs.insert(newIDs.begin(), bid);
              break;
            }
          }
        }

        int dstRank = dsiInfo.BoundsMap.FindRank(newIDs[0]);
        if (dstRank == this->Rank)
        {
          dsiInfo.A.emplace_back(p);
          dsiInfo.IdMapA[p.GetID()] = newIDs;
        }
        else
        {
          dsiInfo.I.emplace_back(p);
          dsiInfo.IdMapI[p.GetID()] = newIDs;
        }
      }
      portal.Set(i, p);
    }
  }

  //Make sure we didn't miss anything. Every particle goes into a single bucket.
  VTKM_ASSERT(static_cast<std::size_t>(n) ==
              (dsiInfo.A.size() + dsiInfo.I.size() + dsiInfo.TermIdx.size()));
  VTKM_ASSERT(dsiInfo.TermIdx.size() == dsiInfo.TermID.size());
}

}
}
}
} //vtkm::filter::flow::internal

#endif //vtk_m_filter_flow_internal_DataSetIntegrator_h
