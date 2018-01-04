#include "contextbuilder.h"

namespace jw_util {

std::vector<void (*)(jw_util::Context &context)> ContextBuilder::provisions;

}
