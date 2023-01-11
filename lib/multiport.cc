//
// Created by revol-xut on 1/9/23.
//

#include "reactor-cpp/multiport.hh"

auto reactor::BaseMultiport::get_set_callback(std::size_t index) noexcept -> std::function<bool(BasePort*)> {
    // tells the parent multiport that this port has been set.
    return [this, index](BasePort* port) {
      if (port->is_present()) {
        return false;
      }

      return this->set_present(index);
  };
}
auto reactor::BaseMultiport::get_clean_callback() noexcept -> std::function<bool(BasePort*)>{
    // resets parent multiport
    return [this]([[maybe_unused]]BasePort* port) {
        this->clear();
        return true;
    };
}
auto reactor::BaseMultiport::set_present(std::size_t index) -> bool {
    auto calculated_index = size_.fetch_add(1, std::memory_order_relaxed);

    reactor_assert(calculated_index < present_ports_.size());

    present_ports_[calculated_index] = index;
    return true;
}