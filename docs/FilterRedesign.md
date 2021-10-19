# New Fileter Design

The structure of filter classes is due for a redesign. After gathering requirements from the
developer and user community, we are proposing the following new design. This design document should
be considered as "living document" and could/should be revisited upon new findings and feedback from
its intended users.

## Decoupling client codes from device compiler

### Problem

Originally, VTK-m was considered a header-only library, partially due to its highly templated code
base. Having filter implementations in header files is problematic on many fronts. One of the
biggest issues is that to use any filter in VTK-m, client code has to be compiled by a device
compiler with all the compiler flags that VTK-m wants. This is a big no-no for many downstream users
including ECP customers. Rather, clients should be able to use VTK-m filters using only a standard
C++ compiler. For example, say VTK-m is compiled with CUDA. VTK should be able to completely include
all the VTK-m accelerator code without ever having to use nvcc.

Currently this is not possible because the filter headers include worklets that need to be compiled
on devices. These should be removed (using some of the following requirements).

### Solution

While the details are still being worked out, we propose to adopt a more *traditional* library
design. A filter will have two parts, the declaration of the Filter subclass resides in a `.h`
header file while the implementation resides in a `.cxx` file. The `.cxx` files, if necessary, will
be compiled by the device compiler.

## Policy objects no longer supported

The concept of policy only works if you do a special compile of the templates with that policy. But
that will not work if filters are compiled into libraries.

That said, the types of a policy have to specified in other ways. The primary way to specify what
types a filter should support are the type lists defined in `vtkm/cont/DefaultTypes.h`. This is
where lists like `VTKM_DEFAULT_TYPE_LIST`, `VTKM_DEFAULT_STORAGE_LIST`,
and `VTKM_DEFAULT_CELL_SET_LIST` are defined. More importantly, these lists can be modified by CMake
configuration options.

## Filter superclasses should support resolving input field array types

In the current implementation of filters, you can implement a `DoExecute` method that is given the
active field resolved to an `ArrayHandle` type. Filters commonly have to discover the type of a
field and instantiate templates accordingly. This can be complicated (and we continually evolve the
best way to do it). Thus, we should continue to have filter superclasses do this resolution for the
subclass.

Note that this is somewhat more complicated to implement because the current implementation can rely
on whatever uses the filter to instantiate the templates. When building for libraries, something has
to create the code to instantiate the templates, and we don't want filter developers to have to
create these by hand.

The current implementation allows the filter subclass to select which types to support in the
field (via filter "traits"). This or something like it should continue to be supported.

It is also possible to bypass this behavior by overriding `PrepareForExecution`. Something like this
should continue to be supported.

## Filter superclasses should support resolving mapped field array types

Similar to `DoExecute`, current filter implementations can define a `DoMapField` and the superclass
will automatically discover the type of the field array to be mapped and pass that to
the `DoMapField` template. This behavior should also continue to be supported.

A filter should also be able to instead override the behavior of `MapFieldOntoOutput` to handle the
field mapping using unknown arrays. (This will be relatively common.) It should also be possible to
do this override of `MapFieldOntoOutput` and then call the superclass to discover the type for some
arrays. For example, a filter might trivially pass point data but need to do some processing for
cell data.

## ApplyPolicy functionality is still necessary

Although policies are being removed, many of the `ApplyPolicy` functions take into account other
factors such as the filter traits. When policies go away, these functions should be renamed and
implemented with the policy part removed. (Perhaps they should be protected methods
in `vtkm::filter::Filter`.)

## Restructuring the `vtkm/filter' directory and compiling into seperate filter libraries

### Problem

The list of filters is already becoming unwieldy, and having them all in a single directory is
problematic. They should each be in their own directory. Likely the namespace will also be different
for each library (e.g. `vtkm::filter::clip::ClipWithField`).

We would also like to be able to compile subsets of the filters into more manageable libraries. This
will allow users to cherry-pick the filters they need to compile by selecting libraries/modules. If
one filter depends on another filter, then CMake needs to ensure that the latter library is enabled
when the former one is.

The granularity of filter libraries should also take link-time and run-time performance into
account.

### Solution

We propose to split the set of filters into the following categories each in their own
sub-directories in `vtkm/filter/SubDir`:

#### CleanGrid

CleanGrid

#### Compression

ZFPCompressor1D, ZFPCompressor2D, ZFPCompressor3D

#### ConnectedComponent

CellSetConnectivity, ImageConnectivity

#### Contour

ClipWithField, ClipWithImplicitFunction, Contour,  MIRFilter, Slice

#### DensityEstimate

Entropy, Histogram, NDEntropy, NDHistogram, ParticleDensityCloudInCell,
ParticleDensityNearestGridPoint

#### EntityExtraction

ExternalFaces, ExtractGeometry, ExtractPoints, ExtractStructured, GhostCellRemove, Mask, MaskPoints,
Threshold, ThresholdPoints

#### FieldConversion

CellAverage, PointAverage

#### FieldTransform

CoordinateSystemTransform, GenerateIds, FieldToColors, PointElevation, PointTransform, WarpScalar,
WarpVector

#### MeshInfo

CellMeasures, GhostCellClassify, MeshQuality

#### GeometryRefinement

SplitSharpEdges, Tetrahedralize, Triangulate, Tube

#### ImageProcessing

ComputeMoment, ImageDifference, ImageMedian,

#### ParticleAdvection

Lagrangian, LagrangianStructure, ParticleAdvection, Pathline, PathParticle, Streamline,
StreamSurface

#### Topology

ContourTreeUniform, ContourTreeUnifromAugmented, ContourTreeDistributed

#### VectorCalculus

CrossProduct, DotProduct, Gradient, SurfaceNormal, VectorMagnitude

#### Misc (to be further categorized)

Probe, VertexClustering

Each `vtkm/filter/SubDir` could also contain its own `worklet` subdirectory for standalone worklet
implementation when necessary and `testing` subdirectory for unit tests.

## Filter code should be colocated with the worklets that implement them

### Problem

For historical reasons, worklet implementations are in `vtkm/worklet` and the filter implementations
that use them are in `vtkm/filter`. This no longer makes sense.

Instead, `vtkm/worklet` should only contain code that makes creating worklets possible (
e.g. `WorkletMapField` and `Keys`).

The actual worklet implementations themselves should be moved to the directory (or a subdirectory)
of the filter. For example, `vtkm/worklet/Clip.h` should be moved to `vtkm/filter/clip` or some
subdirectory of that such as `vtkm/filter/clip/worklet` or `vtkm/filter/clip/internal`.

A nice feature of this change is that we can reduce some redundancy in the tests. There is no real
need to have one test for the worklet and another for the filter. A test for the filter should
exercise both.

Exceptions could be made for worklets that are intended to be used with multiple different filters
or other contexts (such as `AverageByKey`). But these are pretty rare.

### Solution

Several recently developed filters, for example `ParticleDensityCloudInCell`, has adopted this
structure. Instead of a standalone header file in the `vtkm/worklet` directory, the implementation
of the `CICWorklet` is embedded in the `ParticleDensityCloudInCell.hxx`

```c++
namespace vtkm
{
namespace worklet
{
class CICWorklet : public vtkm::worklet::WorkletMapField
{
    ...    
};
} // worklet
} // vtkm

namespace vtkm
{
namespace filter
{
    // Implementation of ParticleDenstiyCloudInCell, e.g. constructors, DoExecute()
} // filter
} // vtkm
```

Since now the `worklet` namespace for the particular worklet does not provide too much meaning and
isolation, we can put the Worklet implenentation in an *anonymous*, *internal* or *detail* namespace
as is done for vtkm dataset sources in the `vtkm/source`. For example, in `Tangle.cxx`:

```c++
namespace vtkm
{
namespace source
{
namespace tangle // could be empty as well
{
class TangleField : public vtkm::worklet::WorkletVisitPointsWithCells
{
    ...
};

vtkm::cont::DataSet Tangle::Execute() const
{
    ...
}

} // namespace tangle
} // namespace source
} // namespace vtkm
```

Some filters and sources contain worklets as part of their data members. This causes a circular
dependence on the `.h/hxx` files. This can either be solved by removing the worklet from being a
data member and only construct an instance when necessary (esp. in `DoExecute()` implementation.)

```c++
template <typename T, typename StorageType, typename Policy>
inline VTKM_CONT vtkm::cont::DataSet ParticleDensityCloudInCell::DoExecute(
  const cont::DataSet& dataSet,
  const cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata&,
  PolicyBase<Policy>)
{
  ...
  this->Invoke(vtkm::worklet::CICWorklet{},
               coords,
               field,
               locator,
               uniform.GetCellSet().template Cast<vtkm::cont::CellSetStructured<3>>(),
               density);
  ...
}
```

However some filters and sources have *stateful* worklets whos states are exposed through the filter
or source's public interface. This requires an instance of the worklet to be constructed when the
filter/source is constructed thus can not be easily removed as a data member. We can solve this by
adopting the *pointer to implementation (PImp)* pattern. We first forward declare the worklet in
the `.h` file, as in `vtkm/source/Oscilltor.h`

```c++
namespace vtkm
{
namespace source
{
namespace internal
{
class OscillatorSource;
} // namespace internal
} // namespace source
} // namespace vtkm
```

We then replace the worklet object in the filter with a poitner

```c++
namespace vtkm
{
namespace source
{
class VTKM_SOURCE_EXPORT Oscillator final : public vtkm::source::Source
{
  ...
  std::unique_ptr<internal::OscillatorSource> Worklet;
  ...
};
} // namespace source
} // namespace vtkm
```

We need to initialize the pointer with `std::make_unique<>()` since the default constructor for
`std::unique_pointer` is essentiall `nullptr`

```c++
Oscillator::Oscillator(vtkm::Id3 dims)
  : Dims(dims)
  , Worklet(std::make_unique<internal::OscillatorSource>())
{
}
```

We can then change filter's Getter/Setters accordingly

```c++
void Oscillator::SetTime(vtkm::FloatDefault time)
{
  this->Worklet->SetTime(time);
}
```

We deference the point an pass the object when calling `Invoke()`

```c++
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;
  this->Invoke(*(this->Worklet), coordinates, outArray);
  dataSet.AddField(vtkm::cont::make_FieldPoint("oscillating", outArray));
```

For worklet implementations that are too large to be embedded in the `.cxx` or `.hxx` files, they
can reside in standalone `.hxx` files in the `vtkm/filter/FilterSubDir/worklet` subdirectory.

## Split filter instantiations into multiple translation units

### Problem

By their nature, the filter implementations tend to compile the same worklets with different
template arguments. Although it is straightforward to compile all implementations using the same
source file, it is often advantagous to split the instantiations into multiple translation units.
Compiling all template resolutions at once can take a long time and might even overwhelm the
compiler. Splitting up the code into multiple translation units means the compiler can take less
memory per instance and can help with the time in parallel compiles.

### Solution

Recent updates to several filters and the CMake build sytem added support for a fine-graind
instantiantion mechanism, see `vtkm/docs/changelog/filters-instantiations-generators.md` for detail.
However, compile time profiling had indicated that there is a deminishing return on the reduction of
compile time and memory usage, thus this feature needs to be used with care. Currently we are
working on using the CMake UNITY_BUILD feature to recombind instantiation filtes back into on single
translation unit. Filter developer and users will be able to provide his/her own choice on the
compile time/memory usage tradeoff for particular filter implementation and/or platform.

## Thread safety

### Problem

The current implementation of filters has thread safety issues. Many filters store an array during
their `DoExecute` in the state that is later used to compute the `MapField` part of their execution.
If multiple threads are using the same filter object, they are liable to overwrite the arrays used
in the state. The problem gets even worse when considering temporal objects that have to hold the
data and results from one `Execute` to the next.

The filter structure should have an elegant way of running a filter on multiple threads without
having to perform the same initialization on every thread (which is likely not reasonable).

### Solution

Recent updates to the Filter interface added several virtual functions to address thread safety
issues and allows filter implementor to selectively support multi-threaded execution.
The `CanThread()` method returns a boolean indicating if the particular filter implementation can
support multi-threaded execution. If a filter implementation supports multi-thread execution, it
needs to override the `Clone()` method to copy shared states. Base classes of different Filter kinds
also provide the `CopyStatesFrom()` method to help with copying states associated with those base
classes.

Example implementation in CleanGrid.h

```c++
  VTKM_CONT
  bool CanThread() const override { return true; }

  VTKM_CONT
  Filter* Clone() const override
  {
    CleanGrid* clone = new CleanGrid();
    clone->CopyStateFrom(this);
    return clone;
  }
  
  VTKM_CONT
  void CopyStateFrom(const CleanGrid* cleanGrid)
  {
    this->FilterDataSet<CleanGrid>::CopyStateFrom(cleanGrid);

    this->CompactPointFields = cleanGrid->CompactPointFields;
    this->MergePoints = cleanGrid->MergePoints;
    this->Tolerance = cleanGrid->Tolerance;
    this->ToleranceIsAbsolute = cleanGrid->ToleranceIsAbsolute;
    this->RemoveDegenerateCells = cleanGrid->RemoveDegenerateCells;
    this->FastMerge = cleanGrid->FastMerge;
  }
```
