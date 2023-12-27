#ifndef SIMULATION_FRAMEWORK_SIMPLEVIEWMATRIX_H
#define SIMULATION_FRAMEWORK_SIMPLEVIEWMATRIX_H

#include "vsg/all.h"

namespace graphics {
class SimpleViewMatrix
    : public vsg::Inherit<vsg::ViewMatrix, SimpleViewMatrix> {
public:
  explicit SimpleViewMatrix(const vsg::dmat4 &m) : matrix(m) {}
  vsg::dmat4 transform() const override { return matrix; }
  vsg::dmat4 inverse() const override { return vsg::inverse(matrix); };
  vsg::dmat4 matrix;
};
} // namespace graphics

#endif // SIMULATION_FRAMEWORK_SIMPLEVIEWMATRIX_H
