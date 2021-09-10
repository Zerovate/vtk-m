//Header for SLIC filter
#ifndef vtk_m_filter_SLIC_h
#define vtk_m_filter_SLIC_h

#include <vtkm/filter/FilterField.h>

namespace vtkm
{
    namespace filter
    {
        class SLIC : public vtkm::filter::FilterField <SLIC>
        {
            private:

                vtkm::Id xdim, ydim, zdim, blockXSize, blockYSize, blockZSize;
                vtkm::Float64 Weight;
                vtkm::Float64 HaltCondition;
                vtkm::Id iterLimit;
                std::string outFieldName;
                std::string SlicFieldName;

            public:

                //Constructor
                VTKM_CONT
                SLIC()
                {
                    this->Weight = 0.5; //default
                    this->HaltCondition = 0.3; //default
                    this->iterLimit = 75; //default
                    this->outFieldName = "ClusterIds";
                };

                ////Method
                VTKM_CONT
                void SetFieldDimension(vtkm::Id3 dims)
                {
                    this->xdim = dims[0];
                    this->ydim = dims[1];
                    this->zdim = dims[2];
                }

                VTKM_CONT
                void SetInitClusterSize(vtkm::Id3 blocksize)
                {
                    this->blockXSize = blocksize[0];
                    this->blockYSize = blocksize[1];
                    this->blockZSize = blocksize[2];
                }

                VTKM_CONT
                void SetWeight(vtkm::Float64 weight)
                {
                    this->Weight = weight;
                }

                VTKM_CONT
                void SetHaltCond(vtkm::Float64 halt_condition)
                {
                    this->HaltCondition = halt_condition;
                }

                VTKM_CONT
                void SetMaxIter(vtkm::Id iter_limit)
                {
                    this->iterLimit = iter_limit;
                }


                VTKM_CONT
                void SetSlicFieldName(std::string fieldname)
                {
                    this->SlicFieldName = fieldname;
                }

                // DoExecute function
                template <typename T, typename StorageType, typename DerivedPolicy>
                VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet,
                                                        const vtkm::cont::ArrayHandle<T, StorageType>& inField,
                                                        const vtkm::filter::FieldMetadata& fieldMetadata,
                                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
        };
    }
}

#include "SLIC.hxx"

#endif // vtk_m_filter_SLIC_h
