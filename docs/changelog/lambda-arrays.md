# ArrayHandleImplicit and ArrayHandleTransform support lambda functions

`ArrayHandleImplicit` and `ArrayHandleTransform` now support using a lambda
function as its functors. Using lambdas, implicit arrays are easily
constructed with `make_ArrayHandleImplicit`.

``` cpp
auto implicitArray = make_ArrayHandleImplicit(
  VTKM_LAMBDA(vtkm::Id index) { return /* value based on index */; }, size);
```

Likewise, transform arrays can be constructed with lambda functions and
`make_ArrayHandleTransform`.

``` cpp
auto transformArray = make_ArrayHandleTransform(
  sourceArray,
  VTKM_LAMBDA(const auto& value) { return /* transformed value */; });
```

Previously, lambdas did not work with these arrays because the
object created for a lambda function does not have a default constructor.
The need for a default constructor has been eliminated.

Note that lambdas used with `ArrayHandle`s should be marked as
runnable in both the control and execution environment as the array may be
accessed in either environment. This is done by marking the lambda function
with `VTKM_LAMBDA`.
