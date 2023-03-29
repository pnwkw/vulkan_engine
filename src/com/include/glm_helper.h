#ifndef DISPLAY_GLM_HELPER_H
#define DISPLAY_GLM_HELPER_H

#include <glm/glm.hpp>
#include <spdlog/fmt/ostr.h>

namespace glm {
	template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
	OStream &operator<<(OStream &os, const glm::vec<L, T, Q> &vec) {
		os << "[";

		for (glm::length_t i = 0; i < L - 1; ++i) {
			os << vec[i] << ", ";
		}

		os << vec[L - 1];

		os << "]";

		return os;
	}
};

#endif //DISPLAY_GLM_HELPER_H
