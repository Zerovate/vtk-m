#ifndef HEAT_DIFFUSION_DIFFUSION_HPP
#define HEAT_DIFFUSION_DIFFUSION_HPP
#define NEUMMAN 0
#define DERICHLET 1

#include <vtkm/worklet/WorkletPointNeighborhood.h>
#include <vtkm/filter/FilterDataSet.h>

struct UpdateHeat : public vtkm::worklet::WorkletPointNeighborhood
{
    using CountingHandle = vtkm::cont::ArrayHandleCounting<vtkm::Id>;

    using ControlSignature = void(CellSetIn,
            FieldInNeighborhood prevstate,
            FieldIn condition,
            FieldIn diffuseCoeff,
            FieldOut state);

    using ExecutionSignature = void(_2, _3, _4, _5);

    template <typename NeighIn>
    VTKM_EXEC void operator()(const NeighIn& prevstate,
                              const vtkm::UInt8& condition,
                              const vtkm::Float32& diffuseCoeff,
                              vtkm::Float32& state) const
    {

        auto current = prevstate.Get(0, 0, 0);

        if (condition == NEUMMAN)
        {
            state = diffuseCoeff * current +
                    (1 - diffuseCoeff) * (0.25f * (prevstate.Get(-1, 0, 0) + prevstate.Get(0, -1, 0) +
                                                   prevstate.Get(0, 1, 0) + prevstate.Get(1, 0, 0)));
        }
        else if (condition == DERICHLET)
        {
            state = current;
        }
    }
};
namespace vtkm {
namespace filter {
class Diffusion : public vtkm::filter::FilterDataSet<Diffusion> {
public:
//     using SupportedTypes = vtkm::List<vtkm::Float32, vtkm::UInt8, int, vtkm::Vec3f_32>;


     template<typename Policy>
     VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet &input,
                vtkm::filter::PolicyBase<Policy> policy) const {

         vtkm::cont::ArrayHandle<vtkm::Float32> temperature;
         vtkm::cont::ArrayHandle<vtkm::Float32> prevTemperature;
         vtkm::cont::ArrayHandle<vtkm::UInt8> condition;
         vtkm::cont::ArrayHandle<vtkm::Float32> diffuse;
         vtkm::cont::ArrayHandle<int>iteration;

         const vtkm::cont::DynamicCellSet &cells = input.GetCellSet();

         input.GetPointField("temperature").GetData().CopyTo(prevTemperature);
         input.GetPointField("condition").GetData().CopyTo(condition);
         input.GetPointField("diffuseCoeff").GetData().CopyTo(diffuse);
         input.GetPointField("iteration").GetData().CopyTo(iteration);

         vtkm::cont::ArrayHandle<vtkm::Float32> *ptra = &prevTemperature, *ptrb = &temperature;
         //Update the temperature
         for (int i = 0; i < iteration.ReadPortal().Get(0); i++)
         {
             this->Invoke(UpdateHeat{},
                          vtkm::filter::ApplyPolicyCellSet(cells, policy, *this),
                          *ptra,
                          condition,
                          diffuse,
                          *ptrb);
             std::swap(ptra, ptrb);
         }


         vtkm::cont::DataSet output;
         output.CopyStructure(input);

         output.AddField(vtkm::cont::make_FieldPoint("temperature", temperature));
         output.AddField(vtkm::cont::make_FieldPoint("condition", condition));
         output.AddField(vtkm::cont::make_FieldPoint("diffuseCoeff", diffuse));
         output.AddField(vtkm::cont::make_FieldPoint("iteration", iteration));

         return output;
     }


     template<typename T, typename StorageType, typename DerivedPolicy>
     VTKM_CONT bool DoMapField(vtkm::cont::DataSet &,
             const vtkm::cont::ArrayHandle<T, StorageType> &,
             const vtkm::filter::FieldMetadata &,
             vtkm::filter::PolicyBase<DerivedPolicy>) { return false; }
};
}
}
#endif //HEAT_DIFFUSION_DIFFUSION_HPP
