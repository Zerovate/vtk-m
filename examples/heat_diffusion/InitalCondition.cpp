#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>

#include "InitalCondition.h"

#define NEUMMAN 0
#define DERICHLET 1

template<typename CoordType>
VTKM_EXEC void FillInitialCondition::operator()(const CoordType &coord,
                          vtkm::UInt8 &boundary,
                          vtkm::Float32 &temperature,
                          vtkm::Float32 &diffusion) const {
    if (coord[0] == -2.0f || coord[0] == 2.0f || coord[1] == -2.0f || coord[1] == 2.0f) {
        temperature = std::get<0>(parameters.temperature);
        boundary = DERICHLET;
        diffusion = parameters.diffuse_coeff;
    } else {

        float dx = (float)coord[0] - 0.0f;
        float dy = (float)coord[1] - 0.0f;
        float distance = dx * dx + dy * dy;

        float rayon = (1.25f * 1.25f);

        if (distance - rayon < 0.1f && distance - rayon > -0.1f) {
            temperature = std::get<0>(parameters.temperature);
            boundary = DERICHLET;
            diffusion = parameters.diffuse_coeff;
        } else {

            temperature = std::get<1>(parameters.temperature);
            boundary = NEUMMAN;
            diffusion = parameters.diffuse_coeff;
        }
    }
}

vtkm::cont::DataSet initial_condition(const Parameters &params) {
    vtkm::Id2 dimensions(params.dimension, params.dimension);
    vtkm::cont::DataSet dataSet = vtkm::cont::DataSetBuilderUniform::Create(dimensions,
                                                                            vtkm::Vec2f{-2.0f, -2.0f},
                                                                            vtkm::Vec2f{
                                                                                    4.0f / (float) (dimensions[0] - 1),
                                                                                    4.0f /
                                                                                    (float) (dimensions[1] - 1)});

    vtkm::cont::CoordinateSystem coords = dataSet.GetCoordinateSystem("coords");

    vtkm::cont::ArrayHandle<vtkm::Float32> temperature;
    vtkm::cont::ArrayHandle<vtkm::UInt8> condition;
    vtkm::cont::ArrayHandle<vtkm::Float32> diffuse;

    vtkm::cont::Invoker invoker;
    invoker(FillInitialCondition{params}, coords, condition, temperature, diffuse);

    dataSet.AddField(vtkm::cont::make_FieldPoint("temperature", temperature));
    dataSet.AddField(vtkm::cont::make_FieldPoint("condition", condition));
    dataSet.AddField(vtkm::cont::make_FieldPoint("diffuseCoeff", diffuse));

    return dataSet;
}
