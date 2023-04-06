# ArrayHandleImplicit supports lambda functions

`ArrayHandleImplicit` now supports using a lambda function as its functor.
Using lambdas, implicit arrays are easily constructed with
`make_ArrayHandleImplicit`.

``` cpp
auto implicitArray = make_ArrayHandleImplicit(
  VTKM_LAMBDA(vtkm::Id index) { return /* value based on index */; }, size);
```

Previously, lambdas did not work with `ArrayHandleImplicit` because the
object created for a lambda function does not have a default constructor.
The need for a default constructor has been eliminated.

Note that lambdas used with `ArrayHandleImplicit` should be marked as
runnable in both the control and execution environment as the array may be
accessed in either environment. This is done by marking the lambda function
with `VTKM_LAMBDA`.
