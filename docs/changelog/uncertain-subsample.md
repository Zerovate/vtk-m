## Add filter for subsampling while capturing uncertainty

Added the `StructuredReduceWithDistributionModel` filter. This will
reduce the size of `vtkm::cont::DataSet` with a structured cell set. It
behaves similarly to simple subsampling, but it also captures the
distribution of the data that are combined. It divides the data into
blocks and then computes the distribute of data within each block. The
parameters of those distributions are then placed as the reduced fields
on the resulting mesh.
