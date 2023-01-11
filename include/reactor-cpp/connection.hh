//
// Created by revol-xut on 1/9/23.
//

#ifndef REACTOR_CPP_CONNECTION_HH
#define REACTOR_CPP_CONNECTION_HH

#include "environment.hh"
#include "reactor.hh"
#include "action.hh"
#include "reaction.hh"

namespace reactor {
  class [[maybe_unused]] BaseConnection : public ReactorElement {
  private:


  public:
    BaseConnection(const std::string& name, Reactor* container)
      : ReactorElement(name, ReactorElement::Type::Connection, container) {}
    ~BaseConnection() noexcept override = default;

  };

  template<class T>
  class [[maybe_unused]] Connection : public Action<T> {
  private:
    Port<T>* incoming_port_interface_{nullptr};
    Port<T>* outgoing_port_interface_{nullptr};
    std::chrono::nanoseconds delay_{0};
  public:
    Connection(const std::string& name, Reactor* container)
        : BaseAction(name, container, true, 0),
        incoming_port_interface_(new Port<T>{name, container}),
        outgoing_port_interface_(new Port<T>{name, container}) {}

    Connection(const std::string& name, Reactor* container, Duration delay, Port<T>* incoming, Port<T>* outgoing)
      : BaseAction(name, container, true, delay),
        incoming_port_interface_(incoming),
        outgoing_port_interface_(outgoing) {}

    ~Connection() noexcept override = default;

    inline auto get_incoming() const noexcept -> const Port<T>* { return  incoming_port_interface_; }
    inline auto get_outgoing() const noexcept -> const Port<T>* { return  outgoing_port_interface_; }

    void trigger() noexcept {
      this->schedule(incoming_port_interface_->get(), delay_);
    }

    void setup() override {
      if(outgoing_port_interface_ != nullptr && outgoing_port_interface_ != nullptr) {
        outgoing_port_interface_->set(incoming_port_interface_->get());
      }
    }

    inline auto get_set_callback() noexcept -> std::function<bool(BasePort*)> {
        return [this](BasePort* port) {
          this->trigger();
          return true;
        };
    }
    inline auto get_clean_callback() noexcept -> std::function<bool(BasePort*)> {
        return [](BasePort* port) {return true;};
    }
  };
} // namespace reactor

#endif // REACTOR_CPP_CONNECTION_HH