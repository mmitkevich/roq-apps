#include "roq/dag/node.hpp"
#include "roq/core/hash.hpp"
#include <initializer_list>

namespace roq::dag {

struct Factory {
    static dag::Compute* get(std::string_view name);

   // Factory (std::initializer_list<std::pair<std::string, std::unique_ptr<dag::Compute>>>&& il) : registry_(std::move(il)) {}

    static void emplace(std::string_view name, std::unique_ptr<dag::Compute> fn);
    static void erase(std::string_view name);
    
    static void reset(Factory* rhs) {
        g_instance_ = rhs;
    }
    template<class...Computes>
    static void initialize();
    static void initialize_all();
private:
    core::Hash<std::string, std::unique_ptr<dag::Compute>> registry_;
    static Factory* g_instance_;
};

template<class...Computes>
void Factory::initialize() {
    (g_instance_->emplace(Computes::NAME, std::make_unique<Computes>()),...);
}


} // roq::dag::ops