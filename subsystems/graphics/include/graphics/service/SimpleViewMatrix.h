#pragma once

#include <utility>

#include "vsg/all.h"

namespace hive::graphics {

/**
 * Contains the view matrix as plain matrix.
 * @note This simple implementation was missing in Vulkan Scene Graph and was
 * therefore added.
 */
class SimpleViewMatrix
    : public vsg::Inherit<vsg::ViewMatrix, SimpleViewMatrix> {
public:
  explicit SimpleViewMatrix(vsg::dmat4 m) : matrix(std::move(m)) {}
  vsg::dmat4 transform() const override { return matrix; }
  vsg::dmat4 inverse() const override { return vsg::inverse(matrix); };
  vsg::dmat4 matrix;
};
} // namespace hive::graphics
