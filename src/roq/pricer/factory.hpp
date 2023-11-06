#include "roq/pricer/node.hpp"
#include "roq/core/hash.hpp"
#include <initializer_list>

namespace roq::pricer {

struct Factory {
    static pricer::Compute* get(std::string_view name);

   // Factory (std::initializer_list<std::pair<std::string, std::unique_ptr<pricer::Compute>>>&& il) : registry_(std::move(il)) {}

    static void emplace(std::string_view name, std::unique_ptr<pricer::Compute> fn);
    static void erase(std::string_view name);
    
    static void reset(Factory* rhs) {
        g_instance_ = rhs;
    }
    template<class...Computes>
    static void initialize();
    static void initialize_all();
private:
    core::Hash<std::string, std::unique_ptr<pricer::Compute>> registry_;
    static Factory* g_instance_;
};

template<class...Computes>
void Factory::initialize() {
    (g_instance_->emplace(Computes::NAME, std::make_unique<Computes>()),...);
}


} // roq::pricer::ops