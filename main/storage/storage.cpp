#include "storage/storage.hpp"

#include "app_context.hpp"

namespace storage {

core::IStorage& nvs() { return app::context().storage; }
core::IFileStore& files() { return app::context().files; }

} // namespace storage
