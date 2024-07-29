## Use a templated locator in the flow grid evaluator

Previously, the flow particle advection used a `CellLocatorGeneral` to find
cells. This required an underlying switch statement and was giving some
compilers a headache.

The particle advection code as changed to template the grid evaluator to
use be templeted on both the locator and cell set type to have one code
path in the worklet.
